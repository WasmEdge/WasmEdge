// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/llvm/compiler.h - Compiler class definition --------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file defines the Compiler class.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/module.h"
#include "common/configure.h"
#include "common/errcode.h"
#include "common/filesystem.h"
#include "common/span.h"
#include "llvm/data.h"

#include <mutex>

namespace WasmEdge::LLVM {

class Module;
class TargetMachine;

/// Compiling Module into LLVM Module.
class Compiler {
public:
  Compiler(const Configure &Conf) noexcept : Context(nullptr), Conf(Conf) {}

  Expect<void> checkConfigure() noexcept;

  /// Compile the whole module.
  Expect<Data> compile(const AST::Module &Module) noexcept;

  struct CompileContext;

  /// Compile only the infrastructure (types, imports, globals, etc.) without
  /// function bodies.
  Expect<Data> compileInfrastructure(const AST::Module &Module) noexcept;
  /// Compile multiple function bodies in one LLVM module for lazy JIT.
  /// \p LocalFuncIndices are indices of defined functions (not imports).
  Expect<Data> compileFunctions(Data &&LLData, const AST::Module &Module,
                                Span<const uint32_t> LocalFuncIndices) noexcept;

private:
  void compile(const AST::ImportSection &ImportSection) noexcept;
  void compile(const AST::ExportSection &ExportSection) noexcept;
  void compile(const AST::TypeSection &TypeSection,
               bool DeclarationsOnly = false) noexcept;
  void compile(const AST::GlobalSection &GlobalSection) noexcept;
  void compile(const AST::MemorySection &MemorySection,
               const AST::DataSection &DataSection) noexcept;
  void compile(const AST::TableSection &TableSection,
               const AST::ElementSection &ElementSection) noexcept;
  void compile(const AST::TagSection &TagSection) noexcept;

  /// Compile all sections and create the function declarations. When
  /// \p DeclarationsOnly is set, the type wrappers are emitted as external
  /// declarations resolved against another module in the same JIT session.
  void compileSections(const AST::Module &Module,
                       bool DeclarationsOnly) noexcept;
  void compileFunctionDeclarations(const AST::FunctionSection &FunctionSec,
                                   const AST::CodeSection &CodeSec) noexcept;
  Expect<void> compileFunctionBody(uint32_t LocalFuncIndex) noexcept;
  Expect<void> optimize(Module &LLModule, TargetMachine &TM) noexcept;

  std::mutex Mutex;
  CompileContext *Context;
  const Configure Conf;
};

} // namespace WasmEdge::LLVM
