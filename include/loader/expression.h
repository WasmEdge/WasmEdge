//===-- ssvm/loader/expression.h - Expression class definition --*- C++ -*-===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the Expression node class, which is
/// the expression node in AST.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "base.h"
#include "instruction.h"

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
  ///
  /// \returns ErrCode.
  virtual Loader::ErrCode loadBinary(FileMgr &Mgr);

protected:
  /// The node type should be Attr::Expression.
  Attr NodeAttr = Attr::Expression;

private:
  /// Instruction set list.
  std::vector<std::unique_ptr<Instruction>> Inst;
};

} // namespace AST