// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "debuginfo/debug_info.h"
#include "debuginfo/dwarf_reader.h"

#include "common/spdlog.h"

#include <llvm/BinaryFormat/Dwarf.h>
#include <llvm/Config/llvm-config.h>
#include <llvm/DebugInfo/DWARF/DWARFCompileUnit.h>
#include <llvm/DebugInfo/DWARF/DWARFContext.h>
#include <llvm/DebugInfo/DWARF/DWARFDebugLine.h>
#include <llvm/DebugInfo/DWARF/DWARFFormValue.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DIBuilder.h>
#include <llvm/IR/DebugInfo.h>
#include <llvm/IR/DebugInfoMetadata.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/raw_ostream.h>

#include <map>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>

namespace WasmEdge::LLVM::DebugInfo {

using namespace std::literals;
using llvm::DWARFDie;
namespace dwarf = llvm::dwarf;

namespace {

constexpr uint32_t MaxDepth = 128;

struct UnitState {
  std::unique_ptr<llvm::DIBuilder> DB;
  llvm::DICompileUnit *CU = nullptr;
};

std::string declFile(DWARFDie D) noexcept {
#if LLVM_VERSION_MAJOR < 12
  auto Index = dwarf::toUnsigned(D.findRecursively(dwarf::DW_AT_decl_file));
  if (!Index) {
    return {};
  }
  auto *U = D.getDwarfUnit();
  const auto *LT = U->getContext().getLineTableForUnit(U);
  std::string Path;
  if (!LT || !LT->getFileNameByIndex(
                 *Index, U->getCompilationDir(),
                 llvm::DILineInfoSpecifier::FileLineInfoKind::AbsoluteFilePath,
                 Path)) {
    return {};
  }
  return Path;
#else
  return D.getDeclFile(
      llvm::DILineInfoSpecifier::FileLineInfoKind::AbsoluteFilePath);
#endif
}

class ModuleDebugInfoImpl;

class FunctionDebugInfoImpl final : public FunctionDebugInfo {
public:
  FunctionDebugInfoImpl(ModuleDebugInfoImpl &Parent, UnitState &S,
                        llvm::Function &Fn, llvm::DISubprogram *SP,
                        DWARFDie Die) noexcept
      : Parent(Parent), S(S), Fn(Fn), SP(SP), Die(Die) {
    Fn.setSubprogram(SP);
    if (!Die) {
      return;
    }
    auto *U = Die.getDwarfUnit();
    LineTable = U->getContext().getLineTableForUnit(U);
    collectScopes(Die, 0);
    std::sort(ScopeBounds.begin(), ScopeBounds.end());
    ScopeBounds.erase(std::unique(ScopeBounds.begin(), ScopeBounds.end()),
                      ScopeBounds.end());
  }

  void setPrologueLocation(LLVMBuilderRef B) noexcept override {
    setArtificialLocation(B);
  }

  void setArtificialLocation(LLVMBuilderRef B) noexcept override {
    llvm::unwrap(B)->SetCurrentDebugLocation(
        llvm::DILocation::get(Fn.getContext(), 0, 0, SP));
  }

  void setLocation(LLVMBuilderRef B, uint64_t FileOffset) noexcept override;

  FrameBase getFrameBase() const noexcept override { return {}; }

  void finish(LLVMBasicBlockRef, Span<const LLVMValueRef>, LLVMValueRef,
              LLVMValueRef, LLVMValueRef) noexcept override {}

private:
  void collectScopes(DWARFDie Node, uint32_t Depth) noexcept {
    if (Depth >= MaxDepth) {
      return;
    }
    for (auto C : Node.children()) {
      const auto Tag = C.getTag();
      if (Tag != dwarf::DW_TAG_lexical_block &&
          Tag != dwarf::DW_TAG_inlined_subroutine) {
        continue;
      }
      if (auto Ranges = C.getAddressRanges()) {
        for (const auto &R : *Ranges) {
          if (R.LowPC < R.HighPC) {
            ScopeBounds.push_back(R.LowPC);
            ScopeBounds.push_back(R.HighPC);
          }
        }
      } else {
        llvm::consumeError(Ranges.takeError());
      }
      collectScopes(C, Depth + 1);
    }
  }

