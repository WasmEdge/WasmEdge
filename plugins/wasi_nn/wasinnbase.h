// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "common/errcode.h"
#include "runtime/hostfunc.h"
#include "wasinnenv.h"

namespace WasmEdge {
namespace Host {

template <typename T> class WasiNN : public Runtime::HostFunction<T> {
public:
  WasiNN(WASINN::WasiNNEnvironment &HostEnv)
      : Runtime::HostFunction<T>(0), Env(HostEnv) {}

protected:
  static constexpr uint32_t castErrNo(WASINN::ErrNo E) noexcept {
    return static_cast<uint32_t>(E);
  }

  WASINN::WasiNNEnvironment &Env;
};

} // namespace Host
} // namespace WasmEdge
