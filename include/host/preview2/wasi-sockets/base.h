#pragma once

#include "common/errcode.h"
#include "host/preview2/wasi-sockets/env.h"
#include "runtime/callingframe.h"
#include "runtime/hostfunc.h"

namespace WasmEdge {
namespace Host {

template <typename T> class WasiSockets : public Runtime::HostFunction<T> {
public:
  WasiSockets(WasiSocketsEnvironment &HostEnv)
      : Runtime::HostFunction<T>(0), Env(HostEnv) {}

protected:
  WasiSocketsEnvironment &Env;
};

} // namespace Host
} // namespace WasmEdge
