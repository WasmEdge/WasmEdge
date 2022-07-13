// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "wasinnfunc.h"
#include "common/log.h"

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_OPENVINO
#include <algorithm>
#include <string>

#include <c_api/ie_c_api.h>
#endif

namespace WasmEdge {
namespace Host {

Expect<uint32_t> WasiNNLoad::body(Runtime::Instance::MemoryInstance *MemInst,
                                  uint32_t BuilderPtr [[maybe_unused]],
                                  uint32_t BuilderLen [[maybe_unused]],
                                  uint32_t Encoding,
                                  uint32_t Target [[maybe_unused]],
                                  uint32_t GraphIdPtr [[maybe_unused]]) {
  // Check memory instance from module.
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::ExecutionFailed);
  }

  if (Encoding == static_cast<uint32_t>(WASINN::Backend::OpenVINO)) {
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_OPENVINO
    // The OpenVINO core must be initialized in constructor.
    if (unlikely(Env.OpenVINOCore == nullptr)) {
      spdlog::error("[WASI-NN] OpenVINO core not initialized.");
      return static_cast<uint32_t>(WASINN::ErrNo::MissingMemory);
    }

    // Check the return value: GraphIdPtr should be valid.
    uint32_t *GraphId = MemInst->getPointer<uint32_t *>(GraphIdPtr, 1);
    if (unlikely(GraphId == nullptr)) {
      spdlog::error(
          "[WASI-NN] Failed when accessing the return GraphID memory.");
      return static_cast<uint32_t>(WASINN::ErrNo::InvalidArgument);
    }

    // The graph builder length must be 2.
    if (BuilderLen != 2) {
      spdlog::error("[WASI-NN] Wrong GraphBuilder Length {:d}, expect 2",
                    BuilderLen);
      return static_cast<uint32_t>(WASINN::ErrNo::InvalidArgument);
    }

    // Get and check the device name string.
    std::string DeviceName;
    switch (Target) {
    case 0:
      DeviceName = "CPU";
      break;
    case 1:
      DeviceName = "GPU";
      break;
    case 2:
      DeviceName = "TPU";
      break;
    default:
      DeviceName = "";
    }
    if (DeviceName.length() == 0) {
      spdlog::error("[WASI-NN] Device target {:d} not support!", Target);
      return static_cast<uint32_t>(WASINN::ErrNo::InvalidArgument);
    } else {
      spdlog::debug("[WASI-NN] Using device: {:s}", DeviceName);
    }

    // Get the graph builders.
    // GraphBuilders' Layout:
    //   | builder-0 | builder-0 len | builder-1 | builder-1 len | ...
    uint32_t *GraphBuilders =
        MemInst->getPointer<uint32_t *>(BuilderPtr, BuilderLen * 2);
    if (unlikely(GraphBuilders == nullptr)) {
      spdlog::error("[WASI-NN] Failed when accessing the GraphBuilder memory.");
      return static_cast<uint32_t>(WASINN::ErrNo::InvalidArgument);
    }

    // Get the XML and Weight raw buffer from memory instance.
    //   Builder-0: the XML string
    //   Builder-1: the Weight binary
    uint32_t XMLStringLen = GraphBuilders[1];
    uint32_t WeightsBinLen = GraphBuilders[3];
    uint8_t *XMLPtr =
        MemInst->getPointer<uint8_t *>(GraphBuilders[0], XMLStringLen);
    uint8_t *BinPtr =
        MemInst->getPointer<uint8_t *>(GraphBuilders[2], WeightsBinLen);
    if (unlikely(XMLPtr == nullptr)) {
      spdlog::error("[WASI-NN] Failed when accessing the XML memory.");
      return static_cast<uint32_t>(WASINN::ErrNo::InvalidArgument);
    }
    if (unlikely(BinPtr == nullptr)) {
      spdlog::error("[WASI-NN] Failed when accessing the Weignt memory.");
      return static_cast<uint32_t>(WASINN::ErrNo::InvalidArgument);
    }

    // Add a new graph.
    Env.NNGraph.emplace_back(static_cast<WASINN::Backend>(Encoding));
    auto &Graph = Env.NNGraph.back();

    // Create the weights blob memory.
    tensor_desc_t WeightsDesc{
        layout_e::ANY, {1, {WeightsBinLen}}, precision_e::U8};
    IEStatusCode Status =
        ie_blob_make_memory(&WeightsDesc, &(Graph.OpenVINOWeightBlob));
    if (Status != IEStatusCode::OK) {
      spdlog::error(
          "[WASI-NN] Unable to create the model's weight blob, error code: {}",
          Status);
      Env.NNGraph.pop_back();
      return static_cast<uint32_t>(WASINN::ErrNo::Busy);
    }

    // Copy the weights buffer to the blob.
    ie_blob_buffer_t BlobBuffer;
    Status = ie_blob_get_buffer(Graph.OpenVINOWeightBlob, &BlobBuffer);
    if (unlikely(Status != IEStatusCode::OK)) {
      spdlog::error(
          "[WASI-NN] Unable to find the weight blob's buffer, error code: {}",
          Status);
      Env.NNGraph.pop_back();
      return static_cast<uint32_t>(WASINN::ErrNo::MissingMemory);
    }
    std::copy_n(BinPtr, WeightsBinLen,
                static_cast<uint8_t *>(BlobBuffer.buffer));

    // Read network from memory.
    Status = ie_core_read_network_from_memory(
        Env.OpenVINOCore, XMLPtr, XMLStringLen, Graph.OpenVINOWeightBlob,
        &(Graph.OpenVINONetwork));
    if (Status != IEStatusCode::OK) {
      spdlog::error("[WASI-NN] Unable to read network from the XML and "
                    "Weights, error code: {}",
                    Status);
      Env.NNGraph.pop_back();
      return static_cast<uint32_t>(WASINN::ErrNo::Busy);
    }

    // Get the network input and output size.
    size_t NetworkInputSize = 0;
    Status =
        ie_network_get_inputs_number(Graph.OpenVINONetwork, &NetworkInputSize);
    if (unlikely(Status != IEStatusCode::OK)) {
      spdlog::error("[WASI-NN] Unable to get the inputs number from the "
                    "network, error code: {}",
                    Status);
      Env.NNGraph.pop_back();
      return static_cast<uint32_t>(WASINN::ErrNo::MissingMemory);
    }
    spdlog::debug("[WASI-NN] Got input size: {}", NetworkInputSize);
    size_t NetworkOutputSize = 0;
    Status = ie_network_get_outputs_number(Graph.OpenVINONetwork,
                                           &NetworkOutputSize);
    if (unlikely(Status != IEStatusCode::OK)) {
      spdlog::error("[WASI-NN] Unable to get the outputs number from the "
                    "network, error code: {}",
                    Status);
      Env.NNGraph.pop_back();
      return static_cast<uint32_t>(WASINN::ErrNo::MissingMemory);
    }
    spdlog::debug("[WASI-NN] Got output size: {}", NetworkOutputSize);

    // Get and store the input and output names.
    Graph.OpenVINOInputNames.resize(NetworkInputSize, nullptr);
    for (size_t I = 0; I < NetworkInputSize; I++) {
      Status = ie_network_get_input_name(Graph.OpenVINONetwork, I,
                                         &(Graph.OpenVINOInputNames[I]));
      if (Status != IEStatusCode::OK) {
        spdlog::error("[WASI-NN] Unable to find input name correctly with "
                      "Index {}, error code: {}",
                      I, Status);
        Env.NNGraph.pop_back();
        return static_cast<uint32_t>(WASINN::ErrNo::MissingMemory);
      }
      spdlog::debug("[WASI-NN] Got input name: {}",
                    Graph.OpenVINOInputNames[I]);
    }
    Graph.OpenVINOOutputNames.resize(NetworkOutputSize, nullptr);
    for (size_t I = 0; I < NetworkOutputSize; I++) {
      Status = ie_network_get_output_name(Graph.OpenVINONetwork, I,
                                          &(Graph.OpenVINOOutputNames[I]));
      if (Status != IEStatusCode::OK) {
        spdlog::error("[WASI-NN] Unable to find output name correctly with "
                      "Index {}, error code: {}",
                      I, Status);
        Env.NNGraph.pop_back();
        return static_cast<uint32_t>(WASINN::ErrNo::MissingMemory);
      }
      spdlog::debug("[WASI-NN] Got output name: {}",
                    Graph.OpenVINOOutputNames[I]);
    }

    // Set the input layout.
    // FIXME: this is a temporary workaround. We need a more eligant way to
    // specify the layout in the long run. However, without this newer versions
    // of OpenVINO will fail due to parameter mismatch.
    for (size_t I = 0; I < NetworkInputSize; I++) {
      // More layouts should be supported.
      Status = ie_network_set_input_layout(
          Graph.OpenVINONetwork, Graph.OpenVINOInputNames[I], layout_e::NHWC);
      spdlog::debug("[WASI-NN] Setting [{}] to NHWC",
                    Graph.OpenVINOInputNames[I]);
      if (Status != IEStatusCode::OK) {
        spdlog::error("[WASI-NN] Unable to set input layout with the input "
                      "name {}, error code: {}",
                      Graph.OpenVINOInputNames[I], Status);
        Env.NNGraph.pop_back();
        return static_cast<uint32_t>(WASINN::ErrNo::MissingMemory);
      }
    }

    // Load network.
    ie_config_t Config = {nullptr, nullptr, nullptr};
    Status = ie_core_load_network(Env.OpenVINOCore, Graph.OpenVINONetwork,
                                  DeviceName.c_str(), &Config,
                                  &(Graph.OpenVINOExecNetwork));
    if (Status != IEStatusCode::OK) {
      spdlog::error(
          "[WASI-NN] Unable to create executable Network, error code: {}",
          Status);
      Env.NNGraph.pop_back();
      return static_cast<uint32_t>(WASINN::ErrNo::Busy);
    }

    // Store the loaded graph.
    *GraphId = Env.NNGraph.size() - 1;

    return static_cast<uint32_t>(WASINN::ErrNo::Success);
#else
    spdlog::error("[WASI-NN] OpenVINO backend is not built. use "
                  "-DWASMEDGE_PLUGIN_WASI_NN_BACKEND_OPENVINO=ON"
                  "to build it.");
#endif
  } else {
    spdlog::error("[WASI-NN] Current backend is not supported.");
  }
  return static_cast<uint32_t>(WASINN::ErrNo::InvalidArgument);
}

