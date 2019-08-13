//===-- ssvm/ast/expression.h - Expression class definition -----*- C++ -*-===//
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
#include "executor/storemgr.h"
#include "instruction.h"

#include <memory>

namespace SSVM {
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

  /// Instantiate to store manager.
  ///
  /// Move the instruction lists to function instance.
  ///
  /// \param Mgr the store manager reference.
  /// \param Instance the function/global/table/memory instance reference.
  ///
  /// \returns ErrCode.
  template <typename T>
  Executor::ErrCode instantiate(StoreManager &Mgr, std::unique_ptr<T> &Instance);

protected:
  /// The node type should be Attr::Expression.
  Attr NodeAttr = Attr::Expression;

private:
  /// Instruction set list.
  std::vector<std::unique_ptr<Instruction>> Instr;
};

} // namespace AST
} // namespace SSVM
