// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "openvino.h"
#include "wasinnenv.h"
#include <algorithm>

namespace WasmEdge::Host::WASINN::OpenVINO {
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_OPENVINO
Expect<WASINN::ErrNo> load(WASINN::WasiNNEnvironment &Env,
                           Span<const Span<uint8_t>> Builders,
                           WASINN::Device Device, uint32_t &GraphId) noexcept {
  // The graph builder length must be 2.
  if (Builders.size() != 2) {
    spdlog::error("[WASI-NN] Wrong GraphBuilder Length {:d}, expect 2",
                  Builders.size());
    return WASINN::ErrNo::InvalidArgument;
  }

  // Get the XML and Weight raw buffer.
  //   Builder-0: the XML string
  //   Builder-1: the Weight binary
  auto XML = Builders[0];
  auto Weight = Builders[1];

  // Add a new graph.
  Env.NNGraph.emplace_back(Backend::OpenVINO);
  auto &GraphRef = Env.NNGraph.back().get<Graph>();

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
    spdlog::error("[WASI-NN] Model Load Exception: {}", EX.what());
    Env.NNGraph.pop_back();
    return WASINN::ErrNo::RuntimeError;
  }
  // Store the loaded graph.
  GraphId = Env.NNGraph.size() - 1;
  return WASINN::ErrNo::Success;
}

Expect<WASINN::ErrNo> initExecCtx(WASINN::WasiNNEnvironment &Env,
                                  uint32_t GraphId,
                                  uint32_t &ContextId) noexcept {
  // Check the network and the execution network with the graph ID.
  auto &GraphRef = Env.NNGraph[GraphId].get<Graph>();
  if (GraphRef.OpenVINOModel == nullptr) {
    spdlog::error("[WASI-NN] Model for Graph:{} is empty!", GraphId);
    return WASINN::ErrNo::MissingMemory;
  }
  // Create context.
  Env.NNContext.emplace_back(GraphId, Env.NNGraph[GraphId]);
  ContextId = Env.NNContext.size() - 1;
  return WASINN::ErrNo::Success;
}

Expect<WASINN::ErrNo> setInput(WASINN::WasiNNEnvironment &Env,
                               uint32_t ContextId, uint32_t Index,
                               const TensorData &Tensor) noexcept {
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  auto &GraphRef = Env.NNGraph[CxtRef.GraphId].get<Graph>();

  if (GraphRef.OpenVINOModel == nullptr) {
    spdlog::error("[WASI-NN] The founded openvino session is empty");
    return WASINN::ErrNo::MissingMemory;
  }

  if (Tensor.Dimension.size() > 8) {
    spdlog::error("[WASI-NN] Tensor dimension is out of range, expect "
                  "it under 8-dim, "
                  "but got {}-dim.",
                  Tensor.Dimension.size());
    return WASINN::ErrNo::InvalidArgument;
  }
  if (Tensor.RType != WASINN::TensorType::F32) {
    spdlog::error(
        "[WASI-NN] Only F32 inputs and outputs are supported for now.");
    return WASINN::ErrNo::InvalidArgument;
  }

  // Check the input index.
  if (GraphRef.OpenVINOModel->inputs().size() <= Index) {
    spdlog::error("[WASI-NN] The input index {} exceeds the inputs number {}.",
                  Index, GraphRef.OpenVINOModel->inputs().size());
    return WASINN::ErrNo::InvalidArgument;
  }

  try {
    ov::element::Type InputType = ov::element::f32;
    ov::Shape InputShape = {1, 224, 224, 3};
    ov::Tensor InputTensor =
        ov::Tensor(InputType, InputShape, Tensor.Tensor.data());
    const ov::Layout InputLayout{"NHWC"};
    ov::preprocess::PrePostProcessor PPP(GraphRef.OpenVINOModel);
    PPP.input()
        .tensor()
        .set_shape(InputShape)
        .set_element_type(InputType)
        .set_layout(InputLayout);
    PPP.input().preprocess().resize(
        ov::preprocess::ResizeAlgorithm::RESIZE_LINEAR);
    PPP.input().model().set_layout("NCHW");
    PPP.output().tensor().set_element_type(ov::element::f32);
    auto model = PPP.build();
    ov::CompiledModel CompiledModel =
        Env.OpenVINOCore.compile_model(model, "CPU");
    CxtRef.OpenVINOInferRequest = CompiledModel.create_infer_request();
    CxtRef.OpenVINOInferRequest.set_input_tensor(Index, InputTensor);
  } catch (const std::exception &EX) {
    spdlog::error("[WASI-NN] Set Input Exception: {}", EX.what());
    return WASINN::ErrNo::RuntimeError;
  }
  return WASINN::ErrNo::Success;
}

Expect<WASINN::ErrNo> getOutput(WASINN::WasiNNEnvironment &Env,
                                uint32_t ContextId, uint32_t Index,
                                Span<uint8_t> OutBuffer,
                                uint32_t &BytesWritten) noexcept {
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  auto &GraphRef = Env.NNGraph[CxtRef.GraphId].get<Graph>();

  // Check the output index.
  if (GraphRef.OpenVINOModel->outputs().size() <= Index) {
    spdlog::error(
        "[WASI-NN] The output index {} exceeds the outputs number {}.", Index,
        GraphRef.OpenVINOModel->outputs().size());
    return WASINN::ErrNo::InvalidArgument;
  }

  try {
    const ov::Tensor &OutputTensor =
        CxtRef.OpenVINOInferRequest.get_output_tensor(Index);
    BytesWritten = OutputTensor.get_byte_size();
    std::copy_n(static_cast<const uint8_t *>(OutputTensor.data()), BytesWritten,
                OutBuffer.data());
  } catch (const std::exception &EX) {
    spdlog::error("[WASI-NN] Get Output Exception: {}", EX.what());
    return WASINN::ErrNo::RuntimeError;
  }
  return WASINN::ErrNo::Success;
}

Expect<WASINN::ErrNo> compute(WASINN::WasiNNEnvironment &Env,
                              uint32_t ContextId) noexcept {
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  try {
    CxtRef.OpenVINOInferRequest.infer();
  } catch (const std::exception &EX) {
    spdlog::error("[WASI-NN] Infer Request Exception: {}", EX.what());
    return WASINN::ErrNo::RuntimeError;
  }
  return WASINN::ErrNo::Success;
}
#else
namespace {
Expect<WASINN::ErrNo> reportBackendNotSupported() noexcept {
  spdlog::error("[WASI-NN] OpenVINO backend is not built. use "
                "-WASMEDGE_PLUGIN_WASI_NN_BACKEND=\"OpenVINO\" to build it.");
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
} // namespace WasmEdge::Host::WASINN::OpenVINO
