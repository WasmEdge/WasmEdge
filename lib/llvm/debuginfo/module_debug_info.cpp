// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "debuginfo/module_debug_info.h"
#include "debuginfo/debug_info.h"
#include "debuginfo/dwarf_reader.h"

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

ModuleDebugInfoImpl::ModuleDebugInfoImpl(
    llvm::Module &M, std::unique_ptr<DwarfReader> R) noexcept
    : M(M), Reader(std::move(R)), Translator(M.getContext()) {
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
ModuleDebugInfoImpl::beginFunction(uint32_t, uint32_t, LLVMValueRef) noexcept {
  return nullptr;
}

void ModuleDebugInfoImpl::finalize() noexcept {
  if (Finalized) {
    return;
  }
  Finalized = true;
  for (auto &[Offset, U] : Units) {
    U.DB->finalize();
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
