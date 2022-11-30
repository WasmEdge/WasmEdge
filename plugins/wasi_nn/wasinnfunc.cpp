// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "wasinnfunc.h"
#include "common/log.h"

#include <string>

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_OPENVINO
#include <algorithm>

#include <c_api/ie_c_api.h>
#endif

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_TORCH
#include <iostream>

#include <torch/torch.h>
#endif

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_TFLITE
#include "tensorflow/lite/c/c_api.h"
#endif

namespace WasmEdge {
namespace Host {

namespace {
[[maybe_unused]] std::string FindDevice(const uint32_t Target) {
  std::string DeviceName;
  switch (Target) {
  case 0:
    DeviceName = "CPU";
    break;
  // case 1:
  //   DeviceName = "GPU";
  //   break;
  // case 2:
  //   DeviceName = "TPU";
  //   break;
  default:
    DeviceName = "";
  }
  return DeviceName;
}

} // namespace

Expect<uint32_t> WasiNNLoad::body(const Runtime::CallingFrame &Frame,
                                  uint32_t BuilderPtr [[maybe_unused]],
                                  uint32_t BuilderLen [[maybe_unused]],
                                  uint32_t Encoding, uint32_t Target,
                                  uint32_t GraphIdPtr [[maybe_unused]]) {
  // Check memory instance from module.
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }
  // Check the return value: GraphIdPtr should be valid.
  uint32_t *GraphId = MemInst->getPointer<uint32_t *>(GraphIdPtr, 1);
  if (unlikely(GraphId == nullptr)) {
    spdlog::error("[WASI-NN] Failed when accessing the return GraphID memory.");
    return static_cast<uint32_t>(WASINN::ErrNo::InvalidArgument);
  }
  // Get and check the device name string.
  std::string DeviceName;
  DeviceName = FindDevice(Target);
  if (unlikely(DeviceName.length() == 0)) {
    spdlog::error("[WASI-NN] Only support CPU target");
    return static_cast<uint32_t>(WASINN::ErrNo::InvalidArgument);
  }
  spdlog::debug("[WASI-NN] Using device: {:s}", DeviceName);

