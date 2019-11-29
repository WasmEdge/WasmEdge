// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "eei.h"

namespace SSVM {
namespace Executor {

class EEIGetCaller : public EEI {
public:
  EEIGetCaller(VM::EVMEnvironment &Env);
  EEIGetCaller() = delete;
  virtual ~EEIGetCaller() = default;

  virtual ErrCode run(std::vector<Value> &Args, std::vector<Value> &Res,
                      StoreManager &Store, Instance::ModuleInstance *ModInst);
};

} // namespace Executor
} // namespace SSVM
