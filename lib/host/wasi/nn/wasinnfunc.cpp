#include "host/wasi/nn/wasinnfunc.h"
#include "common/errcode.h"
#include "runtime/hostfunc.h"
#include "runtime/instance/memory.h"

namespace WasmEdge {
namespace Host {

Expect<uint32_t> WasiNNLoad::body(Runtime::Instance::MemoryInstance *,
                                  uint32_t BuilderPtr, uint32_t BuilderLen,
                                  uint32_t Encoding, uint32_t Target,
                                  uint32_t GraphPtr) {
  (void)BuilderPtr;
  (void)BuilderLen;
  (void)Encoding;
  (void)Target;
  (void)GraphPtr;
  std::cout << "WasiNNLoad::body" << std::endl;
  return 0;
}

Expect<uint32_t> WasiNNInitExecCtx::body(Runtime::Instance::MemoryInstance *,
                                         uint32_t Graph, uint32_t ContextPtr) {
  (void)Graph;
  (void)ContextPtr;
  std::cout << "WasiNNInitExecCtx::body" << std::endl;
  return 0;
}

Expect<uint32_t> WasiNNSetInput::body(Runtime::Instance::MemoryInstance *,
                                      uint32_t Context, uint32_t Index,
                                      uint32_t TensorPtr) {
  (void)Context;
  (void)Index;
  (void)TensorPtr;
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
