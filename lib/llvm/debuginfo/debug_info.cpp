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
#include <llvm/DebugInfo/DWARF/DWARFUnit.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DIBuilder.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/DebugInfo.h>
#include <llvm/IR/DebugInfoMetadata.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InlineAsm.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/ValueHandle.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/raw_ostream.h>
#if LLVM_VERSION_MAJOR < 16
#include <llvm/ADT/Triple.h>
#else
#include <llvm/Support/ModRef.h>
#include <llvm/TargetParser/Triple.h>
#endif

#include <algorithm>
#include <map>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

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

struct LocationEntry {
  uint64_t Low;
  uint64_t High;
  WasmLocation L;
};

struct VariableLocation {
  std::optional<WasmLocation> Single;
  std::vector<LocationEntry> Ranges;
};

bool sameLocation(const WasmLocation &L, const WasmLocation &R) noexcept {
  return L.K == R.K && L.Index == R.Index && L.Offset == R.Offset &&
         L.Indirect == R.Indirect;
}

#if LLVM_VERSION_MAJOR < 10
std::optional<std::vector<LocationEntry>>
locationList(DWARFDie D, const llvm::DWARFFormValue &Attr) noexcept {
  auto Offset = Attr.getAsSectionOffset();
  if (!Offset) {
    return std::nullopt;
  }
  auto *U = D.getDwarfUnit();
  const auto Section = U->getContext().getDWARFObj().getLocSection().Data;
  llvm::DataExtractor Data(Section, true, U->getAddressByteSize());
  const uint64_t Max = U->getAddressByteSize() == 4 ? UINT32_MAX : UINT64_MAX;
  uint64_t Base = 0;
  if (auto B = U->getBaseAddress()) {
    Base = B->Address;
  }
  uint32_t Cursor = static_cast<uint32_t>(*Offset);
  std::vector<LocationEntry> Entries;
  while (Data.isValidOffsetForDataOfSize(Cursor, 2 * U->getAddressByteSize())) {
    const auto Begin = Data.getAddress(&Cursor);
    const auto End = Data.getAddress(&Cursor);
    if (Begin == 0 && End == 0) {
      return Entries;
    }
    if (Begin == Max) {
      Base = End;
      continue;
    }
    const auto Size = Data.getU16(&Cursor);
    if (!Data.isValidOffsetForDataOfSize(Cursor, Size)) {
      return std::nullopt;
    }
    Entries.push_back({Base + Begin, Base + End,
                       decodeLocation(llvm::ArrayRef<uint8_t>(
                                          Section.bytes_begin() + Cursor, Size),
                                      U)});
    Cursor += Size;
  }
  return std::nullopt;
}
#endif

std::optional<VariableLocation> locationOf(DWARFDie D) noexcept {
  auto Attr = D.find(dwarf::DW_AT_location);
  if (!Attr) {
    return std::nullopt;
  }
  if (auto Block = Attr->getAsBlock()) {
    return VariableLocation{decodeLocation(*Block, D.getDwarfUnit()), {}};
  }
  VariableLocation Result;
#if LLVM_VERSION_MAJOR < 10
  auto Locs = locationList(D, *Attr);
  if (!Locs) {
    return std::nullopt;
  }
  Result.Ranges = std::move(*Locs);
#else
  auto Locs = D.getLocations(dwarf::DW_AT_location);
  if (!Locs) {
    llvm::consumeError(Locs.takeError());
    return std::nullopt;
  }
  for (const auto &E : *Locs) {
    const auto L = decodeLocation(E.Expr, D.getDwarfUnit());
    if (E.Range) {
      Result.Ranges.push_back({E.Range->LowPC, E.Range->HighPC, L});
    } else {
      Result.Ranges.push_back({0, UINT64_MAX, L});
    }
  }
#endif
  Result.Ranges.erase(std::remove_if(Result.Ranges.begin(), Result.Ranges.end(),
                                     [](const LocationEntry &E) {
                                       return E.Low >= E.High ||
                                              E.L.K == WasmLocation::Kind::None;
                                     }),
                      Result.Ranges.end());
  if (Result.Ranges.empty()) {
    return std::nullopt;
  }
  return Result;
}

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
                        DWARFDie Die) noexcept;

  void setPrologueLocation(LLVMBuilderRef B) noexcept override {
    setArtificialLocation(B);
  }

  void setArtificialLocation(LLVMBuilderRef B) noexcept override {
    llvm::unwrap(B)->SetCurrentDebugLocation(
        llvm::DILocation::get(Fn.getContext(), 0, 0, SP));
  }

  void setLocation(LLVMBuilderRef B, uint64_t FileOffset) noexcept override;

  Span<const AddressSlot> getAddressSlots() const noexcept override {
    return AddressSlots;
  }

  void bindFrame(Span<const LLVMValueRef> LocalRefs, LLVMValueRef Memory,
                 Span<const LLVMValueRef> SlotRefs,
                 bool InMemory) noexcept override {
    LocalsInMemory = InMemory;
    Locals.clear();
    for (auto *L : LocalRefs) {
      Locals.push_back(llvm::unwrap(L));
    }
    MemorySlot = llvm::unwrap(Memory);
    Slots.clear();
    for (auto *Slot : SlotRefs) {
      Slots.push_back(llvm::unwrap(Slot));
    }
  }

  void localWritten(LLVMBuilderRef B, uint32_t Index,
                    LLVMValueRef Value) noexcept override;

  void finish(LLVMBasicBlockRef EntryRef,
              bool ExtendLiveness) noexcept override;

