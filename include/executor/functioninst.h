//===-- ssvm/executor/functioninst.h - Function Instance definition -------===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the function instance definition in store manager.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/instruction.h"
#include "common.h"
#include <memory>

class FunctionInstance {
public:
  Executor::ErrCode setModuleIdx(unsigned int Id);
  Executor::ErrCode setTypeIdx(unsigned int Id);
  Executor::ErrCode
  setLocals(std::vector<std::pair<unsigned int, AST::ValType>> &Loc);
  Executor::ErrCode
  setExpression(std::vector<std::unique_ptr<AST::Instruction>> &Expr);

  /// Function Instance ID in store manager.
  unsigned int Id;

private:
  unsigned int TypeIdx;
  unsigned int ModuleIdx;
  std::vector<std::pair<unsigned int, AST::ValType>> Locals;
  std::vector<std::unique_ptr<AST::Instruction>> Insts;
};