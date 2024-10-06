// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "plugin/plugin.h"

#include <cstdint>
#include <functional>
#include <vector>

extern "C" {
struct GPT2;
struct Tokenizer;
struct DataLoader;
}

namespace WasmEdge {
namespace Host {
namespace WasmEdgeLLMC {

enum class ErrNo : uint32_t {
  Success = 0,
  InvalidArgument = 1,
  MissingMemory = 2,
};

class LLMCEnv {
  std::vector<GPT2 *> Models;
  std::vector<Tokenizer *> Tokenizers;
  std::vector<DataLoader *> DataLoaders;

public:
  uint32_t addModel(GPT2 *M) noexcept;

  GPT2 *getModel(uint32_t Id) noexcept;

  size_t getModelSize() const noexcept { return Models.size(); }

  uint32_t addTokenizer(Tokenizer *T) noexcept;

  Tokenizer *getTokenizer(uint32_t Id) noexcept;

  size_t getTokenizerSize() const noexcept { return Tokenizers.size(); }

  uint32_t addDataLoader(DataLoader *D) noexcept;

  DataLoader *getDataLoader(uint32_t Id) noexcept;

  size_t getDataLoaderSize() const noexcept { return DataLoaders.size(); }

  ~LLMCEnv();
};

} // namespace WasmEdgeLLMC
} // namespace Host
} // namespace WasmEdge
