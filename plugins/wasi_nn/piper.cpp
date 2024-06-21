// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "piper.h"
#include "wasinnenv.h"

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_PIPER
#include "simdjson.h"
#include "types.h"
#include <cstring>
#include <fstream>
#include <functional>
#include <ios>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <piper.hpp>
#include <string>
#include <string_view>
#include <vector>
#endif

namespace WasmEdge::Host::WASINN::Piper {
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_PIPER

template <typename T, typename U = T>
Expect<WASINN::ErrNo> getOptionalOption(simdjson::dom::element &Doc,
                                        std::string_view Key,
                                        std::optional<T> &Result) {
  if (Doc.at_key(Key).error() == simdjson::SUCCESS) {
    auto Value = U{};
    auto Err = Doc[Key].get<U>().get(Value);
    if (Err) {
      spdlog::error(
          "[WASI-NN] Piper backend: Unable to retrieve the {} option.", Key);
      return WASINN::ErrNo::InvalidArgument;
    }
    Result = Value;
  }
  return WASINN::ErrNo::Success;
}

template <typename T>
Expect<WASINN::ErrNo> getOption(simdjson::dom::element &Doc,
                                std::string_view Key, T &Result) {
  if (Doc.at_key(Key).error() == simdjson::SUCCESS) {
    auto Err = Doc[Key].get<T>().get(Result);
    if (Err) {
      spdlog::error(
          "[WASI-NN] Piper backend: Unable to retrieve the {} option."sv, Key);
      return WASINN::ErrNo::InvalidArgument;
    }
  }
  return WASINN::ErrNo::Success;
}

Expect<WASINN::ErrNo> parseRunConfig(RunConfig &RunConfig,
                                     const std::string &String) noexcept {
  simdjson::dom::parser Parser;
  simdjson::dom::element Doc;
  auto ParseError = Parser.parse(String).get(Doc);
  if (ParseError) {
    spdlog::error("[WASI-NN] Piper backend: Parse run config error");
    return WASINN::ErrNo::InvalidEncoding;
  }

  auto ModelPath = std::optional<std::string_view>{};
  if (auto Err = getOptionalOption(Doc, "model", ModelPath);
      Err != WASINN::ErrNo::Success) {
    return Err;
  }

  // Verify model file exists
  if (ModelPath) {
    auto Path = std::string{ModelPath.value()};
    auto ModelFile = std::ifstream(Path, std::ios::binary);
    if (!ModelFile.good()) {
      spdlog::error("[WASI-NN] Piper backend: Model file doesn't exist");
      return WASINN::ErrNo::InvalidArgument;
    }
    RunConfig.ModelPath = ModelPath.value();
  } else {
    spdlog::error("[WASI-NN] Piper backend: The model option is required but "
                  "not provided");
    return WASINN::ErrNo::InvalidArgument;
  }

  auto ModelConfigPath = std::optional<std::string_view>{};
  if (auto Err = getOptionalOption(Doc, "config", ModelConfigPath);
      Err != WASINN::ErrNo::Success) {
    return Err;
  }

  if (!ModelConfigPath) {
    RunConfig.ModelConfigPath = RunConfig.ModelPath + ".json";
  } else {
    RunConfig.ModelConfigPath = ModelConfigPath.value();
  }

  // Verify model config exists
  auto ModelConfigFile = std::ifstream(RunConfig.ModelConfigPath.c_str());
  if (!ModelConfigFile.good()) {
    spdlog::error("[WASI-NN] Piper backend: Model config doesn't exist");
    return WASINN::ErrNo::InvalidArgument;
  }

  {
    auto Value = std::optional<std::string_view>{};
    if (auto Err = getOptionalOption(Doc, "output_type", Value);
        Err != WASINN::ErrNo::Success) {
      return Err;
    }
    if (Value) {
      if (Value.value() == "wav") {
        RunConfig.OutputType = RunConfigOutputType::OUTPUT_WAV;
      } else if (Value.value() == "raw") {
        RunConfig.OutputType = RunConfigOutputType::OUTPUT_RAW;
      } else {
        spdlog::error("[WASI-NN] Piper backend: The output_type option has an "
                      "unknown value {}.",
                      Value.value());
        return WASINN::ErrNo::InvalidArgument;
      }
    }
  }
  if (auto Err = getOptionalOption(Doc, "speaker", RunConfig.SpeakerId);
      Err != WASINN::ErrNo::Success) {
    return Err;
  }
  if (auto Err = getOptionalOption<float, double>(Doc, "noise_scale",
                                                  RunConfig.NoiseScale);
      Err != WASINN::ErrNo::Success) {
    return Err;
  }
  if (auto Err = getOptionalOption<float, double>(Doc, "length_scale",
                                                  RunConfig.LengthScale);
      Err != WASINN::ErrNo::Success) {
    return Err;
  }
  if (auto Err =
          getOptionalOption<float, double>(Doc, "noise_w", RunConfig.NoiseW);
      Err != WASINN::ErrNo::Success) {
    return Err;
  }
  if (auto Err = getOptionalOption<float, double>(
          Doc, "sentence_silence", RunConfig.SentenceSilenceSeconds);
      Err != WASINN::ErrNo::Success) {
    return Err;
  }
  {
    if (Doc.at_key("phoneme_silence").error() == simdjson::SUCCESS) {
      auto PhonemeSilence = Doc["phoneme_silence"].get_object();
      for (auto [key, value] : PhonemeSilence) {
        auto PhonemeStr = std::string{key};
        if (!piper::isSingleCodepoint(PhonemeStr)) {

          spdlog::error("[WASI-NN] Piper backend: Phoneme '{}' is not a single "
                        "codepoint (phoneme_silence).",
                        PhonemeStr);
          return WASINN::ErrNo::InvalidArgument;
        }

        if (!RunConfig.PhonemeSilenceSeconds) {
          RunConfig.PhonemeSilenceSeconds.emplace();
        }

        auto Phoneme = piper::getCodepoint(PhonemeStr);
        RunConfig.PhonemeSilenceSeconds.value()[Phoneme] =
            value.get_double().value();
      }
    }
  }
  if (auto Err = getOptionalOption<std::string, std::string_view>(
          Doc, "espeak_data", RunConfig.ESpeakDataPath);
      Err != WASINN::ErrNo::Success) {
    return Err;
  }
  if (auto Err = getOptionalOption<std::string, std::string_view>(
          Doc, "tashkeel_model", RunConfig.TashkeelModelPath);
      Err != WASINN::ErrNo::Success) {
    return Err;
  }
  if (auto Err = getOption(Doc, "json_input", RunConfig.JsonInput);
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
        "[WASI-NN] Piper backend: Wrong GraphBuilder Length {:d}, expect 1",
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
    spdlog::error("[WASI-NN] Piper backend: Failed to parse run config.");
    return Res;
  }

  GraphRef.PiperConfig = std::make_unique<piper::PiperConfig>();
  GraphRef.Voice = std::make_unique<piper::Voice>();
  piper::loadVoice(*GraphRef.PiperConfig, GraphRef.Config->ModelPath,
                   GraphRef.Config->ModelConfigPath, *GraphRef.Voice,
                   GraphRef.Config->SpeakerId);
  GraphRef.SpeakerId = GraphRef.Config->SpeakerId;

  if (GraphRef.Voice->phonemizeConfig.phonemeType ==
      piper::PhonemeType::eSpeakPhonemes) {
    if (!GraphRef.Config->ESpeakDataPath) {
      spdlog::error("[WASI-NN] Piper backend: espeak-ng data directory is "
                    "required for eSpeakPhonemes");
      Env.NNGraph.pop_back();
      return WASINN::ErrNo::InvalidArgument;
    }
    // User provided path
    GraphRef.PiperConfig->eSpeakDataPath =
        GraphRef.Config->ESpeakDataPath.value();
  } else {
    // Not using eSpeak
    GraphRef.PiperConfig->useESpeak = false;
  }

  // Enable libtashkeel for Arabic
  if (GraphRef.Voice->phonemizeConfig.eSpeak.voice == "ar") {
    if (!GraphRef.Config->TashkeelModelPath) {
      spdlog::error("[WASI-NN] Piper backend: libtashkeel ort model is "
                    "required for Arabic");
      Env.NNGraph.pop_back();
      return WASINN::ErrNo::InvalidArgument;
    }
    GraphRef.PiperConfig->useTashkeel = true;
    // User provided path
    GraphRef.PiperConfig->tashkeelModelPath =
        GraphRef.Config->TashkeelModelPath;
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
  Env.NNContext.emplace_back(GraphId, Env.NNGraph[GraphId]).get<Context>();
  ContextId = Env.NNContext.size() - 1;
  return WASINN::ErrNo::Success;
}

template <typename T>
Expect<WASINN::ErrNo> getOptionalInputOption(simdjson::dom::element &Doc,
                                             std::string_view Key,
                                             std::optional<T> &Result) {
  if (Doc.at_key(Key).error() == simdjson::SUCCESS) {
    auto Value = T{};
    auto Err = Doc[Key].get<T>().get(Value);
    if (Err) {
      spdlog::error(
          "[WASI-NN] Piper backend: Unable to retrieve {} from json input.",
          Key);
      return WASINN::ErrNo::InvalidArgument;
    }
    Result = Value;
  }
  return WASINN::ErrNo::Success;
}

Expect<WASINN::ErrNo> setInput(WASINN::WasiNNEnvironment &Env,
                               uint32_t ContextId, uint32_t,
                               const TensorData &Tensor) noexcept {
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  auto &GraphRef = Env.NNGraph[CxtRef.GraphId].get<Graph>();

  auto Line = std::string{Tensor.Tensor.begin(), Tensor.Tensor.end()};

  if (GraphRef.Config->JsonInput) {
    simdjson::dom::parser Parser;
    simdjson::dom::element Doc;
    auto ParseError = Parser.parse(Line).get(Doc);
    if (ParseError) {
      spdlog::error("[WASI-NN] Piper backend: Parse json input error");
      return WASINN::ErrNo::InvalidEncoding;
    }

    // Text is required
    if (Doc.at_key("text").error() == simdjson::SUCCESS) {
      auto Text = std::string_view{};
      auto Err = Doc["text"].get<std::string_view>().get(Text);
      if (Err) {
        spdlog::error("[WASI-NN] Piper backend: Unable to retrieve text from "
                      "json input.");
        return WASINN::ErrNo::InvalidArgument;
      }
      Line = Text;
    } else {
      spdlog::error(
          "[WASI-NN] Piper backend: text from is required for json input.");
      return WASINN::ErrNo::InvalidArgument;
    }

    // Override speaker id
    if (auto Err = getOptionalInputOption(
            Doc, "speaker_id", GraphRef.Voice->synthesisConfig.speakerId);
        Err != WASINN::ErrNo::Success) {
      return Err;
    }
    if (!GraphRef.Voice->synthesisConfig.speakerId) {
      auto SpeakerName = std::optional<std::string_view>{};
      if (auto Err = getOptionalInputOption(Doc, "speaker", SpeakerName);
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
          spdlog::warn("[WASI-NN] Piper backend: No speaker named: {}", Name);
        }
      }
    }
  }
  CxtRef.Line = Line;
  return WASINN::ErrNo::Success;
}

Expect<WASINN::ErrNo> getOutput(WASINN::WasiNNEnvironment &Env,
                                uint32_t ContextId, uint32_t,
                                Span<uint8_t> OutBuffer,
                                uint32_t &BytesWritten) noexcept {
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();

  if (!CxtRef.Output) {
    spdlog::error("[WASI-NN] Piper backend: No output available.");
    return WASINN::ErrNo::InvalidArgument;
  }

  if (CxtRef.Output->size() >= std::numeric_limits<uint32_t>::max()) {
    spdlog::error("[WASI-NN] ONNX backend: Output size is greater than "
                  "std::numeric_limits<uint32_t>::max().");
    return WASINN::ErrNo::InvalidArgument;
  }

  if (CxtRef.Output->size() > OutBuffer.size_bytes()) {
    spdlog::error(
        "[WASI-NN] Piper backend: Output size is greater than buffer size.");
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
    spdlog::error("[WASI-NN] Piper backend: Input is not set.");
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
    CxtRef.Output = std::vector<uint8_t>(sizeof(int16_t) * AudioBuffer.size());
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
  spdlog::error("[WASI-NN] Piper backend is not supported.");
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
