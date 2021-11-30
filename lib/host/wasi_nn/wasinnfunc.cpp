#include "host/wasi_nn/wasinnfunc.h"
#include "common/errcode.h"
#include "runtime/hostfunc.h"
#include "runtime/instance/memory.h"
#include "spdlog/spdlog.h"
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
  uint32_t *Context;

  if (this->Ctx.Models.find(Graph) == this->Ctx.Models.end()) {
    spdlog::error("Graph does not exist");
    return -1;
  }

  Context = MemInst->getPointer<uint32_t *>(ContextPtr, 1);
  std::vector<uint8_t> &Model = this->Ctx.Models[Graph];

  OnnxSession Session;
  Session.SessionOpt = std::make_unique<Ort::SessionOptions>();
  Session.SessionOpt->AddConfigEntry("session.load_model_format", "onnx");
  Session.SessionOpt->AddConfigEntry("session.use_ort_model_bytes_directly",
                                     "1");
  Session.Env =
      std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_INFO, "wasi-nn-onnxruntime");
  Session.Session = std::make_unique<Ort::Session>(
      *Session.Env, Model.data(), Model.size(), *Session.SessionOpt);

  this->Ctx.ExecutionsNum++;
  this->Ctx.Executions.emplace(this->Ctx.ExecutionsNum, std::move(Session));

  return 0;
}

Expect<uint32_t>
WasiNNSetInput::body(Runtime::Instance::MemoryInstance *MemInst,
                     uint32_t Context, uint32_t Index, uint32_t TensorPtr) {
  (void)Context;
  (void)Index;
  (void)TensorPtr;
  (void)MemInst;

  uint32_t *Tensor;
  auto log = spdlog::get("WasiNN");

  Tensor = MemInst->getPointer<uint32_t *>(TensorPtr, 1);

  log->info("TensorPtr: {:<d}", Tensor[1]);
  std::cout << "WasiNNSetInput::body" << std::endl;
  return 0;
}

Expect<uint32_t> WasiNNGetOuput::body(Runtime::Instance::MemoryInstance *,
                                      uint32_t Context, uint32_t Index,
                                      uint32_t OutBuffer,
                                      uint32_t OutBufferMaxSize,
                                      uint32_t BytesWrittenPtr) {
  (void)Context;
  (void)Index;
  (void)OutBuffer;
  (void)OutBufferMaxSize;
  (void)BytesWrittenPtr;
  std::cout << "WasiNNGetOnput::body" << std::endl;
  return 0;
}

Expect<uint32_t> WasiNNCompute::body(Runtime::Instance::MemoryInstance *,
                                     uint32_t Context) {
  (void)Context;
  std::cout << "WasiNNCompute::body" << std::endl;
  return 0;
}

} // namespace Host
} // namespace WasmEdge
