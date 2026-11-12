// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "env.h"

#include "common/errcode.h"
#include "runtime/component/hostfunc.h"

namespace WasmEdge {
namespace Host {

template <typename T>
class WasiHttp : public Runtime::Component::HostFunction<T> {
public:
  WasiHttp(WasiHttpEnvironment &HostEnv)
      : Runtime::Component::HostFunction<T>(), Env(HostEnv) {}

protected:
  WasiHttpEnvironment &Env;
};

} // namespace Host
} // namespace WasmEdge
