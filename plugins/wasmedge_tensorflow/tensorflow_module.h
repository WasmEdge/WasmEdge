// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "tensorflow_env.h"

#include "runtime/instance/module.h"

namespace WasmEdge {
namespace Host {

class WasmEdgeTensorflowModule : public Runtime::Instance::ModuleInstance {
public:
  WasmEdgeTensorflowModule();
  ~WasmEdgeTensorflowModule() = default;

  WasmEdgeTensorflow::TFEnv &getEnv() { return Env; }

private:
  WasmEdgeTensorflow::TFEnv Env;
};

} // namespace Host
} // namespace WasmEdge