  if (Encoding == static_cast<uint32_t>(WASINN::Backend::OpenVINO)) {
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_OPENVINO
    // The OpenVINO core must be initialized in constructor.
    if (unlikely(Env.OpenVINOCore == nullptr)) {
      spdlog::error("[WASI-NN] OpenVINO core not initialized.");
      return static_cast<uint32_t>(WASINN::ErrNo::MissingMemory);
    }

    // The graph builder length must be 2.
    if (BuilderLen != 2) {
      spdlog::error("[WASI-NN] Wrong GraphBuilder Length {:d}, expect 2",
                    BuilderLen);
      return static_cast<uint32_t>(WASINN::ErrNo::InvalidArgument);
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
      spdlog::error("[WASI-NN] Failed when accessing the Weight memory.");
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
                  "-WASMEDGE_PLUGIN_WASI_NN_BACKEND=\"OpenVINO\" to build it.");
#endif
  } else if (Encoding == static_cast<uint32_t>(WASINN::Backend::PyTorch)) {
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_TORCH
    // The graph builder length must be 2.
    if (BuilderLen != 1) {
      spdlog::error("[WASI-NN] Wrong GraphBuilder Length {:d}, expect 1",
                    BuilderLen);
      return static_cast<uint32_t>(WASINN::ErrNo::InvalidArgument);
    }
    uint32_t *GraphBuilders =
        MemInst->getPointer<uint32_t *>(BuilderPtr, BuilderLen * 2);
    if (unlikely(GraphBuilders == nullptr)) {
      spdlog::error("[WASI-NN] Failed when accessing the GraphBuilder memory.");
      return static_cast<uint32_t>(WASINN::ErrNo::InvalidArgument);
    }

    uint32_t BinLen = GraphBuilders[1];
    uint8_t *BinPtr = MemInst->getPointer<uint8_t *>(GraphBuilders[0], BinLen);
    if (unlikely(BinPtr == nullptr)) {
      spdlog::error("[WASI-NN] Failed when accessing the Weight memory.");
      return static_cast<uint32_t>(WASINN::ErrNo::InvalidArgument);
    }
    // Add a new graph.
    Env.NNGraph.emplace_back(static_cast<WASINN::Backend>(Encoding));
    auto &Graph = Env.NNGraph.back();
    std::string BinString((char *)BinPtr, BinLen);
    std::stringstream BinRead;
    BinRead.str(BinString);

    try {
      Graph.TorchModel = torch::jit::load(BinRead);
    } catch (const c10::Error &e) {
      spdlog::error("[WASI-NN] Failed when load the TorchScript model.");
      Env.NNGraph.pop_back();
      return static_cast<uint32_t>(WASINN::ErrNo::InvalidArgument);
    }
    // Store the loaded graph.
    *GraphId = Env.NNGraph.size() - 1;
    return static_cast<uint32_t>(WASINN::ErrNo::Success);

#else
    spdlog::error("[WASI-NN] PyTorch backend is not built. use "
                  "-WASMEDGE_PLUGIN_WASI_NN_BACKEND=\"PyTorch\" to build it.");
#endif // WASMEDGE_PLUGIN_WASI_NN_BACKEND_TORCH
  } else if (Encoding ==
             static_cast<uint32_t>(WASINN::Backend::TensorflowLite)) {
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_TFLITE
    // The graph builder length must be 1.
    if (BuilderLen != 1) {
      spdlog::error("[WASI-NN] Wrong GraphBuilder Length {:d}, expect 1",
                    BuilderLen);
      return static_cast<uint32_t>(WASINN::ErrNo::InvalidArgument);
    }
    uint32_t *GraphBuilders =
        MemInst->getPointer<uint32_t *>(BuilderPtr, BuilderLen * 2);
    if (unlikely(GraphBuilders == nullptr)) {
      spdlog::error("[WASI-NN] Failed when accessing the GraphBuilder memory.");
      return static_cast<uint32_t>(WASINN::ErrNo::InvalidArgument);
    }
    uint32_t BinLen = GraphBuilders[1];
    char *BinPtr = MemInst->getPointer<char *>(GraphBuilders[0], BinLen);
    if (unlikely(BinPtr == nullptr)) {
      spdlog::error("[WASI-NN] Failed when accessing the Weight memory.");
      return static_cast<uint32_t>(WASINN::ErrNo::InvalidArgument);
    }
    // Add a new graph.
    Env.NNGraph.emplace_back(static_cast<WASINN::Backend>(Encoding));
    auto &Graph = Env.NNGraph.back();

    Graph.TFLiteMod = TfLiteModelCreate(BinPtr, BinLen);
    if (unlikely(Graph.TFLiteMod == nullptr)) {
      spdlog::error("[WASI-NN] Cannot import TFLite model");
      Env.NNGraph.pop_back();
      return static_cast<uint32_t>(WASINN::ErrNo::InvalidArgument);
    }

    // Store the loaded graph.
    *GraphId = Env.NNGraph.size() - 1;
    return static_cast<uint32_t>(WASINN::ErrNo::Success);
#else
    spdlog::error(
        "[WASI-NN] TensorflowLite backend is not built. use "
        "-WASMEDGE_PLUGIN_WASI_NN_BACKEND=\"Tensorflowlite\" to build it.");
#endif
  } else {
    spdlog::error("[WASI-NN] Current backend is not supported.");
  }
  return static_cast<uint32_t>(WASINN::ErrNo::InvalidArgument);
}

Expect<uint32_t> WasiNNInitExecCtx::body(const Runtime::CallingFrame &Frame,
                                         uint32_t GraphId,
                                         uint32_t ContextPtr [[maybe_unused]]) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  if (Env.NNGraph.size() <= GraphId) {
    spdlog::error("[WASI-NN] init_execution_context: Graph Id does not exist.");
    return static_cast<uint32_t>(WASINN::ErrNo::InvalidArgument);
  }
  // Check the return value: Context should be valid.
  uint32_t *Context = MemInst->getPointer<uint32_t *>(ContextPtr, 1);
  if (unlikely(Context == nullptr)) {
    spdlog::error("[WASI-NN] Failed when accessing the Context memory.");
    return static_cast<uint32_t>(WASINN::ErrNo::InvalidArgument);
  }
  if (Env.NNGraph[GraphId].GraphBackend == WASINN::Backend::OpenVINO) {
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_OPENVINO
    // Check the network and the execution network with the graph ID.
    if (Env.NNGraph[GraphId].OpenVINONetwork == nullptr ||
        Env.NNGraph[GraphId].OpenVINOExecNetwork == nullptr) {
      spdlog::error("[WASI-NN] Model for Graph:{} is empty!", GraphId);
      return static_cast<uint32_t>(WASINN::ErrNo::MissingMemory);
    }

    // Create context.
    Env.NNContext.emplace_back(Env.NNGraph[GraphId]);
    auto &NewContext = Env.NNContext.back();
    if (NewContext.OpenVINOInferRequest == nullptr) {
      spdlog::error("[WASI-NN] Unable to create openvino context");
      Env.NNContext.pop_back();
      return static_cast<uint32_t>(WASINN::ErrNo::Busy);
    }

    *Context = Env.NNContext.size() - 1;
    return static_cast<uint32_t>(WASINN::ErrNo::Success);
#else
    spdlog::error("[WASI-NN] OpenVINO backend is not built. use "
                  "-WASMEDGE_PLUGIN_WASI_NN_BACKEND=\"OpenVINO\" to build it.");
#endif
  } else if (Env.NNGraph[GraphId].GraphBackend == WASINN::Backend::PyTorch) {
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_TORCH
    Env.NNContext.emplace_back(Env.NNGraph[GraphId]);

    *Context = Env.NNContext.size() - 1;
    return static_cast<uint32_t>(WASINN::ErrNo::Success);

#else
    spdlog::error("[WASI-NN] PyTorch backend is not built. use "
                  "-WASMEDGE_PLUGIN_WASI_NN_BACKEND=\"PyTorch\" to build it.");
#endif
  } else if (Env.NNGraph[GraphId].GraphBackend ==
             WASINN::Backend::TensorflowLite) {
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_TFLITE
    // Check the network and the execution network with the graph ID.
    if (Env.NNGraph[GraphId].TFLiteMod == nullptr) {
      spdlog::error("[WASI-NN] Model for Graph:{} is missing!", GraphId);
      return static_cast<uint32_t>(WASINN::ErrNo::MissingMemory);
    }

    Env.NNContext.emplace_back(Env.NNGraph[GraphId]);
    const auto Graph = Env.NNGraph[GraphId];
    auto &NewContext = Env.NNContext.back();
    auto *TFLiteOps = TfLiteInterpreterOptionsCreate();
    TfLiteInterpreterOptionsSetNumThreads(TFLiteOps, 2);
    NewContext.TFLiteInterp =
        TfLiteInterpreterCreate(Graph.TFLiteMod, TFLiteOps);
    TfLiteInterpreterOptionsDelete(TFLiteOps);
    if (unlikely(NewContext.TFLiteInterp == nullptr)) {
      spdlog::error("[WASI-NN] Cannot create TFLite interpreter.");
      Env.NNContext.pop_back();
      return static_cast<uint32_t>(WASINN::ErrNo::Busy);
    }
    TfLiteInterpreterAllocateTensors(NewContext.TFLiteInterp);

    *Context = Env.NNContext.size() - 1;
    return static_cast<uint32_t>(WASINN::ErrNo::Success);
#else
    spdlog::error(
        "[WASI-NN] TensorflowLite backend is not built. use "
        "-WASMEDGE_PLUGIN_WASI_NN_BACKEND=\"Tensorflowlite\" to build it.");
#endif
  } else {
    spdlog::error("[WASI-NN] Current backend is not supported.");
  }
  return static_cast<uint32_t>(WASINN::ErrNo::InvalidArgument);
}

