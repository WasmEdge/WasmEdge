// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "llmc_env.h"

#include "runtime/instance/module.h"

namespace WasmEdge {
namespace Host {

class WasmEdgeLLMCModule : public Runtime::Instance::ModuleInstance {
public:
  WasmEdgeLLMCModule();

private:
  WasmEdgeLLMC::LLMCEnv Env;
};

} // namespace Host
} // namespace WasmEdge
