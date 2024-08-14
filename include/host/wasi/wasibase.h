// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "common/errcode.h"
#include "host/wasi/environ.h"
#include "runtime/callingframe.h"
#include "runtime/hostfunc.h"

namespace WasmEdge {
namespace Host {

template <typename T> class Wasi : public Runtime::HostFunction<T> {
public:
  Wasi(WASI::Environ &HostEnv) : Runtime::HostFunction<T>(0), Env(HostEnv) {}

protected:
  WASI::Environ &Env;
};

} // namespace Host
} // namespace WasmEdge
