// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#pragma once

#include "opencvenv.h"
#include "runtime/instance/module.h"
#include <cstdint>

namespace WasmEdge {
namespace Host {

class WasmEdgeOpenCVCore : public Runtime::Instance::ModuleInstance {
public:
  WasmEdgeOpenCVCore();

  WasmEdgeOpenCVEnvironment &getEnv() { return Env; }

private:
  WasmEdgeOpenCVEnvironment Env;
};

} // namespace Host
} // namespace WasmEdge
