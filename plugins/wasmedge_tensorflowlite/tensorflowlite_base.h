// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "tensorflowlite_env.h"

#include "common/errcode.h"
#include "runtime/hostfunc.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeTensorflowLite {

template <typename T> class Func : public Runtime::HostFunction<T> {
public:
  Func(TFLiteEnv &HostEnv) : Runtime::HostFunction<T>(0), Env(HostEnv) {}

protected:
  TFLiteEnv &Env;
};

} // namespace WasmEdgeTensorflowLite
} // namespace Host
} // namespace WasmEdge
