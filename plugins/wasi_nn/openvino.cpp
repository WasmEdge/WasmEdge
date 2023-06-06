// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "openvino.h"
#include "wasinnenv.h"
#include <algorithm>

namespace WasmEdge::Host::WASINN::OpenVINO {
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_OPENVINO
Expect<WASINN::ErrNo> load(WASINN::WasiNNEnvironment &Env,
                           Span<const Span<uint8_t>> Builders,
                           WASINN::Device Device, uint32_t &GraphId) noexcept {
  // The OpenVINO core must be initialized in constructor.
  if (unlikely(Env.OpenVINOCore == nullptr)) {
    spdlog::error("[WASI-NN] OpenVINO core not initialized.");
    return WASINN::ErrNo::MissingMemory;
  }

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

  // Create the weights blob memory.
  tensor_desc_t WeightsDesc{
      layout_e::ANY, {1, {Weight.size()}}, precision_e::U8};
  IEStatusCode Status =
      ie_blob_make_memory(&WeightsDesc, &(GraphRef.OpenVINOWeightBlob));
  if (Status != IEStatusCode::OK) {
    spdlog::error(
        "[WASI-NN] Unable to create the model's weight blob, error code: {}",
        Status);
    Env.NNGraph.pop_back();
    return WASINN::ErrNo::Busy;
  }

  // Copy the weights buffer to the blob.
  ie_blob_buffer_t BlobBuffer;
  Status = ie_blob_get_buffer(GraphRef.OpenVINOWeightBlob, &BlobBuffer);
  if (unlikely(Status != IEStatusCode::OK)) {
    spdlog::error(
        "[WASI-NN] Unable to find the weight blob's buffer, error code: {}",
        Status);
    Env.NNGraph.pop_back();
    return WASINN::ErrNo::MissingMemory;
  }
  std::copy_n(Weight.data(), Weight.size(),
              static_cast<uint8_t *>(BlobBuffer.buffer));

  // Read network from memory.
  Status = ie_core_read_network_from_memory(
      Env.OpenVINOCore, XML.data(), XML.size(), GraphRef.OpenVINOWeightBlob,
      &(GraphRef.OpenVINONetwork));
  if (Status != IEStatusCode::OK) {
    spdlog::error("[WASI-NN] Unable to read network from the XML and "
                  "Weights, error code: {}",
                  Status);
    Env.NNGraph.pop_back();
    return WASINN::ErrNo::Busy;
  }

  // Get the network input and output size.
  size_t NetworkInputSize = 0;
  Status =
      ie_network_get_inputs_number(GraphRef.OpenVINONetwork, &NetworkInputSize);
  if (unlikely(Status != IEStatusCode::OK)) {
    spdlog::error("[WASI-NN] Unable to get the inputs number from the "
                  "network, error code: {}",
                  Status);
    Env.NNGraph.pop_back();
    return WASINN::ErrNo::MissingMemory;
  }
  spdlog::debug("[WASI-NN] Got input size: {}", NetworkInputSize);
  size_t NetworkOutputSize = 0;
  Status = ie_network_get_outputs_number(GraphRef.OpenVINONetwork,
                                         &NetworkOutputSize);
  if (unlikely(Status != IEStatusCode::OK)) {
    spdlog::error("[WASI-NN] Unable to get the outputs number from the "
                  "network, error code: {}",
                  Status);
    Env.NNGraph.pop_back();
    return WASINN::ErrNo::MissingMemory;
  }
  spdlog::debug("[WASI-NN] Got output size: {}", NetworkOutputSize);

  // Get and store the input and output names.
  GraphRef.OpenVINOInputNames.resize(NetworkInputSize, nullptr);
  for (size_t I = 0; I < NetworkInputSize; I++) {
    Status = ie_network_get_input_name(GraphRef.OpenVINONetwork, I,
                                       &(GraphRef.OpenVINOInputNames[I]));
    if (Status != IEStatusCode::OK) {
      spdlog::error("[WASI-NN] Unable to find input name correctly with "
                    "Index {}, error code: {}",
                    I, Status);
      Env.NNGraph.pop_back();
      return WASINN::ErrNo::MissingMemory;
    }
    spdlog::debug("[WASI-NN] Got input name: {}",
                  GraphRef.OpenVINOInputNames[I]);
  }
  GraphRef.OpenVINOOutputNames.resize(NetworkOutputSize, nullptr);
  for (size_t I = 0; I < NetworkOutputSize; I++) {
    Status = ie_network_get_output_name(GraphRef.OpenVINONetwork, I,
                                        &(GraphRef.OpenVINOOutputNames[I]));
    if (Status != IEStatusCode::OK) {
      spdlog::error("[WASI-NN] Unable to find output name correctly with "
                    "Index {}, error code: {}",
                    I, Status);
      Env.NNGraph.pop_back();
      return WASINN::ErrNo::MissingMemory;
    }
    spdlog::debug("[WASI-NN] Got output name: {}",
                  GraphRef.OpenVINOOutputNames[I]);
  }

  // Set the input layout.
  // FIXME: this is a temporary workaround. We need a more eligant way to
  // specify the layout in the long run. However, without this newer versions
  // of OpenVINO will fail due to parameter mismatch.
  for (size_t I = 0; I < NetworkInputSize; I++) {
    // More layouts should be supported.
    Status = ie_network_set_input_layout(GraphRef.OpenVINONetwork,
                                         GraphRef.OpenVINOInputNames[I],
                                         layout_e::NHWC);
    spdlog::debug("[WASI-NN] Setting [{}] to NHWC",
                  GraphRef.OpenVINOInputNames[I]);
    if (Status != IEStatusCode::OK) {
      spdlog::error("[WASI-NN] Unable to set input layout with the input "
                    "name {}, error code: {}",
                    GraphRef.OpenVINOInputNames[I], Status);
      Env.NNGraph.pop_back();
      return WASINN::ErrNo::MissingMemory;
    }
  }

  // Load network.
  ie_config_t Config = {nullptr, nullptr, nullptr};
  Status = ie_core_load_network(Env.OpenVINOCore, GraphRef.OpenVINONetwork,
                                fmt::format("{}"sv, Device).c_str(), &Config,
                                &GraphRef.OpenVINOExecNetwork);
  if (Status != IEStatusCode::OK) {
    spdlog::error(
        "[WASI-NN] Unable to create executable Network, error code: {}",
        Status);
    Env.NNGraph.pop_back();
    return WASINN::ErrNo::Busy;
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
  if (GraphRef.OpenVINONetwork == nullptr ||
      GraphRef.OpenVINOExecNetwork == nullptr) {
    spdlog::error("[WASI-NN] Model for Graph:{} is empty!", GraphId);
    return WASINN::ErrNo::MissingMemory;
  }

  // Create context.
  Env.NNContext.emplace_back(GraphId, Env.NNGraph[GraphId]);
  auto &CtxRef = Env.NNContext.back().get<Context>();
  if (CtxRef.OpenVINOInferRequest == nullptr) {
    spdlog::error("[WASI-NN] Unable to create openvino context");
    Env.NNContext.pop_back();
    return WASINN::ErrNo::Busy;
  }

  ContextId = Env.NNContext.size() - 1;
  return WASINN::ErrNo::Success;
}

Expect<WASINN::ErrNo> setInput(WASINN::WasiNNEnvironment &Env,
                               uint32_t ContextId, uint32_t Index,
                               const TensorData &Tensor) noexcept {
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  auto &GraphRef = Env.NNGraph[CxtRef.GraphId].get<Graph>();
  // Check the infer request and the network.
  auto *Network = GraphRef.OpenVINONetwork;
  if (Network == nullptr || CxtRef.OpenVINOInferRequest == nullptr) {
    spdlog::error("[WASI-NN] The founded openvino session is empty");
    return WASINN::ErrNo::MissingMemory;
  }

  // Check the input index.
  if (GraphRef.OpenVINOInputNames.size() <= Index) {
    spdlog::error("[WASI-NN] The input index {} exceeds the inputs number {}.",
                  Index, GraphRef.OpenVINOInputNames.size());
    return WASINN::ErrNo::InvalidArgument;
  }
  char *InputName = GraphRef.OpenVINOInputNames[Index];

  if (Tensor.Dimension.size() > 8) {
    spdlog::error(
        "[WASI-NN] Tensor dimension is out of range, expect it under 8-dim, "
        "but got {}-dim.",
        Tensor.Dimension.size());
    return WASINN::ErrNo::InvalidArgument;
  }
  if (Tensor.RType != WASINN::TensorType::F32) {
    spdlog::error(
        "[WASI-NN] Only F32 inputs and outputs are supported for now.");
    return WASINN::ErrNo::InvalidArgument;
  }

  // Set the input resize algorithm.
  // Mark the input as resizable by setting a resize algorithm.
  // In this case we will be able to set an input blob of any shape to an
  // infer request. Resizing and layout conversions are executed automatically
  // when inferring.
  IEStatusCode Status = ie_network_set_input_resize_algorithm(
      Network, InputName, RESIZE_BILINEAR);
  if (Status != IEStatusCode::OK) {
    spdlog::error(
        "[WASI-NN] Unable to set input resize correctly, error code: {}",
        Status);
    return WASINN::ErrNo::InvalidArgument;
  }

  // Set the input layout.
  // More layouts should be supported.
  Status = ie_network_set_input_layout(Network, InputName, layout_e::NHWC);
  if (Status != IEStatusCode::OK) {
    spdlog::error(
        "[WASI-NN] Unable to set input layout correctly, error code: {}",
        Status);
    return WASINN::ErrNo::InvalidArgument;
  }

  // Set the input precision.
  // More types should be supported.
  Status =
      ie_network_set_input_precision(Network, InputName, precision_e::FP32);
  if (Status != IEStatusCode::OK) {
    spdlog::error(
        "[WASI-NN] Unable to set input precision correctly, error code: {}",
        Status);
    return WASINN::ErrNo::InvalidArgument;
  }

  // Set the dimensions and the tensor description.
  dimensions_t Dimens;
  Dimens.ranks = Tensor.Dimension.size();
  for (size_t I = 0; I < Dimens.ranks; I++) {
    Dimens.dims[I] = static_cast<size_t>(Tensor.Dimension[I]);
  }
  tensor_desc_t TensorDesc = {layout_e::NHWC, Dimens, precision_e::FP32};

  // Create the input blob memory.
  ie_blob_t *InputBlob = nullptr;
  Status = ie_blob_make_memory(&TensorDesc, &InputBlob);
  if (Status != IEStatusCode::OK) {
    spdlog::error("[WASI-NN] Unable to allocated input tensor correctly, "
                  "error code: {}",
                  Status);
    return WASINN::ErrNo::Busy;
  }

  // Get the blob buffer size and compare with the tensor size.
  int BlobSize;
  Status = ie_blob_size(InputBlob, &BlobSize);
  if (unlikely(Status != IEStatusCode::OK)) {
    spdlog::error("[WASI-NN] Unable to get the input blob size, error code: {}",
                  Status);
    return WASINN::ErrNo::Busy;
  }
  if (unlikely(static_cast<uint32_t>(BlobSize * 4) != Tensor.Tensor.size())) {
    spdlog::error("[WASI-NN] Blob size {} and the Tensor size {} not matched.",
                  BlobSize * 4, Tensor.Tensor.size());
  }

  // Copy the data into the input blob buffer.
  ie_blob_buffer_t BlobBuffer;
  Status = ie_blob_get_buffer(InputBlob, &BlobBuffer);
  if (unlikely(Status != IEStatusCode::OK)) {
    spdlog::error("[WASI-NN] Unable to find input tensor buffer");
    ie_blob_free(&InputBlob);
    return WASINN::ErrNo::MissingMemory;
  }
  std::copy_n(Tensor.Tensor.data(), Tensor.Tensor.size(),
              static_cast<uint8_t *>(BlobBuffer.buffer));

  // Set input blob.
  Status = ie_infer_request_set_blob(CxtRef.OpenVINOInferRequest, InputName,
                                     InputBlob);
  if (Status != IEStatusCode::OK) {
    spdlog::error("[WASI-NN] Unable to set input tensor to model correctly, "
                  "error code: {}",
                  Status);
    ie_blob_free(&InputBlob);
    return WASINN::ErrNo::Busy;
  }

  ie_blob_free(&InputBlob);

  return WASINN::ErrNo::Success;
}

Expect<WASINN::ErrNo> getOutput(WASINN::WasiNNEnvironment &Env,
                                uint32_t ContextId, uint32_t Index,
                                Span<uint8_t> OutBuffer,
                                uint32_t &BytesWritten) noexcept {
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  auto &GraphRef = Env.NNGraph[CxtRef.GraphId].get<Graph>();
  auto *Network = GraphRef.OpenVINONetwork;

  // Check the output index.
  if (GraphRef.OpenVINOOutputNames.size() <= Index) {
    spdlog::error(
        "[WASI-NN] The output index {} exceeds the outputs number {}.", Index,
        GraphRef.OpenVINOOutputNames.size());
    return WASINN::ErrNo::InvalidArgument;
  }
  char *OutputName = GraphRef.OpenVINOOutputNames[Index];

  // Set output precision.
  IEStatusCode Status =
      ie_network_set_output_precision(Network, OutputName, precision_e::FP32);
  if (Status != IEStatusCode::OK) {
    spdlog::error(
        "[WASI-NN] Unable to set output precision correctly with Index:{}",
        Index);
    return WASINN::ErrNo::InvalidArgument;
  }

  // Get output blob buffer.
  ie_blob_t *OutputBlob = nullptr;
  Status = ie_infer_request_get_blob(CxtRef.OpenVINOInferRequest, OutputName,
                                     &OutputBlob);
  if (Status != IEStatusCode::OK) {
    spdlog::error("[WASI-NN] Unable to retrieve output tensor correctly",
                  Index);
    return WASINN::ErrNo::InvalidArgument;
  }

  // Get the blob size and copy the output buffer.
  int BlobSize;
  Status = ie_blob_size(OutputBlob, &BlobSize);
  ie_blob_buffer_t BlobCBuffer;
  Status = ie_blob_get_cbuffer(OutputBlob, &BlobCBuffer);
  if (Status != IEStatusCode::OK) {
    spdlog::error("[WASI-NN] Unable to retrieve output tensor correctly",
                  Index);
    ie_blob_free(&OutputBlob);
    return WASINN::ErrNo::MissingMemory;
  }
  uint32_t BytesToWrite =
      std::min(static_cast<size_t>(BlobSize * 4), OutBuffer.size());
  std::copy_n(static_cast<const uint8_t *>(BlobCBuffer.cbuffer), BytesToWrite,
              OutBuffer.data());

  // Write the bytes written result.
  BytesWritten = BytesToWrite;

  ie_blob_free(&OutputBlob);

  return WASINN::ErrNo::Success;
}

Expect<WASINN::ErrNo> compute(WASINN::WasiNNEnvironment &Env,
                              uint32_t ContextId) noexcept {
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  IEStatusCode Status = ie_infer_request_infer(CxtRef.OpenVINOInferRequest);
  if (Status != IEStatusCode::OK) {
    spdlog::error(
        "[WASI-NN] Unable to perform computation correctly, error code: {}",
        Status);
    return WASINN::ErrNo::Busy;
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
