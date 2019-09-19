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
#include "executor/instance/entity.h"
#include <memory>
#include <string>
#include <vector>

namespace SSVM {
namespace Executor {
namespace Instance {

class FunctionInstance : public Entity {
public:
  FunctionInstance() = default;
  virtual ~FunctionInstance() = default;

  /// Set the module instance index in store manager.
  ErrCode setModuleAddr(unsigned int Addr);

  /// Set the function type index in module instance.
  ErrCode setTypeIdx(unsigned int Id);

  /// Move the local variables in code section into function instance.
  ErrCode
  setLocals(const std::vector<std::pair<unsigned int, AST::ValType>> &Loc);

  /// Move the instruction list in code segment into function instance.
  ErrCode setInstrs(AST::InstrVec &Expr);

  /// Getter of function type index in module instance.
  unsigned int getTypeIdx() const { return TypeIdx; }

  /// Getter of module address of this function instance.
  unsigned int getModuleAddr() const { return ModuleAddr; }

  /// Getter of function body instrs.
  const std::vector<std::pair<unsigned int, AST::ValType>> &getLocals() const {
    return Locals;
  }

  /// Getter of function body instrs.
  const AST::InstrVec &getInstrs() const { return Instrs; }

private:
  /// \name Data of function instance.
  /// @{
  unsigned int TypeIdx;
  unsigned int ModuleAddr;
  std::vector<std::pair<unsigned int, AST::ValType>> Locals;
  AST::InstrVec Instrs;
  /// @}
};

} // namespace Instance
} // namespace Executor
} // namespace SSVM