  llvm::DIFile *lineFile(uint64_t FileIndex) noexcept;

  ModuleDebugInfoImpl &Parent;
  UnitState &S;
  llvm::Function &Fn;
  llvm::DISubprogram *SP;
  DWARFDie Die;
  const llvm::DWARFDebugLine::LineTable *LineTable = nullptr;
  std::vector<uint64_t> ScopeBounds;
  uint64_t CachedLow = 1;
  uint64_t CachedHigh = 0;
  llvm::DIScope *CachedScope = nullptr;
  llvm::DILocation *CachedInlinedAt = nullptr;
  std::unordered_map<uint64_t, llvm::DIFile *> LineFiles;
};

class ModuleDebugInfoImpl final : public ModuleDebugInfo {
public:
  ModuleDebugInfoImpl(llvm::Module &M, std::unique_ptr<DwarfReader> R,
                      Part P) noexcept
      : M(M), Reader(std::move(R)), P(P) {
    M.addModuleFlag(llvm::Module::Warning, "Debug Info Version",
                    llvm::DEBUG_METADATA_VERSION);
    M.addModuleFlag(llvm::Module::Warning, "Dwarf Version", 4);
  }

  LLVMMetadataRef translateType(uint64_t DieOffset) noexcept override {
    auto Die = Reader->getContext().getDIEForOffset(DieOffset);
    if (!Die) {
      return nullptr;
    }
    return llvm::wrap(type(unit(Die), Die));
  }

  LLVMValueRef getMemoryBaseGlobal() const noexcept override {
    return llvm::wrap(MemoryBase);
  }

  std::unique_ptr<FunctionDebugInfo>
  beginFunction(uint32_t FuncIndex, uint32_t DefinedIndex,
                LLVMValueRef Fn) noexcept override {
    if (P == Part::GlobalsOnly) {
      return nullptr;
    }
    auto &F = *llvm::cast<llvm::Function>(llvm::unwrap(Fn));
    if (auto Die = Reader->getSubprogram(DefinedIndex)) {
      auto &S = unit(Die);
      return std::make_unique<FunctionDebugInfoImpl>(
          *this, S, F, subprogramFor(S, Die), Die);
    }
    auto &S = artificialUnit();
    auto *SP = S.DB->createFunction(
        S.CU->getFile(), Reader->getFunctionName(FuncIndex), F.getName(),
        S.CU->getFile(), 0,
        S.DB->createSubroutineType(S.DB->getOrCreateTypeArray({})), 0,
        llvm::DINode::FlagArtificial, llvm::DISubprogram::SPFlagDefinition);
    return std::make_unique<FunctionDebugInfoImpl>(*this, S, F, SP, DWARFDie());
  }

  void finalize() noexcept override {
    for (auto &[Offset, U] : Units) {
      U.DB->finalize();
    }
    if (Artificial.DB) {
      Artificial.DB->finalize();
    }
  }

  std::optional<std::string> verify() noexcept override {
    std::string Msg;
    llvm::raw_string_ostream OS(Msg);
    bool BrokenDebugInfo = false;
    if (llvm::verifyModule(M, &OS, &BrokenDebugInfo)) {
      return OS.str();
    }
    if (BrokenDebugInfo) {
      spdlog::warn("debug info: verifier rejected debug info, stripped: {}"sv,
                   OS.str());
      llvm::StripDebugInfo(M);
    }
    return std::nullopt;
  }

  uint64_t toAddress(uint64_t FileOffset) const noexcept {
    return Reader->toAddress(FileOffset);
  }