Expect<uint32_t>
WasiNNInitExecCtx::body(Runtime::Instance::MemoryInstance *MemInst,
                        uint32_t GraphId,
                        uint32_t ContextPtr [[maybe_unused]]) {
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::ExecutionFailed);
  }

  if (Env.NNGraph.size() <= GraphId) {
    spdlog::error("[WASI-NN] init_execution_context: Graph Id does not exist.");
    return static_cast<uint32_t>(WASINN::ErrNo::InvalidArgument);
  }

  if (Env.NNGraph[GraphId].GraphBackend == WASINN::Backend::OpenVINO) {
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_OPENVINO
    // Check the return value: Context should be valid.
    uint32_t *Context = MemInst->getPointer<uint32_t *>(ContextPtr, 1);
    if (unlikely(Context == nullptr)) {
      spdlog::error("[WASI-NN] Failed when accessing the Context memory.");
      return static_cast<uint32_t>(WASINN::ErrNo::InvalidArgument);
    }

    // Check the network and the execution network with the graph ID.
    if (Env.NNGraph[GraphId].OpenVINONetwork == nullptr ||
        Env.NNGraph[GraphId].OpenVINOExecNetwork == nullptr) {
      spdlog::error("[WASI-NN] Model for Graph:{} is empty!", GraphId);
      return static_cast<uint32_t>(WASINN::ErrNo::MissingMemory);
    }

    // Create the infer request.
    ie_infer_request_t *InferRequest = nullptr;
    IEStatusCode Status = ie_exec_network_create_infer_request(
        Env.NNGraph[GraphId].OpenVINOExecNetwork, &InferRequest);
    if (Status != IEStatusCode::OK) {
      spdlog::error("[WASI-NN] Unable to create openvino session");
      return static_cast<uint32_t>(WASINN::ErrNo::Busy);
    }

    *Context = Env.NNContext.size();
    Env.NNContext.emplace_back(Env.NNGraph[GraphId], InferRequest);

    return static_cast<uint32_t>(WASINN::ErrNo::Success);
#else
    spdlog::error("[WASI-NN] OpenVINO backend is not built. define "
                  "-DWASMEDGE_PLUGIN_WASI_NN_BACKEND_OPENVINO "
                  "to build it.");
#endif
  } else {
    spdlog::error("[WASI-NN] Current backend is not supported.");
  }
  return static_cast<uint32_t>(WASINN::ErrNo::InvalidArgument);
}

