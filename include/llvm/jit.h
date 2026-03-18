// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

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
#include "llvm/data.h"
#include <memory>
#include <unordered_set>
#include <vector>

namespace WasmEdge::LLVM {
class OrcLLJIT;

class JITLibrary : public Executable {
public:
  JITLibrary(OrcLLJIT JIT, bool IsLazy = false) noexcept;
  ~JITLibrary() noexcept override;

  Symbol<const IntrinsicsTable *> getIntrinsics() noexcept override;

  std::vector<Symbol<Wrapper>> getTypes(size_t Size) noexcept override;

  std::vector<Symbol<void>> getCodes(size_t Offset,
                                     size_t Size) noexcept override;

private:
  OrcLLJIT *J;
  bool IsLazy;
};

class JIT {
public:
  JIT(const Configure &Conf) noexcept : Conf(Conf) {}
  Expect<std::shared_ptr<Executable>> load(Data D,
                                           bool IsLazy = false) noexcept;

private:
  const Configure Conf;
};

struct LazyJITState {
  /// Track which functions have been lazy-compiled.
  std::unordered_set<uint32_t> LazyCompiledFuncs;
  /// Number of import functions (offset for local function indices).
  uint32_t ImportFuncCount = 0;
  /// Number of total functions
  uint32_t TotalFuncCount = 0;
  /// Pointer to the AST module (non-owning pointer, lifetime managed by caller)
  const AST::Module *ModulePtr = nullptr;
  /// Store compiled executables to keep them alive
  std::vector<std::shared_ptr<Executable>> CompiledExecutables;
};

} // namespace WasmEdge::LLVM
