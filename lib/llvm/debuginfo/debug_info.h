// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "ast/module.h"
#include "common/span.h"

#include <llvm-c/Core.h>
#include <llvm-c/Types.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

namespace WasmEdge::LLVM::DebugInfo {

class DwarfReader;

struct AddressSlot {
  enum class Kind : uint8_t { Local, Global, GlobalStorage };
  Kind K = Kind::Local;
  uint32_t Index = 0;
};

class FunctionDebugInfo {
public:
  virtual ~FunctionDebugInfo() noexcept = default;
  virtual void setPrologueLocation(LLVMBuilderRef B) noexcept = 0;
  virtual void setLocation(LLVMBuilderRef B, uint64_t FileOffset) noexcept = 0;
  virtual void setArtificialLocation(LLVMBuilderRef B) noexcept = 0;
  virtual Span<const AddressSlot> getAddressSlots() const noexcept = 0;
  virtual void bindFrame(Span<const LLVMValueRef> Locals,
                         LLVMValueRef MemorySlot,
                         Span<const LLVMValueRef> Slots,
                         bool LocalsInMemory) noexcept = 0;
  virtual void localWritten(LLVMBuilderRef B, uint32_t Index,
                            LLVMValueRef Value) noexcept = 0;
  virtual void finish(LLVMBasicBlockRef Entry,
                      bool ExtendLiveness) noexcept = 0;
};

class ModuleDebugInfo {
public:
  enum class Part : uint8_t { All, GlobalsOnly, FunctionsOnly };
  static std::unique_ptr<ModuleDebugInfo>
  create(const AST::Module &Mod, LLVMModuleRef M, Part P) noexcept;
  static std::unique_ptr<ModuleDebugInfo>
  create(const AST::Module &Mod, LLVMModuleRef M, Part P,
         std::shared_ptr<DwarfReader> &Reader) noexcept;
  virtual ~ModuleDebugInfo() noexcept = default;
  virtual LLVMMetadataRef translateType(uint64_t DieOffset) noexcept = 0;
  virtual LLVMValueRef getMemoryBaseGlobal() const noexcept = 0;
  virtual std::unique_ptr<FunctionDebugInfo>
  beginFunction(uint32_t FuncIndex, uint32_t DefinedIndex,
                LLVMValueRef Fn) noexcept = 0;
  virtual void finalize() noexcept = 0;
  virtual std::optional<std::string> verify() noexcept = 0;
};

} // namespace WasmEdge::LLVM::DebugInfo
