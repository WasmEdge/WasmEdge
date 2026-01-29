// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "plugin/plugin.h"
#include "wasinntypes.h"

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_GGML
#include "wasinntypes.h"
#include <ggml.h>
#include <list>
#include <llama-cpp.h>
#include <llama.h>
#include <mtmd.h>
#include <sampling.h>

#endif

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

struct LocalConfig {
  // Configurations which can be changed in every contexts.
  // The graph handles a default config and parsed from metadata when loading.
  // The context inherits a copy from graph when creating, and can be modified
  // when parsing metadata in set_input.
  bool StreamStdout = false;
  EmbdNormalizeType EmbdNormalize = EmbdNormalizeType::Euclidean;
  int64_t NPredict;
  std::string ReversePrompt;
  std::string ImagePath;
  bool AlwaysRegenerateImageEmbd = false;
};

struct Graph {
  // Plugin parameters:
  bool EnableLog = false;
  bool EnableDebugLog = false;
  common_params Params;
  std::list<std::string> TensorBuftOverrides;
  // Model context:
  common_init_result_ptr LlamaInitResult = nullptr;
  llama_model *LlamaModel = nullptr;
  llama_context *LlamaContext = nullptr;
  // Multimodal context:
  mtmd::context_ptr VisionContext = nullptr;
  mtmd::input_chunks_ptr VisionInputChunks = nullptr;
  // Text-to-speech:
  bool TextToSpeech = false;
  std::string TTSOutputFilePath = "output.wav";
  std::string TTSSpeakerFilePath;
  common_init_result_ptr TTSInitResult = nullptr;
  llama_model *TTSModel = nullptr;
  llama_context *TTSContext = nullptr;
  // Configs.
  LocalConfig Conf;
};

struct Context {
public:
  Context(uint32_t GId, Graph &G) noexcept : GraphId(GId), Conf(G.Conf) {}
  uint32_t GraphId;
  // Llama inputs:
  std::vector<llama_token> LlamaInputs;
  uint64_t LlamaNInputs = 0;
  // Llama outputs:
  std::vector<uint8_t> LlamaOutputs;
  std::vector<llama_token> LlamaOutputTokens;
  // Data for computing:
  bool ComputeSingleStarted = false;
  struct common_sampler *LlamaSampler = nullptr;
  // Handle the batch in the context to prevent from reallocation in every
  // computing.
  struct llama_batch LlamaBatch;
  struct llama_batch OutputBatch;
  int64_t CurrentBatchSize = 0;
  size_t ImagePosition = 0;
  int32_t NPos = 0;
  // Configs:
  LocalConfig Conf;
};
#else
struct Graph {};
struct Context {
  Context(uint32_t, Graph &) noexcept {}
};
#endif

struct Environ {};

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_GGML
namespace {

// Macro for logging debug message.
#define LOG_DEBUG(Debug, ...)                                                  \
  if (Debug) {                                                                 \
    spdlog::info("[WASI-NN][Debug] GGML backend: "sv __VA_ARGS__);             \
  }

// Macro for logging info message.
#define LOG_INFO(Info, ...)                                                    \
  if (Info) {                                                                  \
    spdlog::info("[WASI-NN] GGML backend: "sv __VA_ARGS__);                    \
  }

// Macro for logging warning message.
#define LOG_WARN(...) spdlog::warn("[WASI-NN] GGML backend: "sv __VA_ARGS__);

// Macro for logging error message.
#define LOG_ERROR(...) spdlog::error("[WASI-NN] GGML backend: "sv __VA_ARGS__);

// Macro for logging error message and return.
#define RET_ERROR(Error, ...)                                                  \
  spdlog::error("[WASI-NN] GGML backend: "sv __VA_ARGS__);                     \
  return Error;
} // namespace
#endif

} // namespace WasmEdge::Host::WASINN::GGML
