// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "runtime/callingframe.h"
#include "types.h"
#include "wasillmbase.h"

#include <cstdint>

namespace WasmEdge {
namespace Host {

class WasiLLMModelCreate : public WasiLLM<WasiLLMModelCreate> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame,
                        uint32_t CheckPointPath, uint32_t CheckPointPathLen) {
    return bodyImpl(Frame, CheckPointPath, CheckPointPathLen).map(castErrNo);
  }

private:
  Expect<WASILLM::ErrNo> bodyImpl(const Runtime::CallingFrame &Frame,
                                  uint32_t CheckPointPath,
                                  uint32_t CheckPointPathLen);
};

class WasiLLMModelFree : public WasiLLM<WasiLLMModelFree> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t ModelPtr) {
    return bodyImpl(Frame, ModelPtr).map(castErrNo);
  }

private:
  Expect<WASILLM::ErrNo> bodyImpl(const Runtime::CallingFrame &Frame,
                                  uint32_t ModelPtr);
};

class WasiLLMDataLoaderCreate : public WasiLLM<WasiLLMDataLoaderCreate> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t DataPath,
                        uint32_t DataPathLen) {
    return bodyImpl(Frame, DataPath, DataPathLen).map(castErrNo);
  }

private:
  Expect<WASILLM::ErrNo> bodyImpl(const Runtime::CallingFrame &Frame,
                                  uint32_t DataPath, uint32_t DataPathLen);
};

class WasiLLMDataLoaderFree : public WasiLLM<WasiLLMDataLoaderFree> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame,
                        uint32_t DataLoaderPtr) {
    return bodyImpl(Frame, DataLoaderPtr).map(castErrNo);
  }

private:
  Expect<WASILLM::ErrNo> bodyImpl(const Runtime::CallingFrame &Frame,
                                  uint32_t DataLoaderPtr);
};

class WasiLLMTokenizerCreate : public WasiLLM<WasiLLMTokenizerCreate> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t FilePath,
                        uint32_t FilePathLen) {
    return bodyImpl(Frame, FilePath, FilePathLen).map(castErrNo);
  }

private:
  Expect<WASILLM::ErrNo> bodyImpl(const Runtime::CallingFrame &Frame,
                                  uint32_t FilePath, uint32_t FilePathLen);
};

class WasiLLMTokenizerFree : public WasiLLM<WasiLLMTokenizerFree> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame,
                        uint32_t TokenizerPtr) {
    return bodyImpl(Frame, TokenizerPtr).map(castErrNo);
  }

private:
  Expect<WASILLM::ErrNo> bodyImpl(const Runtime::CallingFrame &Frame,
                                  uint32_t TokenizerPtr);
};

class WasiLLMModelTrain : public WasiLLM<WasiLLMModelTrain> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t ModelPtr,
                        uint32_t TrainDataLoaderPtr, uint32_t ValDataLoaderPtr,
                        uint32_t TokenizerPtr, uint32_t Lr, uint32_t Epoch) {
    return bodyImpl(Frame, ModelPtr, TrainDataLoaderPtr, ValDataLoaderPtr,
                    TokenizerPtr, Lr, Epoch)
        .map(castErrNo);
  }

private:
  Expect<WASILLM::ErrNo>
  bodyImpl(const Runtime::CallingFrame &Frame, uint32_t ModelPtr,
           uint32_t TrainDataLoaderPtr, uint32_t ValDataLoaderPtr,
           uint32_t TokenizerPtr, uint32_t Lr, uint32_t Epoch);
};

} // namespace Host
} // namespace WasmEdge
