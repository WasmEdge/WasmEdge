// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "wasinntypes.h"

#include "plugin/plugin.h"

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_PIPER
#include <piper.h>

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#endif

namespace WasmEdge::Host::WASINN {
struct WasiNNEnvironment;
}

namespace WasmEdge::Host::WASINN::Piper {
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_PIPER
enum class SynthesisConfigOutputType { OUTPUT_WAV, OUTPUT_RAW };
struct SynthesisConfig {
  // Type of output to produce.
  // Default is a WAV file.
  std::optional<SynthesisConfigOutputType> OutputType;

  // Numerical id of the default speaker (multi-speaker voices)
  std::optional<int64_t> SpeakerId;

  // Amount of noise to add during audio generation
  std::optional<float> NoiseScale;

  // Speed of speaking (1 = normal, < 1 is faster, > 1 is slower)
  std::optional<float> LengthScale;

  // Variation in phoneme lengths
  std::optional<float> NoiseW;

  // Seconds of silence to add after each sentence
  std::optional<float> SentenceSilenceSeconds;

  // PhonemeSilenceSeconds removed (not supported)
};
struct RunConfig {
  // Path to .onnx voice file
  std::filesystem::path ModelPath;

  // Path to JSON voice config file
  std::filesystem::path ModelConfigPath;

  // Path to espeak-ng data directory
  std::optional<std::filesystem::path> ESpeakDataPath;

  // Path to libtashkeel ort model
  // https://github.com/mush42/libtashkeel/
  std::optional<std::filesystem::path> TashkeelModelPath;

  // input is JSON with format:
  // {
  //   "text": str,               (required)
  //   "speaker_id": int,         (optional)
  //   "speaker": str,            (optional)
  // }
  // including options in SynthesisConfig
  bool JsonInput = false;

  SynthesisConfig DefaultSynthesisConfig;
};
struct PiperDeleter {
  void operator()(piper_synthesizer *P) { piper_free(P); }
};

struct Graph {
  std::unique_ptr<RunConfig> Config;
  std::unique_ptr<piper_synthesizer, PiperDeleter> Synthesizer;
};
struct Context {
  Context(uint32_t GId, Graph &) noexcept : GraphId(GId) {}
  uint32_t GraphId;
  std::optional<std::string> Line;
  std::unique_ptr<std::optional<SynthesisConfig>> JsonInputSynthesisConfig;
  std::optional<std::vector<uint8_t>> Output;
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
} // namespace WasmEdge::Host::WASINN::Piper
