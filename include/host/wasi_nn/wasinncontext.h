// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#pragma once
#include <map>
#include <stdint.h>
#include <vector>

#ifdef WASMEDGE_WASINN_BUILD_OPENVINO
#include <common/log.h>
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
  ie_network_t *network = nullptr;
  ie_executable_network_t *exe_network = nullptr;
  ie_infer_request_t *infer_request = nullptr;
};
#endif

class WasiNNContext {
public:
  WasiNNContext() : ModelsNum(-1), ExecutionsNum(-1) {
    spdlog::info("Can you see WasiNNContext");
  }
  // context for implementing WASI-NN
  int ModelsNum;
  int ExecutionsNum;
  std::map<Graph, GraphEncoding> GraphBackends;
  std::map<GraphExecutionContext, GraphEncoding> GraphContextBackends;
  std::map<std::string, GraphEncoding> BackendsMapping;
#ifdef WASMEDGE_WASINN_BUILD_OPENVINO
  ie_core_t *openvino_core = nullptr;
  std::map<Graph, ie_network_t *> OpenVINONetworks;
  std::map<Graph, ie_executable_network_t *> OpenVINOExecutions;
  std::map<GraphExecutionContext, OpenVINOSession> OpenVINOInfers;
#endif
};

} // namespace Host
} // namespace WasmEdge
