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

#include "ast/module.h"
#include "common/errcode.h"
#include "common/version.h"
#include <cstdint>
#include <string_view>

namespace SSVM {
namespace AOT {

/// Compiling Module into loadable executable binary.
class Compiler {
public:
  enum class OptimizationLevel {
    /// Disable as many optimizations as possible.
    O0,
    /// Optimize quickly without destroying debuggability.
    O1,
    /// Optimize for fast execution as much as possible without triggering
    /// significant incremental compile time or code size growth.
    O2,
    /// Optimize for fast execution as much as possible.
    O3,
    /// Optimize for small code size as much as possible without triggering
    /// significant incremental compile time or execution time slowdowns.
    Os,
    /// Optimize for small code size as much as possible.
    Oz,
  };

  void setOptimizationLevel(OptimizationLevel Value) { Level = Value; }
  bool optNone() const { return Level == OptimizationLevel::O0; }

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

  void setDumpIR(bool Value = true) { DumpIR = Value; }
  void setInstructionCounting(bool Value = true) {
    InstructionCounting = Value;
  }
  void setGasMeasuring(bool Value = true) { GasMeasuring = Value; }

private:
  CompileContext *Context = nullptr;
  bool DumpIR = false;
  OptimizationLevel Level = OptimizationLevel::O3;
  bool InstructionCounting = false;
  bool GasMeasuring = false;
};

} // namespace AOT
} // namespace SSVM
