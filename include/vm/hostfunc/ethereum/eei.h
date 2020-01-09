// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "executor/hostfunc.h"
#include "vm/environment.h"

namespace SSVM {
namespace Executor {

template <typename T> class EEI : public HostFunction<T> {
public:
  EEI(VM::EVMEnvironment &HostEnv, const uint64_t &Cost)
      : HostFunction<T>(Cost), Env(HostEnv) {}

protected:
  VM::EVMEnvironment &Env;
};

} // namespace Executor
} // namespace SSVM
