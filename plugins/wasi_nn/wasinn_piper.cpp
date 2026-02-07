// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "wasinn_piper.h"
#include "wasinnenv.h"

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_PIPER
#include "simdjson.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <functional>
#include <ios>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#endif

namespace WasmEdge::Host::WASINN::Piper {
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_PIPER

template <typename T>
std::tuple<WASINN::ErrNo, bool> getOption(simdjson::dom::object &Object,
                                          std::string_view Key, T &Result) {
  if (auto Error = Object[Key].get(Result)) {
    if (Error == simdjson::error_code::NO_SUCH_FIELD) {
      return {WASINN::ErrNo::Success, false};
    }
    spdlog::error(
        "[WASI-NN] Piper backend: Unable to retrieve the \"{}\" option: {}"sv,
        Key, simdjson::error_message(Error));
    return {WASINN::ErrNo::InvalidArgument, false};
  }
  return {WASINN::ErrNo::Success, true};
}

template <typename T, typename U = T>
WASINN::ErrNo getOptionalOption(simdjson::dom::object &Object,
                                std::string_view Key,
                                std::optional<T> &Result) {
  auto Value = U{};
  auto [Err, HasValue] = getOption(Object, Key, Value);
  if (HasValue) {
    Result = Value;
  }
  return Err;
}

WASINN::ErrNo parseSynthesisConfig(SynthesisConfig &SynthesisConfig,
                                   simdjson::dom::object &Object,
                                   const bool JsonInput) {
  {
    auto Value = std::optional<std::string_view>{};
    if (auto Err = getOptionalOption(Object, "output_type", Value);
        Err != WASINN::ErrNo::Success) {
      return Err;
    }
    if (Value) {
      if (Value.value() == "wav") {
        SynthesisConfig.OutputType = SynthesisConfigOutputType::OUTPUT_WAV;
      } else if (Value.value() == "raw") {
        SynthesisConfig.OutputType = SynthesisConfigOutputType::OUTPUT_RAW;
      } else {
        spdlog::error(
            "[WASI-NN] Piper backend: The output_type option has an unknown value {}."sv,
            Value.value());
        return WASINN::ErrNo::InvalidArgument;
      }
    }
  }
  if (JsonInput) {
    if (auto Err =
            getOptionalOption(Object, "speaker_id", SynthesisConfig.SpeakerId);
        Err != WASINN::ErrNo::Success) {
      return Err;
    }
  } else {
    if (auto Err =
            getOptionalOption(Object, "speaker", SynthesisConfig.SpeakerId);
        Err != WASINN::ErrNo::Success) {
      return Err;
    }
  }
  if (auto Err = getOptionalOption<float, double>(Object, "noise_scale",
                                                  SynthesisConfig.NoiseScale);
      Err != WASINN::ErrNo::Success) {
    return Err;
  }
  if (auto Err = getOptionalOption<float, double>(Object, "length_scale",
                                                  SynthesisConfig.LengthScale);
      Err != WASINN::ErrNo::Success) {
    return Err;
  }
  if (auto Err = getOptionalOption<float, double>(Object, "noise_w",
                                                  SynthesisConfig.NoiseW);
      Err != WASINN::ErrNo::Success) {
    return Err;
  }
  if (auto Err = getOptionalOption<float, double>(
          Object, "sentence_silence", SynthesisConfig.SentenceSilenceSeconds);
      Err != WASINN::ErrNo::Success) {
    return Err;
  }
  {
    // PhonemeSilenceSeconds is not supported in the new C API
    auto PhonemeSilence = std::optional<simdjson::dom::object>{};
    if (auto Err = getOptionalOption(Object, "phoneme_silence", PhonemeSilence);
        Err != WASINN::ErrNo::Success) {
      return Err;
    }
    if (PhonemeSilence) {
      spdlog::warn(
          "[WASI-NN] Piper backend: 'phoneme_silence' is not supported in this version."sv);
    }
  }
  return WASINN::ErrNo::Success;
}

WASINN::ErrNo parseRunConfig(RunConfig &RunConfig,
                             const std::string &String) noexcept {
  simdjson::dom::parser Parser;
  simdjson::dom::element Doc;
  if (auto Error = Parser.parse(String).get(Doc)) {
    spdlog::error("[WASI-NN] Piper backend: Parse run config error: {}"sv,
                  simdjson::error_message(Error));
    return WASINN::ErrNo::InvalidEncoding;
  }
  simdjson::dom::object Object;
  if (auto Error = Doc.get(Object)) {
    spdlog::error(
        "[WASI-NN] Piper backend: The run config is not an object: {}"sv,
        simdjson::error_message(Error));
    return WASINN::ErrNo::InvalidArgument;
  }

  auto ModelPath = std::optional<std::string_view>{};
  if (auto Err = getOptionalOption(Object, "model", ModelPath);
      Err != WASINN::ErrNo::Success) {
    return Err;
  }
  // Verify model file exists
  if (ModelPath) {
    auto Path = std::filesystem::u8path(ModelPath.value());
    if (!std::filesystem::exists(Path)) {
      spdlog::error("[WASI-NN] Piper backend: Model file doesn't exist"sv);
      return WASINN::ErrNo::InvalidArgument;
    }
    RunConfig.ModelPath = Path;
  } else {
    spdlog::error(
        "[WASI-NN] Piper backend: The model option is required but not provided"sv);
    return WASINN::ErrNo::InvalidArgument;
  }

  auto ModelConfigPath = std::optional<std::string_view>{};
  if (auto Err = getOptionalOption(Object, "config", ModelConfigPath);
      Err != WASINN::ErrNo::Success) {
    return Err;
  }
  if (ModelConfigPath) {
    RunConfig.ModelConfigPath =
        std::filesystem::u8path(ModelConfigPath.value());
  } else {
    RunConfig.ModelConfigPath = RunConfig.ModelPath;
    RunConfig.ModelConfigPath += ".json";
  }
  // Verify model config exists
  if (!std::filesystem::exists(RunConfig.ModelConfigPath)) {
    spdlog::error("[WASI-NN] Piper backend: Model config doesn't exist"sv);
    return WASINN::ErrNo::InvalidArgument;
  }

  if (auto Err =
          parseSynthesisConfig(RunConfig.DefaultSynthesisConfig, Object, false);
      Err != WASINN::ErrNo::Success) {
    return Err;
  }
  {
    auto Path = std::optional<std::string_view>{};
    if (auto Err = getOptionalOption(Object, "espeak_data", Path);
        Err != WASINN::ErrNo::Success) {
      return Err;
    }
    if (Path) {
      RunConfig.ESpeakDataPath = std::filesystem::u8path(Path.value());
    }
  }
  {
    auto Path = std::optional<std::string_view>{};
    if (auto Err = getOptionalOption(Object, "tashkeel_model", Path);
        Err != WASINN::ErrNo::Success) {
      return Err;
    }
    if (Path) {
      RunConfig.TashkeelModelPath = std::filesystem::u8path(Path.value());
    }
  }
  if (auto Err =
          std::get<0>(getOption(Object, "json_input", RunConfig.JsonInput));
      Err != WASINN::ErrNo::Success) {
    return Err;
  }
  return WASINN::ErrNo::Success;
}

void updateSynthesisConfig(SynthesisConfig &SynthesisConfig,
                           piper_synthesize_options &PiperSynthesisConfig,
                           const bool) {
  if (SynthesisConfig.NoiseScale) {
    PiperSynthesisConfig.noise_scale = SynthesisConfig.NoiseScale.value();
  }
  if (SynthesisConfig.LengthScale) {
    PiperSynthesisConfig.length_scale = SynthesisConfig.LengthScale.value();
  }
  if (SynthesisConfig.NoiseW) {
    PiperSynthesisConfig.noise_w_scale = SynthesisConfig.NoiseW.value();
  }
  if (SynthesisConfig.SpeakerId) {
    PiperSynthesisConfig.speaker_id =
        static_cast<int>(SynthesisConfig.SpeakerId.value());
  }
  // SentenceSilenceSeconds and PhonemeSilenceSeconds are not supported
}

void writeWavHeader(std::vector<uint8_t> &Output, uint32_t SampleRate,
                    uint32_t NumSamples) {
  uint32_t NumChannels = 1;
  uint32_t BitsPerSample = 16;
  uint32_t ByteRate = SampleRate * NumChannels * BitsPerSample / 8;
  uint32_t BlockAlign = NumChannels * BitsPerSample / 8;
  uint32_t Subchunk2Size = NumSamples * NumChannels * BitsPerSample / 8;
  uint32_t ChunkSize = 36 + Subchunk2Size;

  // RIFF
  Output.push_back('R');
  Output.push_back('I');
  Output.push_back('F');
  Output.push_back('F');
  Output.resize(8);
  std::memcpy(Output.data() + 4, &ChunkSize, 4);

  // WAVE
  Output.push_back('W');
  Output.push_back('A');
  Output.push_back('V');
  Output.push_back('E');

  // fmt
  Output.push_back('f');
  Output.push_back('m');
  Output.push_back('t');
  Output.push_back(' ');
  uint32_t Subchunk1Size = 16;
  Output.resize(20);
  std::memcpy(Output.data() + 16, &Subchunk1Size, 4);

  uint16_t AudioFormat = 1; // PCM
  Output.resize(22);
  std::memcpy(Output.data() + 20, &AudioFormat, 2);

  uint16_t NumChannels16 = (uint16_t)NumChannels;
  Output.resize(24);
  std::memcpy(Output.data() + 22, &NumChannels16, 2);

  Output.resize(28);
  std::memcpy(Output.data() + 24, &SampleRate, 4);

  Output.resize(32);
  std::memcpy(Output.data() + 28, &ByteRate, 4);

  uint16_t BlockAlign16 = (uint16_t)BlockAlign;
  Output.resize(34);
  std::memcpy(Output.data() + 32, &BlockAlign16, 2);

  uint16_t BitsPerSample16 = (uint16_t)BitsPerSample;
  Output.resize(36);
  std::memcpy(Output.data() + 34, &BitsPerSample16, 2);

  // data
  Output.push_back('d');
  Output.push_back('a');
  Output.push_back('t');
  Output.push_back('a');
  Output.resize(44);
  std::memcpy(Output.data() + 40, &Subchunk2Size, 4);
}

Expect<WASINN::ErrNo> load(WASINN::WasiNNEnvironment &Env,
                           Span<const Span<uint8_t>> Builders, WASINN::Device,
                           uint32_t &GraphId) noexcept {
  // The graph builder length must be 1.
  if (Builders.size() != 1) {
    spdlog::error(
        "[WASI-NN] Piper backend: Wrong GraphBuilder Length {:d}, expect 1"sv,
        Builders.size());
    return WASINN::ErrNo::InvalidArgument;
  }

  // Add a new graph.
  uint32_t GId = Env.newGraph(Backend::Piper);
  auto &GraphRef = Env.NNGraph[GId].get<Graph>();
  GraphRef.Config = std::make_unique<RunConfig>();
  auto String = std::string{Builders[0].begin(), Builders[0].end()};
  if (auto Res = parseRunConfig(*GraphRef.Config, String);
      Res != WASINN::ErrNo::Success) {
    Env.deleteGraph(GId);
    spdlog::error("[WASI-NN] Piper backend: Failed to parse run config."sv);
    return Res;
  }

  const char *espeakDataPathPos = nullptr;
  std::string espeakDataPathStr;
  if (GraphRef.Config->ESpeakDataPath) {
    espeakDataPathStr = GraphRef.Config->ESpeakDataPath->string();
    espeakDataPathPos = espeakDataPathStr.c_str();
    if (!std::filesystem::exists(GraphRef.Config->ESpeakDataPath.value())) {
      spdlog::error(
          "[WASI-NN] Piper backend: espeak-ng data directory doesn't exist"sv);
      Env.deleteGraph(GId);
      return WASINN::ErrNo::InvalidArgument;
    }
  }

  // Tashkeel not supported in current C API
  if (GraphRef.Config->TashkeelModelPath) {
    spdlog::warn(
        "[WASI-NN] Piper backend: Tashkeel model configuration ignored (not supported in new API)."sv);
  }

  auto *synth = piper_create(GraphRef.Config->ModelPath.string().c_str(),
                             GraphRef.Config->ModelConfigPath.string().c_str(),
                             espeakDataPathPos);

  if (!synth) {
    spdlog::error(
        "[WASI-NN] Piper backend: Failed to create piper synthesizer."sv);
    Env.deleteGraph(GId);
    return WASINN::ErrNo::InvalidArgument;
  }

  GraphRef.Synthesizer =
      std::unique_ptr<piper_synthesizer, PiperDeleter>(synth);

  // Update default config logic - extract defaults from synth?
  // piper_synthesize_options defaults =
  // piper_default_synthesize_options(synth);
  // GraphRef.Config->DefaultSynthesisConfig.LengthScale =
  // defaults.length_scale; etc. But we have our own RunConfig logic, we will
  // apply it at compute time.

  // Store the loaded graph.
  GraphId = GId;
  Env.NNGraph[GId].setReady();
  return WASINN::ErrNo::Success;
}

Expect<WASINN::ErrNo> initExecCtx(WASINN::WasiNNEnvironment &Env,
                                  uint32_t GraphId,
                                  uint32_t &ContextId) noexcept {
  // Create context.
  ContextId = Env.newContext(GraphId, Env.NNGraph[GraphId]);
  Env.NNContext[ContextId].setReady();
  return WASINN::ErrNo::Success;
}

Expect<WASINN::ErrNo> setInput(WASINN::WasiNNEnvironment &Env,
                               uint32_t ContextId, uint32_t Index,
                               const TensorData &Tensor) noexcept {
  if (Index != 0) {
    spdlog::error("[WASI-NN] Piper backend: Input index must be 0."sv);
    return WASINN::ErrNo::InvalidArgument;
  }
  if (Tensor.Dimension.size() != 1) {
    spdlog::error(
        "[WASI-NN] Piper backend: Input tensor dimension must be [1] (or [N] for string length)."sv);
    return WASINN::ErrNo::InvalidArgument;
  }

  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  auto &GraphRef = Env.NNGraph[CxtRef.GraphId].get<Graph>();

  auto Line = std::string{Tensor.Tensor.begin(), Tensor.Tensor.end()};

  if (GraphRef.Config->JsonInput) {
    simdjson::dom::parser Parser;
    simdjson::dom::element Doc;
    if (auto Error = Parser.parse(Line).get(Doc)) {
      spdlog::error("[WASI-NN] Piper backend: Parse json input error: {}"sv,
                    simdjson::error_message(Error));
      return WASINN::ErrNo::InvalidEncoding;
    }
    simdjson::dom::object Object;
    if (auto Error = Doc.get(Object)) {
      spdlog::error(
          "[WASI-NN] Piper backend: The json input is not an object: {}"sv,
          simdjson::error_message(Error));
      return WASINN::ErrNo::InvalidArgument;
    }

    // Text is required
    auto Text = std::string_view{};
    if (auto Error = Object["text"].get(Text)) {
      spdlog::error(
          "[WASI-NN] Piper backend: Unable to retrieve required \"text\" from json input: {}"sv,
          simdjson::error_message(Error));
      return WASINN::ErrNo::InvalidArgument;
    }
    Line = Text;

    // Parse override config
    auto JsonInputSynthesisConfig = SynthesisConfig{};
    if (auto Err = parseSynthesisConfig(JsonInputSynthesisConfig, Object, true);
        Err != WASINN::ErrNo::Success) {
      return Err;
    }
    if (!JsonInputSynthesisConfig.SpeakerId) {
      auto SpeakerName = std::optional<std::string_view>{};
      if (auto Err = getOptionalOption(Object, "speaker", SpeakerName);
          Err != WASINN::ErrNo::Success) {
        return Err;
      }
      if (SpeakerName) {
        // Resolve to id using speaker id map
        auto Name = std::string{SpeakerName.value()};
        spdlog::warn(
            "[WASI-NN] Piper backend: Speaker name resolution '{}' not supported in this version. Use speaker_id."sv,
            Name);
        /*
        if (GraphRef.Voice->modelConfig.speakerIdMap &&
            GraphRef.Voice->modelConfig.speakerIdMap->count(Name) > 0) {
          JsonInputSynthesisConfig.SpeakerId =
              GraphRef.Voice->modelConfig.speakerIdMap.value()[Name];
        } else {
          spdlog::warn("[WASI-NN] Piper backend: No speaker named: {}"sv, Name);
        }
        */
      }
    }
    if (!CxtRef.JsonInputSynthesisConfig) {
      CxtRef.JsonInputSynthesisConfig =
          std::make_unique<std::optional<SynthesisConfig>>();
    }
    *CxtRef.JsonInputSynthesisConfig = JsonInputSynthesisConfig;
  }
  CxtRef.Line = Line;
  return WASINN::ErrNo::Success;
}

Expect<WASINN::ErrNo> getOutput(WASINN::WasiNNEnvironment &Env,
                                uint32_t ContextId, uint32_t Index,
                                Span<uint8_t> OutBuffer,
                                uint32_t &BytesWritten) noexcept {
  if (Index != 0) {
    spdlog::error("[WASI-NN] Piper backend: Output index must be 0."sv);
    return WASINN::ErrNo::InvalidArgument;
  }

  auto &CxtRef = Env.NNContext[ContextId].get<Context>();

  if (!CxtRef.Output) {
    spdlog::error("[WASI-NN] Piper backend: No output available."sv);
    return WASINN::ErrNo::InvalidArgument;
  }

  if (CxtRef.Output->size() >= std::numeric_limits<uint32_t>::max()) {
    spdlog::error(
        "[WASI-NN] Piper backend: Output size {} is greater than std::numeric_limits<uint32_t>::max() {}."sv,
        CxtRef.Output->size(), std::numeric_limits<uint32_t>::max());
    return WASINN::ErrNo::InvalidArgument;
  }

  if (CxtRef.Output->size() > OutBuffer.size_bytes()) {
    spdlog::error(
        "[WASI-NN] Piper backend: Output size {} is greater than buffer size {}."sv,
        CxtRef.Output->size(), OutBuffer.size_bytes());
    return WASINN::ErrNo::InvalidArgument;
  }

  std::memcpy(OutBuffer.data(), CxtRef.Output->data(), CxtRef.Output->size());
  BytesWritten = CxtRef.Output->size();
  return WASINN::ErrNo::Success;
}

Expect<WASINN::ErrNo> compute(WASINN::WasiNNEnvironment &Env,
                              uint32_t ContextId) noexcept {
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  auto &GraphRef = Env.NNGraph[CxtRef.GraphId].get<Graph>();

  if (!CxtRef.Line) {
    spdlog::error("[WASI-NN] Piper backend: Input is not set."sv);
    return WASINN::ErrNo::InvalidArgument;
  }

  auto OutputType = SynthesisConfigOutputType::OUTPUT_WAV;
  if (GraphRef.Config->DefaultSynthesisConfig.OutputType) {
    OutputType = GraphRef.Config->DefaultSynthesisConfig.OutputType.value();
  }

  // Prepare options
  piper_synthesize_options options =
      piper_default_synthesize_options(GraphRef.Synthesizer.get());

  // Apply graph defaults
  updateSynthesisConfig(GraphRef.Config->DefaultSynthesisConfig, options,
                        false);

  // Override config
  if (CxtRef.JsonInputSynthesisConfig &&
      CxtRef.JsonInputSynthesisConfig->has_value()) {
    updateSynthesisConfig(CxtRef.JsonInputSynthesisConfig->value(), options,
                          false);
    if (CxtRef.JsonInputSynthesisConfig->value().OutputType) {
      OutputType = CxtRef.JsonInputSynthesisConfig->value().OutputType.value();
    }
  }

  std::vector<int16_t> audioBuffer;
  piper_audio_chunk chunk;

  if (piper_synthesize_start(GraphRef.Synthesizer.get(), CxtRef.Line->c_str(),
                             &options) != PIPER_OK) {
    spdlog::error("[WASI-NN] Piper backend: Synthesis start failed."sv);
    return WASINN::ErrNo::RuntimeError;
  }

  // Loop to get chunks
  int result;
  do {
    result = piper_synthesize_next(GraphRef.Synthesizer.get(), &chunk);
    if (result == PIPER_ERR_GENERIC) {
      spdlog::error(
          "[WASI-NN] Piper backend: Synthesis failed during generation."sv);
      return WASINN::ErrNo::RuntimeError;
    }
    if (chunk.samples && chunk.num_samples > 0) {
      // Convert float samples to int16
      for (size_t i = 0; i < chunk.num_samples; ++i) {
        float val = chunk.samples[i];
        // Clip
        if (val > 1.0f)
          val = 1.0f;
        if (val < -1.0f)
          val = -1.0f;
        // Scale to int16
        int16_t pcm = static_cast<int16_t>(val * 32767.0f);
        audioBuffer.push_back(pcm);
      }
    }
  } while (result != PIPER_DONE);

  if (OutputType == SynthesisConfigOutputType::OUTPUT_WAV) {
    CxtRef.Output = std::vector<uint8_t>{};
    // WAV Header
    // Assume 22050Hz? Using what piper config says.
    // The piper_synthesizer struct isn't exposed here necessarily, but we can
    // assume we got sample rate from chunk or default. Note: chunk.sample_rate
    // is available. If only empty chunk, assume default 22050 or get from first
    // chunk.
    int sampleRate = 22050; // Default fallback
    if (chunk.sample_rate > 0)
      sampleRate = chunk.sample_rate;
    // Actually, chunk.sample_rate is set in every chunk.

    writeWavHeader(CxtRef.Output.value(), sampleRate, audioBuffer.size());
    size_t headerSize = CxtRef.Output->size();
    size_t pcmSize = audioBuffer.size() * sizeof(int16_t);
    CxtRef.Output->resize(headerSize + pcmSize);
    std::memcpy(CxtRef.Output->data() + headerSize, audioBuffer.data(),
                pcmSize);

  } else if (OutputType == SynthesisConfigOutputType::OUTPUT_RAW) {
    size_t pcmSize = audioBuffer.size() * sizeof(int16_t);
    CxtRef.Output = std::vector<uint8_t>(pcmSize);
    std::memcpy(CxtRef.Output->data(), audioBuffer.data(), pcmSize);
  }

  return WASINN::ErrNo::Success;
}
#else
namespace {
Expect<WASINN::ErrNo> reportBackendNotSupported() noexcept {
  spdlog::error("[WASI-NN] Piper backend is not supported."sv);
  return WASINN::ErrNo::InvalidArgument;
}
} // namespace

Expect<WASINN::ErrNo> load(WASINN::WasiNNEnvironment &,
                           Span<const Span<uint8_t>>, WASINN::Device,
                           uint32_t &) noexcept {
  return reportBackendNotSupported();
}
Expect<WASINN::ErrNo> initExecCtx(WASINN::WasiNNEnvironment &, uint32_t,
                                  uint32_t &) noexcept {
  return reportBackendNotSupported();
}
Expect<WASINN::ErrNo> setInput(WASINN::WasiNNEnvironment &, uint32_t, uint32_t,
                               const TensorData &) noexcept {
  return reportBackendNotSupported();
}
Expect<WASINN::ErrNo> getOutput(WASINN::WasiNNEnvironment &, uint32_t, uint32_t,
                                Span<uint8_t>, uint32_t &) noexcept {
  return reportBackendNotSupported();
}
Expect<WASINN::ErrNo> compute(WASINN::WasiNNEnvironment &, uint32_t) noexcept {
  return reportBackendNotSupported();
}
#endif
} // namespace WasmEdge::Host::WASINN::Piper
