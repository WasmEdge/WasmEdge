// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#pragma once

#include "common/errcode.h"
#include "host/wasi_nn/wasinnenv.h"
#include "runtime/hostfunc.h"
#include "runtime/instance/memory.h"

namespace WasmEdge {
namespace Host {

template <typename T> class WasiNN : public Runtime::HostFunction<T> {
public:
  WasiNN(WASINN::WasiNNEnvironment &HostEnv)
      : Runtime::HostFunction<T>(0), Env(HostEnv) {}

protected:
  WASINN::WasiNNEnvironment &Env;
};

class WasiNNLoad : public WasiNN<WasiNNLoad> {
public:
  WasiNNLoad(WASINN::WasiNNEnvironment &HostEnv) : WasiNN(HostEnv) {}
  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *,
                        uint32_t BuilderPtr, uint32_t BuilderLen,
                        uint32_t Encoding, uint32_t Target,
                        uint32_t GraphIdPtr);
};

class WasiNNInitExecCtx : public WasiNN<WasiNNInitExecCtx> {
public:
  WasiNNInitExecCtx(WASINN::WasiNNEnvironment &HostEnv) : WasiNN(HostEnv) {}
  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *, uint32_t GraphId,
                        uint32_t ContextPtr);
};

class WasiNNSetInput : public WasiNN<WasiNNSetInput> {
public:
  WasiNNSetInput(WASINN::WasiNNEnvironment &HostEnv) : WasiNN(HostEnv) {}
  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *, uint32_t Context,
                        uint32_t Index, uint32_t TensorPtr);
};

class WasiNNGetOuput : public WasiNN<WasiNNGetOuput> {
public:
  WasiNNGetOuput(WASINN::WasiNNEnvironment &HostEnv) : WasiNN(HostEnv) {}
  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *, uint32_t Context,
                        uint32_t Index, uint32_t OutBufferPtr,
                        uint32_t OutBufferMaxSize, uint32_t BytesWrittenPtr);
};

class WasiNNCompute : public WasiNN<WasiNNCompute> {
public:
  WasiNNCompute(WASINN::WasiNNEnvironment &HostEnv) : WasiNN(HostEnv) {}
  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *, uint32_t Context);
};

} // namespace Host
} // namespace WasmEdge
