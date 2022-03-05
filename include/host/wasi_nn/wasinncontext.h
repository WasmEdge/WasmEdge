#pragma once
#include <map>
#include <stdint.h>
#include <vector>
#ifdef WASINN_BUILD_ONNX
#include <onnxruntime_cxx_api.h>
#endif

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

#ifdef WASINN_BUILD_ONNX
class OnnxSession {
public:
  std::unique_ptr<Ort::AllocatorWithDefaultOptions> Allocator;
  std::unique_ptr<Ort::SessionOptions> SessionOpt;
  std::unique_ptr<Ort::Env> Env;
  std::unique_ptr<Ort::Session> OrtSession;

  /// Store the data of input and output tensors
  std::vector<std::vector<float>> InputTensorsValue;
  std::vector<std::vector<float>> OutputTensorsValue;

  std::vector<std::vector<int64_t>> InputTensorsDims;
  std::vector<std::vector<int64_t>> OutputTensorsDims;

  /// Data structure in ONNX. Each Ort::Value point to an
  /// element in `InputTensorsValue` or `OutputTensorsValue`
  std::vector<Ort::Value> InputTensors;
  std::vector<Ort::Value> OutputTensors;
};
#endif

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
#ifdef WASINN_BUILD_ONNX
  std::map<Graph, std::vector<uint8_t> > OnnxModels;
  std::map<GraphExecutionContext, OnnxSession> OnnxExecutions;
  std::unique_ptr<Ort::MemoryInfo> OnnxMemoryInfo;
#endif
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