  std::pair<llvm::DIScope *, llvm::DILocation *>
  scopeAt(UnitState &S, llvm::DISubprogram *SP, DWARFDie FnDie,
          uint64_t Addr) noexcept {
    auto &Ctx = M.getContext();
    llvm::SmallVector<DWARFDie, 4> Chain;
    FnDie.getDwarfUnit()->getInlinedChainForAddress(Addr, Chain);
    llvm::DIScope *Outer = scopeFor(S, SP, FnDie, Addr);
    if (Chain.empty() || Chain.back() != FnDie) {
      return {Outer, nullptr};
    }
    llvm::DILocation *InlinedAt = nullptr;
    for (size_t I = Chain.size() - 1; I > 0; --I) {
      auto Call = Chain[I - 1];
      InlinedAt = llvm::DILocation::get(
          Ctx,
          static_cast<unsigned>(
              dwarf::toUnsigned(Call.find(dwarf::DW_AT_call_line), 0)),
          static_cast<unsigned>(
              dwarf::toUnsigned(Call.find(dwarf::DW_AT_call_column), 0)),
          Outer, InlinedAt);
      auto Origin =
          Call.getAttributeValueAsReferencedDie(dwarf::DW_AT_abstract_origin);
      Outer = scopeFor(S, subprogramFor(S, Origin ? Origin : Call), Call, Addr);
    }
    return {Outer, InlinedAt};
  }

  llvm::DIScope *fileScope(UnitState &S, llvm::DIScope *Scope,
                           llvm::DIFile *File) noexcept {
    if (!File || File == Scope->getFile()) {
      return Scope;
    }
    auto &Cached = FileScopes[{Scope, File}];
    if (!Cached) {
      Cached = S.DB->createLexicalBlockFile(Scope, File);
    }
    return Cached;
  }

  llvm::DIFile *fileFor(UnitState &S, llvm::StringRef Path,
                        llvm::StringRef Dir) noexcept {
    if (llvm::sys::path::is_absolute(Path)) {
      return S.DB->createFile(llvm::sys::path::filename(Path),
                              llvm::sys::path::parent_path(Path));
    }
    return S.DB->createFile(Path, Dir);
  }

private:
  llvm::DISubprogram *subprogramFor(UnitState &S, DWARFDie D) noexcept {
    if (auto It = Subprograms.find(D.getOffset()); It != Subprograms.end()) {
      return It->second;
    }
    auto *File = file(S, D);
    const char *Linkage =
        dwarf::toString(D.findRecursively({dwarf::DW_AT_MIPS_linkage_name,
                                           dwarf::DW_AT_linkage_name}),
                        nullptr);
    auto Flags = llvm::DISubprogram::SPFlagDefinition;
    if (!dwarf::toUnsigned(D.find(dwarf::DW_AT_external), 0)) {
      Flags |= llvm::DISubprogram::SPFlagLocalToUnit;
    }
    auto *SP = S.DB->createFunction(File, name(D), Linkage ? Linkage : "", File,
                                    line(D), subroutine(S, D), line(D),
                                    llvm::DINode::FlagZero, Flags);
    Subprograms[D.getOffset()] = SP;
    return SP;
  }

  UnitState &artificialUnit() noexcept {
    if (!Artificial.DB) {
      Artificial.DB = std::make_unique<llvm::DIBuilder>(M);
      Artificial.CU = Artificial.DB->createCompileUnit(
#if LLVM_VERSION_MAJOR < 22
          dwarf::DW_LANG_C99,
#else
          llvm::DISourceLanguageName(dwarf::DW_LANG_C99),
#endif
          Artificial.DB->createFile("<wasm>", ""), "WasmEdge", false, "", 0);
    }
    return Artificial;
  }

  DWARFDie innermostScope(DWARFDie Frame, uint64_t Addr) noexcept {
    for (bool Descended = true; Descended;) {
      Descended = false;
      for (auto C : Frame.children()) {
        if (C.getTag() == dwarf::DW_TAG_lexical_block &&
            C.addressRangeContainsAddress(Addr)) {
          Frame = C;
          Descended = true;
          break;
        }
      }
    }
    return Frame;
  }

  llvm::DIScope *scopeFor(UnitState &S, llvm::DIScope *Parent, DWARFDie Frame,
                          uint64_t Addr) noexcept {
    llvm::DIScope *Scope = Parent;
    llvm::SmallVector<DWARFDie, 4> Blocks;
    for (auto B = innermostScope(Frame, Addr); B != Frame; B = B.getParent()) {
      Blocks.push_back(B);
    }
    for (auto It = Blocks.rbegin(); It != Blocks.rend(); ++It) {
      auto &Cached = Lexical[It->getOffset()];
      if (!Cached) {
        Cached =
            S.DB->createLexicalBlock(Scope, Scope->getFile(), line(*It), 0);
      }
      Scope = Cached;
    }
    return Scope;
  }