Expect<uint32_t>
WasiNNSetInput::body(Runtime::Instance::MemoryInstance *MemInst,
                     uint32_t Context, uint32_t Index [[maybe_unused]],
                     uint32_t TensorPtr [[maybe_unused]]) {
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::ExecutionFailed);
  }

  if (Env.NNContext.size() <= Context) {
    spdlog::error("[WASI-NN] set_input: Execution Context does not exist.");
    return static_cast<uint32_t>(WASINN::ErrNo::InvalidArgument);
  }

  auto &CxtRef = Env.NNContext[Context];
  if (CxtRef.GraphRef.GraphBackend == WASINN::Backend::OpenVINO) {
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_OPENVINO
    // Check the infer request and the network.
    auto *Network = CxtRef.GraphRef.OpenVINONetwork;
    if (Network == nullptr || CxtRef.OpenVINOInferRequest == nullptr) {
      spdlog::error("[WASI-NN] The founded openvino session is empty");
      return static_cast<uint32_t>(WASINN::ErrNo::MissingMemory);
    }

    // Check the input index.
    if (CxtRef.GraphRef.OpenVINOInputNames.size() <= Index) {
      spdlog::error(
          "[WASI-NN] The input index {} exceeds the inputs number {}.", Index,
          CxtRef.GraphRef.OpenVINOInputNames.size());
      return static_cast<uint32_t>(WASINN::ErrNo::InvalidArgument);
    }
    char *InputName = CxtRef.GraphRef.OpenVINOInputNames[Index];

    // Get the tensor.
    // Tensor's Layout:
    //   | dim buf | dim buf len | rtype | data buf | data buf len |
    uint32_t *Tensor = MemInst->getPointer<uint32_t *>(TensorPtr, 5);
    if (unlikely(Tensor == nullptr)) {
      spdlog::error("[WASI-NN] Failed when accessing the Tensor memory.");
      return static_cast<uint32_t>(WASINN::ErrNo::InvalidArgument);
    }
    uint32_t DimensionLen = Tensor[1];
    if (DimensionLen > 8) {
      spdlog::error(
          "[WASI-NN] Tensor dimension is out of range, expect it under 8-dim, "
          "but got {}-dim.",
          DimensionLen);
      return static_cast<uint32_t>(WASINN::ErrNo::InvalidArgument);
    }
    uint32_t *DimensionBuf =
        MemInst->getPointer<uint32_t *>(Tensor[0], DimensionLen);
    if (unlikely(DimensionBuf == nullptr)) {
      spdlog::error("[WASI-NN] Failed when accessing the Dimension memory.");
      return static_cast<uint32_t>(WASINN::ErrNo::InvalidArgument);
    }
    uint32_t TensorDataLen = Tensor[4];
    uint8_t *TensorDataBuf =
        MemInst->getPointer<uint8_t *>(Tensor[3], TensorDataLen);
    if (unlikely(TensorDataBuf == nullptr)) {
      spdlog::error("[WASI-NN] Failed when accessing the TensorData memory.");
      return static_cast<uint32_t>(WASINN::ErrNo::InvalidArgument);
    }
    uint32_t RType = Tensor[2];
    if (RType != 1) {
      spdlog::error(
          "[WASI-NN] Only F32 inputs and outputs are supported for now.");
      return static_cast<uint32_t>(WASINN::ErrNo::InvalidArgument);
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
      return static_cast<uint32_t>(WASINN::ErrNo::InvalidArgument);
    }

    // Set the input layout.
    // More layouts should be supported.
    Status = ie_network_set_input_layout(Network, InputName, layout_e::NHWC);
    if (Status != IEStatusCode::OK) {
      spdlog::error(
          "[WASI-NN] Unable to set input layout correctly, error code: {}",
          Status);
      return static_cast<uint32_t>(WASINN::ErrNo::InvalidArgument);
    }

    // Set the input precision.
    // More types should be supported.
    Status =
        ie_network_set_input_precision(Network, InputName, precision_e::FP32);
    if (Status != IEStatusCode::OK) {
      spdlog::error(
          "[WASI-NN] Unable to set input precision correctly, error code: {}",
          Status);
      return static_cast<uint32_t>(WASINN::ErrNo::InvalidArgument);
    }

    // Set the dimensions and the tensor description.
    dimensions_t Dimens;
    Dimens.ranks = DimensionLen;
    for (size_t I = 0; I < Dimens.ranks; I++) {
      Dimens.dims[I] = static_cast<size_t>(DimensionBuf[I]);
    }
    tensor_desc_t TensorDesc = {layout_e::NHWC, Dimens, precision_e::FP32};

    // Create the input blob memory.
    ie_blob_t *InputBlob = nullptr;
    Status = ie_blob_make_memory(&TensorDesc, &InputBlob);
    if (Status != IEStatusCode::OK) {
      spdlog::error("[WASI-NN] Unable to allocated input tensor correctly, "
                    "error code: {}",
                    Status);
      return static_cast<uint32_t>(WASINN::ErrNo::Busy);
    }

    // Get the blob buffer size and compare with the tensor size.
    int BlobSize;
    Status = ie_blob_size(InputBlob, &BlobSize);
    if (unlikely(Status != IEStatusCode::OK)) {
      spdlog::error(
          "[WASI-NN] Unable to get the input blob size, error code: {}",
          Status);
      return static_cast<uint32_t>(WASINN::ErrNo::Busy);
    }
    if (unlikely(static_cast<uint32_t>(BlobSize * 4) != TensorDataLen)) {
      spdlog::error(
          "[WASI-NN] Blob size {} and the Tensor size {} not matched.",
          BlobSize * 4, TensorDataLen);
    }

    // Copy the data into the input blob buffer.
    ie_blob_buffer_t BlobBuffer;
    Status = ie_blob_get_buffer(InputBlob, &BlobBuffer);
    if (unlikely(Status != IEStatusCode::OK)) {
      spdlog::error("[WASI-NN] Unable to find input tensor buffer");
      ie_blob_free(&InputBlob);
      return static_cast<uint32_t>(WASINN::ErrNo::MissingMemory);
    }
    std::copy_n(TensorDataBuf, TensorDataLen,
                static_cast<uint8_t *>(BlobBuffer.buffer));

    // Set input blob.
    Status = ie_infer_request_set_blob(CxtRef.OpenVINOInferRequest, InputName,
                                       InputBlob);
    if (Status != IEStatusCode::OK) {
      spdlog::error("[WASI-NN] Unable to set input tensor to model correctly, "
                    "error code: {}",
                    Status);
      ie_blob_free(&InputBlob);
      return static_cast<uint32_t>(WASINN::ErrNo::Busy);
    }

    ie_blob_free(&InputBlob);

    return static_cast<uint32_t>(WASINN::ErrNo::Success);
#else
    spdlog::error("[WASI-NN] OpenVINO backend is not built, use "
                  "-DWASMEDGE_PLUGIN_WASI_NN_BACKEND_OPENVINO=ON"
                  "to build it.");
#endif
  } else {
    spdlog::error("[WASI-NN] Current backend is not supported.");
  }
  return static_cast<uint32_t>(WASINN::ErrNo::InvalidArgument);
}

