// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "executor/hostfunc.h"
#include "vm/environment.h"

namespace SSVM {
namespace Executor {

template <typename T> class EEI : public HostFunction<T> {
public:
  EEI(VM::EVMEnvironment &HostEnv, const std::string &FuncName = "",
      const uint64_t &Cost = 0)
      : HostFunction<T>("ethereum", FuncName, Cost), Env(HostEnv) {}

protected:
  VM::EVMEnvironment &Env;
  ErrCode addCopyCost(VM::EnvironmentManager &EnvMgr, uint64_t Length) {
    uint64_t TakeGas = 3 * ((Length + 31) / 32);
    return EnvMgr.addCost(TakeGas) ? ErrCode::Success : ErrCode::Revert;
  }
};

} // namespace Executor
} // namespace SSVM