  UnitState &unit(DWARFDie Die) noexcept {
    auto *U = Die.getDwarfUnit();
    auto &S = Units[U->getOffset()];
    if (S.DB) {
      return S;
    }
    auto CUDie = U->getUnitDIE();
    S.DB = std::make_unique<llvm::DIBuilder>(M);
    const auto Lang = static_cast<uint16_t>(dwarf::toUnsigned(
        CUDie.find(dwarf::DW_AT_language), dwarf::DW_LANG_C99));
    const std::string Producer =
        "WasmEdge from "s +
        dwarf::toString(CUDie.find(dwarf::DW_AT_producer), "unknown");
    S.CU = S.DB->createCompileUnit(
#if LLVM_VERSION_MAJOR < 22
        Lang,
#else
        llvm::DISourceLanguageName(Lang),
#endif
        fileFor(S,
                CUDie.getName(llvm::DINameKind::ShortName)
                    ? CUDie.getName(llvm::DINameKind::ShortName)
                    : "<wasm>",
                dwarf::toString(CUDie.find(dwarf::DW_AT_comp_dir), "")),
        Producer, false, "", 0);
    return S;
  }

  llvm::DIFile *file(UnitState &S, DWARFDie D) noexcept {
    auto Path = declFile(D);
    if (Path.empty()) {
      return S.CU->getFile();
    }
    return fileFor(S, Path, "");
  }

  static unsigned line(DWARFDie D) noexcept {
    return static_cast<unsigned>(D.getDeclLine());
  }
  static llvm::StringRef name(DWARFDie D) noexcept {
    const char *N = D.getName(llvm::DINameKind::ShortName);
    return N ? llvm::StringRef(N) : llvm::StringRef();
  }
  static DWARFDie ref(DWARFDie D) noexcept {
    constexpr uint32_t MaxOriginChain = 8;
    for (uint32_t Depth = 0; D && Depth < MaxOriginChain; ++Depth) {
      if (auto T = D.getAttributeValueAsReferencedDie(dwarf::DW_AT_type)) {
        return T;
      }
      if (auto Origin = D.getAttributeValueAsReferencedDie(
              dwarf::DW_AT_abstract_origin)) {
        D = Origin;
      } else {
        D = D.getAttributeValueAsReferencedDie(dwarf::DW_AT_specification);
      }
    }
    return {};
  }
  static uint64_t byteSize(DWARFDie D) noexcept {
    return dwarf::toUnsigned(D.find(dwarf::DW_AT_byte_size), 0);
  }

  llvm::DIType *uchar(UnitState &S) noexcept {
    return S.DB->createBasicType("unsigned char", 8,
                                 dwarf::DW_ATE_unsigned_char);
  }
  llvm::DIType *uint32(UnitState &S) noexcept {
    return S.DB->createBasicType("uint32_t", 32, dwarf::DW_ATE_unsigned);
  }

  llvm::DIType *type(UnitState &S, DWARFDie D) noexcept {
    if (!D) {
      return nullptr;
    }
    if (auto It = Types.find(D.getOffset()); It != Types.end()) {
      return It->second;
    }
    auto &DB = *S.DB;
    llvm::DIType *T = nullptr;
    switch (D.getTag()) {
    case dwarf::DW_TAG_base_type:
      T = DB.createBasicType(
          name(D), byteSize(D) * 8,
          static_cast<unsigned>(dwarf::toUnsigned(D.find(dwarf::DW_AT_encoding),
                                                  dwarf::DW_ATE_unsigned)));
      break;
    case dwarf::DW_TAG_unspecified_type:
      T = DB.createUnspecifiedType(name(D));
      break;
    case dwarf::DW_TAG_typedef:
      T = DB.createTypedef(type(S, ref(D)), name(D), file(S, D), line(D),
                           typeScope(S, D));
      break;
    case dwarf::DW_TAG_const_type:
    case dwarf::DW_TAG_volatile_type:
    case dwarf::DW_TAG_restrict_type:
    case dwarf::DW_TAG_atomic_type:
      T = DB.createQualifiedType(D.getTag(), type(S, ref(D)));
      break;
    case dwarf::DW_TAG_pointer_type:
    case dwarf::DW_TAG_reference_type:
    case dwarf::DW_TAG_rvalue_reference_type:
      T = wasmPointer(S, D);
      break;
    case dwarf::DW_TAG_structure_type:
    case dwarf::DW_TAG_class_type:
    case dwarf::DW_TAG_union_type:
      T = composite(S, D);
      break;
    case dwarf::DW_TAG_enumeration_type:
      T = enumeration(S, D);
      break;
    case dwarf::DW_TAG_array_type:
      T = array(S, D);
      break;
    case dwarf::DW_TAG_subroutine_type:
      T = subroutine(S, D);
      break;
    default:
      T = opaque(S, byteSize(D));
      break;
    }
    Types[D.getOffset()] = T;
    return T;
  }

