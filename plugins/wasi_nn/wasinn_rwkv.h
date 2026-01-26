// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2026 Second State INC

#pragma once

#include "wasinntypes.h"

#include "plugin/plugin.h"
#include <cstdint>

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_RWKV
#include <rwkv.h>
#include <tokenizers_cpp.h>

#include <algorithm>
#include <memory>
#include <random>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
#endif

namespace WasmEdge::Host::WASINN {
struct WasiNNEnvironment;
}

namespace WasmEdge::Host::WASINN::RWKV {
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_RWKV

struct Config {
  uint64_t ThreadsNum = std::min(
      static_cast<uint64_t>(4),
      static_cast<uint64_t>(std::max(1u, std::thread::hardware_concurrency())));
  bool EnableLog = false;
  bool EnableDebugLog = false;

  uint64_t MaxTokens = 256;
  float Temperature = 0.8f;
  uint64_t TopK = 40;
  float TopP = 0.5f;
  uint64_t MinKeep = 1;
  float PresencePenalty = 0.2f;
  float FrequencyPenalty = 0.2f;

  uint64_t NGPULayers = 0;

  bool ResetStateOnPrompt = true;

  std::string TokenizerPath;

  uint32_t EosTokenId = 0;

  uint64_t Seed = 0;
};

struct RWKVCtxDeleter {
  void operator()(rwkv_context *Ctx) const {
    if (Ctx) {
      rwkv_free(Ctx);
    }
  }
};
using RWKVCtxPtr = std::unique_ptr<rwkv_context, RWKVCtxDeleter>;

struct Graph {
  RWKVCtxPtr RWKVCtx;
  std::string ModelFilePath;
  Config RWKVConfig;
  std::unique_ptr<tokenizers::Tokenizer> Tok = nullptr;
  size_t NVocab = 0;
};

struct Context {
public:
  Context(uint32_t GId, Graph &G) noexcept
      : GraphId(GId), RWKVConfig(G.RWKVConfig) {
    if (RWKVConfig.Seed != 0) {
      Rng.seed(RWKVConfig.Seed);
    } else {
      Rng.seed(std::random_device{}());
    }
  }

  struct Candidate {
    uint32_t Id;
    float Logit;
    float P;
  };

  uint32_t GraphId;
  RWKVCtxPtr RWKVCtx;
  std::string InputPrompt;
  Config RWKVConfig;
  std::vector<float> StateBuffer;
  size_t StateBufferSize = 0;
  std::vector<float> LogitsBuffer;
  size_t LogitsBufferSize = 0;
  std::vector<Candidate> Candidates;
  std::string Outputs;
  std::vector<int32_t> InputTokens;
  std::unordered_map<uint32_t, uint32_t> TokenFrequencies;
  std::mt19937 Rng;

  bool ComputeSingleStarted = false;
  bool PromptProcessed = false;
  uint64_t TokensGenerated = 0;
  std::vector<int32_t> GeneratedTokens;
  int32_t LastToken = -1;
};

#else
struct Graph {};
struct Context {
  Context(uint32_t, Graph &) noexcept {}
};
#endif

struct Environ {};

Expect<WASINN::ErrNo> load(WASINN::WasiNNEnvironment &Env,
                           Span<const Span<uint8_t>> Builders,
                           WASINN::Device Device, uint32_t &GraphId) noexcept;
Expect<WASINN::ErrNo> initExecCtx(WASINN::WasiNNEnvironment &Env,
                                  uint32_t GraphId,
                                  uint32_t &ContextId) noexcept;
Expect<WASINN::ErrNo> setInput(WASINN::WasiNNEnvironment &Env,
                               uint32_t ContextId, uint32_t Index,
                               const TensorData &Tensor) noexcept;
Expect<WASINN::ErrNo> getOutput(WASINN::WasiNNEnvironment &Env,
                                uint32_t ContextId, uint32_t Index,
                                Span<uint8_t> OutBuffer,
                                uint32_t &BytesWritten) noexcept;
Expect<WASINN::ErrNo> compute(WASINN::WasiNNEnvironment &Env,
                              uint32_t ContextId) noexcept;
Expect<WASINN::ErrNo> unload(WASINN::WasiNNEnvironment &Env,
                             uint32_t GraphId) noexcept;
Expect<WASINN::ErrNo> finalizeExecCtx(WASINN::WasiNNEnvironment &Env,
                                      uint32_t ContextId) noexcept;
Expect<WASINN::ErrNo> computeSingle(WASINN::WasiNNEnvironment &Env,
                                    uint32_t ContextId) noexcept;
Expect<WASINN::ErrNo> getOutputSingle(WASINN::WasiNNEnvironment &Env,
                                      uint32_t ContextId, uint32_t Index,
                                      Span<uint8_t> OutBuffer,
                                      uint32_t &BytesWritten) noexcept;
Expect<WASINN::ErrNo> finiSingle(WASINN::WasiNNEnvironment &Env,
                                 uint32_t ContextId) noexcept;
} // namespace WasmEdge::Host::WASINN::RWKV