private:
#if LLVM_VERSION_MAJOR < 20
  using InsertPoint = llvm::Instruction *;
#else
  using InsertPoint = llvm::BasicBlock::iterator;
#endif
  static constexpr size_t NoIndex = SIZE_MAX;

  struct LocalVariable {
    uint64_t ScopeOffset;
    const llvm::DIScope *Scope;
    size_t Index;
  };

  struct Variable {
    llvm::DILocalVariable *Var = nullptr;
    llvm::DILocation *Loc = nullptr;
    uint64_t ScopeOffset = 0;
    const llvm::DIScope *Scope = nullptr;
    WasmLocation L;
    size_t Slot = NoIndex;
    std::vector<LocationEntry> Ranges;
    size_t Active = NoIndex;
  };

  struct ScopeEnd {
    uint64_t End;
    uint64_t Offset;
  };

  struct ScopeExit {
    llvm::WeakVH Block;
    llvm::WeakVH Last;
    bool HasLast;
    std::vector<uint64_t> Offsets;
  };

  size_t addressSlot(AddressSlot::Kind K, uint64_t Index) noexcept;
  bool placeVariable(Variable &V, const WasmLocation &L) noexcept;
  void collectVariables(DWARFDie Node, llvm::DIScope *Scope, unsigned &ArgNo,
                        uint32_t Depth) noexcept;
  void collectScopes(DWARFDie Node, uint32_t Depth, bool Inlined) noexcept;
  void recordScopeExits(llvm::IRBuilderBase &B, uint64_t Addr) noexcept;
  void updateRanges(llvm::IRBuilderBase &B, uint64_t Addr) noexcept;
  void insertValue(llvm::IRBuilderBase &B, llvm::Value *V, const Variable &Var,
                   bool Deref = false) noexcept;
  llvm::DIFile *lineFile(uint64_t FileIndex) noexcept;
  static void keepInMemory(llvm::Value *Slot) noexcept;
  void addFakeUses(const std::vector<LocalVariable> &Declared) noexcept;

  ModuleDebugInfoImpl &Parent;
  UnitState &S;
  llvm::Function &Fn;
  llvm::DISubprogram *SP;
  DWARFDie Die;
  const llvm::DWARFDebugLine::LineTable *LineTable = nullptr;
  size_t FrameBaseSlot = NoIndex;
  std::vector<AddressSlot> AddressSlots;
  std::vector<llvm::Value *> Locals;
  llvm::Value *MemorySlot = nullptr;
  std::vector<llvm::Value *> Slots;
  bool LocalsInMemory = false;
  std::vector<Variable> Variables;
  std::vector<std::pair<uint64_t, size_t>> RangeEvents;
  size_t NextRangeEvent = 0;
  std::unordered_map<uint32_t, std::vector<size_t>> LocalUsers;
  std::vector<uint64_t> ScopeBounds;
  uint64_t CachedLow = 1;
  uint64_t CachedHigh = 0;
  llvm::DIScope *CachedScope = nullptr;
  llvm::DILocation *CachedInlinedAt = nullptr;
  std::unordered_map<uint64_t, llvm::DIFile *> LineFiles;
  std::vector<ScopeEnd> ScopeEnds;
  std::unordered_set<uint64_t> RangedScopes;
  size_t NextScopeEnd = 0;
  std::vector<ScopeExit> ScopeExits;
};