  llvm::DIType *opaque(UnitState &S, uint64_t Size) noexcept {
    auto &DB = *S.DB;
    return DB.createArrayType(Size * 8, 8, uchar(S),
                              DB.getOrCreateArray({DB.getOrCreateSubrange(
                                  0, static_cast<int64_t>(Size))}));
  }

  llvm::DIType *funcRef(UnitState &S) noexcept {
    auto &DB = *S.DB;
    auto *T = DB.createReplaceableCompositeType(
        dwarf::DW_TAG_structure_type, "__wasm_funcref", S.CU, nullptr, 0, 0, 32,
        32, llvm::DINode::FlagZero);
    auto *Index = DB.createMemberType(T, "__index", nullptr, 0, 32, 32, 0,
                                      llvm::DINode::FlagZero, uint32(S));
    DB.replaceArrays(T, DB.getOrCreateArray({Index}));
    return llvm::MDNode::replaceWithDistinct(llvm::TempDICompositeType(T));
  }

  llvm::DIType *wasmPointer(UnitState &S, DWARFDie D) noexcept {
    auto Pointee = ref(D);
    if (Pointee && Pointee.getTag() == dwarf::DW_TAG_subroutine_type) {
      return funcRef(S);
    }
    auto &DB = *S.DB;
    auto *T = DB.createReplaceableCompositeType(
        dwarf::DW_TAG_structure_type, pointerName(Pointee), S.CU, nullptr, 0,
        0, 32, 32, llvm::DINode::FlagZero);
    Types[D.getOffset()] = T;
    llvm::DIType *Elem = Pointee ? type(S, Pointee) : nullptr;
    if (!Elem) {
      Elem = uchar(S);
    }
    auto *Addr = DB.createMemberType(T, "__addr", nullptr, 0, 32, 32, 0,
                                     llvm::DINode::FlagZero, uint32(S));
    auto *Arr = DB.createArrayType(
        0, 0, Elem,
        DB.getOrCreateArray({DB.getOrCreateSubrange(int64_t(0), int64_t(0))}));
    auto *Pt = DB.createMemberType(T, "__pointee", nullptr, 0, uint64_t(0), 0,
                                   uint64_t(0), llvm::DINode::FlagZero, Arr);
    llvm::Metadata *Fields[] = {Addr, Pt};
    DB.replaceArrays(T, DB.getOrCreateArray(Fields));
    return llvm::MDNode::replaceWithDistinct(llvm::TempDICompositeType(T));
  }

  std::string pointerName(DWARFDie Pointee) {
    std::string Name = "__wasm_ptr<" + typeName(Pointee, 0) + ">";
    const uint64_t Key = Pointee ? Pointee.getOffset() : UINT64_MAX;
    auto [It, Inserted] = PointerNames.emplace(Name, Key);
    if (Inserted || It->second == Key) {
      return Name;
    }
    Name += "#" + std::to_string(Key);
    PointerNames.emplace(Name, Key);
    return Name;
  }

