// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "executor/hostfunc.h"
#include "vm/environment.h"
#include "wasi/core.h"

namespace SSVM {
namespace Executor {

template <typename T> class Wasi : public HostFunction<T> {
public:
  Wasi(VM::WasiEnvironment &HostEnv) : Env(HostEnv) {}

protected:
  VM::WasiEnvironment &Env;
};

} // namespace Executor
} // namespace SSVM
