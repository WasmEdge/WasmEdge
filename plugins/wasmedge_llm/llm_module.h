// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "llm_env.h"

#include "runtime/instance/module.h"

namespace WasmEdge {
namespace Host {

class WasmEdgeLLMModule : public Runtime::Instance::ModuleInstance {
public:
  WasmEdgeLLMModule();

private:
  WasmEdgeLLM::LLMEnv Env;
};

} // namespace Host
} // namespace WasmEdge
