// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "llm_base.h"
#include "llm_env.h"

#include "runtime/callingframe.h"

#include <cstdint>

namespace WasmEdge {
namespace Host {
namespace WasmEdgeLLM {

class ModelCreate : public HostFunction<ModelCreate> {
public:
  explicit ModelCreate(LLMEnv &Env) : HostFunction(Env) {}

  Expect<uint32_t> body(const Runtime::CallingFrame &Frame,
                        uint32_t CheckPointPath, uint32_t CheckPointPathLen,
                        uint32_t ModelIdPtr) {
    return bodyImpl(Frame, CheckPointPath, CheckPointPathLen, ModelIdPtr)
        .map(castErrNo);
  }

private:
  Expect<ErrNo> bodyImpl(const Runtime::CallingFrame &Frame,
                         uint32_t CheckPointPath, uint32_t CheckPointPathLen,
                         uint32_t ModelIdPtr);
};

class DataLoaderCreate : public HostFunction<DataLoaderCreate> {
public:
  explicit DataLoaderCreate(LLMEnv &Env) : HostFunction(Env) {}

  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t DataPath,
                        uint32_t DataPathLen, uint32_t B, uint32_t T,
                        uint32_t ProcessRank, uint32_t NumProcesses,
                        int32_t ShouldShuffle, uint32_t DataLoaderIdPtr) {
    return bodyImpl(Frame, DataPath, DataPathLen, B, T, ProcessRank,
                    NumProcesses, ShouldShuffle, DataLoaderIdPtr)
        .map(castErrNo);
  }

private:
  Expect<ErrNo> bodyImpl(const Runtime::CallingFrame &Frame, uint32_t DataPath,
                         uint32_t DataPathLen, uint32_t B, uint32_t T,
                         uint32_t ProcessRank, uint32_t NumProcesses,
                         int32_t ShouldShuffle, uint32_t DataLoaderIdPtr);
};

class TokenizerCreate : public HostFunction<TokenizerCreate> {
public:
  explicit TokenizerCreate(LLMEnv &Env) : HostFunction(Env) {}

  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t FilePath,
                        uint32_t FilePathLen, uint32_t TokenizerIdPtr) {
    return bodyImpl(Frame, FilePath, FilePathLen, TokenizerIdPtr)
        .map(castErrNo);
  }

private:
  Expect<ErrNo> bodyImpl(const Runtime::CallingFrame &Frame, uint32_t FilePath,
                         uint32_t FilePathLen, uint32_t TokenizerIdPtr);
};

class ModelTrain : public HostFunction<ModelTrain> {
public:
  explicit ModelTrain(LLMEnv &Env) : HostFunction(Env) {}

  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t ModelId,
                        uint32_t TrainDataLoaderId, uint32_t ValDataLoaderId,
                        uint32_t TokenizerId, uint32_t B, uint32_t T, float Lr,
                        uint32_t Epoch) {
    return bodyImpl(Frame, ModelId, TrainDataLoaderId, ValDataLoaderId,
                    TokenizerId, B, T, Lr, Epoch)
        .map(castErrNo);
  }

private:
  Expect<ErrNo> bodyImpl(const Runtime::CallingFrame &Frame, uint32_t ModelId,
                         uint32_t TrainDataLoaderId, uint32_t ValDataLoaderId,
                         uint32_t TokenizerId, uint32_t B, uint32_t T, float Lr,
                         uint32_t Epoch);
};

} // namespace WasmEdgeLLM
} // namespace Host
} // namespace WasmEdge
