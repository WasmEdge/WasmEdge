// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "runtime/instance/module.h"
#include "wasillmenv.h"

namespace WasmEdge {
namespace Host {

class WasiLLMModule : public Runtime::Instance::ModuleInstance {
  WASILLM::WASILLMEnv Env;

public:
  WasiLLMModule();
};

} // namespace Host
} // namespace WasmEdge
