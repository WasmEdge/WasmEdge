// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "llm_env.h"

#include "common/errcode.h"
#include "runtime/hostfunc.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeLLM {

template <typename T> class HostFunction : public Runtime::HostFunction<T> {
public:
  HostFunction(LLMEnv &E) : Runtime::HostFunction<T>(0), Env(E) {}

protected:
  static constexpr uint32_t castErrNo(ErrNo E) noexcept {
    return static_cast<uint32_t>(E);
  }
  LLMEnv &Env;
};

} // namespace WasmEdgeLLM
} // namespace Host
} // namespace WasmEdge
