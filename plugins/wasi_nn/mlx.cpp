// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2024 Second State INC
#include "wasinnenv.h"

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_MLX
#include <mlx/mlx.h>
#include <mlx_llm/llm.h>
#endif

namespace WasmEdge::Host::WASINN::MLX {
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_MLX

// TODO: Implementation MLX_LLM adds C++ API NN support
// Track Github branch: https://github.com/guptaaryan16/mlx_llm.cpp
// TODO: Add similar API as llama.cpp for loading LLAMA models in MLX
// Right Now, we have a test model to understand the usage of the API

Expect<ErrNo> load(WasiNNEnvironment &Env, Span<const Span<uint8_t>> Builders,
                   Device Device, uint32_t &GraphId) noexcept {
  // The graph builder length must be 1.
  if (Builders.size() != 1) {
    spdlog::error("[WASI-NN] Wrong GraphBuilder Length {:d}, expect 1",
                  Builders.size());
    return ErrNo::InvalidArgument;
  }

  auto Weight = Builders[0];
  // Add a new graph.
  Env.NNGraph.emplace_back(Backend::MLX);
  auto &GraphRef = Env.NNGraph.back().get<Graph>();
  // Setup Graph Device
  if (Device == Device::CPU) {
    GraphRef.MLXDevice = mlx::core::Device::cpu;
  } else if (Device == Device::GPU) {
    if (!mlx::core::metal::is_available()) {
      spdlog::error(
          "[WASI-NN] METAL Unavailable, platform Cannot support GPU target.");
      return ErrNo::InvalidArgument;
    }
    GraphRef.MLXDevice = mlx::core::Device::gpu;
  } else {
    spdlog::error("[WASI-NN] MLX Only support CPU and GPU target.");
    return ErrNo::InvalidArgument;
  }

  std::istringstream BinRead(
      std::string(reinterpret_cast<char *>(Weight.data()), Weight.size()));

  std::string ModelFilePath;

  if (BinModel.substr(0, 8) == "preload:") {
    ModelFilePath = BinModel.substr(8);
  } else {
    if (GraphRef.EnableDebugLog) {
      spdlog::info(
          "[WASI-NN][Debug] MLX backend: Model path not found in nn-preload, write model into a tmpfile."sv);
    }
    // TODO: pass the model directly to mlx
    // Write mlx model to file.
    ModelFilePath = "mlx-safetensors-model.bin"sv;
    std::ofstream TempFile(ModelFilePath);
    if (!TempFile) {
      spdlog::error(
          "[WASI-NN] MLX backend: Failed to create the temporary file. "
          "Currently, our workaround involves creating a temporary model "
          "file named \"mlx-safetensors-model.bin\" and passing this filename "
          "as a parameter to the mlx llama model."sv);
      Env.NNGraph.pop_back();
      return ErrNo::InvalidArgument;
    }
    TempFile << BinModel;
    TempFile.close();
    if (GraphRef.EnableDebugLog) {
      spdlog::info(
          "[WASI-NN][Debug] MLX backend: Write model into a tmpfile...Done"sv);
    }
  }

  // Initialize MLX model with model parameters.
  GraphRef.ModelFilePath = ModelFilePath;

  try {
    // TODO:: Replace the TestModel with LLM API after mlx_llm gets completed
    auto GraphRef.Model = mlx::core::nn::TestModel(); 
    GraphRef.Model.load_weights(ModelFilePath);
  } catch (const c10::Error &e) {
    spdlog::error("[WASI-NN] Failed when load the MLX model.");
    Env.NNGraph.pop_back();
    return ErrNo::InvalidArgument;
  }
  // Store the loaded graph.
  GraphId = Env.NNGraph.size() - 1;
  GraphRef.GraphId = GraphId;
  return ErrNo::Success;
}

Expect<ErrNo> initExecCtx(WasiNNEnvironment &Env, uint32_t GraphId,
                          uint32_t &ContextId) noexcept {
  Env.NNContext.emplace_back(GraphId, Env.NNGraph[GraphId]);

  ContextId = Env.NNContext.size() - 1;
  return ErrNo::Success;
}

Expect<ErrNo> setInput(WasiNNEnvironment &Env, uint32_t ContextId,
                       uint32_t Index, const mlx::core::array &InputArray) noexcept {
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  if (Index >= CxtRef.TorchInputs.size()) {
    CxtRef.TorchInputs.resize(Index + 1);
  }
  if (InputArray.dtype != mlx::core::float32) {
    spdlog::error(
        "[WASI-NN] Only F32 inputs and outputs are supported for now.");
    return ErrNo::InvalidArgument;
  }
  std::vector<int64_t> Dims;
  for (size_t I = 0; I < InputArray.shape().size(); I++) {
    Dims.push_back(static_cast<int64_t>(InputArray.Dimension[I]));
  }
  auto &GraphRef = Env.NNGraph[CxtRef.GraphId].get<Graph>();
  mlx::core::array InTensor = mlx::core::array(
      reinterpret_cast<float *>(InputArray.item()), Dims, mlx::core::float32, mlx::core::to_stream());

  CxtRef.MLXInputs[Index] = InTensor.copy();
  return ErrNo::Success;
}
Expect<ErrNo> getOutput(WasiNNEnvironment &Env, uint32_t ContextId,
                        uint32_t Index, Span<uint8_t> OutBuffer,
                        uint32_t &BytesWritten) noexcept {
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  if (CxtRef.Outputs.size() <= Index) {
    spdlog::error(
        "[WASI-NN] The output index {} exceeds the outputs number {}.", Index,
        CxtRef.Outputs.size());
    return ErrNo::InvalidArgument;
  }
  torch::Tensor Tensor =
      CxtRef.Outputs[Index].to(at::kCPU).toType(torch::kFloat32);
  float *TensorBuffer = OutTensor.data_ptr<float>();

  size_t BlobSize = 1;
  for (auto I : OutTensor.sizes()) {
    BlobSize *= I;
  }
  uint32_t BytesToWrite =
      std::min(static_cast<size_t>(BlobSize * 4), OutBuffer.size());
  std::copy_n(reinterpret_cast<const uint8_t *>(TensorBuffer), BytesToWrite,
              OutBuffer.data());
  BytesWritten = BytesToWrite;
  return ErrNo::Success;
}
Expect<ErrNo> compute(WasiNNEnvironment &Env, uint32_t ContextId) noexcept {
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  if (CxtRef.MLXInputs.size() == 0) {
    spdlog::error("[WASI-NN] Input is not set!");
    return ErrNo::InvalidArgument;
  }
  for (size_t I = 0; I < CxtRef.MLXInputs.shape().size(); I++) {
    mlx::core::array InTensor = CxtRef.TorchInputs[I];
    if (InTensor.isNone()) {
      spdlog::error("[WASI-NN] Input [{}] is not set!", I);
      return ErrNo::InvalidArgument;
    }
  }
  auto &GraphRef = Env.NNGraph[CxtRef.GraphId].get<Graph>();
  mlx::core::array RawOutput =
      GraphRef.MLXModel.forward(CxtRef.MLXInputs);
  // TODO: Output does not seem correct for now
  // if (mlx::core::is_array_v<RawOutput>()) {
  //   auto OutTensors = mlx::core::array(
  // reinterpret_cast<float *>(InputArray.item()), Dims, mlx::core::float32, mlx::core::to_stream());
  //   for (auto &OneOf : OutTensors) {
  //     CxtRef.MLXOutputs.push_back(OneOf.clone());
  //   } else {
  //   spdlog::error("[WASI-NN] PyTorch backend only supports output a tensor, "
  //                 "a list of tensor or a tuple of tensor");
  //   return ErrNo::InvalidArgument;
  // }
  return ErrNo::Success;
}

#else
namespace {
Expect<ErrNo> reportBackendNotSupported() noexcept {
  spdlog::error("[WASI-NN] MLX backend is not built. use "
                "-WASMEDGE_PLUGIN_WASI_NN_BACKEND=\"MLX\" to build it.");
  return ErrNo::InvalidArgument;
}
} // namespace

Expect<WASINN::ErrNo> load(WASINN::WasiNNEnvironment &,
                           Span<const Span<uint8_t>>, WASINN::Device,
                           uint32_t &) noexcept {
  return reportBackendNotSupported();
}
Expect<WASINN::ErrNo> initExecCtx(WASINN::WasiNNEnvironment &, uint32_t,
                                  uint32_t &) noexcept {
  return reportBackendNotSupported();
}
Expect<WASINN::ErrNo> setInput(WASINN::WasiNNEnvironment &, uint32_t, uint32_t,
                               const TensorData &) noexcept {
  return reportBackendNotSupported();
}
Expect<WASINN::ErrNo> getOutput(WASINN::WasiNNEnvironment &, uint32_t, uint32_t,
                                Span<uint8_t>, uint32_t &) noexcept {
  return reportBackendNotSupported();
}
Expect<WASINN::ErrNo> compute(WASINN::WasiNNEnvironment &, uint32_t) noexcept {
  return reportBackendNotSupported();
}

#endif
} // namespace WasmEdge::Host::WASINN::MLX
