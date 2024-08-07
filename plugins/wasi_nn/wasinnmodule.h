// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "runtime/instance/module.h"
#include "wasinnenv.h"

namespace WasmEdge {
namespace Host {

class WasiNNModule : public Runtime::Instance::ModuleInstance {
public:
  WasiNNModule();

  WASINN::WasiNNEnvironment &getEnv() { return Env; }

private:
  WASINN::WasiNNEnvironment Env;
};

} // namespace Host
} // namespace WasmEdge
