#pragma once

#include "wasi_random/env.h"

#include "common/errcode.h"
#include "runtime/callingframe.h"
#include "runtime/hostfunc.h"

namespace WasmEdge {
namespace Host {

template <typename T> class WasiRandom : public Runtime::HostFunction<T> {
public:
  WasiRandom(WasiRandomEnvironment &HostEnv)
      : Runtime::HostFunction<T>(0), Env(HostEnv) {}

protected:
  WasiRandomEnvironment &Env;
};

} // namespace Host
} // namespace WasmEdge
