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
#include "llvm/compiler.h"
#include "llvm/data.h"
#include <memory>
#include <unordered_set>
#include <vector>

namespace WasmEdge::LLVM {

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

private:
  std::shared_ptr<OrcLLJIT> J;
  std::string Prefix;
  bool IsLazy;
  /// Per lazy-compiled IR chunk (ORC JITDylib); main dylib holds
  /// infrastructure.
  std::vector<void *> LazyIRDylibs;
  friend class JIT;
};

class JIT {
public:
  JIT(const Configure &Conf) noexcept : Conf(Conf) {}
  Expect<std::shared_ptr<Executable>> load(Data &D,
                                           bool IsLazy = false) noexcept;
  /// Resolves the wasm function symbol in the new lazy JITDylib only (global
  /// index: imports + local index).
  Expect<WasmFunctionCodeAddress> add(Executable &Exec, Data &D,
                                      uint32_t GlobalFuncIndex) noexcept;

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
  /// Optional owned module (used when VM takes ownership)
  std::unique_ptr<AST::Module> OwnedModule;
  /// Store compiled executables to keep them alive
  std::shared_ptr<Executable> Exec;
  /// Per-module JIT data and context
  Data LLData;
  /// Pointer to the LLVM context.
  std::unique_ptr<Compiler::CompileContext, Compiler::CompileContextDeleter>
      LLContext;
};

} // namespace WasmEdge::LLVM
