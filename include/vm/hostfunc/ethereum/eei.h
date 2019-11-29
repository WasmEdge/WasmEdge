// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "executor/hostfunc.h"
#include "vm/environment.h"

namespace SSVM {
namespace Executor {

class EEI : public HostFunction {
public:
  EEI(VM::EVMEnvironment &HostEnv) : Env(HostEnv) {}
  EEI() = delete;
  virtual ~EEI() = default;

protected:
  VM::EVMEnvironment &Env;
};

} // namespace Executor
} // namespace SSVM
