// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "env.h"

#include "common/errcode.h"
#include "runtime/instance/component/hostfunc.h"

namespace WasmEdge {
namespace Host {

template <typename T>
class WasiPoll : public Runtime::Instance::Component::HostFunction<T> {
public:
  WasiPoll(WasiPollEnvironment &HostEnv)
      : Runtime::Instance::Component::HostFunction<T>(), Env(HostEnv) {}

protected:
  WasiPollEnvironment &Env;
};

} // namespace Host
} // namespace WasmEdge
