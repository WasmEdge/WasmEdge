// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "host/mock/wasmedge_tensorflowlite_func.h"
#include "runtime/instance/module.h"

namespace WasmEdge {
namespace Host {

class WasmEdgeTensorflowLiteModuleMock
    : public Runtime::Instance::ModuleInstance {
public:
  WasmEdgeTensorflowLiteModuleMock()
      : Runtime::Instance::ModuleInstance("wasmedge_tensorflowlite") {
    addHostFunc("create_session",
                std::make_unique<WasmEdgeTensorflowLiteMock::CreateSession>());
    addHostFunc("delete_session",
                std::make_unique<WasmEdgeTensorflowLiteMock::DeleteSession>());
    addHostFunc("run_session",
                std::make_unique<WasmEdgeTensorflowLiteMock::RunSession>());
    addHostFunc(
        "get_output_tensor",
        std::make_unique<WasmEdgeTensorflowLiteMock::GetOutputTensor>());
    addHostFunc("get_tensor_len",
                std::make_unique<WasmEdgeTensorflowLiteMock::GetTensorLen>());
    addHostFunc("get_tensor_data",
                std::make_unique<WasmEdgeTensorflowLiteMock::GetTensorData>());
    addHostFunc("append_input",
                std::make_unique<WasmEdgeTensorflowLiteMock::AppendInput>());
  }
};

} // namespace Host
} // namespace WasmEdge
