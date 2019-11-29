// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "eei.h"

namespace SSVM {
namespace Executor {

class EEICallStatic : public EEI {
public:
  EEICallStatic(VM::EVMEnvironment &Env);
  EEICallStatic() = delete;
  virtual ~EEICallStatic() = default;

  virtual ErrCode run(std::vector<Value> &Args, std::vector<Value> &Res,
                      StoreManager &Store, Instance::ModuleInstance *ModInst);
};

} // namespace Executor
} // namespace SSVM