  static std::string typeName(DWARFDie D, uint32_t Depth) {
    if (!D) {
      return "void";
    }
    if (Depth >= 16) {
      return "...";
    }
    switch (D.getTag()) {
    case dwarf::DW_TAG_const_type:
      return typeName(ref(D), Depth + 1) + " const";
    case dwarf::DW_TAG_volatile_type:
      return typeName(ref(D), Depth + 1) + " volatile";
    case dwarf::DW_TAG_restrict_type:
      return typeName(ref(D), Depth + 1) + " restrict";
    case dwarf::DW_TAG_atomic_type:
      return "_Atomic " + typeName(ref(D), Depth + 1);
    case dwarf::DW_TAG_pointer_type:
      return typeName(ref(D), Depth + 1) + " *";
    case dwarf::DW_TAG_reference_type:
      return typeName(ref(D), Depth + 1) + " &";
    case dwarf::DW_TAG_rvalue_reference_type:
      return typeName(ref(D), Depth + 1) + " &&";
    case dwarf::DW_TAG_array_type:
      return typeName(ref(D), Depth + 1) + "[]";
    case dwarf::DW_TAG_subroutine_type:
      return "fn";
    default:
      break;
    }
    std::string Name = name(D).str();
    if (Name.empty()) {
      Name = "(anonymous)";
    }
    for (auto P = D.getParent(); P; P = P.getParent()) {
      const auto Tag = P.getTag();
      if (Tag != dwarf::DW_TAG_namespace &&
          Tag != dwarf::DW_TAG_structure_type &&
          Tag != dwarf::DW_TAG_class_type && Tag != dwarf::DW_TAG_union_type) {
        break;
      }
      const auto Outer = name(P);
      Name = (Outer.empty() ? "(anonymous)"s : Outer.str()) + "::" + Name;
    }
    return Name;
  }

  llvm::DIScope *typeScope(UnitState &S, DWARFDie D) noexcept {
    auto P = D.getParent();
    if (!P) {
      return S.CU;
    }
    switch (P.getTag()) {
    case dwarf::DW_TAG_structure_type:
    case dwarf::DW_TAG_class_type:
    case dwarf::DW_TAG_union_type:
      if (auto *T = llvm::dyn_cast_or_null<llvm::DICompositeType>(type(S, P))) {
        return T;
      }
      return S.CU;
    default:
      return S.CU;
    }
  }

  llvm::DIDerivedType *member(UnitState &S, llvm::DIScope *Scope,
                              DWARFDie C) noexcept {
    auto &DB = *S.DB;
    auto *MT = type(S, ref(C));
    if (!MT) {
      return nullptr;
    }
    uint64_t Off = 0;
    if (auto V = C.find(dwarf::DW_AT_data_member_location)) {
      if (auto U = V->getAsUnsignedConstant()) {
        Off = *U * 8;
      }
    }
    if (auto BitSize = dwarf::toUnsigned(C.find(dwarf::DW_AT_bit_size))) {
      const auto BitOff =
          dwarf::toUnsigned(C.find(dwarf::DW_AT_data_bit_offset), Off);
      return DB.createBitFieldMemberType(Scope, name(C), file(S, C), line(C),
                                         *BitSize, BitOff, 0,
                                         llvm::DINode::FlagZero, MT);
    }
    return DB.createMemberType(Scope, name(C), file(S, C), line(C), 0, 0, Off,
                               llvm::DINode::FlagZero, MT);
  }

  llvm::Metadata *variantPart(UnitState &S, llvm::DIScope *Scope,
                              DWARFDie VP) noexcept {
    auto &DB = *S.DB;
    llvm::DIDerivedType *Discr = nullptr;
    if (auto DiscrDie =
            VP.getAttributeValueAsReferencedDie(dwarf::DW_AT_discr)) {
      Discr = member(S, Scope, DiscrDie);
    }
    llvm::SmallVector<llvm::Metadata *, 8> Variants;
    for (auto V : VP.children()) {
      if (V.getTag() != dwarf::DW_TAG_variant) {
        continue;
      }
      llvm::Constant *Value = nullptr;
      if (auto DV = V.find(dwarf::DW_AT_discr_value)) {
        if (auto U = DV->getAsUnsignedConstant()) {
          Value = llvm::ConstantInt::get(llvm::Type::getInt64Ty(M.getContext()),
                                         *U);
        }
      }
      for (auto C : V.children()) {
        if (C.getTag() != dwarf::DW_TAG_member) {
          continue;
        }
        auto *MT = type(S, ref(C));
        if (!MT) {
          continue;
        }
        const auto Off =
            dwarf::toUnsigned(C.find(dwarf::DW_AT_data_member_location), 0) * 8;
        Variants.push_back(DB.createVariantMemberType(
            Scope, name(C), file(S, C), line(C), 0, 0, Off, Value,
            llvm::DINode::FlagZero, MT));
      }
    }
    return DB.createVariantPart(Scope, "", nullptr, 0, 0, 0,
                                llvm::DINode::FlagZero, Discr,
                                DB.getOrCreateArray(Variants));
  }

