// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "llmc_env.h"
#include "runtime/instance/module.h"

namespace WasmEdge::Host {

class LLMCModule : public Runtime::Instance::ModuleInstance {
  LLMC::LLMCEnv Env;

public:
  LLMCModule();
};

} // namespace WasmEdge::Host
