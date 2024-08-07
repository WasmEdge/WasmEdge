// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "plugin/plugin.h"
#include "types.h"

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_GGML
#include <common.h>
#include <llama.h>
#include <llava.h>
#endif

namespace WasmEdge::Host::WASINN {
struct WasiNNEnvironment;
}

namespace WasmEdge::Host::WASINN::GGML {

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_GGML
struct Graph {
  llama_model *LlamaModel = nullptr;
  std::string ModelFilePath;
  // Plugin parameters:
  bool EnableLog = false;
  bool EnableDebugLog = false;
  bool StreamStdout = false;
  bool Embedding = false;
  uint64_t NPredict;
  std::string ReversePrompt;
  std::string MMProjModelPath;
  std::string ImagePath;
  // Model parameters:
  int64_t MainGPU = 0; // Use GPU 0 by default
  int64_t NGPULayers = 0;
  std::vector<float> TensorSplit;
  bool UseMMap = true;
  // Context parameters:
  uint64_t CtxSize;
  uint64_t BatchSize;
  uint64_t UBatchSize;
  uint64_t Threads;
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
  Context(size_t GId, Graph &) noexcept : GraphId(GId) {}
  size_t GraphId;
  std::vector<llama_token> LlamaInputs;
  uint64_t LlamaNInputs = 0;
  std::string LlamaOutputs;
  std::vector<llama_token> LlamaOutputTokens;
  // Preserve for computing single token
  llama_context *LlamaContext = nullptr;
  struct llama_sampling_context *LlamaSampling = nullptr;
  int32_t LlamaNPast = 0;
  // Preserve for llava
  struct llava_image_embed *LlavaImageEmbd = nullptr;
  size_t LlavaImagePosition = 0;
};
#else
struct Graph {};
struct Context {
  Context(size_t, Graph &) noexcept {}
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
} // namespace WasmEdge::Host::WASINN::GGML
