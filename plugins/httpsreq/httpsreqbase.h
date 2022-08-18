// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#pragma once

#include "common/errcode.h"
#include "httpsreqenv.h"
#include "runtime/callingframe.h"
#include "runtime/hostfunc.h"

namespace WasmEdge {
namespace Host {

template <typename T> class HttpsReq : public Runtime::HostFunction<T> {
public:
  HttpsReq(HttpsReqEnvironment &HostEnv)
      : Runtime::HostFunction<T>(0), Env(HostEnv) {}

protected:
  HttpsReqEnvironment &Env;
};

} // namespace Host
} // namespace WasmEdge