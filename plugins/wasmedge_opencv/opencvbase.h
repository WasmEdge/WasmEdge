// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#pragma once

#include "opencvenv.h"

#include "common/errcode.h"
#include "runtime/callingframe.h"
#include "runtime/hostfunc.h"

namespace WasmEdge {
namespace Host {

template <typename T> class WasmEdgeOpenCv : public Runtime::HostFunction<T> {
public:
  WasmEdgeOpenCv(WasmEdgeOpenCvEnvironment &HostEnv)
      : Runtime::HostFunction<T>(0), Env(HostEnv) {}

protected:
  WasmEdgeOpenCvEnvironment &Env;
};

} // namespace Host
} // namespace WasmEdge
