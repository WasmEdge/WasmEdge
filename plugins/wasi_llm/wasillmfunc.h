// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "runtime/callingframe.h"
#include "types.h"
#include "wasillmbase.h"
#include "wasillmenv.h"

#include <cstdint>

namespace WasmEdge {
namespace Host {

class WasiLLMModelCreate : public WasiLLM<WasiLLMModelCreate> {
public:
  explicit WasiLLMModelCreate(WASILLM::WASILLMEnv &Env) : WasiLLM(Env) {}

  Expect<uint32_t> body(const Runtime::CallingFrame &Frame,
                        uint32_t CheckPointPath, uint32_t CheckPointPathLen,
                        uint32_t ModelIdPtr) {
    return bodyImpl(Frame, CheckPointPath, CheckPointPathLen, ModelIdPtr)
        .map(castErrNo);
  }

private:
  Expect<WASILLM::ErrNo> bodyImpl(const Runtime::CallingFrame &Frame,
                                  uint32_t CheckPointPath,
                                  uint32_t CheckPointPathLen,
                                  uint32_t ModelIdPtr);
};

class WasiLLMDataLoaderCreate : public WasiLLM<WasiLLMDataLoaderCreate> {
public:
  explicit WasiLLMDataLoaderCreate(WASILLM::WASILLMEnv &Env) : WasiLLM(Env) {}

  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t DataPath,
                        uint32_t DataPathLen, uint32_t B, uint32_t T,
                        uint32_t ProcessRank, uint32_t NumProcesses,
                        int32_t ShouldShuffle, uint32_t DataLoaderIdPtr) {
    return bodyImpl(Frame, DataPath, DataPathLen, B, T, ProcessRank,
                    NumProcesses, ShouldShuffle, DataLoaderIdPtr)
        .map(castErrNo);
  }

private:
  Expect<WASILLM::ErrNo> bodyImpl(const Runtime::CallingFrame &Frame,
                                  uint32_t DataPath, uint32_t DataPathLen,
                                  uint32_t B, uint32_t T, uint32_t ProcessRank,
                                  uint32_t NumProcesses, int32_t ShouldShuffle,
                                  uint32_t DataLoaderIdPtr);
};

class WasiLLMTokenizerCreate : public WasiLLM<WasiLLMTokenizerCreate> {
public:
  explicit WasiLLMTokenizerCreate(WASILLM::WASILLMEnv &Env) : WasiLLM(Env) {}

  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t FilePath,
                        uint32_t FilePathLen, uint32_t TokenizerIdPtr) {
    return bodyImpl(Frame, FilePath, FilePathLen, TokenizerIdPtr)
        .map(castErrNo);
  }

private:
  Expect<WASILLM::ErrNo> bodyImpl(const Runtime::CallingFrame &Frame,
                                  uint32_t FilePath, uint32_t FilePathLen,
                                  uint32_t TokenizerIdPtr);
};

class WasiLLMModelTrain : public WasiLLM<WasiLLMModelTrain> {
public:
  explicit WasiLLMModelTrain(WASILLM::WASILLMEnv &Env) : WasiLLM(Env) {}

  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t ModelIdPtr,
                        uint32_t TrainDataLoaderIdPtr,
                        uint32_t ValDataLoaderIdPtr, uint32_t TokenizerIdPtr,
                        float Lr, float Epoch) {
    return bodyImpl(Frame, ModelIdPtr, TrainDataLoaderIdPtr, ValDataLoaderIdPtr,
                    TokenizerIdPtr, Lr, Epoch)
        .map(castErrNo);
  }

private:
  Expect<WASILLM::ErrNo>
  bodyImpl(const Runtime::CallingFrame &Frame, uint32_t ModelIdPtr,
           uint32_t TrainDataLoaderIdPtr, uint32_t ValDataLoaderIdPtr,
           uint32_t TokenizerIdPtr, float Lr, float Epoch);
};

} // namespace Host
} // namespace WasmEdge
