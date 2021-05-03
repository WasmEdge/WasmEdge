// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/errcode.h"
#include "runtime/hostfunc.h"
#include "wasienv.h"

namespace WasmEdge {
namespace Host {

template <typename T> class Wasi : public Runtime::HostFunction<T> {
public:
  Wasi(WasiEnvironment &HostEnv) : Runtime::HostFunction<T>(0), Env(HostEnv) {}

protected:
  WasiEnvironment &Env;
};

} // namespace Host
} // namespace WasmEdge
