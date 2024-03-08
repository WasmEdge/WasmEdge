// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2023 Second State INC

#pragma once

#include "runtime/instance/module.h"
#include "wasiocrenv.h"

namespace WasmEdge {
namespace Host {

class WasiOCRModule : public Runtime::Instance::ModuleInstance {
public:
  WasiOCRModule();

  WASIOCR::WasiOCREnvironment &getEnv() { return Env; }

private:
  WASIOCR::WasiOCREnvironment Env;
};

} // namespace Host
} // namespace WasmEdge
