// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#pragma once

#include "imgcodecsenv.h"

#include "common/errcode.h"
#include "runtime/callingframe.h"
#include "runtime/hostfunc.h"

namespace WasmEdge {
namespace Host {

template <typename T>
class WasmEdgeOpenCvImgcodecs : public Runtime::HostFunction<T> {
public:
  WasmEdgeOpenCvImgcodecs(WasmEdgeOpenCvImgcodecsEnvoronment &HostEnv)
      : Runtime::HostFunction<T>(0), Env(HostEnv) {}

protected:
  WasmEdgeOpenCvImgcodecsEnvoronment &Env;
};

} // namespace Host
} // namespace WasmEdge
