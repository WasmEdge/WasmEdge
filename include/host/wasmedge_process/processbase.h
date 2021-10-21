// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/errcode.h"
#include "host/wasmedge_process/processenv.h"
#include "runtime/hostfunc.h"

namespace WasmEdge {
namespace Host {

template <typename T> class WasmEdgeProcess : public Runtime::HostFunction<T> {
public:
  WasmEdgeProcess(WasmEdgeProcessEnvironment &HostEnv)
      : Runtime::HostFunction<T>(0), Env(HostEnv) {}

protected:
  WasmEdgeProcessEnvironment &Env;
};

} // namespace Host
} // namespace WasmEdge
