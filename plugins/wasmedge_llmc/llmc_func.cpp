// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "llmc_func.h"
#include "common/errcode.h"
#include "common/spdlog.h"
#include "llmc_fwd.h"
#include "types.h"
#include <string>
#include <string_view>

namespace WasmEdge::Host {

Expect<LLMC::ErrNo>
LLMCModelCreate::bodyImpl(const Runtime::CallingFrame &Frame,
                          uint32_t CheckPointPath, uint32_t CheckPointPathLen,
                          uint32_t ModelIdPtr) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    spdlog::error("[WasmEdge-LLM] Memory instance not found."sv);
    return LLMC::ErrNo::MissingMemory;
  }
  auto CheckPointPathSpan =
      MemInst->getSpan<char>(CheckPointPath, CheckPointPathLen);
  if (unlikely(CheckPointPathSpan.size() != CheckPointPathLen)) {
    spdlog::error(
        "[WasmEdge-LLM] Failed when accessing the input checkpoint path memory."sv);
    return LLMC::ErrNo::MissingMemory;
  }

  auto *ModelId = MemInst->getPointer<uint32_t *>(ModelIdPtr);
  if (unlikely(ModelId == nullptr)) {
    spdlog::error(
        "[WasmEdge-LLM] Failed when accessing the return model memory."sv);
    return LLMC::ErrNo::InvalidArgument;
  }
  std::string CheckPointPathStr =
      std::string(CheckPointPathSpan.begin(),
                  CheckPointPathSpan.begin() + CheckPointPathSpan.size());
  GPT2 *Model = gpt2_create(CheckPointPathStr.data());
  *ModelId = Env.addModel(Model);
  return LLMC::ErrNo::Success;
}

Expect<LLMC::ErrNo> LLMCDataLoaderCreate::bodyImpl(
    const Runtime::CallingFrame &Frame, uint32_t DataPath, uint32_t DataPathLen,
    uint32_t B, uint32_t T, uint32_t ProcessRank, uint32_t NumProcesses,
    int32_t ShouldShuffle, uint32_t DataLoaderIdPtr) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    spdlog::error("[WasmEdge-LLM] Memory instance not found."sv);
    return LLMC::ErrNo::MissingMemory;
  }
  auto DataPathSpan = MemInst->getSpan<char>(DataPath, DataPathLen);
  if (unlikely(DataPathSpan.size() != DataPathLen)) {
    spdlog::error(
        "[WasmEdge-LLM] Failed when accessing the input dataloader path memory."sv);
    return LLMC::ErrNo::MissingMemory;
  }

  auto *DataLoaderId = MemInst->getPointer<uint32_t *>(DataLoaderIdPtr);
  if (unlikely(DataLoaderId == nullptr)) {
    spdlog::error(
        "[WasmEdge-LLM] Failed when accessing the return dataloader memory."sv);
    return LLMC::ErrNo::InvalidArgument;
  }

  std::string DataPathStr = std::string(
      DataPathSpan.begin(), DataPathSpan.begin() + DataPathSpan.size());
  DataLoader *D = dataloader_create(DataPathStr.data(), B, T, ProcessRank,
                                    NumProcesses, ShouldShuffle);
  *DataLoaderId = Env.addDataLoader(D);
  return LLMC::ErrNo::Success;
}

Expect<LLMC::ErrNo>
LLMCTokenizerCreate::bodyImpl(const Runtime::CallingFrame &Frame,
                              uint32_t FilePath, uint32_t FilePathLen,
                              uint32_t TokenizerIdPtr) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    spdlog::error("[WasmEdge-LLM] Memory instance not found."sv);
    return LLMC::ErrNo::MissingMemory;
  }
  auto FilePathSpan = MemInst->getSpan<char>(FilePath, FilePathLen);
  if (unlikely(FilePathSpan.size() != FilePathLen)) {
    spdlog::error(
        "[WasmEdge-LLM] Failed when accessing the input tokenizer path memory."sv);
    return LLMC::ErrNo::MissingMemory;
  }

  auto *TokenizerId = MemInst->getPointer<uint32_t *>(TokenizerIdPtr);
  if (unlikely(TokenizerId == nullptr)) {
    spdlog::error(
        "[WasmEdge-LLM] Failed when accessing the return tokenizer memory."sv);
    return LLMC::ErrNo::InvalidArgument;
  }
  std::string FilePathStr = std::string(
      FilePathSpan.begin(), FilePathSpan.begin() + FilePathSpan.size());
  Tokenizer *T = tokenizer_create(FilePathStr.data());
  *TokenizerId = Env.addTokenizer(T);
  return LLMC::ErrNo::Success;
}

Expect<LLMC::ErrNo>
LLMCModelTrain::bodyImpl(const Runtime::CallingFrame &Frame, uint32_t ModelId,
                         uint32_t TrainDataLoaderId, uint32_t ValDataLoaderId,
                         uint32_t TokenizerId, uint32_t B, uint32_t T, float Lr,
                         uint32_t Epoch) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    spdlog::error("[WasmEdge-LLM] Memory instance not found."sv);
    return LLMC::ErrNo::MissingMemory;
  }
  GPT2 *Model = Env.getModel(ModelId);
  DataLoader *TrainDataLoader = Env.getDataLoader(TrainDataLoaderId);
  DataLoader *ValDataLoader = Env.getDataLoader(ValDataLoaderId);
  Tokenizer *Tokenizer = Env.getTokenizer(TokenizerId);
  gpt2_train(Model, TrainDataLoader, ValDataLoader, Tokenizer, B, T, Lr, Epoch);
  return LLMC::ErrNo::Success;
}

} // namespace WasmEdge::Host
