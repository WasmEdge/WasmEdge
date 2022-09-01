// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#pragma once

#include "httpsreqenv.h"

#include "common/errcode.h"
#include "runtime/callingframe.h"
#include "runtime/hostfunc.h"

namespace WasmEdge {
namespace Host {

template <typename T> class WasmEdgeHttpsReq : public Runtime::HostFunction<T> {
public:
  WasmEdgeHttpsReq(WasmEdgeHttpsReqEnvironment &HostEnv)
      : Runtime::HostFunction<T>(0), Env(HostEnv) {}

protected:
  WasmEdgeHttpsReqEnvironment &Env;
};

} // namespace Host
} // namespace WasmEdge
