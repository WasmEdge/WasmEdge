// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "common/errcode.h"
#include "runtime/hostfunc.h"
#include "types.h"
#include "wasillmenv.h"

namespace WasmEdge {
namespace Host {

template <typename T> class WasiLLM : public Runtime::HostFunction<T> {
public:
  WasiLLM(WASILLM::WASILLMEnv &E) : Runtime::HostFunction<T>(0), Env(E) {}

protected:
  static constexpr uint32_t castErrNo(WASILLM::ErrNo E) noexcept {
    return static_cast<uint32_t>(E);
  }
  WASILLM::WASILLMEnv &Env;
};

} // namespace Host
} // namespace WasmEdge
