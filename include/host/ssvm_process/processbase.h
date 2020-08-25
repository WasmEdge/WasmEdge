// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/errcode.h"
#include "processenv.h"
#include "runtime/hostfunc.h"

namespace SSVM {
namespace Host {

template <typename T> class SSVMProcess : public Runtime::HostFunction<T> {
public:
  SSVMProcess(SSVMProcessEnvironment &HostEnv)
      : Runtime::HostFunction<T>(0), Env(HostEnv) {}

protected:
  SSVMProcessEnvironment &Env;
};

} // namespace Host
} // namespace SSVM