//===-- ssvm/executor/instance/function.h - Function Instance definition --===//
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
#include "executor/common.h"
#include <memory>
#include <string>
#include <vector>

namespace SSVM {
namespace Executor {
namespace Instance {

class FunctionInstance {
public:
  FunctionInstance() = default;
  ~FunctionInstance() = default;

  /// Set the module instance index in store manager.
  ErrCode setModuleAddr(unsigned int Addr);

  /// Set the function type index in module instance.
  ErrCode setTypeIdx(unsigned int Id);

  /// Move the local variables in code section into function instance.
  ErrCode setLocals(std::vector<std::pair<unsigned int, AST::ValType>> &Loc);

  /// Move the instruction list in code segment into function instance.
  ErrCode setExpression(std::vector<std::unique_ptr<AST::Instruction>> &Expr);

  /// Set the module name and function name.
  ErrCode setNames(const std::string &Mod, const std::string &Func);

  /// Match the module and function name.
  bool isName(const std::string &Mod, const std::string &Func);

  /// Function Instance address in store manager.
  unsigned int Addr;

private:
  /// \name Data of function instance.
  /// @{
  unsigned int TypeIdx;
  unsigned int ModuleAddr;
  std::string ModName = "";
  std::string FuncName = "";
  std::vector<std::pair<unsigned int, AST::ValType>> Locals;
  std::vector<std::unique_ptr<AST::Instruction>> Instrs;
  /// @}
};

} // namespace Instance
} // namespace Executor
} // namespace SSVM
