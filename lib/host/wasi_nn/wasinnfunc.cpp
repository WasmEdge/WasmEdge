// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "host/wasi_nn/wasinnfunc.h"
#include "common/errcode.h"
#include "runtime/hostfunc.h"
#include "runtime/instance/memory.h"

#ifdef WASINN_BUILD_OPENVINO
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
  auto log = spdlog::get("WasiNN");
  // GraphBuilders' Layout: |builder-0|builder-0 len|builder-1|builder-1 len|...
  uint32_t *GraphBuilders;
  uint32_t *Graph;
  GraphBuilders = MemInst->getPointer<uint32_t *>(BuilderPtr, 1);
  Graph = MemInst->getPointer<uint32_t *>(GraphPtr, 1);
  if (Encoding == this->Ctx.BackendsMapping.at("OpenVINO")) {
#ifdef WASINN_BUILD_OPENVINO
    if (BuilderLen != 2) {
      log->error("Wrong GraphBuilder Length {:d}, expecting 2", BuilderLen);
      return -1;
    }
    IE::Core ie_;
    std::string device_name = map_target_to_string(Target);
    if (device_name.length() == 0) {
      log->error("Device target {:d} not support!", Target);
      return -1;
    } else {
      log->info("Using device: {:s}", device_name);
    }
    uint32_t xml_strings_len = GraphBuilders[1];
    uint32_t weight_bins_len = GraphBuilders[3];

    uint8_t *xml_ptr = MemInst->getPointer<uint8_t *>(GraphBuilders[0], 1);
    uint8_t *bin_ptr = MemInst->getPointer<uint8_t *>(GraphBuilders[2], 1);
    std::vector<uint8_t> xml_strings(xml_ptr, xml_ptr + xml_strings_len);
    std::vector<uint8_t> weight_bins(bin_ptr, bin_ptr + weight_bins_len);

    log->info("read xml length {:d}", xml_strings.size());
    log->info("read bin length {:d}", weight_bins.size());

    std::string xml_model(xml_strings.begin(), xml_strings.end());
    IE::TensorDesc binDesc(IE::Precision::U8, {weight_bins.size()},
                           IE::Layout::C);

    std::shared_ptr<IE::Data> data_node(new IE::Data("see", binDesc));
    IE::Blob::Ptr weights_blob = IE::Blob::CreateFromData(data_node);
    weights_blob->allocate();
    uint8_t *buffer_ptr = static_cast<uint8_t *>(weights_blob->buffer());
    for (size_t i = 0; i < weight_bins.size(); i++) {
      buffer_ptr[i] = weight_bins[i];
    }
    IE::CNNNetwork network = ie_.ReadNetwork(xml_model, weights_blob);
    IE::ExecutableNetwork executable_network =
        ie_.LoadNetwork(network, device_name);

    this->Ctx.ModelsNum++;
    this->Ctx.OpenVINONetworks.emplace(this->Ctx.ModelsNum, std::move(network));
    this->Ctx.OpenVINOExecutions.emplace(this->Ctx.ModelsNum,
                                         std::move(executable_network));
    this->Ctx.GraphBackends.emplace(this->Ctx.ModelsNum, Encoding);
    log->info("Network created");
    *Graph = this->Ctx.ModelsNum;
    // std::vector<std::string> availableDevices = ie_.GetAvailableDevices();
    // for (auto x : availableDevices) {
    //   log->info("device: {:s}", x);
    // }
    return 0;
#else
    log->error("OpenVINO backend is not built. define -DWASINN_BUILD_OPENVINO "
               "to build it.");
#endif
  } else {
    log->error("Current backend is not supported.");
  }
  return -1;
}

Expect<uint32_t> WasiNNInitExecCtx::body(
    Runtime::Instance::MemoryInstance *MemInst [[maybe_unused]],
    uint32_t Graph [[maybe_unused]], uint32_t ContextPtr [[maybe_unused]]) {
  auto log = spdlog::get("WasiNN");

  if (this->Ctx.GraphBackends.find(Graph) == this->Ctx.GraphBackends.end()) {
    log->error("init_execution_context: Graph does not exist");
    return -1;
  }
  [[maybe_unused]] uint32_t *Context =
      MemInst->getPointer<uint32_t *>(ContextPtr, 1);

  if (this->Ctx.GraphBackends[Graph] ==
      this->Ctx.BackendsMapping.at("OpenVINO")) {
#ifdef WASINN_BUILD_OPENVINO
    OpenVINOSession Session;
    // std::unique_ptr<IE::InferRequest> unique_ir(&infer_request);
    Session.infer_request =
        this->Ctx.OpenVINOExecutions[Graph].CreateInferRequest();
    Session.network = &(this->Ctx.OpenVINONetworks[Graph]);
    Session.executable_network = &(this->Ctx.OpenVINOExecutions[Graph]);

    this->Ctx.ExecutionsNum++;
    this->Ctx.OpenVINOInfers.emplace(this->Ctx.ExecutionsNum,
                                     std::move(Session));
    this->Ctx.GraphContextBackends.emplace(this->Ctx.ExecutionsNum,
                                           this->Ctx.GraphBackends[Graph]);
    *Context = this->Ctx.ExecutionsNum;
    log->info("Context created");
    return 0;
#else
    log->error("OpenVINO backend is not built. define -DWASINN_BUILD_OPENVINO "
               "to build it.");
#endif
  } else {
    log->error("Current backend is not supported.");
  }
  return -1;
}

