// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/llvm/jit.h - JIT Engine class definition -----------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file defines the JIT engine class.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/module.h"
#include "common/configure.h"
#include "common/errcode.h"
#include "common/executable.h"
#include "common/span.h"
#include "llvm/data.h"
#include <memory>
#include <vector>

namespace WasmEdge::LLVM {

/// Address of JIT- or AOT-generated machine code for a wasm function. Not a
/// typed C++ function pointer; wasm signatures vary and calls go through the
/// executor.
using WasmFunctionCodeAddress = void *;

class OrcLLJIT;

class JITLibrary : public Executable {
public:
  JITLibrary(std::shared_ptr<OrcLLJIT> JIT, bool IsLazy = false) noexcept;
  ~JITLibrary() noexcept override;

  Symbol<const IntrinsicsTable *> getIntrinsics() noexcept override;

  std::vector<Symbol<Wrapper>> getTypes(size_t Size) noexcept override;

  std::vector<Symbol<void>> getCodes(size_t Offset,
                                     size_t Size) noexcept override;

  /// Create a symbol for lazily compiled machine code owned by this library.
  Symbol<void> createCodeSymbol(void *Address) const noexcept {
    return createSymbol<void>(Address);
  }

private:
  std::shared_ptr<OrcLLJIT> J;
  bool IsLazy;
  friend class JIT;
};

class JIT {
public:
  JIT(const Configure &Conf) noexcept : Conf(Conf) {}
  Expect<std::shared_ptr<Executable>> load(Data D) noexcept;

  /// Load for lazy JIT. The data is kept alive by the caller so following
  /// batches can reuse its thread-safe context.
  Expect<std::shared_ptr<Executable>> loadLazy(Data &D) noexcept;

  /// Adds one LLVM IR module and resolves many wasm function symbols.
  Expect<std::vector<WasmFunctionCodeAddress>>
  add(JITLibrary &Lib, Data &D,
      Span<const uint32_t> GlobalFuncIndices) noexcept;

private:
  Expect<std::shared_ptr<Executable>> loadImpl(Data &D, bool IsLazy) noexcept;

  const Configure Conf;
};

} // namespace WasmEdge::LLVM
