// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "eei.h"

namespace SSVM {
namespace Executor {

class EEIStorageStore : public EEI {
public:
  EEIStorageStore(VM::EVMEnvironment &Env, uint64_t Cost = 100);
  EEIStorageStore() = delete;
  virtual ~EEIStorageStore() = default;

  virtual ErrCode run(VM::EnvironmentManager &EnvMgr, std::vector<Value> &Args,
                      std::vector<Value> &Res, StoreManager &Store,
                      Instance::ModuleInstance *ModInst);
};

} // namespace Executor
} // namespace SSVM
