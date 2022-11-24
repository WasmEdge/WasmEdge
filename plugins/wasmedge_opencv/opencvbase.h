// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#pragma once

#include "opencvenv.h"

#include "common/errcode.h"
#include "runtime/callingframe.h"
#include "runtime/hostfunc.h"

namespace WasmEdge {
namespace Host {

template <typename T> class WasmEdgeOpenCV : public Runtime::HostFunction<T> {
public:
  WasmEdgeOpenCV(WasmEdgeOpenCVEnvironment &HostEnv)
      : Runtime::HostFunction<T>(0), Env(HostEnv) {}

protected:
  WasmEdgeOpenCVEnvironment &Env;
};

} // namespace Host
} // namespace WasmEdge
