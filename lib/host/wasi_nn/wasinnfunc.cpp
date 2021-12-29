#include "host/wasi_nn/wasinnfunc.h"
#include "common/errcode.h"
#include "runtime/hostfunc.h"
#include "runtime/instance/memory.h"
#include "spdlog/fmt/ostr.h"
#include "spdlog/spdlog.h"
#include <numeric>
#include <onnxruntime_cxx_api.h>
#include <unistd.h>

namespace WasmEdge {
namespace Host {

Expect<uint32_t> WasiNNLoad::body(Runtime::Instance::MemoryInstance *MemInst,
                                  uint32_t BuilderPtr, uint32_t BuilderLen,
                                  uint32_t Encoding, uint32_t Target,
                                  uint32_t GraphPtr) {
  (void)Target;

  auto log = spdlog::get("WasiNN");
  uint8_t *Model;
  uint32_t *Graph;

  if (Encoding != 1) {
    log->error("load current implementation can only load ONNX models");
    return -1;
  }

  log->info("BuilderPtr: {:<d} BuilderLen: {:<d}", BuilderPtr, BuilderLen);

  Model = MemInst->getPointer<uint8_t *>(BuilderPtr, 1);
  Graph = MemInst->getPointer<uint32_t *>(GraphPtr, 1);

  log->info("model[len - 1]: {:<d}", Model[BuilderLen - 1]);
  log->info("model[0]: {:<d}", Model[1]);

  std::vector<uint8_t> OnnxModel(BuilderLen);
  for (uint32_t i = 0; i < BuilderLen; i++)
    OnnxModel[i] = Model[i];

  this->Ctx.ModelsNum++;
  this->Ctx.Models.emplace(this->Ctx.ModelsNum, std::move(OnnxModel));
  *Graph = this->Ctx.ModelsNum;

  return 0;
}

Expect<uint32_t>
WasiNNInitExecCtx::body(Runtime::Instance::MemoryInstance *MemInst,
                        uint32_t Graph, uint32_t ContextPtr) {
  auto log = spdlog::get("WasiNN");

  if (this->Ctx.Models.find(Graph) == this->Ctx.Models.end()) {
    log->error("init_execution_context: Graph does not exist");
    return -1;
  }

  uint32_t *Context = MemInst->getPointer<uint32_t *>(ContextPtr, 1);
  std::vector<uint8_t> &Model = this->Ctx.Models[Graph];

  OnnxSession Session;
  Session.SessionOpt = std::make_unique<Ort::SessionOptions>();
  Session.SessionOpt->AddConfigEntry("session.load_model_format", "onnx");
  Session.SessionOpt->AddConfigEntry("session.use_ort_model_bytes_directly",
                                     "1");
  Session.Env =
      std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_INFO, "wasi-nn-onnxruntime");
  Session.OrtSession = std::make_unique<Ort::Session>(
      *Session.Env, Model.data(), Model.size(), *Session.SessionOpt);

  Session.Allocator = std::make_unique<Ort::AllocatorWithDefaultOptions>();

  this->Ctx.ExecutionsNum++;
  this->Ctx.Executions.emplace(this->Ctx.ExecutionsNum, std::move(Session));
  *Context = this->Ctx.ExecutionsNum;

