// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "host/wasi_nn/wasinnfunc.h"
#include "common/errcode.h"
#include "runtime/hostfunc.h"
#include "runtime/instance/memory.h"

#ifdef WASMEDGE_WASINN_BUILD_OPENVINO
#include <inference_engine.hpp>
#include <string>
namespace IE = InferenceEngine;
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
  uint32_t *GraphBuilders;
  uint32_t *Graph;
  GraphBuilders = MemInst->getPointer<uint32_t *>(BuilderPtr, 1);
  Graph = MemInst->getPointer<uint32_t *>(GraphPtr, 1);
  if (Encoding == this->Ctx.BackendsMapping.at("OpenVINO")) {
#ifdef WASMEDGE_WASINN_BUILD_OPENVINO
    if (BuilderLen != 2) {
      spdlog::error("Wrong GraphBuilder Length {:d}, expecting 2", BuilderLen);
      return -1;
    }
    IE::Core ie_;
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

    std::string XMLModel(XMLStrings.begin(), XMLStrings.end());
    IE::TensorDesc BinTensorDesc(IE::Precision::U8, {WeightBins.size()},
                           IE::Layout::C);

    std::shared_ptr<IE::Data> WeightDataNode(
        new IE::Data("WeightDataNode", BinTensorDesc));
    IE::Blob::Ptr WeightBlob = IE::Blob::CreateFromData(WeightDataNode);
    WeightBlob->allocate();
    uint8_t *BufferPtr = static_cast<uint8_t *>(WeightBlob->buffer());
    for (size_t I = 0; I < WeightBins.size(); I++) {
      BufferPtr[I] = WeightBins[I];
    }
    IE::CNNNetwork Network = ie_.ReadNetwork(XMLModel, WeightBlob);
    IE::ExecutableNetwork TargetedExecutableNetwork =
        ie_.LoadNetwork(Network, DeviceName);

    this->Ctx.ModelsNum++;
    this->Ctx.OpenVINONetworks.emplace(this->Ctx.ModelsNum, std::move(Network));
    this->Ctx.OpenVINOExecutions.emplace(this->Ctx.ModelsNum,
                                         std::move(TargetedExecutableNetwork));
    this->Ctx.GraphBackends.emplace(this->Ctx.ModelsNum, Encoding);
    *Graph = this->Ctx.ModelsNum;
    // std::vector<std::string> availableDevices = ie_.GetAvailableDevices();
    // for (auto x : availableDevices) {
    //   spdlog::info("device: {:s}", x);
    // }
    return 0;
#else
    spdlog::error("OpenVINO backend is not built. define -DWASINN_BUILD_OPENVINO "
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

  if (this->Ctx.GraphBackends.find(Graph) == this->Ctx.GraphBackends.end()) {
    spdlog::error("init_execution_context: Graph does not exist");
    return -1;
  }
  [[maybe_unused]] uint32_t *Context =
      MemInst->getPointer<uint32_t *>(ContextPtr, 1);

  if (this->Ctx.GraphBackends[Graph] ==
      this->Ctx.BackendsMapping.at("OpenVINO")) {
#ifdef WASMEDGE_WASINN_BUILD_OPENVINO
    OpenVINOSession Session;
    Session.SessionInferRequest =
        this->Ctx.OpenVINOExecutions[Graph].CreateInferRequest();
    Session.Network = &(this->Ctx.OpenVINONetworks[Graph]);
    Session.TargetedExecutableNetwork = &(this->Ctx.OpenVINOExecutions[Graph]);

    this->Ctx.ExecutionsNum++;
    this->Ctx.OpenVINOInfers.emplace(this->Ctx.ExecutionsNum,
                                     std::move(Session));
    this->Ctx.GraphContextBackends.emplace(this->Ctx.ExecutionsNum,
                                           this->Ctx.GraphBackends[Graph]);
    *Context = this->Ctx.ExecutionsNum;
    return 0;
#else
    spdlog::error("OpenVINO backend is not built. define -DWASINN_BUILD_OPENVINO "
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

  if (this->Ctx.GraphContextBackends.find(Context) ==
      this->Ctx.GraphContextBackends.end()) {
    spdlog::error("set_input: Execution Context does not exist");
    return -1;
  }
  if (this->Ctx.GraphContextBackends[Context] ==
      this->Ctx.BackendsMapping.at("OpenVINO")) {
#ifdef WASMEDGE_WASINN_BUILD_OPENVINO
    // reading tensor
    OpenVINOSession &Session = this->Ctx.OpenVINOInfers[Context];
    uint32_t *Tensor = MemInst->getPointer<uint32_t *>(TensorPtr, 1);
    uint32_t *DimensionBuf = MemInst->getPointer<uint32_t *>(Tensor[0]);
    uint32_t DimensionLen = Tensor[1];
    uint32_t RType = Tensor[2];
    uint8_t *TensorDataBuf = MemInst->getPointer<uint8_t *>(Tensor[3]);
    uint32_t TensorDataLen = Tensor[4];

    if (RType != 1) {
      spdlog::error(
          "set_input: only F32 inputs and outputs are supported for now");
      return -1;
    }
    std::vector<size_t> InputDims(DimensionLen);
    for (uint32_t I = 0; I < InputDims.size(); I++)
      InputDims[I] = static_cast<size_t>(DimensionBuf[I]);
    size_t total = 1;
    for (auto X : InputDims)
      total *= X;
    TensorType EnumRType = static_cast<TensorType>(RType);

    IE::Precision TensorPrecision;
    switch (EnumRType) {
    // case TensorType::TENSOR_TYPE_U8:
    //   TensorPrecision = IE::Precision::U8;
    //   spdlog::info("Load tensor type U8");
    //   break;
    // case TensorType::TENSOR_TYPE_I32:
    //   TensorPrecision = IE::Precision::I32;
    //   spdlog::info("Load tensor type I32");
    //   break;

    /*Only FP32 supported*/
    case TensorType::TENSOR_TYPE_F32:
      TensorPrecision = IE::Precision::FP32;
      break;
    // case TensorType::TENSOR_TYPE_F16:
    //   TensorPrecision = IE::Precision::FP16;
    //   spdlog::info("Load tensor type F16");
    //   break;
    default:
      break;
    }
    IE::TensorDesc tDesc(TensorPrecision, InputDims, IE::Layout::NHWC);
    IE::CNNNetwork *Network = Session.Network;

    IE::InputsDataMap InputInfo = Network->getInputsInfo();
    std::string InputName;

    uint32_t InputIndex = 0;
    for (auto &Item : InputInfo) {
      if (InputIndex == Index) {
        InputName = Item.first;
        spdlog::info("Index input: {:s}", InputName);
        Item.second->setPrecision(TensorPrecision);
        break;
      }
      InputIndex++;
    }
    if (InputIndex >= InputInfo.size()) {
      spdlog::error("Index of inputs is out of range");
      return -1;
    }

    IE::Blob::Ptr InputBlob = Session.SessionInferRequest.GetBlob(InputName);
    IE::SizeVector BlobSize = InputBlob->getTensorDesc().getDims();
    const size_t width = BlobSize[3];
    const size_t height = BlobSize[2];
    const size_t channels = BlobSize[1];
    spdlog::info("Input shape for {:s}: {:d}, {:d}, {:d}", InputName, channels,
              width, height);
    IE::MemoryBlob::Ptr MBlob = IE::as<IE::MemoryBlob>(InputBlob);
    if (!MBlob) {
      spdlog::error("We expect image blob to be inherited from MemoryBlob in "
                 "OpenVINO backend, but "
                 "by fact we were not able "
                 "to cast imageInput to MemoryBlob");
      return -1;
    }
    // locked memory holder should be alive all time while access to its buffer
    // happens
    auto MBlobHolder = MBlob->wmap();
    float *BlobData = MBlobHolder.as<float *>();
    for (size_t I = 0; I < TensorDataLen / 4; I++) {
      uint32_t Tmp =
          (TensorDataBuf[4 * I] << 0 | TensorDataBuf[4 * I + 1] << 8 |
           TensorDataBuf[4 * I + 2] << 16 | TensorDataBuf[4 * I + 3] << 24);
      BlobData[I] = *reinterpret_cast<float *>(&Tmp);
    }
    return 0;
#else
    spdlog::error("OpenVINO backend is not built. define -DWASINN_BUILD_OPENVINO "
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
  [[maybe_unused]] uint8_t *OutBufferPtr;
  [[maybe_unused]] uint32_t *BytesWritten;

  if (this->Ctx.GraphContextBackends.find(Context) ==
      this->Ctx.GraphContextBackends.end()) {
    spdlog::error("get_output: Execution Context does not exist");
    return -1;
  }

  if (this->Ctx.GraphContextBackends[Context] ==
      this->Ctx.BackendsMapping.at("OpenVINO")) {
#ifdef WASMEDGE_WASINN_BUILD_OPENVINO
    OutBufferPtr = MemInst->getPointer<uint8_t *>(OutBuffer, 1);
    BytesWritten = MemInst->getPointer<uint32_t *>(BytesWrittenPtr, 1);
    OpenVINOSession &Session = this->Ctx.OpenVINOInfers[Context];
    IE::CNNNetwork *Network = Session.Network;

    IE::OutputsDataMap OutputInfo = Network->getOutputsInfo();
    std::string OutputName;
    uint32_t OutputIndex = 0;
    for (auto &Item : OutputInfo) {
      if (OutputIndex == Index) {
        OutputName = Item.first;
        Item.second->setPrecision(IE::Precision::FP32);
        break;
      }
      OutputIndex++;
    }
    if (OutputIndex >= OutputInfo.size()) {
      spdlog::error("Index of outputs is out of range");
      return -1;
    }

    const IE::Blob::Ptr OutputBlob =
        Session.SessionInferRequest.GetBlob(OutputName);
    IE::MemoryBlob::Ptr MBlob = IE::as<IE::MemoryBlob>(OutputBlob);
    if (!MBlob) {
      spdlog::error("We expect output blob to be inherited from MemoryBlob in "
                 "OpenVINO backend, but "
                 "by fact we were not able "
                 "to cast network output to MemoryBlob");
      return -1;
    }
    auto MBlobHolder = MBlob->wmap();
    float *BlobData = MBlobHolder.as<float *>();
    auto OutputSize = OutputBlob->size();
    spdlog::info("get ouput with total size {:d}", OutputSize);
    for (uint32_t I = 0; I < OutputSize; I++) {
      float Value = BlobData[I];
      OutBufferPtr[4 * I] = *reinterpret_cast<uint32_t *>(&Value) & 0xff;
      OutBufferPtr[4 * I + 1] =
          (*reinterpret_cast<uint32_t *>(&Value) & 0xff00) >> 8;
      OutBufferPtr[4 * I + 2] =
          (*reinterpret_cast<uint32_t *>(&Value) & 0xff0000) >> 16;
      OutBufferPtr[4 * I + 3] =
          (*reinterpret_cast<uint32_t *>(&Value) & 0xff000000) >> 24;
    }
    *BytesWritten = OutputSize * 4;
    return 0;
#else
    spdlog::error("OpenVINO backend is not built. define -DWASINN_BUILD_OPENVINO "
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

  if (this->Ctx.GraphContextBackends.find(Context) ==
      this->Ctx.GraphContextBackends.end()) {
    spdlog::error("compute: Execution Context does not exist");
    return -1;
  }

  if (this->Ctx.GraphContextBackends[Context] ==
      this->Ctx.BackendsMapping.at("OpenVINO")) {
#ifdef WASMEDGE_WASINN_BUILD_OPENVINO
    OpenVINOSession &Session = this->Ctx.OpenVINOInfers[Context];
    Session.SessionInferRequest.Infer();
    return 0;
#else
    spdlog::error("OpenVINO backend is not built. define -DWASINN_BUILD_OPENVINO "
               "to build it.");
#endif
  } else {
    spdlog::error("Current backend is not supported.");
  }

  return -1;
}

} // namespace Host
} // namespace WasmEdge