  llvm::DIType *composite(UnitState &S, DWARFDie D) noexcept {
    auto &DB = *S.DB;
    const bool Decl = dwarf::toUnsigned(D.find(dwarf::DW_AT_declaration), 0);
    auto *T = DB.createReplaceableCompositeType(
        D.getTag(), name(D), typeScope(S, D), file(S, D), line(D), 0,
        byteSize(D) * 8, 0,
        Decl ? llvm::DINode::FlagFwdDecl : llvm::DINode::FlagZero);
    Types[D.getOffset()] = T;
    llvm::SmallVector<llvm::Metadata *, 16> Members;
    for (auto C : D.children()) {
      llvm::Metadata *E = nullptr;
      switch (C.getTag()) {
      case dwarf::DW_TAG_member:
        E = member(S, T, C);
        break;
      case dwarf::DW_TAG_inheritance:
        if (auto *Base = type(S, ref(C))) {
          E = DB.createInheritance(
              T, Base,
              dwarf::toUnsigned(C.find(dwarf::DW_AT_data_member_location), 0) *
                  8,
              0, llvm::DINode::FlagZero);
        }
        break;
      case dwarf::DW_TAG_variant_part:
        E = variantPart(S, T, C);
        break;
      default:
        break;
      }
      if (E) {
        Members.push_back(E);
      }
    }
    DB.replaceArrays(T, DB.getOrCreateArray(Members));
    auto *Final =
        llvm::MDNode::replaceWithDistinct(llvm::TempDICompositeType(T));
    Types[D.getOffset()] = Final;
    return Final;
  }

  llvm::DIType *enumeration(UnitState &S, DWARFDie D) noexcept {
    auto &DB = *S.DB;
    llvm::SmallVector<llvm::Metadata *, 16> Items;
    for (auto C : D.children()) {
      if (C.getTag() != dwarf::DW_TAG_enumerator) {
        continue;
      }
      auto V = C.find(dwarf::DW_AT_const_value);
      if (!V) {
        continue;
      }
      if (auto U = V->getAsUnsignedConstant()) {
        Items.push_back(DB.createEnumerator(name(C), *U, true));
      } else if (auto I = V->getAsSignedConstant()) {
        Items.push_back(
            DB.createEnumerator(name(C), static_cast<uint64_t>(*I), false));
      }
    }
    return DB.createEnumerationType(
        typeScope(S, D), name(D), file(S, D), line(D), byteSize(D) * 8, 0,
        DB.getOrCreateArray(Items), type(S, ref(D)));
  }

  llvm::DIType *array(UnitState &S, DWARFDie D) noexcept {
    auto &DB = *S.DB;
    auto *Elem = type(S, ref(D));
    if (!Elem) {
      Elem = uchar(S);
    }
    llvm::SmallVector<llvm::Metadata *, 4> Subs;
    for (auto C : D.children()) {
      if (C.getTag() != dwarf::DW_TAG_subrange_type) {
        continue;
      }
      const auto Lower = static_cast<int64_t>(
          dwarf::toUnsigned(C.find(dwarf::DW_AT_lower_bound), 0));
      int64_t Count = -1;
      if (auto N = dwarf::toUnsigned(C.find(dwarf::DW_AT_count))) {
        Count = static_cast<int64_t>(*N);
      } else if (auto U = dwarf::toUnsigned(C.find(dwarf::DW_AT_upper_bound))) {
        Count = static_cast<int64_t>(*U - static_cast<uint64_t>(Lower) + 1);
      }
      Subs.push_back(DB.getOrCreateSubrange(Lower, Count));
    }
    return DB.createArrayType(0, 0, Elem, DB.getOrCreateArray(Subs));
  }

