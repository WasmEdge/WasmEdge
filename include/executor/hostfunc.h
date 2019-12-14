// SPDX-License-Identifier: Apache-2.0
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
#include "storemgr.h"
#include "vm/envmgr.h"

#include <memory>
#include <vector>

namespace SSVM {
namespace Executor {

class HostFunction {
public:
  HostFunction(uint64_t FuncCost = 0) : Cost(FuncCost) {}
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
  const Instance::ModuleInstance::FType *getFuncType() { return &FuncType; }

  /// Getter of host function cost.
  uint64_t getCost() { return Cost; }

  virtual ErrCode run(VM::EnvironmentManager &EnvMgr, std::vector<Value> &Args,
                      std::vector<Value> &Res, StoreManager &Store,
                      Instance::ModuleInstance *ModInst) = 0;

protected:
  Instance::ModuleInstance::FType FuncType;
  uint64_t Cost = 0;
};

} // namespace Executor
} // namespace SSVM
