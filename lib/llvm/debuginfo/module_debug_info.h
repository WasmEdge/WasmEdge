// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "debuginfo/debug_info.h"
#include "debuginfo/dwarf_reader.h"
#include "debuginfo/type_translator.h"

#include <llvm/DebugInfo/DWARF/DWARFDie.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Module.h>

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>

namespace WasmEdge::LLVM::DebugInfo {

class ModuleDebugInfoImpl final : public ModuleDebugInfo {
public:
  ModuleDebugInfoImpl(llvm::Module &M, std::unique_ptr<DwarfReader> R) noexcept;

  ~ModuleDebugInfoImpl() noexcept override { finalize(); }

  LLVMMetadataRef translateType(uint64_t DieOffset) noexcept override;

  LLVMValueRef getMemoryBaseGlobal() const noexcept override {
    return llvm::wrap(MemoryBase);
  }

  std::unique_ptr<FunctionDebugInfo>
  beginFunction(uint32_t, uint32_t, LLVMValueRef) noexcept override;

  void finalize() noexcept override;

  std::optional<std::string> verify() noexcept override;

private:
  UnitState &unit(DWARFDie Die) noexcept;

  llvm::Module &M;
  std::unique_ptr<DwarfReader> Reader;
  TypeTranslator Translator;
  std::map<uint64_t, UnitState> Units;
  bool Finalized = false;
  llvm::GlobalVariable *MemoryBase = nullptr;
};

} // namespace WasmEdge::LLVM::DebugInfo