Expect<uint32_t> WasiNNSetInput::body(Runtime::Instance::MemoryInstance *MemInst
                                      [[maybe_unused]],
                                      uint32_t Context [[maybe_unused]],
                                      uint32_t Index [[maybe_unused]],
                                      uint32_t TensorPtr [[maybe_unused]]) {
  auto log = spdlog::get("WasiNN");

  if (this->Ctx.GraphContextBackends.find(Context) ==
      this->Ctx.GraphContextBackends.end()) {
    log->error("set_input: Execution Context does not exist");
    return -1;
  }
  if (this->Ctx.GraphContextBackends[Context] ==
      this->Ctx.BackendsMapping.at("OpenVINO")) {
#ifdef WASINN_BUILD_OPENVINO
    // reading tensor
    OpenVINOSession &Session = this->Ctx.OpenVINOInfers[Context];
    uint32_t *Tensor = MemInst->getPointer<uint32_t *>(TensorPtr, 1);
    uint32_t *DimensionBuf = MemInst->getPointer<uint32_t *>(Tensor[0]);
    uint32_t DimensionLen = Tensor[1];
    uint32_t RType = Tensor[2];
    uint8_t *TensorDataBuf = MemInst->getPointer<uint8_t *>(Tensor[3]);
    uint32_t TensorDataLen = Tensor[4];

    if (RType != 1) {
      log->error(
          "set_input: only F32 inputs and outputs are supported for now");
      return -1;
    }
    std::vector<size_t> InputDims(DimensionLen);
    for (uint32_t i = 0; i < InputDims.size(); i++)
      InputDims[i] = static_cast<size_t>(DimensionBuf[i]);
    size_t total = 1;
    for (auto x : InputDims)
      total *= x;
    log->info("Tensor size: {:d}, {:d}", total, TensorDataLen / 4);
    TensorType rtype = static_cast<TensorType>(RType);

    IE::Precision tensor_precision;
    switch (rtype) {
    // case TensorType::TENSOR_TYPE_U8:
    //   tensor_precision = IE::Precision::U8;
    //   log->info("Load tensor type U8");
    //   break;
    // case TensorType::TENSOR_TYPE_I32:
    //   tensor_precision = IE::Precision::I32;
    //   log->info("Load tensor type I32");
    //   break;

    /*Only FP32 supported*/
    case TensorType::TENSOR_TYPE_F32:
      tensor_precision = IE::Precision::FP32;
      log->info("Load tensor type F32");
      break;
    // case TensorType::TENSOR_TYPE_F16:
    //   tensor_precision = IE::Precision::FP16;
    //   log->info("Load tensor type F16");
    //   break;
    default:
      break;
    }
    IE::TensorDesc tDesc(tensor_precision, InputDims, IE::Layout::NHWC);
    IE::CNNNetwork *network = Session.network;

    IE::InputsDataMap input_info = network->getInputsInfo();
    std::string input_name;

    uint32_t input_counts = 0;
    for (auto &item : input_info) {
      if (input_counts == Index) {
        input_name = item.first;
        log->info("Index input: {:s}", input_name);
        item.second->setPrecision(tensor_precision);
        break;
      }
      input_counts++;
    }
    if (input_counts >= input_info.size()) {
      log->error("Index of inputs is out of range");
      return -1;
    }

    IE::Blob::Ptr input_blob = Session.infer_request.GetBlob(input_name);
    log->info("Context created");
    IE::SizeVector blobSize = input_blob->getTensorDesc().getDims();
    log->info("Context created");
    const size_t width = blobSize[3];
    const size_t height = blobSize[2];
    const size_t channels = blobSize[1];
    log->info("Input shape for {:s}: {:d}, {:d}, {:d}", input_name, channels,
              width, height);
    log->info("Context created");
    IE::MemoryBlob::Ptr mblob = IE::as<IE::MemoryBlob>(input_blob);
    if (!mblob) {
      log->error("We expect image blob to be inherited from MemoryBlob in "
                 "OpenVINO backend, but "
                 "by fact we were not able "
                 "to cast imageInput to MemoryBlob");
      return -1;
    }
    // locked memory holder should be alive all time while access to its buffer
    // happens
    auto mblobHolder = mblob->wmap();
    float *blob_data = mblobHolder.as<float *>();
    for (size_t i = 0; i < TensorDataLen / 4; i++) {
      uint32_t Tmp =
          (TensorDataBuf[4 * i] << 0 | TensorDataBuf[4 * i + 1] << 8 |
           TensorDataBuf[4 * i + 2] << 16 | TensorDataBuf[4 * i + 3] << 24);
      blob_data[i] = *reinterpret_cast<float *>(&Tmp);
    }
    return 0;
#else
    log->error("OpenVINO backend is not built. define -DWASINN_BUILD_OPENVINO "
               "to build it.");
#endif
  } else {
    log->error("Current backend is not supported.");
  }
  return -1;
}

