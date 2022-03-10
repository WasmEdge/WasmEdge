// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#pragma once
#include <map>
#include <stdint.h>
#include <vector>

#ifdef WASINN_BUILD_OPENVINO
#include <inference_engine.hpp>
namespace IE = InferenceEngine;
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

#ifdef WASINN_BUILD_OPENVINO
class OpenVINOSession {
public:
  IE::CNNNetwork *network;
  IE::ExecutableNetwork *executable_network;
  std::unique_ptr<IE::InferRequest> infer_request;
};
#endif

class WasiNNContext {
public:
  WasiNNContext() : ModelsNum(-1), ExecutionsNum(-1) {}
  // context for implementing WASI-NN
  std::map<Graph, GraphEncoding> GraphBackends;
  std::map<GraphExecutionContext, GraphEncoding> GraphContextBackends;
#ifdef WASINN_BUILD_OPENVINO
  IE::Core openvino_ie;
  std::map<Graph, IE::CNNNetwork> OpenVINONetworks;
  std::map<Graph, IE::ExecutableNetwork> OpenVINOExecutions;
  std::map<GraphExecutionContext, OpenVINOSession> OpenVINOInfers;
#endif
  int ModelsNum;
  int ExecutionsNum;
  std::map<std::string, GraphEncoding> BackendsMapping;
};

} // namespace Host
} // namespace WasmEdge
