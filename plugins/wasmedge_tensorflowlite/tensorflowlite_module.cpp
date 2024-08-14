// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "tensorflowlite_module.h"
#include "tensorflowlite_func.h"

#include <memory>

namespace WasmEdge {
namespace Host {

WasmEdgeTensorflowLiteModule::WasmEdgeTensorflowLiteModule()
    : Runtime::Instance::ModuleInstance("wasmedge_tensorflowlite") {
  addHostFunc("create_session",
              std::make_unique<WasmEdgeTensorflowLite::CreateSession>(Env));
  addHostFunc("delete_session",
              std::make_unique<WasmEdgeTensorflowLite::DeleteSession>(Env));
  addHostFunc("run_session",
              std::make_unique<WasmEdgeTensorflowLite::RunSession>(Env));
  addHostFunc("get_output_tensor",
              std::make_unique<WasmEdgeTensorflowLite::GetOutputTensor>(Env));
  addHostFunc("get_tensor_len",
              std::make_unique<WasmEdgeTensorflowLite::GetTensorLen>(Env));
  addHostFunc("get_tensor_data",
              std::make_unique<WasmEdgeTensorflowLite::GetTensorData>(Env));
  addHostFunc("append_input",
              std::make_unique<WasmEdgeTensorflowLite::AppendInput>(Env));
}

} // namespace Host
} // namespace WasmEdge
