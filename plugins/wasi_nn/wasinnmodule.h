// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "wasinnenv.h"

#include "runtime/instance/module.h"

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
