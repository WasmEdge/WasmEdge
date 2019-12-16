// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "eei.h"

namespace SSVM {
namespace Executor {

class EEIRevert : public EEI {
public:
  EEIRevert(VM::EVMEnvironment &Env, uint64_t Cost = 100);
  EEIRevert() = delete;
  virtual ~EEIRevert() = default;

  virtual ErrCode run(VM::EnvironmentManager &EnvMgr, std::vector<Value> &Args,
                      std::vector<Value> &Res, StoreManager &Store,
                      Instance::ModuleInstance *ModInst);
};

} // namespace Executor
} // namespace SSVM
