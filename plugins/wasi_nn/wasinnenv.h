// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#pragma once

#include "common/log.h"
#include "plugin/plugin.h"
#include <cstdint>
#include <vector>

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_OPENVINO
#include <c_api/ie_c_api.h>
#endif
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_TORCH
#include <torch/script.h>
#endif

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_TFLITE
#include "tensorflow/lite/c/c_api.h"
#endif

namespace WasmEdge {
namespace Host {
namespace WASINN {

enum class ErrNo : uint32_t {
  Success = 0,         // No error occurred.
  InvalidArgument = 1, // Caller module passed an invalid argument.
  MissingMemory = 2,   // Caller module is missing a memory export.
  Busy = 3             // Device or resource busy.
};

enum class TensorType : uint8_t { F16 = 0, F32 = 1, U8 = 2, I32 = 3 };

enum class Backend : uint8_t {
  OpenVINO = 0,
  ONNX = 1,
  Tensorflow = 2,
  PyTorch = 3,
  TensorflowLite = 4
};

class Graph {
public:
  Graph() = delete;
  Graph(Backend BE) noexcept : GraphBackend(BE) {
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_OPENVINO
    OpenVINONetwork = nullptr;
    OpenVINOExecNetwork = nullptr;
    OpenVINOWeightBlob = nullptr;
#endif
  }
  ~Graph() noexcept {
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_OPENVINO
    if (OpenVINONetwork) {
      ie_network_free(&OpenVINONetwork);
    }
    if (OpenVINOExecNetwork) {
      ie_exec_network_free(&OpenVINOExecNetwork);
    }
    if (OpenVINOWeightBlob) {
      ie_blob_free(&OpenVINOWeightBlob);
    }
    for (auto &I : OpenVINOInputNames) {
      if (I) {
        ie_network_name_free(&I);
      }
    }
    for (auto &I : OpenVINOOutputNames) {
      if (I) {
        ie_network_name_free(&I);
      }
    }
#endif
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_TFLITE
    if (TFLiteMod) {
      TfLiteModelDelete(TFLiteMod);
    }
#endif
  }

  Backend GraphBackend;
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_OPENVINO
  ie_network_t *OpenVINONetwork;
  ie_executable_network_t *OpenVINOExecNetwork;
  ie_blob_t *OpenVINOWeightBlob;
  std::vector<char *> OpenVINOInputNames;
  std::vector<char *> OpenVINOOutputNames;
#endif
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_TORCH
  torch::jit::Module TorchModel;
#endif
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_TFLITE
  TfLiteModel *TFLiteMod = nullptr;
#endif
};

class Context {
public:
  Context() = delete;

  Context(Graph &G) noexcept : GraphRef(G) {
    if (G.GraphBackend == Backend::OpenVINO) {
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_OPENVINO
      IEStatusCode Status = ie_exec_network_create_infer_request(
          G.OpenVINOExecNetwork, &OpenVINOInferRequest);
      if (Status != IEStatusCode::OK) {
        OpenVINOInferRequest = nullptr;
        spdlog::error("[WASI-NN] Unable to create infer request for OpenVINO");
      }
#endif
    }
  }

  ~Context() noexcept {
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_OPENVINO
    if (OpenVINOInferRequest) {
      ie_infer_request_free(&OpenVINOInferRequest);
    }
#endif
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_TFLITE
    if (TFLiteInterp) {
      TfLiteInterpreterDelete(TFLiteInterp);
    }
#endif
  }

  Graph &GraphRef;
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_OPENVINO
  ie_infer_request_t *OpenVINOInferRequest = nullptr;
#endif
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_TORCH
  std::vector<torch::jit::IValue> TorchInputs;
  std::vector<at::Tensor> TorchOutputs;
#endif
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_TFLITE
  TfLiteInterpreter *TFLiteInterp = nullptr;
#endif
};

class WasiNNEnvironment {
public:
  WasiNNEnvironment() noexcept {
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_OPENVINO
    if (ie_core_create("", &OpenVINOCore) != IEStatusCode::OK) {
      spdlog::error(
          "[WASI-NN] Error happened when initializing OpenVINO core.");
    }
#endif
    NNGraph.reserve(16U);
    NNContext.reserve(16U);
  }
  ~WasiNNEnvironment() noexcept {
    NNContext.clear();
    NNGraph.clear();
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_OPENVINO
    if (OpenVINOCore) {
      ie_core_free(&OpenVINOCore);
    }
#endif
  }

  std::vector<Graph> NNGraph;
  std::vector<Context> NNContext;
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_OPENVINO
  ie_core_t *OpenVINOCore = nullptr;
#endif

  static Plugin::PluginRegister Register;
};

} // namespace WASINN
} // namespace Host
} // namespace WasmEdge
