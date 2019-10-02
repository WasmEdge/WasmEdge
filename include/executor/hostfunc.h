//===-- ssvm/executor/hostfunc.h - host function interface ----------------===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the interface of host function class.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common.h"
#include "entry/value.h"
#include "instance/module.h"

#include <memory>
#include <vector>

namespace SSVM {
namespace Executor {

class HostFunction {
public:
  HostFunction() = default;
  virtual ~HostFunction() = default;

  /// Setter of function parameter.
  ErrCode appendParamDef(AST::ValType Type) {
    if (Type == AST::ValType::None) {
      return ErrCode::TypeNotMatch;
    }
    FuncType.Params.push_back(Type);
    return ErrCode::Success;
  }

  /// Setter of function return value.
  ErrCode appendReturnDef(AST::ValType Type) {
    if (Type == AST::ValType::None) {
      return ErrCode::TypeNotMatch;
    }
    FuncType.Returns.push_back(Type);
    return ErrCode::Success;
  }

  /// Getter of function type.
  Instance::ModuleInstance::FType *getFuncType() { return &FuncType; }

  virtual ErrCode run(std::vector<std::unique_ptr<ValueEntry>> &Args,
                      std::vector<std::unique_ptr<ValueEntry>> &Res) = 0;

protected:
  Instance::ModuleInstance::FType FuncType;
};

} // namespace Executor
} // namespace SSVM