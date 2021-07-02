// SPDX-License-Identifier: Apache-2.0
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

#include <memory>
#include <vector>

#include "common/span.h"

#include "base.h"
#include "instruction.h"

namespace WasmEdge {
namespace AST {

/// AST Expression node.
class Expression : public Base {
public:
  /// Load binary from file manager.
  ///
  /// Inheritted and overrided from Base.
  /// Read and make Instruction nodes until the OpCode of End.
  ///
  /// \param Mgr the file manager reference.
  /// \param Conf the WasmEdge configuration reference.
  ///
  /// \returns void when success, ErrCode when failed.
  Expect<void> loadBinary(FileMgr &Mgr, const Configure &Conf) override;

  /// Getter of instructions vector.
  InstrView getInstrs() const { return Instrs; }

  /// Append instruction.
  void pushInstr(Instruction &&Instr) { Instrs.push_back(std::move(Instr)); }
  void pushInstr(const Instruction &Instr) { Instrs.emplace_back(Instr); }

  /// The node type should be ASTNodeAttr::Expression.
  static inline constexpr const ASTNodeAttr NodeAttr = ASTNodeAttr::Expression;

private:
  /// Instruction sequence.
  InstrVec Instrs;
};

} // namespace AST
} // namespace WasmEdge
