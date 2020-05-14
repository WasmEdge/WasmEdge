// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/aot/compiler.h - Compiler class definition -------------------===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file is the definition class of Compiler class.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/ast/module.h"
#include "common/errcode.h"
#include "common/version.h"
#include <cstdint>
#include <string_view>

namespace SSVM {
namespace AOT {

/// Compiling Module into loadable executable binary.
class Compiler {
public:
  Expect<void> compile(Span<const Byte> Data, const AST::Module &Module,
                       std::string_view OutputPath);
  Expect<void> compile(const AST::ImportSection &ImportSection);
  Expect<void> compile(const AST::ExportSection &ExportSection);
  Expect<void> compile(const AST::TypeSection &TypeSection);
  Expect<void> compile(const AST::GlobalSection &GlobalSection);
  Expect<void> compile(const AST::MemorySection &MemorySection,
                       const AST::DataSection &DataSection);
  Expect<void> compile(const AST::TableSection &TableSection,
                       const AST::ElementSection &ElementSection);
  Expect<void> compile(const AST::FunctionSection &FunctionSection,
                       const AST::CodeSection &CodeSection);

  struct CompileContext;

  /// Setter of module name.
  void setDumpIR(bool Value = true) { DumpIR = Value; }

private:
  CompileContext *Context = nullptr;
  bool DumpIR = false;
};

} // namespace AOT
} // namespace SSVM
