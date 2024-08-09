// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "wasillmfunc.h"
#include "common/errcode.h"
#include "common/spdlog.h"
#include "llmc_fwd.h"
#include <string>
#include <string_view>

namespace WasmEdge {
namespace Host {

Expect<WASILLM::ErrNo>
WasiLLMModelCreate::bodyImpl(const Runtime::CallingFrame &Frame,
                             uint32_t CheckPointPath,
                             uint32_t CheckPointPathLen, uint32_t ModelIdPtr) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }
  auto CheckPointPathSpan =
      MemInst->getSpan<char>(CheckPointPath, CheckPointPathLen);
  if (unlikely(CheckPointPathSpan.size() != CheckPointPathLen)) {
    return WASILLM::ErrNo::InvalidArgument;
  }

  auto *ModelId = MemInst->getPointer<uint32_t *>(ModelIdPtr);
  if (unlikely(ModelId == nullptr)) {
    return WASILLM::ErrNo::InvalidArgument;
  }
  GPT2 *Model = gpt2_create(CheckPointPathSpan.data());
  *ModelId = Env.addModel(Model);
  return WASILLM::ErrNo::InvalidArgument;
}

Expect<WASILLM::ErrNo> WasiLLMDataLoaderCreate::bodyImpl(
    const Runtime::CallingFrame &Frame, uint32_t DataPath, uint32_t DataPathLen,
    uint32_t B, uint32_t T, uint32_t ProcessRank, uint32_t NumProcesses,
    int32_t ShouldShuffle, uint32_t DataLoaderIdPtr) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }
  auto DataPathSpan = MemInst->getSpan<char>(DataPath, DataPathLen);
  if (unlikely(DataPathSpan.size() != DataPathLen)) {
    return WASILLM::ErrNo::InvalidArgument;
  }

  auto *DataLoaderId = MemInst->getPointer<uint32_t *>(DataLoaderIdPtr);
  if (unlikely(DataLoaderId == nullptr)) {
    return WASILLM::ErrNo::InvalidArgument;
  }
  DataLoader *D = dataloader_create(DataPathSpan.data(), B, T, ProcessRank,
                                    NumProcesses, ShouldShuffle);
  *DataLoaderId = Env.addDataLoader(D);
  return WASILLM::ErrNo::InvalidArgument;
}

Expect<WASILLM::ErrNo>
WasiLLMTokenizerCreate::bodyImpl(const Runtime::CallingFrame &Frame,
                                 uint32_t FilePath, uint32_t FilePathLen,
                                 uint32_t TokenizerIdPtr) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }
  auto FilePathSpan = MemInst->getSpan<char>(FilePath, FilePathLen);
  if (unlikely(FilePathSpan.size() != FilePathLen)) {
    return WASILLM::ErrNo::InvalidArgument;
  }

  auto *TokenizerId = MemInst->getPointer<uint32_t *>(TokenizerIdPtr);
  if (unlikely(TokenizerId == nullptr)) {
    return WASILLM::ErrNo::InvalidArgument;
  }
  Tokenizer *T = tokenizer_create(FilePathSpan.data());
  *TokenizerId = Env.addTokenizer(T);
  return WASILLM::ErrNo::InvalidArgument;
}

Expect<WASILLM::ErrNo>
WasiLLMModelTrain::bodyImpl(const Runtime::CallingFrame &Frame,
                            uint32_t ModelIdPtr, uint32_t TrainDataLoaderIdPtr,
                            uint32_t ValDataLoaderIdPtr,
                            uint32_t TokenizerIdPtr, float Lr, float Epoch) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }
  auto *ModelId = MemInst->getPointer<uint32_t *>(ModelIdPtr);
  if (unlikely(ModelId == nullptr)) {
    spdlog::error("[WASI-LLM] Failed when accessing model");
    return WASILLM::ErrNo::InvalidArgument;
  }
  auto *TokenizerId = MemInst->getPointer<uint32_t *>(TokenizerIdPtr);
  if (unlikely(TokenizerId == nullptr)) {
    spdlog::error("[WASI-LLM] Failed when accessing tokenizer");
    return WASILLM::ErrNo::InvalidArgument;
  }
  auto *TrainDataLoaderId =
      MemInst->getPointer<uint32_t *>(TrainDataLoaderIdPtr);
  if (unlikely(TrainDataLoaderId == nullptr)) {
    spdlog::error("[WASI-LLM] Failed when accessing train data loader");
    return WASILLM::ErrNo::InvalidArgument;
  }
  auto *ValDataLoaderId = MemInst->getPointer<uint32_t *>(ValDataLoaderIdPtr);
  if (unlikely(ValDataLoaderId == nullptr)) {
    spdlog::error("[WASI-LLM] Failed when accessing validation data loader");
    return WASILLM::ErrNo::InvalidArgument;
  }
  GPT2 *Model = Env.getModel(*ModelId);
  DataLoader *TrainDataLoader = Env.getDataLoader(*TrainDataLoaderId);
  DataLoader *ValDataLoader = Env.getDataLoader(*ValDataLoaderId);
  Tokenizer *Tokenizer = Env.getTokenizer(*TokenizerId);
  gpt2_train(Model, TrainDataLoader, ValDataLoader, Tokenizer, 0, 0, Lr, Epoch);
  return WASILLM::ErrNo::InvalidArgument;
}

} // namespace Host
} // namespace WasmEdge
