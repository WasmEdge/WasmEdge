// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "tensorflow_module.h"
#include "tensorflow_func.h"

#include <memory>

namespace WasmEdge {
namespace Host {

WasmEdgeTensorflowModule::WasmEdgeTensorflowModule()
    : Runtime::Instance::ModuleInstance("wasmedge_tensorflow") {
  addHostFunc("create_session",
              std::make_unique<WasmEdgeTensorflow::CreateSession>(Env));
  addHostFunc(
      "create_session_saved_model",
      std::make_unique<WasmEdgeTensorflow::CreateSessionSavedModel>(Env));
  addHostFunc("delete_session",
              std::make_unique<WasmEdgeTensorflow::DeleteSession>(Env));
  addHostFunc("run_session",
              std::make_unique<WasmEdgeTensorflow::RunSession>(Env));
  addHostFunc("get_output_tensor",
              std::make_unique<WasmEdgeTensorflow::GetOutputTensor>(Env));
  addHostFunc("get_tensor_len",
              std::make_unique<WasmEdgeTensorflow::GetTensorLen>(Env));
  addHostFunc("get_tensor_data",
              std::make_unique<WasmEdgeTensorflow::GetTensorData>(Env));
  addHostFunc("append_input",
              std::make_unique<WasmEdgeTensorflow::AppendInput>(Env));
  addHostFunc("append_output",
              std::make_unique<WasmEdgeTensorflow::AppendOutput>(Env));
  addHostFunc("clear_input",
              std::make_unique<WasmEdgeTensorflow::ClearInput>(Env));
  addHostFunc("clear_output",
              std::make_unique<WasmEdgeTensorflow::ClearOutput>(Env));
}

} // namespace Host
} // namespace WasmEdge
