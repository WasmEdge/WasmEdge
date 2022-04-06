// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "host/wasi_nn/wasinnfunc.h"
#include "common/errcode.h"
#include "runtime/hostfunc.h"
#include "runtime/instance/memory.h"

#ifdef WASMEDGE_WASINN_BUILD_OPENVINO
#include <c_api/ie_c_api.h>
#include <string>
#endif
namespace WasmEdge {
namespace Host {

const std::string map_target_to_string(uint32_t Target) {
  switch (Target) {
  case 0:
    return "CPU";
  case 1:
    return "GPU";
  case 2:
    return "TPU";
  default:
    return "";
  }
}

Expect<uint32_t> WasiNNLoad::body(Runtime::Instance::MemoryInstance *MemInst
                                  [[maybe_unused]],
                                  uint32_t BuilderPtr [[maybe_unused]],
                                  uint32_t BuilderLen [[maybe_unused]],
                                  uint32_t Encoding [[maybe_unused]],
                                  uint32_t Target [[maybe_unused]],
                                  uint32_t GraphPtr [[maybe_unused]]) {
  // GraphBuilders' Layout: |builder-0|builder-0 len|builder-1|builder-1 len|...
  // Check memory instance from module.
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::ExecutionFailed);
  }
  uint32_t *GraphBuilders;
  uint32_t *Graph;
  GraphBuilders = MemInst->getPointer<uint32_t *>(BuilderPtr, 1);
  Graph = MemInst->getPointer<uint32_t *>(GraphPtr, 1);
  if (Encoding == Ctx.BackendsMapping.at("OpenVINO")) {
#ifdef WASMEDGE_WASINN_BUILD_OPENVINO
    spdlog::info("Sizes: {}, {}, {}", Ctx.OpenVINONetworks.size(),
                 Ctx.OpenVINOExecutions.size(), Ctx.OpenVINOInfers.size());

    if (BuilderLen != 2) {
      spdlog::error("Wrong GraphBuilder Length {:d}, expecting 2", BuilderLen);
      return -1;
    }
    IEStatusCode status;
    if (Ctx.openvino_core == nullptr) {
      status = ie_core_create("", &Ctx.openvino_core);
      if (status != OK || Ctx.openvino_core == nullptr) {
        spdlog::error("Error happened when init OpenVINO core.");
        return -1;
      }
      spdlog::info("Initialize OpenVINO Core");
    }

    std::string DeviceName = map_target_to_string(Target);
    if (DeviceName.length() == 0) {
      spdlog::error("Device target {:d} not support!", Target);
      return -1;
    } else {
      spdlog::info("Using device: {:s}", DeviceName);
    }

    uint32_t XMLStringLen = GraphBuilders[1];
    uint32_t WeightBinsLen = GraphBuilders[3];
    uint8_t *XMLPtr = MemInst->getPointer<uint8_t *>(GraphBuilders[0], 1);
    uint8_t *BinPtr = MemInst->getPointer<uint8_t *>(GraphBuilders[2], 1);
    std::vector<uint8_t> XMLStrings(XMLPtr, XMLPtr + XMLStringLen);
    std::vector<uint8_t> WeightBins(BinPtr, BinPtr + WeightBinsLen);
    spdlog::info("read xml length {:d}", XMLStrings.size());
    spdlog::info("read bin length {:d}", WeightBins.size());

    tensor_desc_t WeightsDesc{ANY, {1, {WeightBins.size()}}, U8};
    ie_blob_t *WeightsBlob = nullptr;
    ie_blob_buffer_t BlobCBuffer;

    status = ie_blob_make_memory(&WeightsDesc, &WeightsBlob);
    if (status != OK || WeightsBlob == nullptr) {
      spdlog::error("Unable to create model's weight blob, error code: {}",
                    status);
      return -1;
    }
    status = ie_blob_get_cbuffer(WeightsBlob, &BlobCBuffer);
    if (status != OK) {
      spdlog::error("Unable to find weight blob's buffer, error code: {}",
                    status);
      return -1;
    }
    uint8_t *BlobData = (uint8_t *)(BlobCBuffer.cbuffer);
    for (size_t I = 0; I < WeightBins.size(); I++)
      BlobData[I] = WeightBins[I];
    // status = ie_blob_make_memory_from_preallocated(
    //     &WeightsDesc, WeightBins.data(), WeightBins.size(), &WeightsBlob);

    ie_network_t *Network = nullptr;
    ie_executable_network_t *ExeNetwork = nullptr;
    status = ie_core_read_network_from_memory(
        Ctx.openvino_core, XMLStrings.data(), XMLStrings.size(), WeightsBlob,
        &Network);
    if (status != OK || Network == nullptr) {
      spdlog::error("Unable to create network");
      return -1;
    }

    size_t NetworkInputSize;
    status = ie_network_get_inputs_number(Network, &NetworkInputSize);
    // TODO this is a temporary workaround. We need a more eligant way to
    // specify the layout in the long run. However, without this newer versions
    // of OpenVINO will fail due to parameter mismatch.
    for (size_t I = 0; I < NetworkInputSize; I++) {
      char *InputName = nullptr;
      status = ie_network_get_input_name(Network, I, &InputName);
      spdlog::info("Setting [{}] to NHWC", InputName);
      status = ie_network_set_input_layout(
          Network, InputName, NHWC); // more layouts should be supported
      if (status != OK) {
        spdlog::error("Unable to set input name, error code {}", status);
        return -1;
      }
    }

    ie_config_t Config = {NULL, NULL, NULL};
    status = ie_core_load_network(Ctx.openvino_core, Network,
                                  DeviceName.c_str(), &Config, &ExeNetwork);
    if (status != OK || ExeNetwork == nullptr) {
      spdlog::error("Unable to create executable network");
      return -1;
    }

    Ctx.ModelsNum = Ctx.OpenVINONetworks.size();
    Ctx.OpenVINONetworks.push_back(Network);
    Ctx.OpenVINOExecutions.push_back(ExeNetwork);
    Ctx.GraphBackends.push_back(Encoding);
    *Graph = Ctx.ModelsNum;

    return 0;
#else
    spdlog::error(
        "OpenVINO backend is not built. define -DWASINN_BUILD_OPENVINO "
        "to build it.");
#endif
  } else {
    spdlog::error("Current backend is not supported.");
  }
  return -1;
}