class ModuleDebugInfoImpl final : public ModuleDebugInfo {
public:
  ModuleDebugInfoImpl(llvm::Module &M, std::shared_ptr<DwarfReader> R, Part P,
                      bool HasMemory) noexcept
      : M(M), Reader(std::move(R)), P(P) {
    M.addModuleFlag(llvm::Module::Warning, "Debug Info Version",
                    llvm::DEBUG_METADATA_VERSION);
    M.addModuleFlag(llvm::Module::Warning, "Dwarf Version", 4);
    if (HasMemory) {
      createMemoryBase();
      if (P != Part::FunctionsOnly) {
        addUnitGlobals();
      }
    }
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

  llvm::DIScope *lexicalBlock(UnitState &S, llvm::DIScope *Scope,
                              DWARFDie Block) noexcept {
    auto &Cached = Lexical[Block.getOffset()];
    if (!Cached) {
      Cached =
          S.DB->createLexicalBlock(Scope, Scope->getFile(), line(Block), 0);
    }
    return Cached;
  }

  llvm::DIFile *variableFile(UnitState &S, DWARFDie Var,
                             llvm::DIFile *Fallback) noexcept {
    auto Path = declFile(Var);
    return Path.empty() ? Fallback : fileFor(S, Path, "");
  }

  llvm::DIFile *fileFor(UnitState &S, llvm::StringRef Path,
                        llvm::StringRef Dir) noexcept {
    if (llvm::sys::path::is_absolute(Path)) {
      return S.DB->createFile(llvm::sys::path::filename(Path),
                              llvm::sys::path::parent_path(Path));
    }
    return S.DB->createFile(Path, Dir);
  }

  llvm::DIType *variableType(UnitState &S, DWARFDie Var) noexcept {
    auto *T = type(S, ref(Var));
    return T ? T : opaque(S, 0);
  }

  void addGlobal(UnitState &S, llvm::DIScope *Scope, DWARFDie D,
                 uint64_t Address) noexcept {
    if (P == Part::FunctionsOnly || !MemoryBase ||
        !Globals.insert(D.getOffset()).second) {
      return;
    }
    const bool Dead = isTombstoneAddress(Address);
    auto *Expr = Dead ? S.DB->createExpression()
                      : S.DB->createExpression(llvm::SmallVector<uint64_t, 3>{
                            dwarf::DW_OP_deref, dwarf::DW_OP_plus_uconst,
                            Address & 0xFFFFFFFFU});
    auto *GVE = S.DB->createGlobalVariableExpression(
        Scope, name(D), "", file(S, D), line(D), variableType(S, D),
        Dead || !dwarf::toUnsigned(D.find(dwarf::DW_AT_external), 0),
#if LLVM_VERSION_MAJOR < 10
#else
        true,
#endif
        Expr);
    if (!Dead) {
      MemoryBase->addDebugInfo(GVE);
    }
  }

private:
  void createMemoryBase() noexcept {
#if LLVM_VERSION_MAJOR < 15
    auto *PtrTy = llvm::Type::getInt8PtrTy(M.getContext());
#else
    auto *PtrTy = llvm::PointerType::getUnqual(M.getContext());
#endif
    MemoryBase = new llvm::GlobalVariable(
        M, PtrTy, false, llvm::GlobalValue::ExternalLinkage,
        P == Part::FunctionsOnly ? nullptr
                                 : llvm::ConstantPointerNull::get(PtrTy),
        "__wasmedge_debug_membase");
    MemoryBase->setVisibility(llvm::GlobalValue::HiddenVisibility);
#if LLVM_VERSION_MAJOR < 10
    MemoryBase->setAlignment(8);
#else
    MemoryBase->setAlignment(llvm::Align(8));
#endif
    if (P == Part::FunctionsOnly) {
      return;
    }
    if (llvm::Triple(M.getTargetTriple()).isOSBinFormatELF()) {
      MemoryBase->setSection(".data");
    }
    auto &A = artificialUnit();
    MemoryBase->addDebugInfo(A.DB->createGlobalVariableExpression(
        A.CU, "__wasmedge_debug_membase", "", A.CU->getFile(), 0,
        A.DB->createPointerType(uchar(A), 64), true,
#if LLVM_VERSION_MAJOR < 10
#else
        true,
#endif
        A.DB->createExpression()));
  }

  void addUnitGlobals() noexcept {
    for (const auto &CU : Reader->getContext().compile_units()) {
      addScopeGlobals(CU->getUnitDIE(), 0);
    }
  }

  void addScopeGlobals(DWARFDie Parent, uint32_t Depth) noexcept {
    for (auto C : Parent.children()) {
      const auto Tag = C.getTag();
      if (Tag == dwarf::DW_TAG_namespace) {
        if (Depth < MaxDepth) {
          addScopeGlobals(C, Depth + 1);
        }
        continue;
      }
      if (Tag != dwarf::DW_TAG_variable) {
        continue;
      }
      if (auto L = locationOf(C);
          L && L->Single && L->Single->K == WasmLocation::Kind::Address) {
        auto &S = unit(C);
        addGlobal(S, namespaceScope(S, Parent), C, L->Single->Index);
      }
    }
  }

  llvm::DIScope *namespaceScope(UnitState &S, DWARFDie D) noexcept {
    if (D.getTag() != dwarf::DW_TAG_namespace) {
      return S.CU;
    }
    if (auto It = Namespaces.find(D.getOffset()); It != Namespaces.end()) {
      return It->second;
    }
    auto *Scope =
        S.DB->createNameSpace(namespaceScope(S, D.getParent()), name(D), false);
    Namespaces[D.getOffset()] = Scope;
    return Scope;
  }

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
      Scope = lexicalBlock(S, Scope, *It);
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
    if (TypeDepth >= MaxDepth) {
      return nullptr;
    }
    Types[D.getOffset()] = nullptr;
    ++TypeDepth;
    auto *T = buildType(S, D);
    --TypeDepth;
    Types[D.getOffset()] = T;
    if (TypeDepth == 0) {
      completePointers();
    }
    return T;
  }

