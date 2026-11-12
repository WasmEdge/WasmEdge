// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "tensorflowlite_env.h"

#include "runtime/instance/module.h"

namespace WasmEdge {
namespace Host {

class WasmEdgeTensorflowLiteModule : public Runtime::Instance::ModuleInstance {
public:
  WasmEdgeTensorflowLiteModule();
  ~WasmEdgeTensorflowLiteModule() = default;

  WasmEdgeTensorflowLite::TFLiteEnv &getEnv() { return Env; }

private:
  WasmEdgeTensorflowLite::TFLiteEnv Env;
};

} // namespace Host
} // namespace WasmEdge
