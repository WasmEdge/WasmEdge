// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#pragma once

#include "runtime/instance/module.h"
#include "opencvenv.h"
#include <cstdint>

namespace WasmEdge {
namespace Host {

class WasmEdgeOpenCvImgcodecsModule : public Runtime::Instance::ModuleInstance {
public:
  WasmEdgeOpenCvImgcodecsModule();

  WasmEdgeOpenCvEnvironment &getEnv() { return Env; }

private:
  WasmEdgeOpenCvEnvironment Env;
};

} // namespace Host
} // namespace WasmEdge
