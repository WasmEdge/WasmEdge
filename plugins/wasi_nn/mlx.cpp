// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2024 Second State INC
#include "wasinnenv.h"

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_MLX
#include <mlx/mlx.h>
#endif

namespace WasmEdge::Host::WASINN::MLX {
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_MLX

// TODO: Implementation MLX adds C++ API NN support
// Track Github branch: https://github.com/guptaaryan16/mlx/tree/Cpp_api
// TODO: Add similar API as llama.cpp for loading LLAMA models in MLX
// struct LLM_MODEL :: mlx::core::nn::Module {
//   StreamOrDevice device = metal::is_available() ? Device::gpu : Device::cpu;
//   // Dummy forward method for inference
// };

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
    auto{GraphRef.ModelWeights, GraphRef.ModelMetadata} =
        mlx::core::load_safetensors(ModelFilePath);
  } catch (const c10::Error &e) {
    spdlog::error("[WASI-NN] Failed when load the MLX model.");
    Env.NNGraph.pop_back();
    return ErrNo::InvalidArgument;
  }
  // Store the loaded graph.
  GraphId = Env.NNGraph.size() - 1;
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