Expect<uint32_t>
WasiNNGetOuput::body(Runtime::Instance::MemoryInstance *MemInst,
                     uint32_t Context, uint32_t Index [[maybe_unused]],
                     uint32_t OutBufferPtr [[maybe_unused]],
                     uint32_t OutBufferMaxSize [[maybe_unused]],
                     uint32_t BytesWrittenPtr [[maybe_unused]]) {
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::ExecutionFailed);
  }

  if (Env.NNContext.size() <= Context) {
    spdlog::error("[WASI-NN] get_output: Execution Context does not exist");
    return static_cast<uint32_t>(WASINN::ErrNo::InvalidArgument);
  }

  auto &CxtRef = Env.NNContext[Context];
  if (CxtRef.GraphRef.GraphBackend == WASINN::Backend::OpenVINO) {
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_OPENVINO
    auto *Network = CxtRef.GraphRef.OpenVINONetwork;

    // Check the output index.
    if (CxtRef.GraphRef.OpenVINOOutputNames.size() <= Index) {
      spdlog::error(
          "[WASI-NN] The output index {} exceeds the outputs number {}.", Index,
          CxtRef.GraphRef.OpenVINOOutputNames.size());
      return static_cast<uint32_t>(WASINN::ErrNo::InvalidArgument);
    }
    char *OutputName = CxtRef.GraphRef.OpenVINOOutputNames[Index];

    // Set output precision.
    IEStatusCode Status =
        ie_network_set_output_precision(Network, OutputName, precision_e::FP32);
    if (Status != IEStatusCode::OK) {
      spdlog::error(
          "[WASI-NN] Unable to set output precision correctly with Index:{}",
          Index);
      return static_cast<uint32_t>(WASINN::ErrNo::InvalidArgument);
    }

    // Get output blob buffer.
    ie_blob_t *OutputBlob = nullptr;
    Status = ie_infer_request_get_blob(CxtRef.OpenVINOInferRequest, OutputName,
                                       &OutputBlob);
    if (Status != IEStatusCode::OK) {
      spdlog::error("[WASI-NN] Unable to retrieve output tensor correctly",
                    Index);
      return static_cast<uint32_t>(WASINN::ErrNo::InvalidArgument);
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
      return static_cast<uint32_t>(WASINN::ErrNo::MissingMemory);
    }
    uint32_t BytesToWrite =
        std::min(static_cast<uint32_t>(BlobSize * 4), OutBufferMaxSize);
    uint8_t *OutBuffer =
        MemInst->getPointer<uint8_t *>(OutBufferPtr, BytesToWrite);
    if (unlikely(OutBuffer == nullptr)) {
      spdlog::error(
          "[WASI-NN] Failed when accessing the Output Buffer memory.");
      ie_blob_free(&OutputBlob);
      return static_cast<uint32_t>(WASINN::ErrNo::InvalidArgument);
    }
    std::copy_n(static_cast<const uint8_t *>(BlobCBuffer.cbuffer), BytesToWrite,
                OutBuffer);

    // Write the bytes written result.
    uint32_t *BytesWritten =
        MemInst->getPointer<uint32_t *>(BytesWrittenPtr, 1);
    if (unlikely(BytesWritten == nullptr)) {
      spdlog::error("[WASI-NN] Failed when accessing the BytesWritten memory.");
      ie_blob_free(&OutputBlob);
      return static_cast<uint32_t>(WASINN::ErrNo::InvalidArgument);
    }
    *BytesWritten = BytesToWrite;

    ie_blob_free(&OutputBlob);

    return static_cast<uint32_t>(WASINN::ErrNo::Success);
#else
    spdlog::error("[WASI-NN] OpenVINO backend is not built. use "
                  "-DWASMEDGE_PLUGIN_WASI_NN_BACKEND_OPENVINO=ON"
                  "to build it.");
#endif
  } else {
    spdlog::error("[WASI-NN] Current backend is not supported.");
  }
  return static_cast<uint32_t>(WASINN::ErrNo::InvalidArgument);
}