Expect<uint32_t> WasiNNGetOuput::body(
    Runtime::Instance::MemoryInstance *MemInst [[maybe_unused]],
    uint32_t Context [[maybe_unused]], uint32_t Index [[maybe_unused]],
    uint32_t OutBuffer [[maybe_unused]],
    uint32_t OutBufferMaxSize [[maybe_unused]],
    uint32_t BytesWrittenPtr [[maybe_unused]]) {
  auto log = spdlog::get("WasiNN");
  [[maybe_unused]] uint8_t *OutBufferPtr;
  [[maybe_unused]] uint32_t *BytesWritten;

  if (this->Ctx.GraphContextBackends.find(Context) ==
      this->Ctx.GraphContextBackends.end()) {
    log->error("get_output: Execution Context does not exist");
    return -1;
  }

  if (this->Ctx.GraphContextBackends[Context] ==
      this->Ctx.BackendsMapping.at("OpenVINO")) {
#ifdef WASINN_BUILD_OPENVINO
    OutBufferPtr = MemInst->getPointer<uint8_t *>(OutBuffer, 1);
    BytesWritten = MemInst->getPointer<uint32_t *>(BytesWrittenPtr, 1);
    OpenVINOSession &Session = this->Ctx.OpenVINOInfers[Context];
    IE::CNNNetwork *network = Session.network;

    IE::OutputsDataMap output_info = network->getOutputsInfo();
    std::string output_name;
    uint32_t output_counts = 0;
    for (auto &item : output_info) {
      if (output_counts == Index) {
        output_name = item.first;
        item.second->setPrecision(IE::Precision::FP32);
        break;
      }
      output_counts++;
    }
    if (output_counts >= output_info.size()) {
      log->error("Index of outputs is out of range");
      return -1;
    }

    const IE::Blob::Ptr output_blob =
        Session.infer_request.GetBlob(output_name);
    IE::MemoryBlob::Ptr mblob = IE::as<IE::MemoryBlob>(output_blob);
    if (!mblob) {
      log->error("We expect output blob to be inherited from MemoryBlob in "
                 "OpenVINO backend, but "
                 "by fact we were not able "
                 "to cast network output to MemoryBlob");
      return -1;
    }
    auto mblobHolder = mblob->wmap();
    float *blob_data = mblobHolder.as<float *>();
    auto output_size = output_blob->size();
    log->info("get ouput with total size {:d}", output_size);
    for (uint32_t i = 0; i < output_size; i++) {
      float Value = blob_data[i];
      OutBufferPtr[4 * i] = *reinterpret_cast<uint32_t *>(&Value) & 0xff;
      OutBufferPtr[4 * i + 1] =
          (*reinterpret_cast<uint32_t *>(&Value) & 0xff00) >> 8;
      OutBufferPtr[4 * i + 2] =
          (*reinterpret_cast<uint32_t *>(&Value) & 0xff0000) >> 16;
      OutBufferPtr[4 * i + 3] =
          (*reinterpret_cast<uint32_t *>(&Value) & 0xff000000) >> 24;
    }
    *BytesWritten = output_size * 4;
    return 0;
#else
    log->error("OpenVINO backend is not built. define -DWASINN_BUILD_OPENVINO "
               "to build it.");
#endif
  } else {
    log->error("Current backend is not supported.");
  }
  return -1;
}

Expect<uint32_t> WasiNNCompute::body(Runtime::Instance::MemoryInstance *MemInst
                                     [[maybe_unused]],
                                     uint32_t Context [[maybe_unused]]) {
  auto log = spdlog::get("WasiNN");

  if (this->Ctx.GraphContextBackends.find(Context) ==
      this->Ctx.GraphContextBackends.end()) {
    log->error("compute: Execution Context does not exist");
    return -1;
  }

  if (this->Ctx.GraphContextBackends[Context] ==
      this->Ctx.BackendsMapping.at("OpenVINO")) {
#ifdef WASINN_BUILD_OPENVINO
    OpenVINOSession &Session = this->Ctx.OpenVINOInfers[Context];
    Session.infer_request.Infer();
    return 0;
#else
    log->error("OpenVINO backend is not built. define -DWASINN_BUILD_OPENVINO "
               "to build it.");
#endif
  } else {
    log->error("Current backend is not supported.");
  }

  return -1;
}

} // namespace Host
} // namespace WasmEdge
