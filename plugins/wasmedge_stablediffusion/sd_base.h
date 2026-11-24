// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "sd_abi.h"
#include "sd_env.h"

#include "runtime/hostfunc.h"

#include <cstdint>

namespace WasmEdge {
namespace Host {
namespace StableDiffusion {

template <typename T> class Func : public Runtime::HostFunction<T> {
public:
  explicit Func(SDEnvironment &HostEnv)
      : Runtime::HostFunction<T>(0), Env(HostEnv) {}

protected:
  static constexpr uint32_t castErrNo(ErrNo E) noexcept {
    return static_cast<uint32_t>(E);
  }
  SDEnvironment &Env;
};

} // namespace StableDiffusion
} // namespace Host
} // namespace WasmEdge
