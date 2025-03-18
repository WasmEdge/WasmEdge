// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "wasinn_openvino_genai.h"
#include "wasinnenv.h"

#include <algorithm>

using namespace std::literals;

namespace WasmEdge::Host::WASINN::OpenVINOGenAI {
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_OPENVINOGENAI

Expect<WASINN::ErrNo> GetDeviceString(WASINN::Device TargetDevice,
                                      std::string &DeviceString) noexcept {
  switch (TargetDevice) {
  case Device::CPU:
    DeviceString = "CPU";
    break;
  case Device::GPU:
    DeviceString = "GPU";
    break;
  default:
    spdlog::error("[WASI-NN] Unsupported device type"sv);
    return WASINN::ErrNo::InvalidArgument;
  }
  return WASINN::ErrNo::Success;
}

Expect<WASINN::ErrNo> isStringTensor(const TensorData &Tensor) noexcept {
  if (Tensor.RType != WASINN::TensorType::U8) {
    spdlog::error(
        "[WASI-NN] Only STRING (u8) inputs and outputs are supported for "
        "now."sv);
    return WASINN::ErrNo::InvalidArgument;
  }
  if (Tensor.Dimension.size() != 1) {
    spdlog::error("[WASI-NN] Tensor dimension is out of range, expect it under "
                  "1-dim, but got {}-dim."sv,
                  Tensor.Dimension.size());
    return WASINN::ErrNo::InvalidArgument;
  }
  return WASINN::ErrNo::Success;
}

Expect<WASINN::ErrNo>
LLMPipelineBackend::SetContextInput(Context &CxtRef, uint32_t Index,
                                    const TensorData &Tensor) {

  if (Index != 0) {
    spdlog::error("[WASI-NN] The input index {} is out of range."sv, Index);
    return WASINN::ErrNo::InvalidArgument;
  }

  if (auto Res = isStringTensor(Tensor); !Res) {
    return Res;
  }

  try {
    CxtRef.StringInput =
        std::string(reinterpret_cast<const char *>(Tensor.Tensor.data()),
                    Tensor.Tensor.size());
  } catch (const std::exception &EX) {
    spdlog::error("[WASI-NN] Set Input Exception: {}"sv, EX.what());
    return WASINN::ErrNo::RuntimeError;
  }
  return WASINN::ErrNo::Success;
}

Expect<WASINN::ErrNo> LLMPipelineBackend::Generate(Context &CxtRef) {
  try {
    CxtRef.StringOutput = Model->generate(CxtRef.StringInput);
  } catch (const std::exception &EX) {
    spdlog::error("[WASI-NN] Generate Exception: {}"sv, EX.what());
    return WASINN::ErrNo::RuntimeError;
  }
  return WASINN::ErrNo::Success;
}

Expect<WASINN::ErrNo>
LLMPipelineBackend::GetContextOutput(Context &CxtRef, uint32_t Index,
                                     Span<uint8_t> OutBuffer,
                                     uint32_t &BytesWritten) {
  if (Index != 0) {
    spdlog::error("[WASI-NN] The output index {} is out of range."sv, Index);
    return WASINN::ErrNo::InvalidArgument;
  }

  try {
    BytesWritten = CxtRef.StringOutput.size();
    std::copy_n(reinterpret_cast<const uint8_t *>(CxtRef.StringOutput.data()),
                BytesWritten, OutBuffer.data());
  } catch (const std::exception &EX) {
    spdlog::error("[WASI-NN] Get Output Exception: {}"sv, EX.what());
    return WASINN::ErrNo::RuntimeError;
  }
  return WASINN::ErrNo::Success;
}

Expect<WASINN::ErrNo> load(WASINN::WasiNNEnvironment &Env,
                           Span<const Span<uint8_t>> Builders,
                           WASINN::Device Device, uint32_t &GraphId) noexcept {
  // The graph builder length must be 3.
  if (Builders.size() != 3) {
    spdlog::error("[WASI-NN] Wrong GraphBuilder Length {:d}, expect 3"sv,
                  Builders.size());
    return WASINN::ErrNo::InvalidArgument;
  }

  // Get the XML and Weight raw buffer.
  //   Builder-0: Reserved, the string "LLMPipeline"
  //   Builder-1: Path to the dir model xml/bin files
  //   Builder-2: Empty for now (reserved for future use)

  // There are 4 types (text or img) x (text or img), we assume the input is 0
  // for now.
  auto ModelType = std::string(
      reinterpret_cast<const char *>(Builders[0].data()), Builders[0].size());
  auto ModelPath = std::string(
      reinterpret_cast<const char *>(Builders[1].data()), Builders[1].size());
  // TODO: Support extra model information. (ex. enable kv cache)
  [[maybe_unused]] auto ModelExtra = std::string(
      reinterpret_cast<const char *>(Builders[2].data()), Builders[2].size());

  // Add a new graph.
  uint32_t GId = Env.newGraph(Backend::OpenVINOGenAI);
  auto &GraphRef = Env.NNGraph[GId].get<Graph>();

  // Store device information
  GraphRef.TargetDevice = Device;
  std::string DeviceString;
  if (auto Err = GetDeviceString(Device, DeviceString);
      Err != WASINN::ErrNo::Success) {
    return Err;
  }

  try {
    // Create the OpenVINO GenAI Backend.
    // Currently, we only support LLMPipeline.
    if (ModelType == "LLMPipeline") {
      GraphRef.OpenVINOGenAI =
          std::make_shared<LLMPipelineBackend>(ModelPath, DeviceString);
    } else {
      spdlog::error("[WASI-NN] Unsupported model type: {}"sv, ModelType);
      return WASINN::ErrNo::InvalidArgument;
    }

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
  if (GraphRef.OpenVINOGenAI == nullptr) {
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

  if (GraphRef.OpenVINOGenAI == nullptr) {
    spdlog::error("[WASI-NN] The founded openvino genei session is empty"sv);
    return WASINN::ErrNo::MissingMemory;
  }

  return GraphRef.OpenVINOGenAI->SetContextInput(CxtRef, Index, Tensor);
}

Expect<WASINN::ErrNo> getOutput(WASINN::WasiNNEnvironment &Env,
                                uint32_t ContextId, uint32_t Index,
                                Span<uint8_t> OutBuffer,
                                uint32_t &BytesWritten) noexcept {
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  auto &GraphRef = Env.NNGraph[CxtRef.GraphId].get<Graph>();

  if (GraphRef.OpenVINOGenAI == nullptr) {
    spdlog::error("[WASI-NN] The founded openvino genei session is empty"sv);
    return WASINN::ErrNo::MissingMemory;
  }

  return GraphRef.OpenVINOGenAI->GetContextOutput(CxtRef, Index, OutBuffer,
                                                  BytesWritten);
}

Expect<WASINN::ErrNo> compute(WASINN::WasiNNEnvironment &Env,
                              uint32_t ContextId) noexcept {
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  auto &GraphRef = Env.NNGraph[CxtRef.GraphId].get<Graph>();
  try {
    GraphRef.OpenVINOGenAI->Generate(CxtRef);
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
