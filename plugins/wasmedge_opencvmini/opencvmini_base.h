// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "opencvmini_env.h"

#include "common/errcode.h"
#include "runtime/hostfunc.h"

namespace WasmEdge {
namespace Host {

template <typename T>
class WasmEdgeOpenCVMini : public Runtime::HostFunction<T> {
public:
  WasmEdgeOpenCVMini(WasmEdgeOpenCVMiniEnvironment &HostEnv)
      : Runtime::HostFunction<T>(0), Env(HostEnv) {}

protected:
  WasmEdgeOpenCVMiniEnvironment &Env;
};

} // namespace Host
} // namespace WasmEdge
