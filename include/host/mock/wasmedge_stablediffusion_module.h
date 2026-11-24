// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "host/mock/wasmedge_stablediffusion_func.h"
#include "runtime/instance/module.h"

#include <memory>

namespace WasmEdge {
namespace Host {

class WasmEdgeStableDiffusionModuleMock
    : public Runtime::Instance::ModuleInstance {
public:
  WasmEdgeStableDiffusionModuleMock()
      : Runtime::Instance::ModuleInstance("wasmedge_stablediffusion") {
    addHostFunc("abi_version",
                std::make_unique<WasmEdgeStableDiffusionMock::AbiVersion>());
    addHostFunc("session_open",
                std::make_unique<WasmEdgeStableDiffusionMock::SessionOpen>());
    addHostFunc("session_close",
                std::make_unique<WasmEdgeStableDiffusionMock::SessionClose>());
    addHostFunc("generate",
                std::make_unique<WasmEdgeStableDiffusionMock::Generate>());
    addHostFunc("result_info",
                std::make_unique<WasmEdgeStableDiffusionMock::ResultInfo>());
    addHostFunc("result_read",
                std::make_unique<WasmEdgeStableDiffusionMock::ResultRead>());
  }
};

} // namespace Host
} // namespace WasmEdge
