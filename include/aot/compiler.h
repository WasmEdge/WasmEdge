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
#include <cstdint>
#include <string_view>

namespace SSVM {
namespace AOT {

class Compiler {
public:
  static inline uint32_t kVersion = 1;

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

private:
  CompileContext *Context = nullptr;
};

} // namespace AOT
} // namespace SSVM
