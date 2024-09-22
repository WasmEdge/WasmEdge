// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "plugin/plugin.h"
#include "types.h"

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_WHISPER
#include <whisper.h>

#include <string>
#include <vector>
#endif

namespace WasmEdge::Host::WASINN {
struct WasiNNEnvironment;
}

namespace WasmEdge::Host::WASINN::Whisper {
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_WHISPER

struct Config {
  // Whisper parameters:
  bool EnableLog = false;
  bool EnableDebugLog = false;
  bool Translate = false;
  bool DetectLanguage = false;
  std::string SpokenLanguage;
  std::string InitialPrompt;
  // Sampling parameters:
  float WordThreshold = 0.01f;
  float EntropyThreshold = 2.40f;
  float LogprobThreshold = -1.00f;
  float Temperature = 0.0f;
  float TemperatureInc = 0.2f;
  float GrammarPenalty = 100.0f;
};

struct Graph {
  whisper_context *WhisperCtx = nullptr;
  std::string ModelFilePath;
  // Whisper config:
  Config WhisperConfig;
  // Context parameters:
  bool UseGPU = true;
  int64_t MainGPU = 0; // Use GPU 0 by default
};

struct Context {
public:
  Context(size_t GId, Graph &G) noexcept
      : GraphId(GId), WhisperConfig(G.WhisperConfig) {}
  size_t GraphId;
  // mono-channel F32 PCM input.
  std::vector<float> InputPCM;
  // Whisper config. Inherit from the graph and accept metadata when setting
  // input.
  Config WhisperConfig;
  whisper_full_params WhisperParams = whisper_full_default_params(
      whisper_sampling_strategy::WHISPER_SAMPLING_BEAM_SEARCH);
  // Recognition outputs.
  std::string Outputs;
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
Expect<WASINN::ErrNo> compute(WASINN::WasiNNEnvironment &Env,
                              uint32_t ContextId) noexcept;
} // namespace WasmEdge::Host::WASINN::Whisper