Expect<uint32_t> WasiNNCompute::body(Runtime::Instance::MemoryInstance *MemInst,
                                     uint32_t Context) {
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::ExecutionFailed);
  }

  if (Env.NNContext.size() <= Context) {
    spdlog::error("[WASI-NN] compute: Execution Context does not exist.");
    return static_cast<uint32_t>(WASINN::ErrNo::InvalidArgument);
  }

  auto &CxtRef = Env.NNContext[Context];
  if (CxtRef.GraphRef.GraphBackend == WASINN::Backend::OpenVINO) {
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_OPENVINO
    IEStatusCode Status = ie_infer_request_infer(CxtRef.OpenVINOInferRequest);
    if (Status != IEStatusCode::OK) {
      spdlog::error(
          "[WASI-NN] Unable to perform computation correctly, error code: {}",
          Status);
      return static_cast<uint32_t>(WASINN::ErrNo::Busy);
    }
    return static_cast<uint32_t>(WASINN::ErrNo::Success);
#else
    spdlog::error("[WASI-NN] OpenVINO backend is not built. use "
                  "-DWASMEDGE_PLUGIN_WASI_NN_BACKEND_OPENVINO=ON"
                  "to build it.");
#endif
  } else {
    spdlog::error("[WASI-NN] Current backend is not supported.");
  }

  return static_cast<uint32_t>(WASINN::ErrNo::InvalidArgument);
}

} // namespace Host
} // namespace WasmEdge
