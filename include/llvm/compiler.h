// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/llvm/compiler.h - Compiler class definition --------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file is the definition class of Compiler class.
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

/// Compiling Module into LLVM Module.
class Compiler {
public:
  Compiler(const Configure &Conf) noexcept : Context(nullptr), Conf(Conf) {}

  Expect<Data> compile(const AST::Module &Module) noexcept;

  struct CompileContext;

private:
  void compile(const AST::ImportSection &ImportSection) noexcept;
  void compile(const AST::ExportSection &ExportSection) noexcept;
  void compile(const AST::TypeSection &TypeSection) noexcept;
  void compile(const AST::GlobalSection &GlobalSection) noexcept;
  void compile(const AST::MemorySection &MemorySection,
               const AST::DataSection &DataSection) noexcept;
  void compile(const AST::TableSection &TableSection,
               const AST::ElementSection &ElementSection) noexcept;
  void compile(const AST::FunctionSection &FunctionSection,
               const AST::CodeSection &CodeSection) noexcept;

  std::mutex Mutex;
  CompileContext *Context;
  const Configure Conf;
};

} // namespace WasmEdge::LLVM
