#pragma once

#include "common/errcode.h"
#include "host/wasi/nn/wasinncontext.h"
#include "runtime/hostfunc.h"
#include "runtime/instance/memory.h"

namespace WasmEdge {
namespace Host {

template <typename T> class WasiNN : public Runtime::HostFunction<T> {
public:
  WasiNN(WasiNNContext &HostCtx) : Runtime::HostFunction<T>(0), Ctx(HostCtx) {}

protected:
  WasiNNContext &Ctx;
};

class WasiNNLoad : public WasiNN<WasiNNLoad> {
public:
  WasiNNLoad(WasiNNContext &HostCtx) : WasiNN(HostCtx) {}
  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *,
                        uint32_t BuilderPtr, uint32_t BuilderLen,
                        uint32_t Encoding, uint32_t Target, uint32_t GraphPtr);
};

class WasiNNInitExecCtx : public WasiNN<WasiNNInitExecCtx> {
public:
  WasiNNInitExecCtx(WasiNNContext &HostCtx) : WasiNN(HostCtx) {}
  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *, uint32_t Graph,
                        uint32_t ContextPtr);
};

class WasiNNSetInput : public WasiNN<WasiNNSetInput> {
public:
  WasiNNSetInput(WasiNNContext &HostCtx) : WasiNN(HostCtx) {}
  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *, uint32_t Context,
                        uint32_t Index, uint32_t TensorPtr);
};

class WasiNNGetOuput : public WasiNN<WasiNNGetOuput> {
public:
  WasiNNGetOuput(WasiNNContext &HostCtx) : WasiNN(HostCtx) {}
  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *, uint32_t Context,
                        uint32_t Index, uint32_t OutBuffer,
                        uint32_t OutBufferMaxSize, uint32_t BytesWrittenPtr);
};

class WasiNNCompute : public WasiNN<WasiNNCompute> {
public:
  WasiNNCompute(WasiNNContext &HostCtx) : WasiNN(HostCtx) {}
  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *, uint32_t Context);
};

} // namespace Host
} // namespace WasmEdge