  llvm::DIType *buildType(UnitState &S, DWARFDie D) noexcept {
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
    auto *T = S.DB->createReplaceableCompositeType(
        dwarf::DW_TAG_structure_type, pointerName(Pointee), S.CU, nullptr, 0, 0,
        32, 32, llvm::DINode::FlagZero);
    PendingPointers.push_back({&S, T, Pointee});
    return T;
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
    case dwarf::DW_TAG_namespace:
      return namespaceScope(S, P);
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

  void completePointers() noexcept {
    ++TypeDepth;
    while (!PendingPointers.empty()) {
      const auto Pending = PendingPointers.back();
      PendingPointers.pop_back();
      completePointer(*Pending.S, Pending.T, Pending.Pointee);
    }
    --TypeDepth;
  }

  void completePointer(UnitState &S, llvm::DICompositeType *T,
                       DWARFDie Pointee) noexcept {
    auto &DB = *S.DB;
    llvm::DIType *Elem = Pointee ? type(S, Pointee) : nullptr;
    if (!Elem) {
      Elem = uchar(S);
    }
    auto *Addr = DB.createMemberType(T, "__addr", nullptr, 0, 32, 32, 0,
                                     llvm::DINode::FlagZero, uint32(S));
    auto *Arr = DB.createArrayType(
        0, 0, DB.createPointerType(Elem, 64),
        DB.getOrCreateArray({DB.getOrCreateSubrange(int64_t(0), int64_t(0))}));
    auto *Pt = DB.createMemberType(T, "__pointee", nullptr, 0, uint64_t(0), 0,
                                   uint64_t(0), llvm::DINode::FlagZero, Arr);
    llvm::Metadata *Fields[] = {Addr, Pt};
    DB.replaceArrays(T, DB.getOrCreateArray(Fields));
    llvm::MDNode::replaceWithDistinct(llvm::TempDICompositeType(T));
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

  static bool isStaticMember(DWARFDie C) noexcept {
    return !C.find(dwarf::DW_AT_data_member_location) &&
           (dwarf::toUnsigned(C.find(dwarf::DW_AT_declaration), 0) ||
            dwarf::toUnsigned(C.find(dwarf::DW_AT_external), 0));
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
        if (!isStaticMember(C)) {
          E = member(S, T, C);
        }
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

  struct PendingPointer {
    UnitState *S;
    llvm::DICompositeType *T;
    DWARFDie Pointee;
  };

  llvm::Module &M;
  std::shared_ptr<DwarfReader> Reader;
  std::map<uint64_t, UnitState> Units;
  std::unordered_map<uint64_t, llvm::DIType *> Types;
  std::unordered_map<uint64_t, llvm::DISubprogram *> Subprograms;
  std::unordered_map<uint64_t, llvm::DIScope *> Lexical;
  std::map<std::pair<llvm::DIScope *, llvm::DIFile *>, llvm::DIScope *>
      FileScopes;
  std::unordered_map<std::string, uint64_t> PointerNames;
  std::unordered_map<uint64_t, llvm::DIScope *> Namespaces;
  std::unordered_set<uint64_t> Globals;
  std::vector<PendingPointer> PendingPointers;
  uint32_t TypeDepth = 0;
  UnitState Artificial;
  Part P;
  llvm::GlobalVariable *MemoryBase = nullptr;
};

FunctionDebugInfoImpl::FunctionDebugInfoImpl(ModuleDebugInfoImpl &Parent,
                                             UnitState &S, llvm::Function &Fn,
                                             llvm::DISubprogram *SP,
                                             DWARFDie Die) noexcept
    : Parent(Parent), S(S), Fn(Fn), SP(SP), Die(Die) {
  Fn.setSubprogram(SP);
  if (!Die) {
    return;
  }
  auto *U = Die.getDwarfUnit();
  LineTable = U->getContext().getLineTableForUnit(U);
  if (auto FB = Die.find(dwarf::DW_AT_frame_base)) {
    if (auto Block = FB->getAsBlock()) {
      const auto L = decodeLocation(*Block, U);
      if (L.K == WasmLocation::Kind::Local) {
        FrameBaseSlot = addressSlot(AddressSlot::Kind::Local, L.Index);
      } else if (L.K == WasmLocation::Kind::Global) {
        FrameBaseSlot = addressSlot(AddressSlot::Kind::Global, L.Index);
      }
    }
  }
  collectScopes(Die, 0, false);
  std::stable_sort(
      ScopeEnds.begin(), ScopeEnds.end(),
      [](const ScopeEnd &L, const ScopeEnd &R) { return L.End < R.End; });
  std::sort(ScopeBounds.begin(), ScopeBounds.end());
  ScopeBounds.erase(std::unique(ScopeBounds.begin(), ScopeBounds.end()),
                    ScopeBounds.end());
  unsigned ArgNo = 0;
  collectVariables(Die, SP, ArgNo, 0);
  for (size_t I = 0; I < Variables.size(); ++I) {
    for (const auto &E : Variables[I].Ranges) {
      RangeEvents.emplace_back(E.Low, I);
      RangeEvents.emplace_back(E.High, I);
      auto &Users = LocalUsers[static_cast<uint32_t>(E.L.Index)];
      if (Users.empty() || Users.back() != I) {
        Users.push_back(I);
      }
    }
  }
  std::sort(RangeEvents.begin(), RangeEvents.end());
}

size_t FunctionDebugInfoImpl::addressSlot(AddressSlot::Kind K,
                                          uint64_t Index) noexcept {
  const auto I = static_cast<uint32_t>(Index);
  for (size_t N = 0; N < AddressSlots.size(); ++N) {
    if (AddressSlots[N].K == K && AddressSlots[N].Index == I) {
      return N;
    }
  }
  AddressSlots.push_back({K, I});
  return AddressSlots.size() - 1;
}

bool FunctionDebugInfoImpl::placeVariable(Variable &V,
                                          const WasmLocation &L) noexcept {
  V.L = L;
  switch (L.K) {
  case WasmLocation::Kind::Local:
    if (L.Indirect) {
      V.Slot = addressSlot(AddressSlot::Kind::Local, L.Index);
    }
    return true;
  case WasmLocation::Kind::Global:
    V.Slot = addressSlot(L.Indirect ? AddressSlot::Kind::Global
                                    : AddressSlot::Kind::GlobalStorage,
                         L.Index);
    return true;
  case WasmLocation::Kind::FrameBaseOffset:
    V.Slot = FrameBaseSlot;
    return FrameBaseSlot != NoIndex;
  default:
    return false;
  }
}

void FunctionDebugInfoImpl::collectVariables(DWARFDie Node,
                                             llvm::DIScope *Scope,
                                             unsigned &ArgNo,
                                             uint32_t Depth) noexcept {
  auto &DB = *S.DB;
  for (auto C : Node.children()) {
    const auto Tag = C.getTag();
    if (Tag == dwarf::DW_TAG_lexical_block) {
      if (Depth < MaxDepth) {
        collectVariables(C, Parent.lexicalBlock(S, Scope, C), ArgNo, Depth + 1);
      }
      continue;
    }
    if (Tag != dwarf::DW_TAG_formal_parameter &&
        Tag != dwarf::DW_TAG_variable) {
      continue;
    }
    const bool IsParam = Tag == dwarf::DW_TAG_formal_parameter;
    if (IsParam) {
      ++ArgNo;
    }
    auto Loc = locationOf(C);
    if (!Loc) {
      continue;
    }
    if (Loc->Single && Loc->Single->K == WasmLocation::Kind::Address) {
      Parent.addGlobal(S, Scope, C, Loc->Single->Index);
      continue;
    }
    Variable V;
    V.ScopeOffset = Node.getOffset();
    V.Scope = Scope;
    if (Loc->Single) {
      if (!placeVariable(V, *Loc->Single)) {
        continue;
      }
    } else {
      const auto &First = Loc->Ranges.front().L;
      const bool Same = std::all_of(
          Loc->Ranges.begin(), Loc->Ranges.end(),
          [&](const LocationEntry &E) { return sameLocation(E.L, First); });
      const bool LocalValue =
          First.K == WasmLocation::Kind::Local && !First.Indirect;
      if (Same && !LocalValue && First.K != WasmLocation::Kind::Address) {
        if (!placeVariable(V, First)) {
          continue;
        }
      } else {
        for (const auto &E : Loc->Ranges) {
          if (E.L.K == WasmLocation::Kind::Local && !E.L.Indirect) {
            V.Ranges.push_back(E);
          }
        }
        if (V.Ranges.empty()) {
          continue;
        }
      }
    }
    auto *Ty = Parent.variableType(S, C);
    const char *Name = C.getName(llvm::DINameKind::ShortName);
    const auto Line = static_cast<unsigned>(C.getDeclLine());
    auto *File = Parent.variableFile(S, C, SP->getFile());
    V.Var = IsParam ? DB.createParameterVariable(Scope, Name ? Name : "", ArgNo,
                                                 File, Line, Ty, true)
                    : DB.createAutoVariable(Scope, Name ? Name : "", File, Line,
                                            Ty, true);
    V.Loc = llvm::DILocation::get(Fn.getContext(), Line, 0, Scope);
    Variables.push_back(std::move(V));
  }
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

void FunctionDebugInfoImpl::setLocation(LLVMBuilderRef B,
                                        uint64_t FileOffset) noexcept {
  if (!Die) {
    setArtificialLocation(B);
    return;
  }
  auto &Builder = *llvm::unwrap(B);
  const auto Addr = Parent.toAddress(FileOffset);
  recordScopeExits(Builder, Addr);
  updateRanges(Builder, Addr);
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

void FunctionDebugInfoImpl::insertValue(llvm::IRBuilderBase &B, llvm::Value *V,
                                        const Variable &Var,
                                        bool Deref) noexcept {
  auto *BB = B.GetInsertBlock();
  if (!BB) {
    return;
  }
  auto *Expr = Deref ? S.DB->createExpression(
                           llvm::SmallVector<uint64_t, 1>{dwarf::DW_OP_deref})
                     : S.DB->createExpression();
#if LLVM_VERSION_MAJOR < 20
  if (B.GetInsertPoint() == BB->end()) {
    S.DB->insertDbgValueIntrinsic(V, Var.Var, Expr, Var.Loc, BB);
  } else {
    S.DB->insertDbgValueIntrinsic(V, Var.Var, Expr, Var.Loc,
                                  &*B.GetInsertPoint());
  }
#elif LLVM_VERSION_MAJOR < 24
  S.DB->insertDbgValueIntrinsic(V, Var.Var, Expr, Var.Loc, B.GetInsertPoint());
#else
  S.DB->insertDbgValue(V, Var.Var, Expr, Var.Loc, B.GetInsertPoint());
#endif
}

void FunctionDebugInfoImpl::updateRanges(llvm::IRBuilderBase &B,
                                         uint64_t Addr) noexcept {
  if (NextRangeEvent >= RangeEvents.size() ||
      RangeEvents[NextRangeEvent].first > Addr) {
    return;
  }
  std::vector<size_t> Changed;
  for (; NextRangeEvent < RangeEvents.size() &&
         RangeEvents[NextRangeEvent].first <= Addr;
       ++NextRangeEvent) {
    Changed.push_back(RangeEvents[NextRangeEvent].second);
  }
  std::sort(Changed.begin(), Changed.end());
  Changed.erase(std::unique(Changed.begin(), Changed.end()), Changed.end());
  for (const auto I : Changed) {
    auto &V = Variables[I];
    size_t Active = NoIndex;
    for (size_t E = 0; E < V.Ranges.size(); ++E) {
      if (V.Ranges[E].Low <= Addr && Addr < V.Ranges[E].High) {
        Active = E;
        break;
      }
    }
    if (Active == V.Active) {
      continue;
    }
    V.Active = Active;
    if (Active == NoIndex || V.Ranges[Active].L.Index >= Locals.size()) {
      insertValue(
          B, llvm::UndefValue::get(llvm::Type::getInt32Ty(Fn.getContext())), V);
      continue;
    }
    auto *Slot = llvm::cast<llvm::AllocaInst>(Locals[V.Ranges[Active].L.Index]);
    if (LocalsInMemory) {
      insertValue(B, Slot, V, true);
    } else {
      insertValue(B, B.CreateLoad(Slot->getAllocatedType(), Slot), V);
    }
  }
}

void FunctionDebugInfoImpl::localWritten(LLVMBuilderRef B, uint32_t Index,
                                         LLVMValueRef Value) noexcept {
  auto It = LocalUsers.find(Index);
  if (It == LocalUsers.end()) {
    return;
  }
  if (LocalsInMemory) {
    return;
  }
  for (const auto I : It->second) {
    const auto &V = Variables[I];
    if (V.Active != NoIndex && V.Ranges[V.Active].L.Index == Index) {
      insertValue(*llvm::unwrap(B), llvm::unwrap(Value), V);
    }
  }
}

void FunctionDebugInfoImpl::finish(LLVMBasicBlockRef EntryRef,
                                   bool ExtendLiveness) noexcept {
  auto *Terminator = llvm::unwrap(EntryRef)->getTerminator();
  if (!Terminator) {
    return;
  }
  if (ExtendLiveness) {
    keepInMemory(MemorySlot);
    for (auto *Slot : Slots) {
      keepInMemory(Slot);
    }
  }
#if LLVM_VERSION_MAJOR < 20
  const InsertPoint Where = Terminator;
#else
  const InsertPoint Where = Terminator->getIterator();
#endif
  auto &DB = *S.DB;
  if (MemorySlot) {
    auto *Ty = DB.createPointerType(
        DB.createBasicType("unsigned char", 8, dwarf::DW_ATE_unsigned_char),
        64);
    auto *Var = DB.createAutoVariable(SP, "__wasm_memory", SP->getFile(), 0, Ty,
                                      true, llvm::DINode::FlagArtificial);
    DB.insertDeclare(
        MemorySlot, Var, DB.createExpression(),
        llvm::DILocation::get(Fn.getContext(), SP->getLine(), 0, SP), Where);
  }
  std::vector<LocalVariable> Declared;
  for (const auto &V : Variables) {
    if (!V.Ranges.empty()) {
      const auto Index = V.Ranges.front().L.Index;
      if (Index < Locals.size() &&
          std::all_of(
              V.Ranges.begin(), V.Ranges.end(),
              [Index](const LocationEntry &E) { return E.L.Index == Index; })) {
        Declared.push_back(
            {V.ScopeOffset, V.Scope, static_cast<size_t>(Index)});
      }
      continue;
    }
    if (V.Slot != NoIndex) {
      if (V.Slot >= Slots.size() || !Slots[V.Slot]) {
        continue;
      }
      llvm::SmallVector<uint64_t, 4> Ops = {dwarf::DW_OP_deref};
      if (V.L.K == WasmLocation::Kind::FrameBaseOffset) {
        if (V.L.Offset >= 0) {
          Ops.append(
              {dwarf::DW_OP_plus_uconst, static_cast<uint64_t>(V.L.Offset)});
        } else {
          Ops.append({dwarf::DW_OP_constu,
                      UINT64_C(0) - static_cast<uint64_t>(V.L.Offset),
                      dwarf::DW_OP_minus});
        }
      }
      DB.insertDeclare(Slots[V.Slot], V.Var, DB.createExpression(Ops), V.Loc,
                       Where);
      continue;
    }
    if (V.L.K == WasmLocation::Kind::Local && V.L.Index < Locals.size()) {
      DB.insertDeclare(Locals[V.L.Index], V.Var, DB.createExpression(), V.Loc,
                       Where);
      Declared.push_back(
          {V.ScopeOffset, V.Scope, static_cast<size_t>(V.L.Index)});
    }
  }
  if (ExtendLiveness) {
    addFakeUses(Declared);
  }
}

void FunctionDebugInfoImpl::collectScopes(DWARFDie Node, uint32_t Depth,
                                          bool Inlined) noexcept {
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
        if (R.LowPC >= R.HighPC) {
          continue;
        }
        ScopeBounds.push_back(R.LowPC);
        ScopeBounds.push_back(R.HighPC);
        if (Tag == dwarf::DW_TAG_lexical_block && !Inlined) {
          ScopeEnds.push_back({R.HighPC, C.getOffset()});
          RangedScopes.insert(C.getOffset());
        }
      }
    } else {
      llvm::consumeError(Ranges.takeError());
    }
    collectScopes(C, Depth + 1,
                  Inlined || Tag == dwarf::DW_TAG_inlined_subroutine);
  }
}

void FunctionDebugInfoImpl::recordScopeExits(llvm::IRBuilderBase &B,
                                             uint64_t Addr) noexcept {
  if (NextScopeEnd >= ScopeEnds.size() || ScopeEnds[NextScopeEnd].End > Addr) {
    return;
  }
  auto *BB = B.GetInsertBlock();
  if (!BB) {
    return;
  }
  const auto Pos = B.GetInsertPoint();
  ScopeExit Exit;
  Exit.Block = BB;
  Exit.HasLast = Pos != BB->begin();
  if (Exit.HasLast) {
    Exit.Last = &*std::prev(Pos);
  }
  for (; NextScopeEnd < ScopeEnds.size() && ScopeEnds[NextScopeEnd].End <= Addr;
       ++NextScopeEnd) {
    Exit.Offsets.push_back(ScopeEnds[NextScopeEnd].Offset);
  }
  ScopeExits.push_back(std::move(Exit));
}

void FunctionDebugInfoImpl::keepInMemory(llvm::Value *Slot) noexcept {
  if (!Slot) {
    return;
  }
  for (auto *U : Slot->users()) {
    if (auto *Load = llvm::dyn_cast<llvm::LoadInst>(U)) {
      Load->setVolatile(true);
    } else if (auto *Store = llvm::dyn_cast<llvm::StoreInst>(U);
               Store && Store->getPointerOperand() == Slot) {
      Store->setVolatile(true);
    }
  }
}

#if LLVM_VERSION_MAJOR < 20
void emitAsmFakeUse(llvm::IRBuilder<> &B, const llvm::Triple &TT,
                    llvm::Value *V) noexcept {
  std::vector<llvm::Value *> Ops;
  std::string Constraints;
  auto Add = [&](llvm::Value *Op, const char *C) {
    if (!Constraints.empty()) {
      Constraints += ',';
    }
    Constraints += C;
    Ops.push_back(Op);
  };
  auto *Ty = V->getType();
  if (!Ty->isFloatingPointTy() && !Ty->isVectorTy()) {
    Add(V, "r");
  } else if (TT.getArch() == llvm::Triple::x86 ||
             TT.getArch() == llvm::Triple::x86_64) {
    Add(V, "x");
  } else if (TT.isAArch64()) {
    Add(V, "w");
  } else if (Ty->isVectorTy()) {
#if LLVM_VERSION_MAJOR < 11
    auto *VecTy = llvm::VectorType::get(B.getInt64Ty(), 2);
#else
    auto *VecTy = llvm::FixedVectorType::get(B.getInt64Ty(), 2);
#endif
    auto *Vec = B.CreateBitCast(V, VecTy);
    Add(B.CreateExtractElement(Vec, uint64_t(0)), "r");
    Add(B.CreateExtractElement(Vec, uint64_t(1)), "r");
  } else {
    Add(B.CreateBitCast(V, B.getIntNTy(Ty->getPrimitiveSizeInBits())), "r");
  }
  std::vector<llvm::Type *> Types;
  for (auto *Op : Ops) {
    Types.push_back(Op->getType());
  }
  auto *FTy = llvm::FunctionType::get(B.getVoidTy(), Types, false);
  auto *Call =
      B.CreateCall(FTy, llvm::InlineAsm::get(FTy, "", Constraints, true), Ops);
  Call->setDoesNotThrow();
#if LLVM_VERSION_MAJOR < 14
  Call->addAttribute(llvm::AttributeList::FunctionIndex,
                     llvm::Attribute::InaccessibleMemOnly);
#elif LLVM_VERSION_MAJOR < 16
  Call->addFnAttr(llvm::Attribute::InaccessibleMemOnly);
#else
  Call->setMemoryEffects(llvm::MemoryEffects::inaccessibleMemOnly());
#endif
  Call->setTailCallKind(llvm::CallInst::TCK_NoTail);
}
#endif

void FunctionDebugInfoImpl::addFakeUses(
    const std::vector<LocalVariable> &Declared) noexcept {
  if (Declared.empty()) {
    return;
  }
  std::vector<bool> FunctionScope(Locals.size(), false);
  std::unordered_map<uint64_t, std::vector<size_t>> ScopeLocals;
  std::unordered_map<const llvm::DIScope *, uint64_t> ScopeOffsets;
  for (const auto &V : Declared) {
    if (RangedScopes.count(V.ScopeOffset)) {
      ScopeLocals[V.ScopeOffset].push_back(V.Index);
      ScopeOffsets.emplace(V.Scope, V.ScopeOffset);
    } else {
      FunctionScope[V.Index] = true;
    }
  }

  std::vector<std::pair<llvm::Instruction *, std::vector<bool>>> Points;
  std::unordered_map<llvm::Instruction *, size_t> PointIndex;
  auto Want = [&](llvm::Instruction *At) -> std::vector<bool> & {
    if (auto *Ret = llvm::dyn_cast<llvm::ReturnInst>(At)) {
      if (auto *Call =
              llvm::dyn_cast_or_null<llvm::CallInst>(Ret->getPrevNode());
          Call && Call->isTailCall()) {
        At = Call;
      }
    }
    auto [It, Inserted] = PointIndex.emplace(At, Points.size());
    if (Inserted) {
      Points.emplace_back(At, std::vector<bool>(Locals.size(), false));
    }
    return Points[It->second].second;
  };
  auto AddScope = [&](std::vector<bool> &Set, uint64_t Offset) {
    if (auto It = ScopeLocals.find(Offset); It != ScopeLocals.end()) {
      for (auto I : It->second) {
        Set[I] = true;
      }
    }
  };

  for (const auto &Exit : ScopeExits) {
    auto *BB = llvm::cast_or_null<llvm::BasicBlock>(Exit.Block);
    if (!BB) {
      continue;
    }
    llvm::Instruction *At = nullptr;
    if (Exit.HasLast) {
      if (auto *Last = llvm::cast_or_null<llvm::Instruction>(Exit.Last);
          Last && Last->getParent() == BB) {
        At = Last->getNextNode();
      }
    } else if (auto It = BB->getFirstInsertionPt(); It != BB->end()) {
      At = &*It;
    }
    if (!At || llvm::isa<llvm::PHINode>(At) || At->isEHPad()) {
      continue;
    }
    auto &Set = Want(At);
    for (auto Offset : Exit.Offsets) {
      AddScope(Set, Offset);
    }
  }
  for (auto &BB : Fn) {
    auto *Ret = llvm::dyn_cast_or_null<llvm::ReturnInst>(BB.getTerminator());
    if (!Ret) {
      continue;
    }
    auto &Set = Want(Ret);
    for (size_t I = 0; I < Locals.size(); ++I) {
      if (FunctionScope[I]) {
        Set[I] = true;
      }
    }
    const auto *Loc = Ret->getDebugLoc().get();
    for (const llvm::DIScope *Scope = Loc ? Loc->getInlinedAtScope() : nullptr;
         Scope && Scope != SP; Scope = Scope->getScope()) {
      if (auto It = ScopeOffsets.find(Scope); It != ScopeOffsets.end()) {
        AddScope(Set, It->second);
      }
    }
  }

#if LLVM_VERSION_MAJOR < 20
  const llvm::Triple TT(Fn.getParent()->getTargetTriple());
#else
  llvm::Function *FakeUse = nullptr;
#endif
  for (const auto &[At, Set] : Points) {
    llvm::IRBuilder<> B(At);
    B.SetCurrentDebugLocation(llvm::DebugLoc());
    for (size_t I = 0; I < Locals.size(); ++I) {
      auto *Slot = llvm::dyn_cast<llvm::AllocaInst>(Locals[I]);
      if (!Set[I] || !Slot) {
        continue;
      }
      auto *Value = B.CreateLoad(Slot->getAllocatedType(), Slot, "fake.use");
#if LLVM_VERSION_MAJOR < 20
      emitAsmFakeUse(B, TT, Value);
#else
      if (!FakeUse) {
        FakeUse = llvm::Intrinsic::getOrInsertDeclaration(
            Fn.getParent(), llvm::Intrinsic::fake_use);
      }
      auto *Call = B.CreateCall(FakeUse, {Value});
      Call->setDoesNotThrow();
      Call->setTailCallKind(llvm::CallInst::TCK_NoTail);
#endif
    }
  }
}

} // namespace

std::unique_ptr<ModuleDebugInfo> ModuleDebugInfo::create(const AST::Module &Mod,
                                                         LLVMModuleRef M,
                                                         Part P) noexcept {
  std::shared_ptr<DwarfReader> Reader;
  return create(Mod, M, P, Reader);
}

std::unique_ptr<ModuleDebugInfo>
ModuleDebugInfo::create(const AST::Module &Mod, LLVMModuleRef M, Part P,
                        std::shared_ptr<DwarfReader> &Reader) noexcept {
  if (!Reader) {
    Reader = DwarfReader::create(Mod);
  }
  if (!Reader) {
    if (P != Part::FunctionsOnly) {
      spdlog::info("debug info: module has no DWARF sections"sv);
    }
    return nullptr;
  }
  bool HasMemory = !Mod.getMemorySection().getContent().empty();
  for (const auto &Desc : Mod.getImportSection().getContent()) {
    HasMemory |= Desc.getExternalType() == ExternalType::Memory;
  }
  return std::make_unique<ModuleDebugInfoImpl>(*llvm::unwrap(M), Reader, P,
                                               HasMemory);
}

} // namespace WasmEdge::LLVM::DebugInfo
