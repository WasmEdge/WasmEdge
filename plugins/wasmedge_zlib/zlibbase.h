// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "zlibenv.h"

#include "common/errcode.h"
#include "runtime/callingframe.h"
#include "runtime/hostfunc.h"

namespace WasmEdge {
namespace Host {

template <typename T> class WasmEdgeZlib : public Runtime::HostFunction<T> {
public:
  WasmEdgeZlib(WasmEdgeZlibEnvironment &HostEnv)
      : Runtime::HostFunction<T>(0), Env(HostEnv) {}

protected:
  WasmEdgeZlibEnvironment &Env;
};

} // namespace Host
} // namespace WasmEdge
