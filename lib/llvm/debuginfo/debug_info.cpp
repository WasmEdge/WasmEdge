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
#include <string>
#include <unordered_map>

namespace WasmEdge::LLVM::DebugInfo {

using namespace std::literals;
using llvm::DWARFDie;
namespace dwarf = llvm::dwarf;

namespace {

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

class ModuleDebugInfoImpl final : public ModuleDebugInfo {
public:
  ModuleDebugInfoImpl(llvm::Module &M, std::unique_ptr<DwarfReader> R) noexcept
      : M(M), Reader(std::move(R)) {
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
  beginFunction(uint32_t, uint32_t, LLVMValueRef) noexcept override {
    return nullptr;
  }

  void finalize() noexcept override {
    for (auto &[Offset, U] : Units) {
      U.DB->finalize();
    }
  }

  bool verifyOrStrip() noexcept override {
    std::string Msg;
    llvm::raw_string_ostream OS(Msg);
    bool BrokenDebugInfo = false;
    if (llvm::verifyModule(M, &OS, &BrokenDebugInfo)) {
      return false;
    }
    if (BrokenDebugInfo) {
      spdlog::warn("debug info: verifier rejected debug info, stripped: {}"sv,
                   OS.str());
      llvm::StripDebugInfo(M);
    }
    return true;
  }

private:
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

  llvm::DIFile *fileFor(UnitState &S, llvm::StringRef Path,
                        llvm::StringRef Dir) noexcept {
    if (llvm::sys::path::is_absolute(Path)) {
      return S.DB->createFile(llvm::sys::path::filename(Path),
                              llvm::sys::path::parent_path(Path));
    }
    return S.DB->createFile(Path, Dir);
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
    return D.getAttributeValueAsReferencedDie(dwarf::DW_AT_type);
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
      T = DB.createTypedef(type(S, ref(D)), name(D), file(S, D), line(D), S.CU);
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
        dwarf::DW_TAG_structure_type, "__wasm_ptr", S.CU, nullptr, 0, 0, 32, 32,
        llvm::DINode::FlagZero);
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
        D.getTag(), name(D), S.CU, file(S, D), line(D), 0, byteSize(D) * 8, 0,
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
        S.CU, name(D), file(S, D), line(D), byteSize(D) * 8, 0,
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
        Count = static_cast<int64_t>(*U) - Lower + 1;
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
  llvm::GlobalVariable *MemoryBase = nullptr;
};

} // namespace

std::unique_ptr<ModuleDebugInfo> ModuleDebugInfo::create(const AST::Module &Mod,
                                                         LLVMModuleRef M,
                                                         Part) noexcept {
  auto Reader = DwarfReader::create(Mod);
  if (!Reader) {
    spdlog::info("debug info: module has no DWARF sections"sv);
    return nullptr;
  }
  return std::make_unique<ModuleDebugInfoImpl>(*llvm::unwrap(M),
                                               std::move(Reader));
}

} // namespace WasmEdge::LLVM::DebugInfo