  llvm::DISubroutineType *subroutine(UnitState &S, DWARFDie D) noexcept {
    llvm::SmallVector<llvm::Metadata *, 8> Sig;
    Sig.push_back(type(S, ref(D)));
    for (auto C : D.children()) {
      if (C.getTag() == dwarf::DW_TAG_formal_parameter) {
        Sig.push_back(type(S, ref(C)));
      }
    }
    return S.DB->createSubroutineType(S.DB->getOrCreateTypeArray(Sig));
  }

  llvm::Module &M;
  std::unique_ptr<DwarfReader> Reader;
  std::map<uint64_t, UnitState> Units;
  std::unordered_map<uint64_t, llvm::DIType *> Types;
  std::unordered_map<uint64_t, llvm::DISubprogram *> Subprograms;
  std::unordered_map<uint64_t, llvm::DIScope *> Lexical;
  std::map<std::pair<llvm::DIScope *, llvm::DIFile *>, llvm::DIScope *>
      FileScopes;
  std::unordered_map<std::string, uint64_t> PointerNames;
  UnitState Artificial;
  Part P;
  llvm::GlobalVariable *MemoryBase = nullptr;
};

void FunctionDebugInfoImpl::setLocation(LLVMBuilderRef B,
                                        uint64_t FileOffset) noexcept {
  if (!Die) {
    setArtificialLocation(B);
    return;
  }
  auto &Builder = *llvm::unwrap(B);
  const auto Addr = Parent.toAddress(FileOffset);
  if (Addr < CachedLow || Addr >= CachedHigh) {
    std::tie(CachedScope, CachedInlinedAt) = Parent.scopeAt(S, SP, Die, Addr);
    const auto It =
        std::upper_bound(ScopeBounds.begin(), ScopeBounds.end(), Addr);
    CachedHigh = It == ScopeBounds.end() ? UINT64_MAX : *It;
    CachedLow = It == ScopeBounds.begin() ? 0 : *std::prev(It);
  }
  unsigned Line = 0;
  unsigned Column = 0;
  llvm::DIScope *Scope = CachedScope;
  if (LineTable) {
    const auto Row = LineTable->lookupAddress(
        {Addr, llvm::object::SectionedAddress::UndefSection});
    if (Row != UINT32_MAX) {
      const auto &R = LineTable->Rows[Row];
      Line = R.Line;
      Column = R.Column;
      Scope = Parent.fileScope(S, Scope, lineFile(R.File));
    }
  }
  Builder.SetCurrentDebugLocation(llvm::DILocation::get(
      Fn.getContext(), Line, Column, Scope, CachedInlinedAt));
}

llvm::DIFile *FunctionDebugInfoImpl::lineFile(uint64_t FileIndex) noexcept {
  if (auto It = LineFiles.find(FileIndex); It != LineFiles.end()) {
    return It->second;
  }
  std::string Path;
  llvm::DIFile *File = nullptr;
  if (LineTable->getFileNameByIndex(
          FileIndex, Die.getDwarfUnit()->getCompilationDir(),
          llvm::DILineInfoSpecifier::FileLineInfoKind::AbsoluteFilePath,
          Path)) {
    File = Parent.fileFor(S, Path, "");
  }
  LineFiles.emplace(FileIndex, File);
  return File;
}

} // namespace

std::unique_ptr<ModuleDebugInfo> ModuleDebugInfo::create(const AST::Module &Mod,
                                                         LLVMModuleRef M,
                                                         Part P) noexcept {
  auto Reader = DwarfReader::create(Mod);
  if (!Reader) {
    spdlog::info("debug info: module has no DWARF sections"sv);
    return nullptr;
  }
  return std::make_unique<ModuleDebugInfoImpl>(*llvm::unwrap(M),
                                               std::move(Reader), P);
}

} // namespace WasmEdge::LLVM::DebugInfo
