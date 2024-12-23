// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "wasinntypes.h"

#include "plugin/plugin.h"

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_GGML
#include <common.h>
#include <llama.h>
#include <llava.h>
#include <sampling.h>
#endif

namespace WasmEdge::Host::WASINN {
struct WasiNNEnvironment;
}

namespace WasmEdge::Host::WASINN::GGML {

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_GGML

enum class EmbdNormalizeType : int32_t {
  // Follow:
  // https://github.com/ggerganov/llama.cpp/blob/0bf2d10c5514ff61b99897a4a5054f846e384e1e/common/common.h#L312
  None = -1,
  MaxAbsolute = 0,
  Taxicab = 1,
  Euclidean = 2,
  PNorm = 3,
};

enum class VisionModel : uint8_t {
  Llava = 0,
  Qwen2VL = 1,
};

struct Graph {
  llama_model *LlamaModel = nullptr;
  std::string ModelFilePath;
  llama_context *LlamaContext = nullptr;
  struct clip_ctx *ClipContext = nullptr;
  // Plugin parameters:
  bool EnableLog = false;
  bool EnableDebugLog = false;
  bool StreamStdout = false;
  bool Embedding = false;
  EmbdNormalizeType EmbdNormalize = EmbdNormalizeType::Euclidean;
  bool ComputeSingleStarted = false;
  int64_t NPredict;
  std::string ReversePrompt;
  std::string MMProjModelPath;
  std::string ImagePath;
  VisionModel VisionModelType = VisionModel::Llava;
  // Model parameters:
  int64_t MainGPU = 0; // Use GPU 0 by default
  int64_t NGPULayers = 0;
  std::vector<float> TensorSplit;
  bool UseMMap = true;
  bool WarmUp = false;
  // Context parameters:
  int64_t CtxSize;
  int64_t BatchSize;
  int64_t UBatchSize;
  int64_t Threads;
  // Sampling parameters:
  double Temp = 0.80;
  double TopP = 0.95;
  double RepeatPenalty = 1.10;
  double PresencePenalty = 0.00;
  double FrequencyPenalty = 0.00;
  std::string Grammar;
};

struct Context {
public:
  Context(uint32_t GId, Graph &) noexcept : GraphId(GId) {}
  uint32_t GraphId;
  std::vector<llama_token> LlamaInputs;
  uint64_t LlamaNInputs = 0;
  std::string LlamaOutputs;
  std::vector<llama_token> LlamaOutputTokens;
  // Preserve for computing single token
  common_sampler *LlamaSampler = nullptr;
  int32_t LlamaNPast = 0;
  int32_t LlamaNPos = 0;
  // Preserve for llava
  struct llava_image_embed *LlavaImageEmbd = nullptr;
  size_t ImagePosition = 0;
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
Expect<WASINN::ErrNo> getOutputSingle(WASINN::WasiNNEnvironment &Env,
                                      uint32_t ContextId, uint32_t Index,
                                      Span<uint8_t> OutBuffer,
                                      uint32_t &BytesWritten) noexcept;
Expect<WASINN::ErrNo> compute(WASINN::WasiNNEnvironment &Env,
                              uint32_t ContextId) noexcept;
Expect<WASINN::ErrNo> computeSingle(WASINN::WasiNNEnvironment &Env,
                                    uint32_t ContextId) noexcept;
Expect<WASINN::ErrNo> finiSingle(WASINN::WasiNNEnvironment &Env,
                                 uint32_t ContextId) noexcept;
Expect<WASINN::ErrNo> unload(WASINN::WasiNNEnvironment &Env,
                             uint32_t GraphId) noexcept;
Expect<WASINN::ErrNo> finalizeExecCtx(WASINN::WasiNNEnvironment &Env,
                                      uint32_t ContextId) noexcept;
} // namespace WasmEdge::Host::WASINN::GGML
