#pragma once
#include <map>
#include <onnxruntime_cxx_api.h>

namespace WasmEdge {
namespace Host {

using GraphExecutionContext = uint32_t;
using Graph = uint32_t;

class OnnxSession {
public:
  std::unique_ptr<Ort::SessionOptions> SessionOpt;
  std::unique_ptr<Ort::Env> Env;
  std::unique_ptr<Ort::Session> Session;
};

class WasiNNContext {
public:
  WasiNNContext() : ModelsNum(-1), ExecutionsNum(-1) {}
  // context for implementing WASI-NN
  std::map<Graph, std::vector<uint8_t>> Models;
  std::map<GraphExecutionContext, OnnxSession> Executions;
  int ModelsNum;
  int ExecutionsNum;
};

} // namespace Host
} // namespace WasmEdge
