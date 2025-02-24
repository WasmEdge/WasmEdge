// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "wasinn_openvino_genai.h"
#include "wasinnenv.h"

#include <algorithm>

using namespace std::literals;

namespace WasmEdge::Host::WASINN::OpenVINOGenAI {
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_OPENVINOGENAI
Expect<WASINN::ErrNo> load(WASINN::WasiNNEnvironment &Env,
                           Span<const Span<uint8_t>> Builders,
                           WASINN::Device Device, uint32_t &GraphId) noexcept {
  // The graph builder length must be 2.
  if (Builders.size() != 3) {
    spdlog::error("[WASI-NN] Wrong GraphBuilder Length {:d}, expect 3"sv,
                  Builders.size());
    return WASINN::ErrNo::InvalidArgument;
  }

  // Get the XML and Weight raw buffer.
  //   Builder-0: Reserved, the string is "0"
  //   Builder-1: Models Path

  // There are 4 types (text or img) x (text or img), we assume the input is 0
  // for now.
  [[maybe_unused]] auto ModelType = Builders[0];
  auto ModelsPath = Builders[1];

  // Add a new graph.
  uint32_t GId = Env.newGraph(Backend::OpenVINOGenAI);
  auto &GraphRef = Env.NNGraph[GId].get<Graph>();

  // Store device information
  GraphRef.TargetDevice = Device;

  try {
    std::string ModelPathString(
        reinterpret_cast<const char *>(ModelsPath.data()), ModelsPath.size());
    std::string ModelTypeString(
        reinterpret_cast<const char *>(ModelsPath.data()), ModelsPath.size());

    std::string DeviceString;
    switch (Device) {
    case WASINN::Device::CPU:
      DeviceString = "CPU";
      break;
    case WASINN::Device::GPU:
      DeviceString = "GPU";
      break;
    default:
      throw std::runtime_error("Unsupported device type");
    }

    // Load the model.
    if (ModelTypeString != "0") {
      spdlog::warn("[WASI-NN] Unsupported model type: {}"sv, ModelTypeString);
      spdlog::warn(
          "[WASI-NN] Please set ModelType to 0 to ensure the compatibility"sv);
    }

    GraphRef.OpenVINOLLMPipeline =
        std::make_shared<ov::genai::LLMPipeline>(ModelPathString, DeviceString);

  } catch (const std::exception &EX) {
    spdlog::error("[WASI-NN] Model Load Exception: {}"sv, EX.what());
    Env.deleteGraph(GId);
    return WASINN::ErrNo::RuntimeError;
  }
  // Store the loaded graph.
  GraphId = GId;
  Env.NNGraph[GId].setReady();
  return WASINN::ErrNo::Success;
}

Expect<WASINN::ErrNo> initExecCtx(WASINN::WasiNNEnvironment &Env,
                                  uint32_t GraphId,
                                  uint32_t &ContextId) noexcept {
  // Check the network and the execution network with the graph ID.
  auto &GraphRef = Env.NNGraph[GraphId].get<Graph>();
  if (GraphRef.OpenVINOLLMPipeline == nullptr) {
    spdlog::error("[WASI-NN] Model for Graph:{} is empty!"sv, GraphId);
    return WASINN::ErrNo::MissingMemory;
  }
  // Create context.
  ContextId = Env.newContext(GraphId, Env.NNGraph[GraphId]);
  Env.NNContext[ContextId].setReady();
  return WASINN::ErrNo::Success;
}

Expect<WASINN::ErrNo> setInput(WASINN::WasiNNEnvironment &Env,
                               uint32_t ContextId, uint32_t Index,
                               const TensorData &Tensor) noexcept {
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  auto &GraphRef = Env.NNGraph[CxtRef.GraphId].get<Graph>();

  if (GraphRef.OpenVINOModel == nullptr) {
    spdlog::error("[WASI-NN] The founded openvino session is empty"sv);
    return WASINN::ErrNo::MissingMemory;
  }

  if (Tensor.Dimension.size() > 1) {
    spdlog::error("[WASI-NN] Tensor dimension is out of range, expect it under "
                  "8-dim, but got {}-dim."sv,
                  Tensor.Dimension.size());
    return WASINN::ErrNo::InvalidArgument;
  }

  if (Index != 0) {
    spdlog::error("[WASI-NN] The output index {} is out of range."sv, Index);
    return WASINN::ErrNo::InvalidArgument;
  }

  if (Tensor.RType != WASINN::TensorType::U8) {
    spdlog::error(
        "[WASI-NN] Only F32 inputs and outputs are supported for now."sv);
    return WASINN::ErrNo::InvalidArgument;
  }

  try {
    CxtRef.input =
        std::string(reinterpret_cast<const char *>(Tensor.Tensor.data()),
                    Tensor.Tensor.size());
  } catch (const std::exception &EX) {
    spdlog::error("[WASI-NN] Set Input Exception: {}"sv, EX.what());
    return WASINN::ErrNo::RuntimeError;
  }
  return WASINN::ErrNo::Success;
}

Expect<WASINN::ErrNo> getOutput(WASINN::WasiNNEnvironment &Env,
                                uint32_t ContextId, uint32_t Index,
                                Span<uint8_t> OutBuffer,
                                uint32_t &BytesWritten) noexcept {
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();

  // Check the output index.
  if (Index != 0) {
    spdlog::error("[WASI-NN] The output index {} is out of range."sv, Index);
    return WASINN::ErrNo::InvalidArgument;
  }

  try {
    BytesWritten = CxtRef.answer.size();
    std::copy_n(reinterpret_cast<const uint8_t *>(CxtRef.answer.data()),
                BytesWritten, OutBuffer.data());
  } catch (const std::exception &EX) {
    spdlog::error("[WASI-NN] Get Output Exception: {}"sv, EX.what());
    return WASINN::ErrNo::RuntimeError;
  }
  return WASINN::ErrNo::Success;
}

Expect<WASINN::ErrNo> compute(WASINN::WasiNNEnvironment &Env,
                              uint32_t ContextId) noexcept {
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  auto &GraphRef = Env.NNGraph[CxtRef.GraphId].get<Graph>();
  try {
    CxtRef.answer = GraphRef.OpenVINOLLMPipeline->generate(CxtRef.input);
  } catch (const std::exception &EX) {
    spdlog::error("[WASI-NN] Infer Request Exception: {}"sv, EX.what());
    return WASINN::ErrNo::RuntimeError;
  }
  return WASINN::ErrNo::Success;
}
#else
namespace {
Expect<WASINN::ErrNo> reportBackendNotSupported() noexcept {
  spdlog::error(
      "[WASI-NN] OpenVINO GenAI backend is not built. use "
      "-WASMEDGE_PLUGIN_WASI_NN_BACKEND=\"OpenVINOGenAI\" to build it."sv);
  return WASINN::ErrNo::InvalidArgument;
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
} // namespace WasmEdge::Host::WASINN::OpenVINOGenAI
