// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "runtime/callingframe.h"
#include "wasinnbase.h"

#include <cstdint>

namespace WasmEdge {
namespace Host {

class WasiNNLoad : public WasiNN<WasiNNLoad> {
public:
  WasiNNLoad(WASINN::WasiNNEnvironment &HostEnv) : WasiNN(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t BuilderPtr,
                        uint32_t BuilderLen, uint32_t Encoding, uint32_t Target,
                        uint32_t GraphIdPtr) {
    return bodyImpl(Frame, BuilderPtr, BuilderLen, Encoding, Target, GraphIdPtr)
        .map(castErrNo);
  }

private:
  Expect<WASINN::ErrNo> bodyImpl(const Runtime::CallingFrame &,
                                 uint32_t BuilderPtr, uint32_t BuilderLen,
                                 uint32_t Encoding, uint32_t Target,
                                 uint32_t GraphIdPtr);
};

class WasiNNLoadByName : public WasiNN<WasiNNLoadByName> {
public:
  WasiNNLoadByName(WASINN::WasiNNEnvironment &HostEnv) : WasiNN(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t NamePtr,
                        uint32_t NameLen, uint32_t GraphIdPtr) {
    return bodyImpl(Frame, NamePtr, NameLen, GraphIdPtr).map(castErrNo);
  }

private:
  Expect<WASINN::ErrNo> bodyImpl(const Runtime::CallingFrame &,
                                 uint32_t NamePtr, uint32_t NameLen,
                                 uint32_t GraphIdPtr);
};

class WasiNNLoadByNameWithConfig : public WasiNN<WasiNNLoadByNameWithConfig> {
public:
  WasiNNLoadByNameWithConfig(WASINN::WasiNNEnvironment &HostEnv)
      : WasiNN(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t NamePtr,
                        uint32_t NameLen, uint32_t ConfigPtr,
                        uint32_t ConfigLen, uint32_t GraphIdPtr) {
    return bodyImpl(Frame, NamePtr, NameLen, ConfigPtr, ConfigLen, GraphIdPtr)
        .map(castErrNo);
  }

private:
  Expect<WASINN::ErrNo> bodyImpl(const Runtime::CallingFrame &,
                                 uint32_t NamePtr, uint32_t NameLen,
                                 uint32_t ConfigPtr, uint32_t ConfigLen,
                                 uint32_t GraphIdPtr);
};

class WasiNNInitExecCtx : public WasiNN<WasiNNInitExecCtx> {
public:
  WasiNNInitExecCtx(WASINN::WasiNNEnvironment &HostEnv) : WasiNN(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t GraphId,
                        uint32_t ContextPtr) {
    return bodyImpl(Frame, GraphId, ContextPtr).map(castErrNo);
  }

private:
  Expect<WASINN::ErrNo> bodyImpl(const Runtime::CallingFrame &Frame,
                                 uint32_t GraphId, uint32_t ContextPtr);
};

class WasiNNSetInput : public WasiNN<WasiNNSetInput> {
public:
  WasiNNSetInput(WASINN::WasiNNEnvironment &HostEnv) : WasiNN(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t Context,
                        uint32_t Index, uint32_t TensorPtr) {
    return bodyImpl(Frame, Context, Index, TensorPtr).map(castErrNo);
  }

private:
  Expect<WASINN::ErrNo> bodyImpl(const Runtime::CallingFrame &Frame,
                                 uint32_t Context, uint32_t Index,
                                 uint32_t TensorPtr);
};

class WasiNNGetOutput : public WasiNN<WasiNNGetOutput> {
public:
  WasiNNGetOutput(WASINN::WasiNNEnvironment &HostEnv) : WasiNN(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t Context,
                        uint32_t Index, uint32_t OutBufferPtr,
                        uint32_t OutBufferMaxSize, uint32_t BytesWrittenPtr) {
    return bodyImpl(Frame, Context, Index, OutBufferPtr, OutBufferMaxSize,
                    BytesWrittenPtr)
        .map(castErrNo);
  }

private:
  Expect<WASINN::ErrNo> bodyImpl(const Runtime::CallingFrame &Frame,
                                 uint32_t Context, uint32_t Index,
                                 uint32_t OutBufferPtr,
                                 uint32_t OutBufferMaxSize,
                                 uint32_t BytesWrittenPtr);
};

class WasiNNGetOutputSingle : public WasiNN<WasiNNGetOutputSingle> {
public:
  WasiNNGetOutputSingle(WASINN::WasiNNEnvironment &HostEnv) : WasiNN(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t Context,
                        uint32_t Index, uint32_t OutBufferPtr,
                        uint32_t OutBufferMaxSize, uint32_t BytesWrittenPtr) {
    return bodyImpl(Frame, Context, Index, OutBufferPtr, OutBufferMaxSize,
                    BytesWrittenPtr)
        .map(castErrNo);
  }

private:
  Expect<WASINN::ErrNo> bodyImpl(const Runtime::CallingFrame &Frame,
                                 uint32_t Context, uint32_t Index,
                                 uint32_t OutBufferPtr,
                                 uint32_t OutBufferMaxSize,
                                 uint32_t BytesWrittenPtr);
};

class WasiNNCompute : public WasiNN<WasiNNCompute> {
public:
  WasiNNCompute(WASINN::WasiNNEnvironment &HostEnv) : WasiNN(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t Context) {
    return bodyImpl(Frame, Context).map(castErrNo);
  }

private:
  Expect<WASINN::ErrNo> bodyImpl(const Runtime::CallingFrame &Frame,
                                 uint32_t Context);
};

class WasiNNComputeSingle : public WasiNN<WasiNNComputeSingle> {
public:
  WasiNNComputeSingle(WASINN::WasiNNEnvironment &HostEnv) : WasiNN(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t Context) {
    return bodyImpl(Frame, Context).map(castErrNo);
  }

private:
  Expect<WASINN::ErrNo> bodyImpl(const Runtime::CallingFrame &Frame,
                                 uint32_t Context);
};

class WasiNNFiniSingle : public WasiNN<WasiNNFiniSingle> {
public:
  WasiNNFiniSingle(WASINN::WasiNNEnvironment &HostEnv) : WasiNN(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t Context) {
    return bodyImpl(Frame, Context).map(castErrNo);
  }

private:
  Expect<WASINN::ErrNo> bodyImpl(const Runtime::CallingFrame &Frame,
                                 uint32_t Context);
};

class WasiNNUnload : public WasiNN<WasiNNUnload> {
public:
  WasiNNUnload(WASINN::WasiNNEnvironment &HostEnv) : WasiNN(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t GraphId) {
    return bodyImpl(Frame, GraphId).map(castErrNo);
  }

private:
  Expect<WASINN::ErrNo> bodyImpl(const Runtime::CallingFrame &Frame,
                                 uint32_t GraphId);
};

} // namespace Host
} // namespace WasmEdge
