// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "env.h"

#include "common/errcode.h"
#include "runtime/hostfunc.h"

namespace WasmEdge {
namespace Host {

template <typename T>
class WasiIO : public Runtime::Component::HostFunction<T> {
public:
  WasiIO(WasiIOEnvironment &HostEnv)
      : Runtime::Component::HostFunction<T>(), Env(HostEnv) {}

protected:
  WasiIOEnvironment &Env;
};

} // namespace Host
} // namespace WasmEdge
