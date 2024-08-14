// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "tensorflow_env.h"

#include "common/errcode.h"
#include "runtime/hostfunc.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeTensorflow {

template <typename T> class Func : public Runtime::HostFunction<T> {
public:
  Func(TFEnv &HostEnv) : Runtime::HostFunction<T>(0), Env(HostEnv) {}

protected:
  TFEnv &Env;
};

} // namespace WasmEdgeTensorflow
} // namespace Host
} // namespace WasmEdge
