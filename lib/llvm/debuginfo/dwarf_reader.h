// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "ast/module.h"

#include <llvm/ADT/StringMap.h>
#include <llvm/DebugInfo/DWARF/DWARFContext.h>
#include <llvm/DebugInfo/DWARF/DWARFDie.h>
#include <llvm/Support/MemoryBuffer.h>

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace WasmEdge::LLVM::DebugInfo {

class DwarfReader {
public:
  static std::unique_ptr<DwarfReader> create(const AST::Module &Mod) noexcept;

  llvm::DWARFContext &getContext() noexcept { return *Ctx; }
  uint64_t toAddress(uint64_t FileOffset) const noexcept {
    return FileOffset - CodeOffset;
  }
  llvm::DWARFDie getSubprogram(uint32_t DefinedIndex) const noexcept;
  std::string getFunctionName(uint32_t FuncIndex) const;
  uint32_t getErrorCount() const noexcept { return *Errors; }

private:
  DwarfReader() = default;
  void indexSubprograms(const AST::Module &Mod) noexcept;
  void parseNames(const AST::Module &Mod) noexcept;

  llvm::StringMap<std::unique_ptr<llvm::MemoryBuffer>> Sections;
  std::unique_ptr<llvm::DWARFContext> Ctx;
  std::shared_ptr<uint32_t> Errors = std::make_shared<uint32_t>(0);
  uint64_t CodeOffset = 0;
  std::vector<llvm::DWARFDie> Subprograms;
  std::unordered_map<uint32_t, std::string> Names;
};

} // namespace WasmEdge::LLVM::DebugInfo
