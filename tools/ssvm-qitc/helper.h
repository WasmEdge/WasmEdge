// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "executor/common.h"
#include "executor/hostfunc.h"
#include "executor/worker/util.h"

#include <iostream>
#include <string>

namespace SSVM {
namespace Executor {

class ONNCTimeStart : public HostFunction {
public:
  ONNCTimeStart() { appendParamDef(AST::ValType::I32); }
  virtual ~ONNCTimeStart() = default;

  virtual ErrCode run(std::vector<Value> &Args, std::vector<Value> &Res,
                      StoreManager &Store, Instance::ModuleInstance *ModInst) {
    return ErrCode::Success;
  }
};

class ONNCTimeStop : public HostFunction {
public:
  ONNCTimeStop() {
    appendParamDef(AST::ValType::I32);
    appendParamDef(AST::ValType::I32);
  }
  virtual ~ONNCTimeStop() = default;

  virtual ErrCode run(std::vector<Value> &Args, std::vector<Value> &Res,
                      StoreManager &Store, Instance::ModuleInstance *ModInst) {
    return ErrCode::Success;
  }
};

class ONNCTimeClear : public HostFunction {
public:
  ONNCTimeClear() { appendParamDef(AST::ValType::I32); }
  virtual ~ONNCTimeClear() = default;

  virtual ErrCode run(std::vector<Value> &Args, std::vector<Value> &Res,
                      StoreManager &Store, Instance::ModuleInstance *ModInst) {
    return ErrCode::Success;
  }
};

} // namespace Executor
} // namespace SSVM