  return 0;
}

Expect<uint32_t>
WasiNNSetInput::body(Runtime::Instance::MemoryInstance *MemInst,
                     uint32_t Context, uint32_t Index, uint32_t TensorPtr) {
  /// Index is not used in this implementation of the proposal.
  /// Users are required to call `set_input` sequentially if they have more than
  /// one tensor to inference at once.
  /// Also, they are required to record the index of the input tensor in order
  /// to get the corresponding output tensor
  (void)Index;

  auto log = spdlog::get("WasiNN");

  if (this->Ctx.Executions.find(Context) == this->Ctx.Executions.end()) {
    log->error("set_input: Execution Context does not exist");
    return -1;
  }

  OnnxSession &Session = this->Ctx.Executions[Context];

  uint32_t *Tensor = MemInst->getPointer<uint32_t *>(TensorPtr, 1);
  uint32_t *DimensionBuf = MemInst->getPointer<uint32_t *>(Tensor[0]);
  uint32_t DimensionLen = Tensor[1];
  uint32_t RType = Tensor[2];
  uint8_t *TensorDataBuf = MemInst->getPointer<uint8_t *>(Tensor[3]);
  uint32_t TensorDataLen = Tensor[4];

  if (RType != 1) {
    log->error("set_input: only F32 inputs and outputs are supported for now");
    return -1;
  }

  /// Create a vector for input dimension
  /// TODO: Check if the model input shape match the shape gave from users
  std::vector<int64_t> InputDims(DimensionLen);
  for (uint32_t i = 0; i < InputDims.size(); i++)
    InputDims[i] = static_cast<int64_t>(DimensionBuf[i]);
  Session.InputTensorsDims.push_back(std::move(InputDims));
  std::for_each(Session.InputTensorsDims.back().begin(),
                Session.InputTensorsDims.back().end(),
                [&, i = 0](int64_t len) mutable {
                  log->info("InputDims[{:d}]: {:d}", i++, len);
                });
  log->info("IntputTensorSize: {:d} bytes", TensorDataLen);

  /// Convert bytes into float array and place it back in the `InputTensors`
  std::vector<float> InputTensor(TensorDataLen / 4);
  for (uint32_t i = 0; i < InputTensor.size(); i++) {
    uint32_t Tmp =
        (TensorDataBuf[4 * i] << 0 | TensorDataBuf[4 * i + 1] << 8 |
         TensorDataBuf[4 * i + 2] << 16 | TensorDataBuf[4 * i + 3] << 24);
    InputTensor[i] = *reinterpret_cast<float *>(&Tmp);
  }
  Session.InputTensorsValue.push_back(std::move(InputTensor));
  Session.InputTensors.push_back(Ort::Value::CreateTensor(
      *this->Ctx.MemoryInfo, Session.InputTensorsValue.back().data(),
      Session.InputTensorsValue.back().size(),
      Session.InputTensorsDims.back().data(),
      Session.InputTensorsDims.back().size()));

  /// Get output dimension
  Ort::TypeInfo OutputTypeInfo = Session.OrtSession->GetOutputTypeInfo(0);
  std::vector<int64_t> OutputDims =
      OutputTypeInfo.GetTensorTypeAndShapeInfo().GetShape();
  Session.OutputTensorsDims.push_back(std::move(OutputDims));
  std::for_each(Session.OutputTensorsDims.back().begin(),
                Session.OutputTensorsDims.back().end(),
                [&, i = 0](int64_t len) mutable {
                  log->info("OutputDims[{:d}]: {:d}", i++, len);
                });

  /// Calulate total size of output tensor
  /// Account for the case where first dimension is batch size
  /// in which its value is -1
  std::size_t OutputTensorSize;
  if (Session.OutputTensorsDims.back().at(0) == -1)
    Session.OutputTensorsDims.back().at(0) =
        Session.InputTensorsDims.back().at(0);
  OutputTensorSize = std::accumulate(Session.OutputTensorsDims.back().begin(),
                                     Session.OutputTensorsDims.back().end(), 1,
                                     std::multiplies<int64_t>());
  log->info("OutputTensorSize: {:d}", OutputTensorSize);

  /// Allocate space for output tensor
  std::vector<float> OutputTensor(OutputTensorSize);
  Session.OutputTensorsValue.push_back(std::move(OutputTensor));
  Session.OutputTensors.push_back(Ort::Value::CreateTensor(
      *this->Ctx.MemoryInfo, Session.OutputTensorsValue.back().data(),
      Session.OutputTensorsValue.back().size(),
      Session.OutputTensorsDims.back().data(),
      Session.OutputTensorsDims.back().size()));

  return 0;
}

Expect<uint32_t>
WasiNNGetOuput::body(Runtime::Instance::MemoryInstance *MemInst,
                     uint32_t Context, uint32_t Index, uint32_t OutBuffer,
                     uint32_t OutBufferMaxSize, uint32_t BytesWrittenPtr) {
  (void)OutBufferMaxSize;
  auto log = spdlog::get("WasiNN");

  uint8_t *OutBufferPtr;
  uint32_t *BytesWritten;

  if (this->Ctx.Executions.find(Context) == this->Ctx.Executions.end()) {
    log->error("get_output: Execution Context does not exist");
    return -1;
  }

  log->info("get_output: getting output");

  OutBufferPtr = MemInst->getPointer<uint8_t *>(OutBuffer, 1);
  BytesWritten = MemInst->getPointer<uint32_t *>(BytesWrittenPtr, 1);

  OnnxSession &Session = this->Ctx.Executions[Context];
  log->info("get_output: OutputTensorsValue.size(): {:d}",
            Session.OutputTensorsValue[Index].size());

  for (uint32_t i = 0; i < Session.OutputTensorsValue[Index].size(); i++) {
    float Value = Session.OutputTensorsValue[Index][i];
    OutBufferPtr[4 * i] = *reinterpret_cast<uint32_t *>(&Value) & 0xff;
    OutBufferPtr[4 * i + 1] = *reinterpret_cast<uint32_t *>(&Value) & 0xff00;
    OutBufferPtr[4 * i + 2] = *reinterpret_cast<uint32_t *>(&Value) & 0xff0000;
    OutBufferPtr[4 * i + 3] =
        *reinterpret_cast<uint32_t *>(&Value) & 0xff000000;
  }

  *BytesWritten = Session.OutputTensorsValue[Index].size() * 4;

  return 0;
}

Expect<uint32_t> WasiNNCompute::body(Runtime::Instance::MemoryInstance *,
                                     uint32_t Context) {
  auto log = spdlog::get("WasiNN");

  if (this->Ctx.Executions.find(Context) == this->Ctx.Executions.end()) {
    log->error("compute: Execution Context does not exist");
    return -1;
  }

  log->info("compute: Start inferencing");

  OnnxSession &Session = this->Ctx.Executions[Context];

  const char *InputName =
      Session.OrtSession->GetInputName(0, *Session.Allocator);
  const char *OutputName =
      Session.OrtSession->GetOutputName(0, *Session.Allocator);

  std::vector<const char *> InputNames{InputName};
  std::vector<const char *> OutputNames{OutputName};
  Session.OrtSession->Run(
      nullptr, InputNames.data(), Session.InputTensors.data(),
      Session.InputTensors.size(), OutputNames.data(),
      Session.OutputTensors.data(), Session.OutputTensors.size());

  log->info("compute: End inferencing");

  return 0;
}

} // namespace Host
} // namespace WasmEdge
