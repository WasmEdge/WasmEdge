// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "image_env.h"

#include "common/errcode.h"
#include "runtime/hostfunc.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeImage {

template <typename T> class Func : public Runtime::HostFunction<T> {
public:
  Func(ImgEnv &HostEnv) : Runtime::HostFunction<T>(0), Env(HostEnv) {}

protected:
  ImgEnv &Env;
};

} // namespace WasmEdgeImage
} // namespace Host
} // namespace WasmEdge
