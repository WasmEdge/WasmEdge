// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "debuginfo/module_debug_info.h"
#include "debuginfo/debug_info.h"
#include "debuginfo/dwarf_reader.h"
#include "debuginfo/function_debug_info.h"

#include "common/spdlog.h"

#include <llvm/BinaryFormat/Dwarf.h>
#include <llvm/Config/llvm-config.h>
#include <llvm/DebugInfo/DWARF/DWARFCompileUnit.h>
#include <llvm/DebugInfo/DWARF/DWARFContext.h>
#include <llvm/DebugInfo/DWARF/DWARFDie.h>
#include <llvm/DebugInfo/DWARF/DWARFFormValue.h>
#include <llvm/DebugInfo/DWARF/DWARFUnit.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DIBuilder.h>
#include <llvm/IR/DebugInfo.h>
#include <llvm/IR/DebugInfoMetadata.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/raw_ostream.h>
#if LLVM_VERSION_MAJOR < 16
#include <llvm/ADT/Triple.h>
#else
#include <llvm/TargetParser/Triple.h>
#endif

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>

namespace WasmEdge::LLVM::DebugInfo {

using namespace std::literals;

namespace {

llvm::DIGlobalVariableExpression *
globalVariable(llvm::DIBuilder &DB, llvm::DIScope *Scope, llvm::StringRef Name,
               llvm::DIFile *File, unsigned Line, llvm::DIType *Type,
               bool LocalToUnit, llvm::DIExpression *Expr) noexcept {
  return DB.createGlobalVariableExpression(Scope, Name, "", File, Line, Type,
                                           LocalToUnit,
#if LLVM_VERSION_MAJOR < 10
#else
                                           true,
#endif
                                           Expr);
}

} // namespace

ModuleDebugInfoImpl::ModuleDebugInfoImpl(llvm::Module &M,
                                         std::shared_ptr<DwarfReader> R, Part P,
                                         bool HasMemory,
                                         bool LinesOnly) noexcept
    : M(M), Reader(std::move(R)),
      Translator(M.getContext(),
                 llvm::Triple(M.getTargetTriple()).isArch32Bit() ? 32 : 64),
      P(P), LinesOnly(LinesOnly) {
  M.addModuleFlag(llvm::Module::Warning, "Debug Info Version",
                  llvm::DEBUG_METADATA_VERSION);
  M.addModuleFlag(llvm::Module::Warning, "Dwarf Version", 4);
  if (HasMemory && !LinesOnly && Reader->hasGlobalVariables()) {
    createMemoryBase();
  }
  if (!LinesOnly && P != Part::All) {
    addModuleId();
  }
  if (!LinesOnly && P != Part::FunctionsOnly) {
    addUnitGlobals();
  }
}

LLVMMetadataRef
ModuleDebugInfoImpl::translateType(uint64_t DieOffset) noexcept {
  auto Die = Reader->getContext().getDIEForOffset(DieOffset);
  if (!Die) {
    return nullptr;
  }
  return llvm::wrap(Translator.type(unit(Die), Die));
}

std::unique_ptr<FunctionDebugInfo>
ModuleDebugInfoImpl::beginFunction(uint32_t FuncIndex, uint32_t DefinedIndex,
                                   LLVMValueRef Fn,
                                   bool ExtendLiveness) noexcept {
  if (P == Part::GlobalsOnly) {
    return nullptr;
  }
  auto &F = *llvm::cast<llvm::Function>(llvm::unwrap(Fn));
  if (auto Die = Reader->getSubprogram(DefinedIndex)) {
    auto &S = unit(Die);
    auto *SP = subprogramFor(Die);
    if (!Attached.insert(SP).second) {
      SP = createSubprogram(S, Die);
    }
    return std::make_unique<FunctionDebugInfoImpl>(*this, S, F, SP, Die,
                                                   ExtendLiveness);
  }
  auto &S = artificialUnit();
  auto *SP = S.DB->createFunction(
      S.CU->getFile(), Reader->getFunctionName(FuncIndex), F.getName(),
      S.CU->getFile(), 0,
      S.DB->createSubroutineType(S.DB->getOrCreateTypeArray({})), 0,
      llvm::DINode::FlagArtificial, llvm::DISubprogram::SPFlagDefinition);
  return std::make_unique<FunctionDebugInfoImpl>(*this, S, F, SP, DWARFDie(),
                                                 ExtendLiveness);
}

void ModuleDebugInfoImpl::finalize() noexcept {
  if (Finalized) {
    return;
  }
  Finalized = true;
  for (auto &[Offset, U] : Units) {
    U.DB->finalize();
  }
  if (Artificial.DB) {
    Artificial.DB->finalize();
  }
}

std::optional<std::string> ModuleDebugInfoImpl::verify() noexcept {
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

std::pair<llvm::DIScope *, llvm::DILocation *>
ModuleDebugInfoImpl::scopeAt(UnitState &S, llvm::DISubprogram *SP,
                             DWARFDie FnDie, uint64_t Addr,
                             const ScopeIndex &Scopes) noexcept {
  auto &Ctx = M.getContext();
  llvm::SmallVector<DWARFDie, 4> Chain;
  inlinedChain(*FnDie.getDwarfUnit(), Addr, Chain);
  llvm::DIScope *Outer = scopeFor(S, SP, FnDie, Addr, Scopes);
  if (Chain.empty() || Chain.back() != FnDie) {
    return {Outer, nullptr};
  }
  llvm::DILocation *InlinedAt = nullptr;
  for (size_t I = Chain.size() - 1; I > 0; --I) {
    auto Call = Chain[I - 1];
    auto CallPath = callFile(Call);
    auto *CallScope = CallPath.empty()
                          ? Outer
                          : fileScope(S, Outer, fileFor(S, CallPath, ""));
    InlinedAt = llvm::DILocation::get(
        Ctx,
        static_cast<unsigned>(
            dwarf::toUnsigned(Call.find(dwarf::DW_AT_call_line), 0)),
        static_cast<unsigned>(
            dwarf::toUnsigned(Call.find(dwarf::DW_AT_call_column), 0)),
        CallScope, InlinedAt);
    auto Origin =
        Call.getAttributeValueAsReferencedDie(dwarf::DW_AT_abstract_origin);
    Outer =
        scopeFor(S, subprogramFor(Origin ? Origin : Call), Call, Addr, Scopes);
  }
  return {Outer, InlinedAt};
}

llvm::DIScope *ModuleDebugInfoImpl::fileScope(UnitState &S,
                                              llvm::DIScope *Scope,
                                              llvm::DIFile *File,
                                              unsigned Discriminator) noexcept {
  if (!File) {
    File = Scope->getFile();
  }
  if (File == Scope->getFile() && Discriminator == 0) {
    return Scope;
  }
  auto &Cached = FileScopes[{Scope, File, Discriminator}];
  if (!Cached) {
    Cached = S.DB->createLexicalBlockFile(Scope, File, Discriminator);
  }
  return Cached;
}

llvm::DIScope *ModuleDebugInfoImpl::lexicalBlock(UnitState &S,
                                                 llvm::DIScope *Scope,
                                                 DWARFDie Block) noexcept {
  auto &Cached = Lexical[{Scope, Block.getOffset()}];
  if (!Cached) {
    Cached = S.DB->createLexicalBlock(Scope, Scope->getFile(), line(Block), 0);
  }
  return Cached;
}

llvm::DIFile *
ModuleDebugInfoImpl::variableFile(UnitState &S, DWARFDie Var,
                                  llvm::DIFile *Fallback) noexcept {
  auto Path = declFile(Var);
  return Path.empty() ? Fallback : fileFor(S, Path, "");
}

llvm::ConstantInt *ModuleDebugInfoImpl::constantOf(DWARFDie Var) noexcept {
  const auto Attr = Var.find(dwarf::DW_AT_const_value);
  const auto Type = integerType(ref(Var));
  if (!Attr || !Type) {
    return nullptr;
  }
  const auto Value = integerConstant(*Attr, Type->second);
  if (!Value) {
    return nullptr;
  }
  return integerValue(Type->first, Value->bits());
}

llvm::ConstantInt *ModuleDebugInfoImpl::integerValue(unsigned Width,
                                                     uint64_t Value) noexcept {
  const auto Mask = Width == 64 ? UINT64_MAX : (UINT64_C(1) << Width) - 1;
  return llvm::ConstantInt::get(llvm::IntegerType::get(M.getContext(), Width),
                                Value & Mask);
}

void ModuleDebugInfoImpl::addGlobal(UnitState &S, llvm::DIScope *Scope,
                                    DWARFDie D, uint64_t Address) noexcept {
  const bool Dead = isTombstoneDataAddress(Address);
  if (P == Part::FunctionsOnly || (!MemoryBase && !Dead) ||
      !Globals.insert(D.getOffset()).second) {
    return;
  }
  auto *Expr = Dead ? S.DB->createExpression()
                    : S.DB->createExpression(llvm::SmallVector<uint64_t, 3>{
                          dwarf::DW_OP_deref, dwarf::DW_OP_plus_uconst,
                          Address & 0xFFFFFFFFU});
  auto *GVE = globalVariable(
      *S.DB, Scope, name(D), file(S, D), line(D), Translator.variableType(S, D),
      Dead || !dwarf::toUnsigned(D.findRecursively(dwarf::DW_AT_external), 0),
      Expr);
  if (!Dead) {
    MemoryBase->addDebugInfo(GVE);
  }
}

void ModuleDebugInfoImpl::addConstantGlobal(UnitState &S, llvm::DIScope *Scope,
                                            DWARFDie D) noexcept {
  if (P == Part::FunctionsOnly) {
    return;
  }
  auto *Value = constantOf(D);
  if (!Value || !Globals.insert(D.getOffset()).second) {
    return;
  }
  globalVariable(
      *S.DB, Scope, name(D), file(S, D), line(D), Translator.variableType(S, D),
      !dwarf::toUnsigned(D.findRecursively(dwarf::DW_AT_external), 0),
      S.DB->createConstantValueExpression(Value->getZExtValue()));
}

void ModuleDebugInfoImpl::createMemoryBase() noexcept {
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
#if LLVM_VERSION_MAJOR < 10
  MemoryBase->setAlignment(8);
#else
  MemoryBase->setAlignment(llvm::Align(8));
#endif
  if (P == Part::FunctionsOnly) {
    return;
  }
  MemoryBase->setVisibility(llvm::GlobalValue::HiddenVisibility);
  if (llvm::Triple(M.getTargetTriple()).isOSBinFormatELF()) {
    MemoryBase->setSection(".data");
  }
  auto &A = artificialUnit();
  MemoryBase->addDebugInfo(globalVariable(
      *A.DB, A.CU, "__wasmedge_debug_membase", A.CU->getFile(), 0,
      A.DB->createPointerType(Translator.uchar(A), Translator.pointerBits()),
      true, A.DB->createExpression()));
}

void ModuleDebugInfoImpl::addModuleId() noexcept {
  auto &A = artificialUnit();
  globalVariable(
      *A.DB, A.CU, "__wasmedge_debug_module", A.CU->getFile(), 0,
      A.DB->createBasicType("unsigned long long", 64, dwarf::DW_ATE_unsigned),
      false,
      A.DB->createExpression(llvm::SmallVector<uint64_t, 3>{
          dwarf::DW_OP_constu, Reader->getModuleId(),
          dwarf::DW_OP_stack_value}));
}

void ModuleDebugInfoImpl::addUnitGlobals() noexcept {
  for (const auto &CU : Reader->getContext().compile_units()) {
    addScopeGlobals(CU->getUnitDIE(), 0);
  }
}

void ModuleDebugInfoImpl::addScopeGlobals(DWARFDie Parent,
                                          uint32_t Depth) noexcept {
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
    auto &S = unit(C);
    if (const auto Address = globalAddress(C)) {
      addGlobal(S, Translator.namespaceScope(S, Parent), C, *Address);
    } else if (!C.find(dwarf::DW_AT_location)) {
      addConstantGlobal(S, Translator.namespaceScope(S, Parent), C);
    }
  }
}

llvm::DISubprogram *ModuleDebugInfoImpl::subprogramFor(DWARFDie D) noexcept {
  if (auto It = Subprograms.find(D.getOffset()); It != Subprograms.end()) {
    return It->second;
  }
  auto *SP = createSubprogram(unit(D), D);
  Subprograms[D.getOffset()] = SP;
  return SP;
}

llvm::DISubprogram *ModuleDebugInfoImpl::createSubprogram(UnitState &S,
                                                          DWARFDie D) noexcept {
  auto *File = file(S, D);
  const char *Linkage =
      dwarf::toString(D.findRecursively({dwarf::DW_AT_MIPS_linkage_name,
                                         dwarf::DW_AT_linkage_name}),
                      nullptr);
  auto Flags = llvm::DISubprogram::SPFlagDefinition;
  if (!dwarf::toUnsigned(D.findRecursively(dwarf::DW_AT_external), 0)) {
    Flags |= llvm::DISubprogram::SPFlagLocalToUnit;
  }
  auto *Type = LinesOnly
                   ? S.DB->createSubroutineType(S.DB->getOrCreateTypeArray({}))
                   : Translator.subroutine(S, D);
  const auto Declared = declarationOf(D);
  const auto Parent = Declared.getParent();
  llvm::DIScope *Scope = File;
  llvm::DISubprogram *Method = nullptr;
  if (Parent && Parent.getTag() == dwarf::DW_TAG_namespace) {
    Scope = Translator.namespaceScope(S, Parent);
  } else if (Parent && isAggregate(Parent.getTag()) && !LinesOnly) {
    if (auto *Class = llvm::dyn_cast_or_null<llvm::DICompositeType>(
            Translator.typeScope(S, Declared))) {
      Scope = Class;
      Method = S.DB->createMethod(Class, name(D), Linkage ? Linkage : "",
                                  file(S, Declared), line(Declared), Type);
    }
  }
  auto *SP = S.DB->createFunction(
      Scope, name(D), Linkage ? Linkage : "", File, line(D), Type, line(D),
      llvm::DINode::FlagZero, Flags, nullptr, Method);
  return SP;
}

UnitState &ModuleDebugInfoImpl::artificialUnit() noexcept {
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

llvm::DIScope *
ModuleDebugInfoImpl::scopeFor(UnitState &S, llvm::DIScope *Parent,
                              DWARFDie Frame, uint64_t Addr,
                              const ScopeIndex &Scopes) noexcept {
  llvm::DIScope *Scope = Parent;
  llvm::SmallVector<DWARFDie, 4> Blocks;
  for (auto B = Scopes.innermost(Frame, Addr); B && B != Frame;
       B = B.getParent()) {
    Blocks.push_back(B);
  }
  for (auto It = Blocks.rbegin(); It != Blocks.rend(); ++It) {
    Scope = lexicalBlock(S, Scope, *It);
  }
  return Scope;
}

UnitState &ModuleDebugInfoImpl::unit(DWARFDie Die) noexcept {
  auto *U = Die.getDwarfUnit();
  auto &S = Units[U->getOffset()];
  if (S.DB) {
    return S;
  }
  auto CUDie = U->getUnitDIE();
  S.DB = std::make_unique<llvm::DIBuilder>(M);
  const auto Lang = static_cast<uint16_t>(
      dwarf::toUnsigned(CUDie.find(dwarf::DW_AT_language), dwarf::DW_LANG_C99));
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

std::unique_ptr<ModuleDebugInfo> ModuleDebugInfo::create(const AST::Module &Mod,
                                                         LLVMModuleRef M,
                                                         Part P) noexcept {
  std::optional<std::shared_ptr<DwarfReader>> Reader;
  return create(Mod, M, P, Reader);
}

std::unique_ptr<ModuleDebugInfo> ModuleDebugInfo::create(
    const AST::Module &Mod, LLVMModuleRef M, Part P,
    std::optional<std::shared_ptr<DwarfReader>> &Reader) noexcept {
  if (!Reader) {
    Reader.emplace(DwarfReader::create(Mod));
  }
  if (!*Reader) {
    if (P != Part::FunctionsOnly) {
      spdlog::info("debug info: module has no DWARF sections"sv);
    }
    return nullptr;
  }
  const AST::MemoryType *Memory = nullptr;
  for (const auto &Desc : Mod.getImportSection().getContent()) {
    if (Desc.getExternalType() == ExternalType::Memory) {
      Memory = &Desc.getExternalMemoryType();
      break;
    }
  }
  if (!Memory && !Mod.getMemorySection().getContent().empty()) {
    Memory = &Mod.getMemorySection().getContent().front();
  }
  const bool Memory64 = Memory && Memory->getLimit().is64();
  if (Memory64 && P != Part::FunctionsOnly) {
    spdlog::warn("debug info: 64-bit memories are not supported, so only "
                 "functions and lines get debug info"sv);
  }
  const llvm::Triple Target(llvm::unwrap(M)->getTargetTriple());
  const bool BigEndian =
      Target.getArch() != llvm::Triple::UnknownArch && !Target.isLittleEndian();
  if (BigEndian && !Memory64 && P != Part::FunctionsOnly) {
    spdlog::warn("debug info: wasm memory is little-endian and the target is "
                 "big-endian, so only functions and lines get debug info"sv);
  }
  return std::make_unique<ModuleDebugInfoImpl>(
      *llvm::unwrap(M), *Reader, P, Memory != nullptr, Memory64 || BigEndian);
}

} // namespace WasmEdge::LLVM::DebugInfo