Expect<uint32_t> WasiNNSetInput::body(const Runtime::CallingFrame &Frame,
                                      uint32_t Context,
                                      uint32_t Index [[maybe_unused]],
                                      uint32_t TensorPtr [[maybe_unused]]) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
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
    WASINN::TensorType RType = static_cast<WASINN::TensorType>(Tensor[2]);
    if (RType != WASINN::TensorType::F32) {
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
    spdlog::error("[WASI-NN] OpenVINO backend is not built. use "
                  "-WASMEDGE_PLUGIN_WASI_NN_BACKEND=\"OpenVINO\" to build it.");
#endif
  } else if (CxtRef.GraphRef.GraphBackend == WASINN::Backend::PyTorch) {
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_TORCH
    if (Index >= CxtRef.TorchInputs.size()) {
      CxtRef.TorchInputs.resize(Index + 1);
    }
    uint32_t *Tensor = MemInst->getPointer<uint32_t *>(TensorPtr, 5);
    if (unlikely(Tensor == nullptr)) {
      spdlog::error("[WASI-NN] Failed when accessing the Tensor memory.");
      return static_cast<uint32_t>(WASINN::ErrNo::InvalidArgument);
    }
    uint32_t DimensionLen = Tensor[1];
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
    WASINN::TensorType RType = static_cast<WASINN::TensorType>(Tensor[2]);
    if (RType != WASINN::TensorType::F32) {
      spdlog::error(
          "[WASI-NN] Only F32 inputs and outputs are supported for now.");
      return static_cast<uint32_t>(WASINN::ErrNo::InvalidArgument);
    }
    auto Options =
        torch::TensorOptions().dtype(torch::kFloat32).requires_grad(false);
    std::vector<int64_t> Dims;
    for (size_t I = 0; I < DimensionLen; I++) {
      Dims.push_back(static_cast<int64_t>(DimensionBuf[I]));
    }
    torch::Tensor InTensor = torch::from_blob(
        reinterpret_cast<float *>(TensorDataBuf), Dims, Options);

    CxtRef.TorchInputs[Index] = InTensor.clone();
    return static_cast<uint32_t>(WASINN::ErrNo::Success);
#else
    spdlog::error("[WASI-NN] PyTorch backend is not built. use "
                  "-WASMEDGE_PLUGIN_WASI_NN_BACKEND=\"PyTorch\" to build it.");
#endif
  } else if (CxtRef.GraphRef.GraphBackend == WASINN::Backend::TensorflowLite) {
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_TFLITE
    uint32_t InCnt = TfLiteInterpreterGetInputTensorCount(CxtRef.TFLiteInterp);
    if (Index >= InCnt) {
      spdlog::error("[WASI-NN] Invalid index id {} for the input, only {} "
                    "inputs are allowed",
                    Index, InCnt);
      return static_cast<uint32_t>(WASINN::ErrNo::InvalidArgument);
    }
    uint32_t *Tensor = MemInst->getPointer<uint32_t *>(TensorPtr, 5);
    if (unlikely(Tensor == nullptr)) {
      spdlog::error("[WASI-NN] Failed when accessing the Tensor memory.");
      return static_cast<uint32_t>(WASINN::ErrNo::InvalidArgument);
    }
    uint32_t DimensionLen = Tensor[1];
    std::vector<int64_t> TFDimension(DimensionLen);
    uint32_t *DimensionBuf =
        MemInst->getPointer<uint32_t *>(Tensor[0], DimensionLen);
    for (uint32_t I = 0; I < DimensionLen; I++) {
      TFDimension.push_back(static_cast<uint64_t>(DimensionBuf[I]));
    }
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

    WASINN::TensorType RType = static_cast<WASINN::TensorType>(Tensor[2]);
    auto *HoldTensor =
        TfLiteInterpreterGetInputTensor(CxtRef.TFLiteInterp, Index);
    WASINN::TensorType LiteType;
    switch (TfLiteTensorType(HoldTensor)) {
    case TfLiteType::kTfLiteUInt8:
      LiteType = WASINN::TensorType::U8;
      break;
    case TfLiteType::kTfLiteFloat16:
      LiteType = WASINN::TensorType::F16;
      break;
    case TfLiteType::kTfLiteFloat32:
      LiteType = WASINN::TensorType::F32;
      break;
    case TfLiteType::kTfLiteInt32:
      LiteType = WASINN::TensorType::I32;
      break;
    default:
      spdlog::error("[WASI-NN] Unsupported TFLite type: {}", LiteType);
      return static_cast<uint32_t>(WASINN::ErrNo::InvalidArgument);
    }

    if (unlikely(LiteType != RType)) {
      spdlog::error("[WASI-NN] Expect tensor type {}, but got {}",
                    static_cast<uint32_t>(LiteType),
                    static_cast<uint32_t>(RType));
      return static_cast<uint32_t>(WASINN::ErrNo::InvalidArgument);
    }
    TfLiteStatus Stat =
        TfLiteTensorCopyFromBuffer(HoldTensor, TensorDataBuf, TensorDataLen);
    if (unlikely(Stat != TfLiteStatus::kTfLiteOk)) {
      spdlog::error("[WASI-NN] Copy tensor memory failed");
      return static_cast<uint32_t>(WASINN::ErrNo::Busy);
    }

    return static_cast<uint32_t>(WASINN::ErrNo::Success);

#else
    spdlog::error(
        "[WASI-NN] TensorflowLite backend is not built. use "
        "-WASMEDGE_PLUGIN_WASI_NN_BACKEND=\"Tensorflowlite\" to build it.");
#endif
  } else {
    spdlog::error("[WASI-NN] Current backend is not supported.");
  }
  return static_cast<uint32_t>(WASINN::ErrNo::InvalidArgument);
}

