// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2023 Second State INC

#pragma once

#include "env.h"

#include "common/errcode.h"
#include "runtime/hostfunc.h"

namespace WasmEdge {
namespace Host {

template <typename T>
class WasmEdgeWasiClocks : public Runtime::HostFunction<T> {
public:
  WasmEdgeWasiClocks(WasmEdgeWasiClocksEnvironment &HostEnv)
      : Runtime::HostFunction<T>(0), Env(HostEnv) {}

protected:
  WasmEdgeWasiClocksEnvironment &Env;
};

} // namespace Host
} // namespace WasmEdge
