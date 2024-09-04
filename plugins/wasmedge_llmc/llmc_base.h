// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "common/errcode.h"
#include "llmc_env.h"
#include "runtime/hostfunc.h"
#include "types.h"

namespace WasmEdge::Host {

template <typename T> class LLMCBase : public Runtime::HostFunction<T> {
public:
  LLMCBase(LLMC::LLMCEnv &E) : Runtime::HostFunction<T>(0), Env(E) {}

protected:
  static constexpr uint32_t castErrNo(LLMC::ErrNo E) noexcept {
    return static_cast<uint32_t>(E);
  }
  LLMC::LLMCEnv &Env;
};

} // namespace WasmEdge::Host