Expect<uint32_t>
WasiNNGetOuput::body(const Runtime::CallingFrame &Frame, uint32_t Context,
                     uint32_t Index [[maybe_unused]],
                     uint32_t OutBufferPtr [[maybe_unused]],
                     uint32_t OutBufferMaxSize [[maybe_unused]],
                     uint32_t BytesWrittenPtr [[maybe_unused]]) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
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
                  "-WASMEDGE_PLUGIN_WASI_NN_BACKEND=\"OpenVINO\" to build it.");
#endif
  } else if (CxtRef.GraphRef.GraphBackend == WASINN::Backend::PyTorch) {
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_TORCH
    if (CxtRef.TorchOutputs.size() <= Index) {
      spdlog::error(
          "[WASI-NN] The output index {} exceeds the outputs number {}.", Index,
          CxtRef.TorchOutputs.size());
      return static_cast<uint32_t>(WASINN::ErrNo::InvalidArgument);
    }
    torch::Tensor OutTensor =
        CxtRef.TorchOutputs[Index].toType(torch::kFloat32);
    float *TensorBuffer = OutTensor.data_ptr<float>();

    size_t BlobSize = 1;
    for (auto I : OutTensor.sizes()) {
      BlobSize *= I;
    }
    uint32_t BytesToWrite =
        std::min(static_cast<uint32_t>(BlobSize * 4), OutBufferMaxSize);
    uint8_t *OutBuffer =
        MemInst->getPointer<uint8_t *>(OutBufferPtr, BytesToWrite);
    if (unlikely(OutBuffer == nullptr)) {
      spdlog::error(
          "[WASI-NN] Failed when accessing the Output Buffer memory.");
      return static_cast<uint32_t>(WASINN::ErrNo::InvalidArgument);
    }
    std::copy_n(reinterpret_cast<const uint8_t *>(TensorBuffer), BytesToWrite,
                OutBuffer);
    uint32_t *BytesWritten =
        MemInst->getPointer<uint32_t *>(BytesWrittenPtr, 1);
    if (unlikely(BytesWritten == nullptr)) {
      spdlog::error("[WASI-NN] Failed when accessing the BytesWritten memory.");
      return static_cast<uint32_t>(WASINN::ErrNo::InvalidArgument);
    }
    *BytesWritten = BytesToWrite;
    return static_cast<uint32_t>(WASINN::ErrNo::Success);
#else
    spdlog::error("[WASI-NN] PyTorch backend is not built. use "
                  "-WASMEDGE_PLUGIN_WASI_NN_BACKEND=\"PyTorch\" to build it.");
#endif
  } else if (CxtRef.GraphRef.GraphBackend == WASINN::Backend::TensorflowLite) {
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_TFLITE
    uint32_t OutCnt =
        TfLiteInterpreterGetOutputTensorCount(CxtRef.TFLiteInterp);
    if (Index >= OutCnt) {
      spdlog::error("[WASI-NN] Invalid index id {} for the input, only {} "
                    "outputs are allowed",
                    Index, OutCnt);
      return static_cast<uint32_t>(WASINN::ErrNo::InvalidArgument);
    }
    const TfLiteTensor *HoldTensor =
        TfLiteInterpreterGetOutputTensor(CxtRef.TFLiteInterp, Index);
    const uint32_t BlobSize = TfLiteTensorByteSize(HoldTensor);
    uint32_t BytesToWrite = std::min(BlobSize, OutBufferMaxSize);
    uint8_t *OutBuffer =
        MemInst->getPointer<uint8_t *>(OutBufferPtr, BytesToWrite);
    if (unlikely(OutBuffer == nullptr)) {
      spdlog::error(
          "[WASI-NN] Failed when accessing the Output Buffer memory.");
      return static_cast<uint32_t>(WASINN::ErrNo::InvalidArgument);
    }
    TfLiteTensorCopyToBuffer(HoldTensor, OutBuffer, BytesToWrite);
    uint32_t *BytesWritten =
        MemInst->getPointer<uint32_t *>(BytesWrittenPtr, 1);
    if (unlikely(BytesWritten == nullptr)) {
      spdlog::error("[WASI-NN] Failed when accessing the BytesWritten memory.");
      return static_cast<uint32_t>(WASINN::ErrNo::InvalidArgument);
    }
    *BytesWritten = BytesToWrite;
    return static_cast<uint32_t>(WASINN::ErrNo::Success);

#else
    spdlog::error(
        "[WASI-NN] Tensorflowlite backend is not built. use "
        "-WASMEDGE_PLUGIN_WASI_NN_BACKEND=\"Tensorflowlite\" to build it.");
#endif
  } else {
    spdlog::error("[WASI-NN] Current backend is not supported.");
  }
  return static_cast<uint32_t>(WASINN::ErrNo::InvalidArgument);
}

