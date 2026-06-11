// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2026 Second State INC

//===-- wasmedge/llvm/lazyjit.h - Lazy JIT engine class definition --------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the LazyJITEngine class, which owns
/// all per-module state and orchestration for lazy (per-function) JIT
/// compilation. The VM only forwards thin hooks to this engine.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/configure.h"
#include "common/errcode.h"

#include <cstdint>
#include <memory>

namespace WasmEdge {

class Executable;

namespace AST {
class Module;
} // namespace AST

namespace Runtime {
namespace Instance {
class ModuleInstance;
class FunctionInstance;
} // namespace Instance
} // namespace Runtime

namespace LLVM {

/// Engine driving lazy (per-function) JIT compilation.
///
/// Lifecycle per wasm module:
/// 1. \c prepare compiles the module infrastructure (types, imports, globals,
///    tables, memories, and function declarations without bodies) into a
///    dedicated ORC LLJIT instance and returns the executable so the caller
///    can hook it into the AST module via \c Loader::loadExecutable .
/// 2. \c registerInstance binds the instantiated module instance to the
///    prepared state. The engine shares ownership of the AST module for
///    later on-demand compilation.
/// 3. \c compileOnDemand compiles the requested function together with its
///    not-yet-compiled callees and upgrades the bound function instances to
///    compiled mode.
class LazyJITEngine {
public:
  LazyJITEngine(const Configure &Conf) noexcept;
  ~LazyJITEngine() noexcept;
  LazyJITEngine(const LazyJITEngine &) = delete;
  LazyJITEngine &operator=(const LazyJITEngine &) = delete;

  /// Compile the module infrastructure and create the per-module JIT.
  Expect<std::shared_ptr<Executable>> prepare(const AST::Module &Module);

  /// Bind an instantiated module instance to its prepared AST module. No-op
  /// when the AST module has not been prepared.
  void registerInstance(const Runtime::Instance::ModuleInstance &ModInst,
                        std::shared_ptr<const AST::Module> Module) noexcept;

  /// Drop the state bound to a module instance.
  void
  unregisterInstance(const Runtime::Instance::ModuleInstance &ModInst) noexcept;

  /// Compile the function and its reachable callees on demand. No-op for
  /// functions which are not bound to this engine or already compiled.
  Expect<void>
  compileOnDemand(const Runtime::Instance::FunctionInstance *FuncInst);

  /// Get the total number of lazily compiled functions.
  uint32_t compiledFunctionCount() const noexcept;

  /// Drop all states.
  void clear() noexcept;

private:
  struct Impl;
  std::unique_ptr<Impl> PImpl;
};

} // namespace LLVM
} // namespace WasmEdge
