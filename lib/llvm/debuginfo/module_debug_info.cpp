// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "debuginfo/module_debug_info.h"
#include "debuginfo/debug_info.h"
#include "debuginfo/dwarf_reader.h"
#include "debuginfo/function_debug_info.h"

#include "common/spdlog.h"

#include <llvm/BinaryFormat/Dwarf.h>
#include <llvm/Config/llvm-config.h>
#include <llvm/DebugInfo/DWARF/DWARFContext.h>
#include <llvm/DebugInfo/DWARF/DWARFDie.h>
#include <llvm/DebugInfo/DWARF/DWARFFormValue.h>
#include <llvm/DebugInfo/DWARF/DWARFUnit.h>
#include <llvm/IR/DIBuilder.h>
#include <llvm/IR/DebugInfo.h>
#include <llvm/IR/DebugInfoMetadata.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/raw_ostream.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>

namespace WasmEdge::LLVM::DebugInfo {

using namespace std::literals;

ModuleDebugInfoImpl::ModuleDebugInfoImpl(llvm::Module &M,
                                         std::unique_ptr<DwarfReader> R,
                                         Part P) noexcept
    : M(M), Reader(std::move(R)), Translator(M.getContext()), P(P) {
  M.addModuleFlag(llvm::Module::Warning, "Debug Info Version",
                  llvm::DEBUG_METADATA_VERSION);
  M.addModuleFlag(llvm::Module::Warning, "Dwarf Version", 4);
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
                                   LLVMValueRef Fn) noexcept {
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
    return std::make_unique<FunctionDebugInfoImpl>(*this, S, F, SP, Die);
  }
  auto &S = artificialUnit();
  auto *SP = S.DB->createFunction(
      S.CU->getFile(), Reader->getFunctionName(FuncIndex), F.getName(),
      S.CU->getFile(), 0,
      S.DB->createSubroutineType(S.DB->getOrCreateTypeArray({})), 0,
      llvm::DINode::FlagArtificial, llvm::DISubprogram::SPFlagDefinition);
  return std::make_unique<FunctionDebugInfoImpl>(*this, S, F, SP, DWARFDie());
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
  auto *SP = S.DB->createFunction(File, name(D), Linkage ? Linkage : "", File,
                                  line(D), Translator.subroutine(S, D), line(D),
                                  llvm::DINode::FlagZero, Flags);
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
    auto &Cached = Lexical[{Scope, It->getOffset()}];
    if (!Cached) {
      Cached = S.DB->createLexicalBlock(Scope, Scope->getFile(), line(*It), 0);
    }
    Scope = Cached;
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
  auto Reader = DwarfReader::create(Mod);
  if (!Reader) {
    spdlog::info("debug info: module has no DWARF sections"sv);
    return nullptr;
  }
  return std::make_unique<ModuleDebugInfoImpl>(*llvm::unwrap(M),
                                               std::move(Reader), P);
}

} // namespace WasmEdge::LLVM::DebugInfo
