// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#pragma once
#include <cstdint>
#include <map>
#include <string>
#include <vector>
#ifdef WASMEDGE_WASINN_BUILD_OPENVINO
#include <c_api/ie_c_api.h>
#endif
namespace WasmEdge {
namespace Host {

using Graph = uint32_t;
using GraphEncoding = uint8_t;
using ExecutionTarget = uint8_t;
using GraphExecutionContext = uint32_t;

enum class TensorType {
  TENSOR_TYPE_F16,
  TENSOR_TYPE_F32,
  TENSOR_TYPE_U8,
  TENSOR_TYPE_I32
};

#ifdef WASMEDGE_WASINN_BUILD_OPENVINO
class OpenVINOSession {
public:
  ~OpenVINOSession() {
    if (InferRequest != nullptr) {
      ie_infer_request_free(&(InferRequest));
    }
  }
  ie_network_t *Network = nullptr;
  ie_executable_network_t *ExeNetwork = nullptr;
  ie_infer_request_t *InferRequest = nullptr;
};
#endif

class WasiNNContext {
public:
  WasiNNContext() : ModelsNum(-1), ExecutionsNum(-1) {}
  ~WasiNNContext() {
#ifdef WASMEDGE_WASINN_BUILD_OPENVINO
    if (OpenVINOCore != nullptr)
      ie_core_free(&OpenVINOCore);
    for (auto &I : OpenVINONetworks) {
      ie_network_free(&I);
    }
    for (auto &I : OpenVINOExecutions) {
      ie_exec_network_free(&I);
    }
    for (auto &I : OpenVINOInfers) {
      delete I;
    }
    for (auto &I : OpenVINOInputs) {
      if (I != nullptr)
        ie_blob_free(&I);
    }
    for (auto &I : OpenVINOModelWeights) {
      if (I != nullptr)
        ie_blob_free(&I);
    }
#endif
  }

  // context for implementing WASI-NN
  int ModelsNum;
  int ExecutionsNum;
  std::vector<GraphEncoding> GraphBackends;
  std::vector<GraphEncoding> GraphContextBackends;
  std::map<std::string, GraphEncoding> BackendsMapping;
#ifdef WASMEDGE_WASINN_BUILD_OPENVINO
  ie_core_t *OpenVINOCore = nullptr;
  std::vector<ie_network_t *> OpenVINONetworks;
  std::vector<ie_executable_network_t *> OpenVINOExecutions;
  std::vector<ie_blob_t *> OpenVINOInputs;
  std::vector<ie_blob_t *> OpenVINOModelWeights;
  std::vector<OpenVINOSession *> OpenVINOInfers;
#endif
};

} // namespace Host
} // namespace WasmEdge
