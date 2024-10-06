// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "piper.h"
#include "wasinnenv.h"

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_PIPER
#include "simdjson.h"
#include "types.h"
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <ios>
#include <map>
#include <memory>
#include <optional>
#include <piper.hpp>
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

Expect<WASINN::ErrNo> parseRunConfig(RunConfig &RunConfig,
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

  {
    auto Value = std::optional<std::string_view>{};
    if (auto Err = getOptionalOption(Object, "output_type", Value);
        Err != WASINN::ErrNo::Success) {
      return Err;
    }
    if (Value) {
      if (Value.value() == "wav") {
        RunConfig.OutputType = RunConfigOutputType::OUTPUT_WAV;
      } else if (Value.value() == "raw") {
        RunConfig.OutputType = RunConfigOutputType::OUTPUT_RAW;
      } else {
        spdlog::error(
            "[WASI-NN] Piper backend: The output_type option has an unknown value {}."sv,
            Value.value());
        return WASINN::ErrNo::InvalidArgument;
      }
    }
  }
  if (auto Err = getOptionalOption(Object, "speaker", RunConfig.SpeakerId);
      Err != WASINN::ErrNo::Success) {
    return Err;
  }
  if (auto Err = getOptionalOption<float, double>(Object, "noise_scale",
                                                  RunConfig.NoiseScale);
      Err != WASINN::ErrNo::Success) {
    return Err;
  }
  if (auto Err = getOptionalOption<float, double>(Object, "length_scale",
                                                  RunConfig.LengthScale);
      Err != WASINN::ErrNo::Success) {
    return Err;
  }
  if (auto Err =
          getOptionalOption<float, double>(Object, "noise_w", RunConfig.NoiseW);
      Err != WASINN::ErrNo::Success) {
    return Err;
  }
  if (auto Err = getOptionalOption<float, double>(
          Object, "sentence_silence", RunConfig.SentenceSilenceSeconds);
      Err != WASINN::ErrNo::Success) {
    return Err;
  }
  {
    auto PhonemeSilence = std::optional<simdjson::dom::object>{};
    if (auto Err = getOptionalOption(Object, "phoneme_silence", PhonemeSilence);
        Err != WASINN::ErrNo::Success) {
      return Err;
    }
    if (PhonemeSilence) {
      for (auto [Key, Value] : PhonemeSilence.value()) {
        auto PhonemeStr = std::string{Key};
        if (!piper::isSingleCodepoint(PhonemeStr)) {
          spdlog::error(
              "[WASI-NN] Piper backend: Phoneme '{}' is not a single codepoint (phoneme_silence)."sv,
              PhonemeStr);
          return WASINN::ErrNo::InvalidArgument;
        }
        auto Seconds = Value.get_double();
        if (auto Error = Seconds.error()) {
          spdlog::error(
              "[WASI-NN] Piper backend: Failed to get silence seconds for phoneme '{}' as a double: {}"sv,
              PhonemeStr, simdjson::error_message(Error));
          return WASINN::ErrNo::InvalidArgument;
        }
        if (!RunConfig.PhonemeSilenceSeconds) {
          RunConfig.PhonemeSilenceSeconds.emplace();
        }
        auto Phoneme = piper::getCodepoint(PhonemeStr);
        RunConfig.PhonemeSilenceSeconds.value()[Phoneme] = Seconds.value();
      }
    }
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
  auto &GraphRef = Env.NNGraph.emplace_back(Backend::Piper).get<Graph>();
  GraphRef.Config = std::make_unique<RunConfig>();
  auto String = std::string{Builders[0].begin(), Builders[0].end()};
  if (auto Res = parseRunConfig(*GraphRef.Config, String);
      Res != WASINN::ErrNo::Success) {
    Env.NNGraph.pop_back();
    spdlog::error("[WASI-NN] Piper backend: Failed to parse run config."sv);
    return Res;
  }

  GraphRef.PiperConfig = std::make_unique<piper::PiperConfig>();
  GraphRef.Voice = std::make_unique<piper::Voice>();
  piper::loadVoice(*GraphRef.PiperConfig, GraphRef.Config->ModelPath.string(),
                   GraphRef.Config->ModelConfigPath.string(), *GraphRef.Voice,
                   GraphRef.Config->SpeakerId);
  GraphRef.SpeakerId = GraphRef.Config->SpeakerId;

  if (GraphRef.Voice->phonemizeConfig.phonemeType ==
      piper::PhonemeType::eSpeakPhonemes) {
    if (!GraphRef.Config->ESpeakDataPath) {
      spdlog::error(
          "[WASI-NN] Piper backend: espeak-ng data directory is required for eSpeakPhonemes"sv);
      Env.NNGraph.pop_back();
      return WASINN::ErrNo::InvalidArgument;
    }
    if (!std::filesystem::exists(GraphRef.Config->ESpeakDataPath.value())) {
      spdlog::error(
          "[WASI-NN] Piper backend: espeak-ng data directory doesn't exist"sv);
      Env.NNGraph.pop_back();
      return WASINN::ErrNo::InvalidArgument;
    }
    // User provided path
    GraphRef.PiperConfig->eSpeakDataPath =
        GraphRef.Config->ESpeakDataPath->string();
  } else {
    // Not using eSpeak
    GraphRef.PiperConfig->useESpeak = false;
  }

  // Enable libtashkeel for Arabic
  if (GraphRef.Voice->phonemizeConfig.eSpeak.voice == "ar") {
    if (!GraphRef.Config->TashkeelModelPath) {
      spdlog::error(
          "[WASI-NN] Piper backend: libtashkeel ort model is required for Arabic"sv);
      Env.NNGraph.pop_back();
      return WASINN::ErrNo::InvalidArgument;
    }
    if (!std::filesystem::exists(GraphRef.Config->TashkeelModelPath.value())) {
      spdlog::error(
          "[WASI-NN] Piper backend: libtashkeel ort model doesn't exist"sv);
      Env.NNGraph.pop_back();
      return WASINN::ErrNo::InvalidArgument;
    }
    GraphRef.PiperConfig->useTashkeel = true;
    // User provided path
    GraphRef.PiperConfig->tashkeelModelPath =
        GraphRef.Config->TashkeelModelPath->string();
  }

  piper::initialize(*GraphRef.PiperConfig);

  // Scales
  if (GraphRef.Config->NoiseScale) {
    GraphRef.Voice->synthesisConfig.noiseScale =
        GraphRef.Config->NoiseScale.value();
  }

  if (GraphRef.Config->LengthScale) {
    GraphRef.Voice->synthesisConfig.lengthScale =
        GraphRef.Config->LengthScale.value();
  }

  if (GraphRef.Config->NoiseW) {
    GraphRef.Voice->synthesisConfig.noiseW = GraphRef.Config->NoiseW.value();
  }

  if (GraphRef.Config->SentenceSilenceSeconds) {
    GraphRef.Voice->synthesisConfig.sentenceSilenceSeconds =
        GraphRef.Config->SentenceSilenceSeconds.value();
  }

  if (GraphRef.Config->PhonemeSilenceSeconds) {
    if (!GraphRef.Voice->synthesisConfig.phonemeSilenceSeconds) {
      // Overwrite
      GraphRef.Voice->synthesisConfig.phonemeSilenceSeconds =
          GraphRef.Config->PhonemeSilenceSeconds;
    } else {
      // Merge
      for (const auto &[Phoneme, SilenceSeconds] :
           *GraphRef.Config->PhonemeSilenceSeconds) {
        GraphRef.Voice->synthesisConfig.phonemeSilenceSeconds->try_emplace(
            Phoneme, SilenceSeconds);
      }
    }
  } // if phonemeSilenceSeconds

  // Store the loaded graph.
  GraphId = Env.NNGraph.size() - 1;
  return WASINN::ErrNo::Success;
}

Expect<WASINN::ErrNo> initExecCtx(WASINN::WasiNNEnvironment &Env,
                                  uint32_t GraphId,
                                  uint32_t &ContextId) noexcept {
  // Create context.
  Env.NNContext.emplace_back(GraphId, Env.NNGraph[GraphId]);
  ContextId = Env.NNContext.size() - 1;
  return WASINN::ErrNo::Success;
}

template <typename T>
WASINN::ErrNo getOptionalInputOption(simdjson::dom::object &Object,
                                     std::string_view Key,
                                     std::optional<T> &Result) {
  auto Value = T{};
  if (auto Error = Object[Key].get(Value)) {
    if (Error == simdjson::error_code::NO_SUCH_FIELD) {
      return WASINN::ErrNo::Success;
    }
    spdlog::error(
        "[WASI-NN] Piper backend: Unable to retrieve \"{}\" from json input: {}"sv,
        Key, simdjson::error_message(Error));
    return WASINN::ErrNo::InvalidArgument;
  }
  Result = Value;
  return WASINN::ErrNo::Success;
}

Expect<WASINN::ErrNo> setInput(WASINN::WasiNNEnvironment &Env,
                               uint32_t ContextId, uint32_t Index,
                               const TensorData &Tensor) noexcept {
  if (Index != 0) {
    spdlog::error("[WASI-NN] Piper backend: Input index must be 0."sv);
    return WASINN::ErrNo::InvalidArgument;
  }
  if (!(Tensor.Dimension.size() == 1 && Tensor.Dimension[0] == 1)) {
    spdlog::error(
        "[WASI-NN] Piper backend: Input tensor dimension must be [1]."sv);
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

    // Override speaker id
    auto SpeakerId = std::optional<piper::SpeakerId>{};
    if (auto Err = getOptionalInputOption(Object, "speaker_id", SpeakerId);
        Err != WASINN::ErrNo::Success) {
      return Err;
    }
    if (SpeakerId) {
      GraphRef.Voice->synthesisConfig.speakerId = SpeakerId;
    } else {
      auto SpeakerName = std::optional<std::string_view>{};
      if (auto Err = getOptionalInputOption(Object, "speaker", SpeakerName);
          Err != WASINN::ErrNo::Success) {
        return Err;
      }
      if (SpeakerName) {
        // Resolve to id using speaker id map
        auto Name = std::string{SpeakerName.value()};
        if (GraphRef.Voice->modelConfig.speakerIdMap &&
            GraphRef.Voice->modelConfig.speakerIdMap->count(Name) > 0) {
          GraphRef.Voice->synthesisConfig.speakerId =
              GraphRef.Voice->modelConfig.speakerIdMap.value()[Name];
        } else {
          spdlog::warn("[WASI-NN] Piper backend: No speaker named: {}"sv, Name);
        }
      }
    }
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

  auto Result = piper::SynthesisResult{};
  if (GraphRef.Config->OutputType == RunConfigOutputType::OUTPUT_WAV) {
    auto AudioFile =
        std::stringstream{std::ios::binary | std::ios::in | std::ios::out};
    piper::textToWavFile(*GraphRef.PiperConfig, *GraphRef.Voice,
                         CxtRef.Line.value(), AudioFile, Result);
    auto String = AudioFile.str();
    CxtRef.Output = std::vector<uint8_t>{String.begin(), String.end()};
  } else if (GraphRef.Config->OutputType == RunConfigOutputType::OUTPUT_RAW) {
    auto AudioBuffer = std::vector<int16_t>{};
    piper::textToAudio(*GraphRef.PiperConfig, *GraphRef.Voice,
                       CxtRef.Line.value(), AudioBuffer, Result, nullptr);
    CxtRef.Output = std::vector<uint8_t>(
        sizeof(decltype(AudioBuffer)::value_type) * AudioBuffer.size());
    std::memcpy(CxtRef.Output->data(), AudioBuffer.data(),
                CxtRef.Output->size());
  }

  // Restore config (json_input)
  GraphRef.Voice->synthesisConfig.speakerId = GraphRef.SpeakerId;
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
