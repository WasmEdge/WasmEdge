// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "host/mock/wasmedge_tensorflow_func.h"
#include "runtime/instance/module.h"

namespace WasmEdge {
namespace Host {

class WasmEdgeTensorflowModuleMock : public Runtime::Instance::ModuleInstance {
public:
  WasmEdgeTensorflowModuleMock()
      : Runtime::Instance::ModuleInstance("wasmedge_tensorflow") {
    addHostFunc("create_session",
                std::make_unique<WasmEdgeTensorflowMock::CreateSession>());
    addHostFunc("delete_session",
                std::make_unique<WasmEdgeTensorflowMock::DeleteSession>());
    addHostFunc("run_session",
                std::make_unique<WasmEdgeTensorflowMock::RunSession>());
    addHostFunc("get_output_tensor",
                std::make_unique<WasmEdgeTensorflowMock::GetOutputTensor>());
    addHostFunc("get_tensor_len",
                std::make_unique<WasmEdgeTensorflowMock::GetTensorLen>());
    addHostFunc("get_tensor_data",
                std::make_unique<WasmEdgeTensorflowMock::GetTensorData>());
    addHostFunc("append_input",
                std::make_unique<WasmEdgeTensorflowMock::AppendInput>());
    addHostFunc("append_output",
                std::make_unique<WasmEdgeTensorflowMock::AppendOutput>());
    addHostFunc("clear_input",
                std::make_unique<WasmEdgeTensorflowMock::ClearInput>());
    addHostFunc("clear_output",
                std::make_unique<WasmEdgeTensorflowMock::ClearOutput>());
  }
};

} // namespace Host
} // namespace WasmEdge
