// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2026 Second State INC

//===-- wasmedge/llvm/jit.h - JIT Engine class definition -----------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file is the definition class of JIT engine class.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/module.h"
#include "common/configure.h"
#include "common/errcode.h"
#include "common/span.h"
#include "llvm/compiler.h"
#include "llvm/data.h"
#include <memory>
#include <mutex>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace llvm {
class Module;
}

namespace WasmEdge::LLVM {

class Module;

/// Address of JIT- or AOT-generated machine code for a wasm function. Not a
/// typed C++ function pointer; wasm signatures vary and calls go through the
/// executor.
using WasmFunctionCodeAddress = void *;

class OrcLLJIT;

class JITLibrary : public Executable {
public:
  JITLibrary(std::shared_ptr<OrcLLJIT> JIT, std::string Prefix = "",
             bool IsLazy = false) noexcept;
  ~JITLibrary() noexcept override;

  Symbol<const IntrinsicsTable *> getIntrinsics() noexcept override;

  std::vector<Symbol<Wrapper>> getTypes(size_t Size) noexcept override;

  std::vector<Symbol<void>> getCodes(size_t Offset,
                                     size_t Size) noexcept override;
  bool isLazy() const noexcept override { return IsLazy; }
  void setPrefix(std::string NewPrefix) noexcept {
    this->Prefix = std::move(NewPrefix);
  }

private:
  std::shared_ptr<OrcLLJIT> J;
  std::string Prefix;
  bool IsLazy;
  mutable std::mutex LazyAddMutex;
  friend class JIT;
};

class JIT {
public:
  JIT(const Configure &Conf) noexcept : Conf(Conf) {}
  Expect<std::shared_ptr<Executable>> load(Data &D,
                                           bool IsLazy = false) noexcept;

  /// Adds one LLVM IR module and resolves many wasm function symbols.
  Expect<std::vector<WasmFunctionCodeAddress>>
  add(JITLibrary &Lib, Data &D,
      Span<const uint32_t> GlobalFuncIndices) noexcept;

  /// Look up already-loaded symbols (no IR add). Same index convention as
  /// \c add .
  Expect<std::vector<WasmFunctionCodeAddress>>
  lookupWasmFunctionSymbols(JITLibrary &Lib, std::string_view Prefix,
                            Span<const uint32_t> GlobalFuncIndices) noexcept;

private:
  const Configure Conf;
};

struct LazyJITState {
  /// Track which functions have been lazy-compiled.
  std::unordered_set<uint32_t> LazyCompiledFuncs;
  /// Number of import functions (offset for local function indices).
  uint32_t ImportFuncCount = 0;
  /// Store compiled JIT library to keep it alive
  std::shared_ptr<JITLibrary> JITLib;
  /// Per-module JIT data and context
  Data LLData;
  /// Pointer to the LLVM context.
  std::unique_ptr<Compiler::CompileContext, Compiler::CompileContextDeleter>
      LLContext;
};

} // namespace WasmEdge::LLVM