Expect<uint32_t> WasiNNCompute::body(const Runtime::CallingFrame &Frame,
                                     uint32_t Context) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
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
                  "-WASMEDGE_PLUGIN_WASI_NN_BACKEND=\"OpenVINO\" to build it.");
#endif
  } else if (CxtRef.GraphRef.GraphBackend == WASINN::Backend::PyTorch) {
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_TORCH
    if (CxtRef.TorchInputs.size() == 0) {
      spdlog::error("[WASI-NN] Input is not set!");
      return static_cast<uint32_t>(WASINN::ErrNo::InvalidArgument);
    }
    for (size_t I = 0; I < CxtRef.TorchInputs.size(); I++) {
      torch::jit::IValue InTensor = CxtRef.TorchInputs[I];
      if (InTensor.isNone()) {
        spdlog::error("[WASI-NN] Input [{}] is not set!", I);
        return static_cast<uint32_t>(WASINN::ErrNo::InvalidArgument);
      }
    }
    torch::jit::IValue RawOutput =
        CxtRef.GraphRef.TorchModel.forward(CxtRef.TorchInputs);
    // TODO: more output type should be supported here
    if (RawOutput.isTensorList()) {
      auto OutTensors = RawOutput.toTensorVector();
      for (auto &OneOf : OutTensors) {
        CxtRef.TorchOutputs.push_back(OneOf.clone());
      }
    } else if (RawOutput.isTensor()) {
      auto OutTensor = RawOutput.toTensor();
      CxtRef.TorchOutputs.push_back(OutTensor.clone());
    } else {
      spdlog::error("[WASI-NN] PyTorch backend only supports output a tensor "
                    "or a list of tensor");
      return static_cast<uint32_t>(WASINN::ErrNo::InvalidArgument);
    }
    return static_cast<uint32_t>(WASINN::ErrNo::Success);
#else
    spdlog::error("[WASI-NN] PyTorch backend is not built. use "
                  "-WASMEDGE_PLUGIN_WASI_NN_BACKEND=\"PyTorch\" to build it.");
#endif
  } else if (CxtRef.GraphRef.GraphBackend == WASINN::Backend::TensorflowLite) {
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_TFLITE
    // Run session
    if (unlikely(CxtRef.TFLiteInterp == nullptr)) {
      spdlog::error("[WASI-NN] Tensorflow Lite context empty");
      return static_cast<uint32_t>(WASINN::ErrNo::MissingMemory);
    }
    TfLiteStatus Stat = TfLiteInterpreterInvoke(CxtRef.TFLiteInterp);
    if (unlikely(Stat != TfLiteStatus::kTfLiteOk)) {
      spdlog::error("[WASI-NN] Invokation failed.");
      return static_cast<uint32_t>(WASINN::ErrNo::Busy);
    }
    return static_cast<uint32_t>(WASINN::ErrNo::Success);
#else
    spdlog::error(
        "[WASI-NN] Tensorflowlite backend is not built. use "
        "-WASMEDGE_PLUGIN_WASI_NN_BACKEND=\"Tensorflowlite\" to build it.");
#endif
  } else {
    spdlog::error("[WASI-NN] Current backend is not supported.");
  }

  return static_cast<uint32_t>(WASINN::ErrNo::InvalidArgument);
}

} // namespace Host
} // namespace WasmEdge
