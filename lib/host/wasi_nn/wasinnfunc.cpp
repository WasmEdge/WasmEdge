// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "host/wasi_nn/wasinnfunc.h"
#include "common/errcode.h"
#include "common/log.h"
#include "runtime/hostfunc.h"
#include "runtime/instance/memory.h"

#ifdef WASMEDGE_WASINN_BUILD_OPENVINO
#include <c_api/ie_c_api.h>
#include <string>
#endif

namespace WasmEdge {
namespace Host {

#ifdef WASMEDGE_WASINN_BUILD_OPENVINO
namespace {
const std::string mapTargetToString(uint32_t Target) {
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
} // namespace
#endif

Expect<uint32_t> WasiNNLoad::body(Runtime::Instance::MemoryInstance *MemInst,
                                  uint32_t BuilderPtr [[maybe_unused]],
                                  uint32_t BuilderLen [[maybe_unused]],
                                  uint32_t Encoding,
                                  uint32_t Target [[maybe_unused]],
                                  uint32_t GraphPtr [[maybe_unused]]) {
  // GraphBuilders' Layout: |builder-0|builder-0 len|builder-1|builder-1 len|...
  // Check memory instance from module.
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::ExecutionFailed);
  }

  if (Encoding == Ctx.BackendsMapping.at("OpenVINO")) {
#ifdef WASMEDGE_WASINN_BUILD_OPENVINO
    if (BuilderLen != 2) {
      spdlog::error("[WASI-NN] Wrong GraphBuilder Length {:d}, expecting 2",
                    BuilderLen);
      return static_cast<uint32_t>(WASINN::ErrNo::InvalidArgument);
    }
    uint32_t *GraphBuilders =
        MemInst->getPointer<uint32_t *>(BuilderPtr, BuilderLen * 2);
    uint32_t *GraphId = MemInst->getPointer<uint32_t *>(GraphPtr, 1);

    IEStatusCode Status;
    if (Ctx.OpenVINOCore == nullptr) {
      Status = ie_core_create("", &Ctx.OpenVINOCore);
      if (Status != IEStatusCode::OK || Ctx.OpenVINOCore == nullptr) {
        spdlog::error("[WASI-NN] Error happened when init OpenVINO core.");
        return static_cast<uint32_t>(WASINN::ErrNo::MissingMemory);
      }
      spdlog::debug("[WASI-NN] Initialize OpenVINO Core");
    }

    std::string DeviceName = mapTargetToString(Target);
    if (DeviceName.length() == 0) {
      spdlog::error("[WASI-NN] Device target {:d} not support!", Target);
      return static_cast<uint32_t>(WASINN::ErrNo::InvalidArgument);
    } else {
      spdlog::debug("[WASI-NN] Using device: {:s}", DeviceName);
    }

    uint32_t XMLStringLen = GraphBuilders[1];
    uint32_t WeightBinsLen = GraphBuilders[3];
    uint8_t *XMLPtr =
        MemInst->getPointer<uint8_t *>(GraphBuilders[0], XMLStringLen);
    uint8_t *BinPtr =
        MemInst->getPointer<uint8_t *>(GraphBuilders[2], WeightBinsLen);
    tensor_desc_t WeightsDesc{
        layout_e::ANY, {1, {WeightBinsLen}}, precision_e::U8};
    ie_blob_t *WeightsBlob = nullptr;
    ie_blob_buffer_t BlobCBuffer;

    Status = ie_blob_make_memory(&WeightsDesc, &WeightsBlob);
    if (Status != IEStatusCode::OK || WeightsBlob == nullptr) {
      spdlog::error(
          "[WASI-NN] Unable to create model's weight blob, error code: {}",
          Status);
      return static_cast<uint32_t>(WASINN::ErrNo::Busy);
    }
    Status = ie_blob_get_cbuffer(WeightsBlob, &BlobCBuffer);
    if (Status != IEStatusCode::OK) {
      spdlog::error(
          "[WASI-NN] Unable to find weight blob's buffer, error code: {}",
          Status);
      return static_cast<uint32_t>(WASINN::ErrNo::MissingMemory);
    }
    uint8_t *BlobData = static_cast<uint8_t *>(BlobCBuffer.buffer);
    for (size_t I = 0; I < WeightBinsLen; I++) {
      BlobData[I] = BinPtr[I];
    }

    ie_network_t *Network = nullptr;
    ie_executable_network_t *ExeNetwork = nullptr;
    Status = ie_core_read_network_from_memory(
        Ctx.OpenVINOCore, XMLPtr, XMLStringLen, WeightsBlob, &Network);
    if (Status != IEStatusCode::OK || Network == nullptr) {
      spdlog::error("[WASI-NN] Unable to create Network");
      return static_cast<uint32_t>(WASINN::ErrNo::Busy);
    }

    size_t NetworkInputSize;
    Status = ie_network_get_inputs_number(Network, &NetworkInputSize);
    // FIXME: this is a temporary workaround. We need a more eligant way to
    // specify the layout in the long run. However, without this newer versions
    // of OpenVINO will fail due to parameter mismatch.
    for (size_t I = 0; I < NetworkInputSize; I++) {
      char *InputName = nullptr;
      Status = ie_network_get_input_name(Network, I, &InputName);
      spdlog::debug("[WASI-NN] Setting [{}] to NHWC", InputName);
      Status = ie_network_set_input_layout(
          Network, InputName,
          layout_e::NHWC); // more layouts should be supported
      ie_network_name_free(&InputName);
      if (Status != IEStatusCode::OK) {
        spdlog::error("[WASI-NN] Unable to set input name, error code {}",
                      Status);
        return static_cast<uint32_t>(WASINN::ErrNo::MissingMemory);
      }
    }

    ie_config_t Config = {nullptr, nullptr, nullptr};
    Status = ie_core_load_network(Ctx.OpenVINOCore, Network, DeviceName.c_str(),
                                  &Config, &ExeNetwork);
    if (Status != IEStatusCode::OK || ExeNetwork == nullptr) {
      spdlog::error("[WASI-NN] Unable to create executable Network");
      return static_cast<uint32_t>(WASINN::ErrNo::Busy);
    }

    Ctx.ModelsNum = Ctx.OpenVINONetworks.size();
    Ctx.OpenVINONetworks.push_back(Network);
    Ctx.OpenVINOExecutions.push_back(ExeNetwork);
    Ctx.OpenVINOModelWeights.push_back(WeightsBlob);
    Ctx.GraphBackends.push_back(Encoding);
    *GraphId = Ctx.ModelsNum;

    return static_cast<uint32_t>(WASINN::ErrNo::Success);
#else
    spdlog::error("[WASI-NN] OpenVINO backend is not built. use "
                  "-DWASMEDGE_WASINN_BUILD_OPENVINO=ON"
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

  if (Ctx.GraphBackends.size() <= GraphId) {
    spdlog::error("[WASI-NN] init_execution_context: Graph Id does not exist");
    return static_cast<uint32_t>(WASINN::ErrNo::InvalidArgument);
  }

  if (Ctx.GraphBackends[GraphId] == Ctx.BackendsMapping.at("OpenVINO")) {
#ifdef WASMEDGE_WASINN_BUILD_OPENVINO
    IEStatusCode Status;
    uint32_t *Context = MemInst->getPointer<uint32_t *>(ContextPtr, 1);
    if (Ctx.OpenVINOExecutions[GraphId] == nullptr ||
        Ctx.OpenVINONetworks[GraphId] == nullptr) {
      spdlog::error("[WASI-NN] Model for Graph:{} is empty!", GraphId);
      return static_cast<uint32_t>(WASINN::ErrNo::MissingMemory);
    }
    WASINN::OpenVINOSession *Session = new WASINN::OpenVINOSession();
    Session->ExeNetwork = Ctx.OpenVINOExecutions[GraphId];
    Session->Network = Ctx.OpenVINONetworks[GraphId];
    Status = ie_exec_network_create_infer_request(Session->ExeNetwork,
                                                  &(Session->InferRequest));

    if (Status != IEStatusCode::OK || Session->InferRequest == nullptr) {
      spdlog::error("[WASI-NN] Unable to create openvino session");
      return static_cast<uint32_t>(WASINN::ErrNo::Busy);
    }

    Ctx.ExecutionsNum = Ctx.OpenVINOInfers.size();
    Ctx.OpenVINOInfers.push_back(Session);
    Ctx.GraphContextBackends.push_back(GraphId);
    *Context = Ctx.ExecutionsNum;

    return static_cast<uint32_t>(WASINN::ErrNo::Success);
#else
    spdlog::error("[WASI-NN] OpenVINO backend is not built. define "
                  "-DWASMEDGE_WASINN_BUILD_OPENVINO "
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

  if (Ctx.GraphContextBackends.size() <= Context) {
    spdlog::error("[WASI-NN] set_input: Execution Context does not exist");
    return static_cast<uint32_t>(WASINN::ErrNo::InvalidArgument);
  }
  if (Ctx.GraphContextBackends[Context] == Ctx.BackendsMapping.at("OpenVINO")) {
#ifdef WASMEDGE_WASINN_BUILD_OPENVINO
    IEStatusCode Status;
    WASINN::OpenVINOSession *Session = Ctx.OpenVINOInfers[Context];
    ie_network_t *Network = Session->Network;
    ie_infer_request_t *InferRequest = Session->InferRequest;
    ie_blob_t *InputBlob = nullptr;
    ie_blob_buffer_t BlobCBuffer;
    int BlobSize;
    char *InputName = nullptr;

    if (Network == nullptr || InferRequest == nullptr) {
      spdlog::error("[WASI-NN] The founded openvino session is empty");
      return static_cast<uint32_t>(WASINN::ErrNo::MissingMemory);
    }

    uint32_t *Tensor = MemInst->getPointer<uint32_t *>(TensorPtr, 5);
    uint32_t DimensionLen = Tensor[1];
    uint32_t *DimensionBuf =
        MemInst->getPointer<uint32_t *>(Tensor[0], DimensionLen);
    uint32_t RType = Tensor[2];
    uint32_t TensorDataLen = Tensor[4];
    uint8_t *TensorDataBuf =
        MemInst->getPointer<uint8_t *>(Tensor[3], TensorDataLen);
    if (RType != 1) {
      spdlog::error(
          "[WASI-NN] Only F32 inputs and outputs are supported for now");
      return static_cast<uint32_t>(WASINN::ErrNo::InvalidArgument);
    }
    if (DimensionLen > 8) {
      spdlog::error(
          "[WASI-NN] Tensor dimension is out of range, expect it under 8-dim, "
          "but got {}-dim",
          DimensionLen);
      return static_cast<uint32_t>(WASINN::ErrNo::InvalidArgument);
    }

    Status = ie_network_get_input_name(Network, Index, &InputName);
    if (Status != IEStatusCode::OK) {
      spdlog::error(
          "[WASI-NN] Unable to find input name correctly with Index:{}", Index);
      ie_network_name_free(&InputName);
      return static_cast<uint32_t>(WASINN::ErrNo::InvalidArgument);
    }
    /* Mark input as resizable by setting of a resize algorithm.
     * In this case we will be able to set an input blob of any shape to an
     * infer request. Resize and layout conversions are executed automatically
     * during inference */
    Status = ie_network_set_input_resize_algorithm(Network, InputName,
                                                   RESIZE_BILINEAR);
    if (Status != IEStatusCode::OK) {
      spdlog::error("[WASI-NN] Unable to set input resize correctly");
      ie_network_name_free(&InputName);
      return static_cast<uint32_t>(WASINN::ErrNo::InvalidArgument);
    }
    Status = ie_network_set_input_layout(
        Network, InputName,
        layout_e::NHWC); // more layouts should be supported
    if (Status != IEStatusCode::OK) {
      spdlog::error("[WASI-NN] Unable to set input layout correctly");
      ie_network_name_free(&InputName);
      return static_cast<uint32_t>(WASINN::ErrNo::InvalidArgument);
    }
    Status = ie_network_set_input_precision(
        Network, InputName,
        precision_e::FP32); // more types should be supported
    if (Status != IEStatusCode::OK) {
      spdlog::error("[WASI-NN] Unable to set input precision correctly");
      ie_network_name_free(&InputName);
      return static_cast<uint32_t>(WASINN::ErrNo::InvalidArgument);
    }

    dimensions_t Dimens;
    Dimens.ranks = DimensionLen;
    for (size_t I = 0; I < Dimens.ranks; I++) {
      Dimens.dims[I] = static_cast<size_t>(DimensionBuf[I]);
    }

    tensor_desc_t TensorDesc = {
        layout_e::NHWC, Dimens,
        precision_e::FP32}; // more types should be supported

    Status = ie_blob_make_memory(&TensorDesc, &InputBlob);
    if (Status != IEStatusCode::OK) {
      spdlog::error("[WASI-NN] Unable to allocated input tensor correctly");
      ie_blob_free(&InputBlob);
      ie_network_name_free(&InputName);
      return static_cast<uint32_t>(WASINN::ErrNo::Busy);
    }
    Status = ie_blob_size(InputBlob, &BlobSize);
    spdlog::debug("[WASI-NN] Blob size {}, with Tensor size {}", BlobSize,
                  TensorDataLen / 4);
    Status = ie_blob_get_cbuffer(InputBlob, &BlobCBuffer);
    if (Status != IEStatusCode::OK) {
      spdlog::error("[WASI-NN] Unable to find input tensor buffer");
      ie_blob_free(&InputBlob);
      ie_network_name_free(&InputName);
      return static_cast<uint32_t>(WASINN::ErrNo::MissingMemory);
    }

    float *BlobData = static_cast<float *>(BlobCBuffer.buffer);
    float *CastedTensorDataBuf = reinterpret_cast<float *>(TensorDataBuf);
    for (size_t I = 0; I < (TensorDataLen / 4); I++) {
      BlobData[I] = CastedTensorDataBuf[I];
    }
    Status = ie_infer_request_set_blob(InferRequest, InputName, InputBlob);
    if (Status != IEStatusCode::OK) {
      spdlog::error("[WASI-NN] Unable to set input tensor to model correctly: "
                    "erro code {}",
                    Status);
      ie_blob_free(&InputBlob);
      ie_network_name_free(&InputName);
      return static_cast<uint32_t>(WASINN::ErrNo::Busy);
    }
    ie_blob_free(&InputBlob);
    ie_network_name_free(&InputName);

    return static_cast<uint32_t>(WASINN::ErrNo::Success);
#else
    spdlog::error("[WASI-NN] OpenVINO backend is not built, use "
                  "-DWASMEDGE_WASINN_BUILD_OPENVINO=ON"
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
                     uint32_t OutBuffer [[maybe_unused]],
                     uint32_t OutBufferMaxSize [[maybe_unused]],
                     uint32_t BytesWrittenPtr [[maybe_unused]]) {
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::ExecutionFailed);
  }

  if (Ctx.GraphContextBackends.size() <= Context) {
    spdlog::error("[WASI-NN] get_output: Execution Context does not exist");
    return static_cast<uint32_t>(WASINN::ErrNo::InvalidArgument);
  }

  if (Ctx.GraphContextBackends[Context] == Ctx.BackendsMapping.at("OpenVINO")) {
#ifdef WASMEDGE_WASINN_BUILD_OPENVINO
    uint8_t *OutBufferPtr;
    uint32_t *BytesWritten;

    IEStatusCode Status;
    WASINN::OpenVINOSession *Session = Ctx.OpenVINOInfers[Context];
    ie_network_t *Network = Session->Network;
    ie_infer_request_t *InferRequest = Session->InferRequest;
    ie_blob_t *OutputBlob = nullptr;
    ie_blob_buffer_t BlobCBuffer;
    char *OutputName = nullptr;
    int BlobSize;

    Status = ie_network_get_output_name(Network, Index, &OutputName);
    if (Status != IEStatusCode::OK) {
      spdlog::error(
          "[WASI-NN] Unable to find output name correctly with Index:{}",
          Index);
      ie_network_name_free(&OutputName);
      return static_cast<uint32_t>(WASINN::ErrNo::InvalidArgument);
    }
    Status =
        ie_network_set_output_precision(Network, OutputName, precision_e::FP32);
    if (Status != IEStatusCode::OK) {
      spdlog::error(
          "[WASI-NN] Unable to set output precision correctly with Index:{}",
          Index);
      ie_network_name_free(&OutputName);
      return static_cast<uint32_t>(WASINN::ErrNo::InvalidArgument);
    }

    Status = ie_infer_request_get_blob(InferRequest, OutputName, &OutputBlob);
    if (Status != IEStatusCode::OK || OutputBlob == nullptr) {
      spdlog::error("[WASI-NN] Unable to retrieve output tensor correctly",
                    Index);
      ie_network_name_free(&OutputName);
      ie_blob_free(&OutputBlob);
      return static_cast<uint32_t>(WASINN::ErrNo::InvalidArgument);
    }
    Status = ie_blob_size(OutputBlob, &BlobSize);
    Status = ie_blob_get_cbuffer(OutputBlob, &BlobCBuffer);
    if (Status != IEStatusCode::OK) {
      spdlog::error("[WASI-NN] Unable to retrieve output tensor correctly",
                    Index);
      ie_network_name_free(&OutputName);
      ie_blob_free(&OutputBlob);
      return static_cast<uint32_t>(WASINN::ErrNo::MissingMemory);
    }
    float *BlobData = static_cast<float *>(BlobCBuffer.buffer);
    size_t BytesToWrite = BlobSize * 4;
    if (BytesToWrite > OutBufferMaxSize) {
      BytesToWrite = OutBufferMaxSize;
    }
    uint8_t *CastedOutputData = reinterpret_cast<uint8_t *>(BlobData);

    OutBufferPtr = MemInst->getPointer<uint8_t *>(OutBuffer, BytesToWrite);
    for (size_t I = 0; I < BytesToWrite; I++) {
      OutBufferPtr[I] = CastedOutputData[I];
    }
    BytesWritten = MemInst->getPointer<uint32_t *>(BytesWrittenPtr, 1);
    *BytesWritten = BytesToWrite;
    ie_network_name_free(&OutputName);
    ie_blob_free(&OutputBlob);

    return static_cast<uint32_t>(WASINN::ErrNo::Success);
#else
    spdlog::error("[WASI-NN] OpenVINO backend is not built. use "
                  "-DWASMEDGE_WASINN_BUILD_OPENVINO=ON"
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
  if (Ctx.GraphContextBackends.size() <= Context) {
    spdlog::error("[WASI-NN] compute: Execution Context does not exist");
    return static_cast<uint32_t>(WASINN::ErrNo::InvalidArgument);
  }

  if (Ctx.GraphContextBackends[Context] == Ctx.BackendsMapping.at("OpenVINO")) {
#ifdef WASMEDGE_WASINN_BUILD_OPENVINO
    IEStatusCode Status;
    WASINN::OpenVINOSession *Session = Ctx.OpenVINOInfers[Context];
    Status = ie_infer_request_infer(Session->InferRequest);
    if (Status != IEStatusCode::OK) {
      spdlog::error("[WASI-NN] Unable to perform computation correctly");
      return static_cast<uint32_t>(WASINN::ErrNo::Busy);
    }
    return static_cast<uint32_t>(WASINN::ErrNo::Success);
#else
    spdlog::error("[WASI-NN] OpenVINO backend is not built. use "
                  "-DWASMEDGE_WASINN_BUILD_OPENVINO=ON"
                  "to build it.");
#endif
  } else {
    spdlog::error("[WASI-NN] Current backend is not supported.");
  }

  return static_cast<uint32_t>(WASINN::ErrNo::InvalidArgument);
}

} // namespace Host
} // namespace WasmEdge
