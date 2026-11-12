// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "opencvmini_env.h"

#include "runtime/instance/module.h"

namespace WasmEdge {
namespace Host {

class WasmEdgeOpenCVMiniModule : public Runtime::Instance::ModuleInstance {
public:
  WasmEdgeOpenCVMiniModule();

  WasmEdgeOpenCVMiniEnvironment &getEnv() { return Env; }

private:
  WasmEdgeOpenCVMiniEnvironment Env;
};

} // namespace Host
} // namespace WasmEdge
