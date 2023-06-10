#pragma once

#include "wasi_logging/env.h"

#include "common/errcode.h"
#include "runtime/callingframe.h"
#include "runtime/hostfunc.h"

namespace WasmEdge {
namespace Host {

template <typename T> class WasiLogging : public Runtime::HostFunction<T> {
public:
  WasiLogging(WasiLoggingEnvironment &HostEnv)
      : Runtime::HostFunction<T>(0), Env(HostEnv) {}

protected:
  WasiLoggingEnvironment &Env;
};

} // namespace Host
} // namespace WasmEdge