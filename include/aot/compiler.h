// SPDX-License-Identifier: Apache-2.0
//===-- wasmedge/aot/compiler.h - Compiler class definition ---------------===//
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

#include <cstdint>
#include <string_view>

namespace WasmEdge {
namespace AOT {

/// Compiling Module into loadable executable binary.
class Compiler {
public:
  Compiler(const Configure &Conf) noexcept : Context(nullptr), Conf(Conf) {}

  Expect<void> compile(Span<const Byte> Data, const AST::Module &Module,
                       std::filesystem::path OutputPath);
  void compile(const AST::ImportSection &ImportSection);
  void compile(const AST::ExportSection &ExportSection);
  void compile(const AST::TypeSection &TypeSection);
  void compile(const AST::GlobalSection &GlobalSection);
  void compile(const AST::MemorySection &MemorySection,
               const AST::DataSection &DataSection);
  void compile(const AST::TableSection &TableSection,
               const AST::ElementSection &ElementSection);
  void compile(const AST::FunctionSection &FunctionSection,
               const AST::CodeSection &CodeSection);

  struct CompileContext;

private:
  CompileContext *Context;
  const Configure Conf;
};

} // namespace AOT
} // namespace WasmEdge