Expect<uint32_t> WasiNNInitExecCtx::body(
    Runtime::Instance::MemoryInstance *MemInst [[maybe_unused]],
    uint32_t Graph [[maybe_unused]], uint32_t ContextPtr [[maybe_unused]]) {
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::ExecutionFailed);
  }

  if (Ctx.GraphBackends.size() <= Graph) {
    spdlog::error("init_execution_context: Graph does not exist");
    return -1;
  }
  [[maybe_unused]] uint32_t *Context =
      MemInst->getPointer<uint32_t *>(ContextPtr, 1);

  if (Ctx.GraphBackends[Graph] == Ctx.BackendsMapping.at("OpenVINO")) {
#ifdef WASMEDGE_WASINN_BUILD_OPENVINO
    IEStatusCode status;
    OpenVINOSession *Session = new OpenVINOSession();
    Session->exe_network = Ctx.OpenVINOExecutions[Graph];
    Session->network = Ctx.OpenVINONetworks[Graph];
    if (Session->exe_network == nullptr || Session->network == nullptr) {
      spdlog::error("Model for Graph:{} is empty!", Graph);
      return -1;
    }
    status = ie_exec_network_create_infer_request(Session->exe_network,
                                                  &(Session->infer_request));

    if (status != OK || Session->infer_request == nullptr) {
      spdlog::error("Unable to create openvino session");
      return -1;
    }

    Ctx.ExecutionsNum = Ctx.OpenVINOInfers.size();
    Ctx.OpenVINOInfers.push_back(Session);
    Ctx.GraphContextBackends.push_back(Graph);
    *Context = Ctx.ExecutionsNum;

    return 0;
#else
    spdlog::error(
        "OpenVINO backend is not built. define -DWASINN_BUILD_OPENVINO "
        "to build it.");
#endif
  } else {
    spdlog::error("Current backend is not supported.");
  }
  return -1;
}

