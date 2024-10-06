// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "llmc_env.h"

#include "common/errcode.h"
#include "runtime/hostfunc.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeLLMC {

template <typename T> class HostFunction : public Runtime::HostFunction<T> {
public:
  HostFunction(LLMCEnv &E) : Runtime::HostFunction<T>(0), Env(E) {}

protected:
  static constexpr uint32_t castErrNo(ErrNo E) noexcept {
    return static_cast<uint32_t>(E);
  }
  LLMCEnv &Env;
};

} // namespace WasmEdgeLLMC
} // namespace Host
} // namespace WasmEdge
