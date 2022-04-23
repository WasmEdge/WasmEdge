// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#pragma once

#include "common/errcode.h"
#include "host/wasi_nn/wasinncontext.h"
#include "runtime/hostfunc.h"
#include "runtime/instance/memory.h"

namespace WasmEdge {
namespace Host {

template <typename T> class WasiNN : public Runtime::HostFunction<T> {
public:
  WasiNN(WASINN::WasiNNContext &HostCtx)
      : Runtime::HostFunction<T>(0), Ctx(HostCtx) {}

protected:
  WASINN::WasiNNContext &Ctx;
};

class WasiNNLoad : public WasiNN<WasiNNLoad> {
public:
  WasiNNLoad(WASINN::WasiNNContext &HostCtx) : WasiNN(HostCtx) {}
  Expect<WASINN::NNErrNo> body(Runtime::Instance::MemoryInstance *,
                               uint32_t BuilderPtr, uint32_t BuilderLen,
                               uint32_t Encoding, uint32_t Target,
                               uint32_t GraphPtr);
};

class WasiNNInitExecCtx : public WasiNN<WasiNNInitExecCtx> {
public:
  WasiNNInitExecCtx(WASINN::WasiNNContext &HostCtx) : WasiNN(HostCtx) {}
  Expect<WASINN::NNErrNo> body(Runtime::Instance::MemoryInstance *,
                               uint32_t GraphId, uint32_t ContextPtr);
};

class WasiNNSetInput : public WasiNN<WasiNNSetInput> {
public:
  WasiNNSetInput(WASINN::WasiNNContext &HostCtx) : WasiNN(HostCtx) {}
  Expect<WASINN::NNErrNo> body(Runtime::Instance::MemoryInstance *,
                               uint32_t Context, uint32_t Index,
                               uint32_t TensorPtr);
};

class WasiNNGetOuput : public WasiNN<WasiNNGetOuput> {
public:
  WasiNNGetOuput(WASINN::WasiNNContext &HostCtx) : WasiNN(HostCtx) {}
  Expect<WASINN::NNErrNo> body(Runtime::Instance::MemoryInstance *,
                               uint32_t Context, uint32_t Index,
                               uint32_t OutBuffer, uint32_t OutBufferMaxSize,
                               uint32_t BytesWrittenPtr);
};

class WasiNNCompute : public WasiNN<WasiNNCompute> {
public:
  WasiNNCompute(WASINN::WasiNNContext &HostCtx) : WasiNN(HostCtx) {}
  Expect<WASINN::NNErrNo> body(Runtime::Instance::MemoryInstance *,
                               uint32_t Context);
};

} // namespace Host
} // namespace WasmEdge
