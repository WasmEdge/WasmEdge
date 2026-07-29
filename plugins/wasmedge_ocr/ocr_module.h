// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "ocr_env.h"

#include "runtime/instance/module.h"

namespace WasmEdge {
namespace Host {

class WasmEdgeOCRModule : public Runtime::Instance::ModuleInstance {
public:
  WasmEdgeOCRModule();

  WasmEdgeOCR::OCREnv &getEnv() { return Env; }

private:
  WasmEdgeOCR::OCREnv Env;
};

} // namespace Host
} // namespace WasmEdge
