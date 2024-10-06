// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "sd_env.h"

#include "common/errcode.h"
#include "runtime/hostfunc.h"

namespace WasmEdge {
namespace Host {
namespace StableDiffusion {

template <typename T> class Func : public Runtime::HostFunction<T> {
public:
  Func(SDEnviornment &HostEnv) : Runtime::HostFunction<T>(0), Env(HostEnv) {}

protected:
  static constexpr uint32_t castErrNo(StableDiffusion::ErrNo E) noexcept {
    return static_cast<uint32_t>(E);
  }
  SDEnviornment &Env;
};

} // namespace StableDiffusion
} // namespace Host
} // namespace WasmEdge
