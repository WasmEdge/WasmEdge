// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/ast/expression.h - Expression class definition -----------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the Expression node class, which is
/// the expression node in AST.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/instruction.h"

namespace WasmEdge {
namespace AST {

/// AST Expression node.
class Expression {
public:
  /// Getter of instructions vector.
  InstrView getInstrs() const noexcept { return Instrs; }
  InstrVec &getInstrs() noexcept { return Instrs; }

private:
  /// \name Data of Expression.
  /// @{
  InstrVec Instrs;
  /// @}
};

} // namespace AST
} // namespace WasmEdge