Expect<uint32_t> WasiNNSetInput::body(Runtime::Instance::MemoryInstance *MemInst
                                      [[maybe_unused]],
                                      uint32_t Context [[maybe_unused]],
                                      uint32_t Index [[maybe_unused]],
                                      uint32_t TensorPtr [[maybe_unused]]) {
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::ExecutionFailed);
  }

  if (Ctx.GraphContextBackends.size() <= Context) {
    spdlog::error("set_input: Execution Context does not exist");
    return -1;
  }
  if (Ctx.GraphContextBackends[Context] == Ctx.BackendsMapping.at("OpenVINO")) {
#ifdef WASMEDGE_WASINN_BUILD_OPENVINO
    IEStatusCode status;
    OpenVINOSession *Session = Ctx.OpenVINOInfers[Context];
    ie_network_t *Network = Session->network;
    ie_infer_request_t *InferRequest = Session->infer_request;
    ie_blob_t *InputBlob = NULL;
    ie_blob_buffer_t BlobCBuffer;
    int BlobSize;
    char *InputName = nullptr;

    if (Network == nullptr || InferRequest == nullptr) {
      spdlog::error("The founded openvino session is empty");
      return -1;
    }

    uint32_t *Tensor = MemInst->getPointer<uint32_t *>(TensorPtr, 1);
    uint32_t *DimensionBuf = MemInst->getPointer<uint32_t *>(Tensor[0]);
    uint32_t DimensionLen = Tensor[1];
    uint32_t RType = Tensor[2];
    uint8_t *TensorDataBuf = MemInst->getPointer<uint8_t *>(Tensor[3]);
    uint32_t TensorDataLen = Tensor[4];
    if (RType != 1) {
      spdlog::error("Only F32 inputs and outputs are supported for now");
      return -1;
    }
    std::vector<size_t> InputDims(DimensionLen);
    for (uint32_t I = 0; I < InputDims.size(); I++)
      InputDims[I] = static_cast<size_t>(DimensionBuf[I]);

    status = ie_network_get_input_name(Network, Index, &InputName);
    if (status != OK) {
      spdlog::error("Unable to find input name correctly with Index:{}", Index);
      return -1;
    }
    /* Mark input as resizable by setting of a resize algorithm.
     * In this case we will be able to set an input blob of any shape to an
     * infer request. Resize and layout conversions are executed automatically
     * during inference */
    status = ie_network_set_input_resize_algorithm(Network, InputName,
                                                   RESIZE_BILINEAR);
    if (status != OK) {
      spdlog::error("Unable to set input resize correctly");
      return -1;
    }
    status = ie_network_set_input_layout(
        Network, InputName, NHWC); // more layouts should be supported
    if (status != OK) {
      spdlog::error("Unable to set input layout correctly");
      return -1;
    }
    status =
        ie_network_set_input_precision(Network, InputName,
                                       FP32); // more types should be supported
    if (status != OK) {
      spdlog::error("Unable to set input precision correctly");
      return -1;
    }

    dimensions_t Dimens;
    Dimens.ranks = InputDims.size();
    for (size_t I = 0; I < Dimens.ranks; I++)
      Dimens.dims[I] = InputDims[I];
    tensor_desc_t TensorDesc = {NHWC, Dimens,
                                FP32}; // more types should be supported

    status = ie_blob_make_memory(&TensorDesc, &InputBlob);
    if (status != OK) {
      spdlog::error("Unable to allocated input tensor correctly");
      return -1;
    }
    status = ie_blob_size(InputBlob, &BlobSize);
    spdlog::info("Blob size {}, with Tensor size {}", BlobSize,
                 TensorDataLen / 4);
    status = ie_blob_get_cbuffer(InputBlob, &BlobCBuffer);
    if (status != OK) {
      spdlog::error("Unable to find input tensor buffer");
      return -1;
    }

    float *BlobData = (float *)(BlobCBuffer.cbuffer);
    float *CastedTensorDataBuf = reinterpret_cast<float *>(TensorDataBuf);
    for (size_t I = 0; I < (TensorDataLen / 4); I++) {
      BlobData[I] = CastedTensorDataBuf[I];
    }
    status = ie_infer_request_set_blob(InferRequest, InputName, InputBlob);
    if (status != OK) {
      spdlog::error(
          "Unable to set input tensor to model correctly: erro code {}",
          status);
      return -1;
    }
    Ctx.OpenVINOInputs.push_back(InputBlob);

    return 0;
#else
    spdlog::error(
        "OpenVINO backend is not built. define -DWASINN_BUILD_OPENVINO "
        "to build it.");
#endif
  } else {
    spdlog::error("Current backend is not supported.");
  }
  return -1;
}

