// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "host/mock/wasi_nn_func.h"
#include "runtime/instance/module.h"

namespace WasmEdge {
namespace Host {

class WasiNNModuleMock : public Runtime::Instance::ModuleInstance {
public:
  WasiNNModuleMock() : Runtime::Instance::ModuleInstance("wasi_ephemeral_nn") {
    addHostFunc("load", std::make_unique<WasiNNMock::Load>());
    addHostFunc("init_execution_context",
                std::make_unique<WasiNNMock::InitExecCtx>());
    addHostFunc("set_input", std::make_unique<WasiNNMock::SetInput>());
    addHostFunc("get_output", std::make_unique<WasiNNMock::GetOutput>());
    addHostFunc("compute", std::make_unique<WasiNNMock::Compute>());
  }
};

} // namespace Host
} // namespace WasmEdge
