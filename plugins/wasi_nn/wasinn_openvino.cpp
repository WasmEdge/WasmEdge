// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "wasinn_openvino.h"
#include "wasinnenv.h"

#include <algorithm>

using namespace std::literals;

namespace WasmEdge::Host::WASINN::OpenVINO {
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_OPENVINO

static Expect<WASINN::ErrNo>
GetDeviceString(WASINN::Device TargetDevice,
                std::string &DeviceString) noexcept {
  switch (TargetDevice) {
  case Device::AUTO:
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

Expect<WASINN::ErrNo> load(WASINN::WasiNNEnvironment &Env, WASINN::Graph &G,
                           Span<const Span<uint8_t>> Builders,
                           WASINN::Device Device) noexcept {
  auto &GraphRef = G.get<Graph>();

  // The graph builder length must be 2.
  if (Builders.size() != 2) {
    spdlog::error("[WASI-NN] Wrong GraphBuilder Length {:d}, expect 2"sv,
                  Builders.size());
    return WASINN::ErrNo::InvalidArgument;
  }

  // Get the XML and Weight raw buffer.
  //   Builder-0: the XML string
  //   Builder-1: the Weight binary
  auto XML = Builders[0];
  auto Weight = Builders[1];

  // Store device information
  GraphRef.TargetDevice = Device;

  try {
    std::string ModelString(reinterpret_cast<const char *>(XML.data()),
                            XML.size());
    GraphRef.OpenVINOIWeightTensor =
        ov::Tensor(ov::element::Type_t::u8, {Weight.size()});
    std::copy_n(Weight.data(), Weight.size(),
                static_cast<uint8_t *>(GraphRef.OpenVINOIWeightTensor.data()));
    GraphRef.OpenVINOModel = Env.OpenVINOCore.read_model(
        ModelString, GraphRef.OpenVINOIWeightTensor);
  } catch (const std::exception &EX) {
    spdlog::error("[WASI-NN] Model Load Exception: {}"sv, EX.what());
    return WASINN::ErrNo::RuntimeError;
  }
  return WASINN::ErrNo::Success;
}

Expect<WASINN::ErrNo>
initExecCtx([[maybe_unused]] WASINN::WasiNNEnvironment &Env, WASINN::Graph &G,
            [[maybe_unused]] WASINN::Context &C) noexcept {
  // Check the network and the execution network with the graph.
  auto &GraphRef = G.get<Graph>();
  if (GraphRef.OpenVINOModel == nullptr) {
    spdlog::error("[WASI-NN] Model for the graph is empty!"sv);
    return WASINN::ErrNo::MissingMemory;
  }
  return WASINN::ErrNo::Success;
}

Expect<WASINN::ErrNo> setInput(WASINN::WasiNNEnvironment &Env, WASINN::Graph &G,
                               WASINN::Context &C, uint32_t Index,
                               const TensorData &Tensor) noexcept {
  auto &GraphRef = G.get<Graph>();
  auto &CxtRef = C.get<Context>();

  if (GraphRef.OpenVINOModel == nullptr) {
    spdlog::error("[WASI-NN] The founded openvino session is empty"sv);
    return WASINN::ErrNo::MissingMemory;
  }

  if (Tensor.Dimension.size() > 8) {
    spdlog::error("[WASI-NN] Tensor dimension is out of range, expect it under "
                  "8-dim, but got {}-dim."sv,
                  Tensor.Dimension.size());
    return WASINN::ErrNo::InvalidArgument;
  }
  if (Tensor.RType != WASINN::TensorType::F32) {
    spdlog::error(
        "[WASI-NN] Only F32 inputs and outputs are supported for now."sv);
    return WASINN::ErrNo::InvalidArgument;
  }

  // Check the input index.
  if (GraphRef.OpenVINOModel->inputs().size() <= Index) {
    spdlog::error(
        "[WASI-NN] The input index {} exceeds the inputs number {}."sv, Index,
        GraphRef.OpenVINOModel->inputs().size());
    return WASINN::ErrNo::InvalidArgument;
  }

  try {
    ov::element::Type InputType = ov::element::f32;
    ov::Shape InputShape(Tensor.Dimension.data(),
                         Tensor.Dimension.data() + Tensor.Dimension.size());
    ov::Tensor InputTensor =
        ov::Tensor(InputType, InputShape, Tensor.Tensor.data());
    std::string Device;
    if (!GetDeviceString(GraphRef.TargetDevice, Device)) {
      spdlog::error("[WASI-NN] Failed to get device string for OpenVINO."sv);
      return WASINN::ErrNo::InvalidArgument;
    }

    ov::CompiledModel CompiledModel =
        Env.OpenVINOCore.compile_model(GraphRef.OpenVINOModel, Device);
    CxtRef.OpenVINOInferRequest = CompiledModel.create_infer_request();
    CxtRef.OpenVINOInferRequest.set_input_tensor(Index, InputTensor);
  } catch (const std::exception &EX) {
    spdlog::error("[WASI-NN] Set Input Exception: {}"sv, EX.what());
    return WASINN::ErrNo::RuntimeError;
  }
  return WASINN::ErrNo::Success;
}

Expect<WASINN::ErrNo> getOutput([[maybe_unused]] WASINN::WasiNNEnvironment &Env,
                                WASINN::Graph &G, WASINN::Context &C,
                                uint32_t Index, Span<uint8_t> OutBuffer,
                                uint32_t &BytesWritten) noexcept {
  auto &GraphRef = G.get<Graph>();
  auto &CxtRef = C.get<Context>();

  // Check the output index.
  if (GraphRef.OpenVINOModel->outputs().size() <= Index) {
    spdlog::error(
        "[WASI-NN] The output index {} exceeds the outputs number {}."sv, Index,
        GraphRef.OpenVINOModel->outputs().size());
    return WASINN::ErrNo::InvalidArgument;
  }

  try {
    const ov::Tensor &OutputTensor =
        CxtRef.OpenVINOInferRequest.get_output_tensor(Index);
    BytesWritten = static_cast<uint32_t>(OutputTensor.get_byte_size());
    if (OutBuffer.size() < OutputTensor.get_byte_size()) {
      spdlog::error("[WASI-NN] OpenVINO backend: output buffer too small, "
                    "need {} bytes but got {}."sv,
                    OutputTensor.get_byte_size(), OutBuffer.size());
      return WASINN::ErrNo::TooLarge;
    }
    std::copy_n(static_cast<const uint8_t *>(OutputTensor.data()), BytesWritten,
                OutBuffer.data());
  } catch (const std::exception &EX) {
    spdlog::error("[WASI-NN] Get Output Exception: {}"sv, EX.what());
    return WASINN::ErrNo::RuntimeError;
  }
  return WASINN::ErrNo::Success;
}

Expect<WASINN::ErrNo> compute([[maybe_unused]] WASINN::WasiNNEnvironment &Env,
                              [[maybe_unused]] WASINN::Graph &G,
                              WASINN::Context &C) noexcept {
  auto &CxtRef = C.get<Context>();
  try {
    CxtRef.OpenVINOInferRequest.infer();
  } catch (const std::exception &EX) {
    spdlog::error("[WASI-NN] Infer Request Exception: {}"sv, EX.what());
    return WASINN::ErrNo::RuntimeError;
  }
  return WASINN::ErrNo::Success;
}
#else
namespace {
Expect<WASINN::ErrNo> reportBackendNotSupported() noexcept {
  spdlog::error("[WASI-NN] OpenVINO backend is not built. use "
                "-WASMEDGE_PLUGIN_WASI_NN_BACKEND=\"OpenVINO\" to build it."sv);
  return WASINN::ErrNo::InvalidArgument;
}
} // namespace

Expect<WASINN::ErrNo> load(WASINN::WasiNNEnvironment &, WASINN::Graph &,
                           Span<const Span<uint8_t>>, WASINN::Device) noexcept {
  return reportBackendNotSupported();
}
Expect<WASINN::ErrNo> initExecCtx(WASINN::WasiNNEnvironment &, WASINN::Graph &,
                                  WASINN::Context &) noexcept {
  return reportBackendNotSupported();
}
Expect<WASINN::ErrNo> setInput(WASINN::WasiNNEnvironment &, WASINN::Graph &,
                               WASINN::Context &, uint32_t,
                               const TensorData &) noexcept {
  return reportBackendNotSupported();
}
Expect<WASINN::ErrNo> getOutput(WASINN::WasiNNEnvironment &, WASINN::Graph &,
                                WASINN::Context &, uint32_t, Span<uint8_t>,
                                uint32_t &) noexcept {
  return reportBackendNotSupported();
}
Expect<WASINN::ErrNo> compute(WASINN::WasiNNEnvironment &, WASINN::Graph &,
                              WASINN::Context &) noexcept {
  return reportBackendNotSupported();
}
#endif
} // namespace WasmEdge::Host::WASINN::OpenVINO
