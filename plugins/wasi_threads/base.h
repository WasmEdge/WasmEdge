// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#pragma once

#include "common/errcode.h"
#include "runtime/hostfunc.h"
#include "threadenv.h"

namespace WasmEdge {
namespace Host {

template <typename T> class WasiThreads : public Runtime::HostFunction<T> {
public:
  WasiThreads(WasiThreadsEnvironment &HostEnv)
      : Runtime::HostFunction<T>(0), Env(HostEnv) {}

protected:
  WasiThreadsEnvironment &Env;
};

} // namespace Host
} // namespace WasmEdge
