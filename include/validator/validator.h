// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//===-- wasmedge/validator/validator.h - validator class definition -------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the validator class, which controls
/// flow of WASM validation.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/module.h"
#include "common/configure.h"
#include "validator/formchecker.h"

#include <cstdint>
#include <memory>

namespace WasmEdge {
namespace Validator {

/// Validator flow control class.
class Validator {
public:
  Validator(const Configure &Conf) noexcept : Conf(Conf) {}
  ~Validator() noexcept = default;

  /// Validate AST::Module.
  Expect<void> validate(const AST::Module &Mod);

private:
  /// Validate AST::Types
  Expect<void> validate(const AST::Limit &Lim);
  Expect<void> validate(const AST::TableType &Tab);
  Expect<void> validate(const AST::MemoryType &Mem);
  /// GlobalType is always valid.

  /// Validate AST::Segments
  Expect<void> validate(const AST::GlobalSegment &GlobSeg);
  Expect<void> validate(const AST::ElementSegment &ElemSeg);
  Expect<void> validate(const AST::CodeSegment &CodeSeg,
                        const uint32_t TypeIdx);
  Expect<void> validate(const AST::DataSegment &DataSeg);

  /// Validate AST::Desc
  Expect<void> validate(const AST::ImportDesc &ImpDesc);
  Expect<void> validate(const AST::ExportDesc &ExpDesc);

  /// Validate AST::Sections
  Expect<void> validate(const AST::ImportSection &ImportSec);
  Expect<void> validate(const AST::FunctionSection &FuncSec);
  Expect<void> validate(const AST::TableSection &TabSec);
  Expect<void> validate(const AST::MemorySection &MemSec);
  Expect<void> validate(const AST::GlobalSection &GlobSec);
  Expect<void> validate(const AST::ElementSection &ElemSec);
  Expect<void> validate(const AST::CodeSection &CodeSec);
  Expect<void> validate(const AST::DataSection &DataSec);
  Expect<void> validate(const AST::StartSection &StartSec);
  Expect<void> validate(const AST::ExportSection &ExportSec);

  /// Validate const expression
  Expect<void> validateConstExpr(AST::InstrView Instrs,
                                 Span<const ValType> Returns);

  static inline const uint32_t LIMIT_MEMORYTYPE = 1U << 16;
  /// Proposal configure
  const Configure Conf;
  /// Formal checker
  FormChecker Checker;
};

} // namespace Validator
} // namespace WasmEdge