Expect<uint32_t> WasiNNGetOuput::body(
    Runtime::Instance::MemoryInstance *MemInst [[maybe_unused]],
    uint32_t Context [[maybe_unused]], uint32_t Index [[maybe_unused]],
    uint32_t OutBuffer [[maybe_unused]],
    uint32_t OutBufferMaxSize [[maybe_unused]],
    uint32_t BytesWrittenPtr [[maybe_unused]]) {
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::ExecutionFailed);
  }

  [[maybe_unused]] uint8_t *OutBufferPtr;
  [[maybe_unused]] uint32_t *BytesWritten;

  if (Ctx.GraphContextBackends.size() <= Context) {
    spdlog::error("get_output: Execution Context does not exist");
    return -1;
  }

  OutBufferPtr = MemInst->getPointer<uint8_t *>(OutBuffer, 1);
  BytesWritten = MemInst->getPointer<uint32_t *>(BytesWrittenPtr, 1);

  if (Ctx.GraphContextBackends[Context] == Ctx.BackendsMapping.at("OpenVINO")) {
#ifdef WASMEDGE_WASINN_BUILD_OPENVINO
    IEStatusCode status;
    OpenVINOSession *Session = Ctx.OpenVINOInfers[Context];
    ie_network_t *Network = Session->network;
    ie_infer_request_t *InferRequest = Session->infer_request;
    ie_blob_t *OutputBlob = nullptr;
    ie_blob_buffer_t BlobCBuffer;
    char *OutputName = nullptr;
    int BlobSize;

    status = ie_network_get_output_name(Network, Index, &OutputName);
    if (status != OK) {
      spdlog::error("Unable to find output name correctly with Index:{}",
                    Index);
      return -1;
    }
    status = ie_network_set_output_precision(Network, OutputName, FP32);
    if (status != OK) {
      spdlog::error("Unable to set output precision correctly with Index:{}",
                    Index);
      return -1;
    }

    status = ie_infer_request_get_blob(InferRequest, OutputName, &OutputBlob);
    if (status != OK || OutputName == nullptr) {
      spdlog::error("Unable to retrieve output tensor correctly", Index);
      return -1;
    }
    status = ie_blob_size(OutputBlob, &BlobSize);
    status = ie_blob_get_cbuffer(OutputBlob, &BlobCBuffer);
    if (status != OK) {
      spdlog::error("Unable to retrieve output tensor correctly", Index);
      return -1;
    }
    float *BlobData = (float *)(BlobCBuffer.cbuffer);
    uint8_t *CastedOutputData = reinterpret_cast<uint8_t *>(BlobData);
    for (int I = 0; I < BlobSize * 4; I++) {
      OutBufferPtr[I] = CastedOutputData[I];
    }
    *BytesWritten = BlobSize * 4;
    return 0;
#else
    spdlog::error(
        "OpenVINO backend is not built. define -DWASINN_BUILD_OPENVINO "
        "to build it.");
#endif
  } else {
    spdlog::error("Current backend is not supported.");
  }
  return -1;
}

Expect<uint32_t> WasiNNCompute::body(Runtime::Instance::MemoryInstance *MemInst
                                     [[maybe_unused]],
                                     uint32_t Context [[maybe_unused]]) {

  if (MemInst == nullptr) {
    return Unexpect(ErrCode::ExecutionFailed);
  }

  if (Ctx.GraphContextBackends.size() <= Context) {
    spdlog::error("compute: Execution Context does not exist");
    return -1;
  }

  if (Ctx.GraphContextBackends[Context] == Ctx.BackendsMapping.at("OpenVINO")) {
#ifdef WASMEDGE_WASINN_BUILD_OPENVINO
    IEStatusCode status;
    OpenVINOSession *Session = Ctx.OpenVINOInfers[Context];
    status = ie_infer_request_infer(Session->infer_request);
    if (status != OK) {
      spdlog::error("Unable to perform computation correctly");
      return -1;
    }
    return 0;
#else
    spdlog::error(
        "OpenVINO backend is not built. define -DWASINN_BUILD_OPENVINO "
        "to build it.");
#endif
  } else {
    spdlog::error("Current backend is not supported.");
  }

  return -1;
}

} // namespace Host
} // namespace WasmEdge
