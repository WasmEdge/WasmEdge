// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "eei.h"

namespace SSVM {
namespace Executor {

class EEIGetCallDataSize : public EEI {
public:
  EEIGetCallDataSize(VM::EVMEnvironment &Env, uint64_t Cost = 100);
  EEIGetCallDataSize() = delete;
  virtual ~EEIGetCallDataSize() = default;

  virtual ErrCode run(VM::EnvironmentManager &EnvMgr, std::vector<Value> &Args,
                      std::vector<Value> &Res, StoreManager &Store,
                      Instance::ModuleInstance *ModInst);
};

} // namespace Executor
} // namespace SSVM
