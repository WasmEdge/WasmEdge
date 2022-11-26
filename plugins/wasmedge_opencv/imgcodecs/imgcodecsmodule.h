// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#pragma once

#include "imgcodecsenv.h"
#include "runtime/instance/module.h"
#include <cstdint>

namespace WasmEdge {
namespace Host {

class WasmEdgeOpenCvImgcodecsModule : public Runtime::Instance::ModuleInstance {
public:
  WasmEdgeOpenCvImgcodecsModule();

  WasmEdgeOpenCvImgcodecsEnvoronment &getEnv() { return Env; }

private:
  WasmEdgeOpenCvImgcodecsEnvoronment Env;
};

} // namespace Host
} // namespace WasmEdge
