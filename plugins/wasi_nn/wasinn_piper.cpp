// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "wasinn_piper.h"
#include "common/errcode.h"
#include "common/span.h"
#include "wasinnenv.h"
#include "wasinntypes.h"

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_PIPER
#include "simdjson.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>
#endif

namespace WasmEdge::Host::WASINN::Piper {
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_PIPER

namespace {

// helper function to write WAV header
void writeWavHeader(int SampleRate, int16_t NumChannels, int32_t NumSamples,
                    std::vector<uint8_t> &OutputBuffer) {
  int32_t const ByteRate = SampleRate * NumChannels * sizeof(int16_t);
  int32_t const DataSize = NumSamples * NumChannels * sizeof(int16_t);
  int32_t const RiffSize = 36 + DataSize;

  auto PushU32 = [&](int32_t Val) {
    OutputBuffer.push_back(Val & 0xFF);
    OutputBuffer.push_back((Val >> 8) & 0xFF);
    OutputBuffer.push_back((Val >> 16) & 0xFF);
    OutputBuffer.push_back((Val >> 24) & 0xFF);
  };
  auto PushU16 = [&](int16_t Val) {
    OutputBuffer.push_back(Val & 0xFF);
    OutputBuffer.push_back((Val >> 8) & 0xFF);
  };
  auto PushStr = [&](const char *Str) {
    for (int I = 0; I < 4; I++)
      OutputBuffer.push_back(Str[I]);
  };

  PushStr("RIFF");
  PushU32(RiffSize);
  PushStr("WAVE");
  PushStr("fmt ");
  PushU32(16);
  PushU16(1);
  PushU16(NumChannels);
  PushU32(SampleRate);
  PushU32(ByteRate);
  PushU16(NumChannels * sizeof(int16_t));
  PushU16(16);
  PushStr("data");
  PushU32(DataSize);
}

} // namespace
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
                                   simdjson::dom::object &Object) {
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
  {
    auto SpeakerId = std::optional<int64_t>{};
    if (auto Err = getOptionalOption(Object, "speaker_id", SpeakerId);
        Err != WASINN::ErrNo::Success) {
      return Err;
    }
    if (SpeakerId.has_value()) {
      SynthesisConfig.SpeakerId = static_cast<int>(SpeakerId.value());
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

  if (auto Err = parseSynthesisConfig(RunConfig.DefaultSynthesisConfig, Object);
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
  if (auto Err =
          std::get<0>(getOption(Object, "json_input", RunConfig.JsonInput));
      Err != WASINN::ErrNo::Success) {
    return Err;
  }
  return WASINN::ErrNo::Success;
}

void updatePiperOptions(const SynthesisConfig &SynthesisConfig,
                        piper_synthesize_options &Options) {
  if (SynthesisConfig.SpeakerId) {
    Options.speaker_id = SynthesisConfig.SpeakerId.value();
  }
  if (SynthesisConfig.NoiseScale) {
    Options.noise_scale = SynthesisConfig.NoiseScale.value();
  }
  if (SynthesisConfig.LengthScale) {
    Options.length_scale = SynthesisConfig.LengthScale.value();
  }
  if (SynthesisConfig.NoiseW) {
    Options.noise_w_scale = SynthesisConfig.NoiseW.value();
  }
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
  uint32_t const GId = Env.newGraph(Backend::Piper);
  auto &GraphRef = Env.NNGraph[GId].get<Graph>();
  GraphRef.Config = std::make_unique<RunConfig>();
  auto String = std::string{Builders[0].begin(), Builders[0].end()};
  if (auto Res = parseRunConfig(*GraphRef.Config, String);
      Res != WASINN::ErrNo::Success) {
    Env.deleteGraph(GId);
    spdlog::error("[WASI-NN] Piper backend: Failed to parse run config."sv);
    return Res;
  }

  std::string EspeakPath = "";
  if (GraphRef.Config->ESpeakDataPath) {
    EspeakPath = GraphRef.Config->ESpeakDataPath->string();
  }

  piper_synthesizer *Synth =
      piper_create(GraphRef.Config->ModelPath.string().c_str(),
                   GraphRef.Config->ModelConfigPath.string().c_str(),
                   EspeakPath.empty() ? nullptr : EspeakPath.c_str());

  if (!Synth) {
    spdlog::error(
        "[WASI-NN] Piper backend: Failed to create piper synthesizer."sv);
    Env.deleteGraph(GId);
    return WASINN::ErrNo::InvalidArgument;
  }

  GraphRef.Synth = std::unique_ptr<piper_synthesizer, PiperDeleter>(Synth);
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
        "[WASI-NN] Piper backend: Input tensor dimension must be 1D."sv);
    return WASINN::ErrNo::InvalidArgument;
  }

  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  auto &GraphRef = Env.NNGraph[CxtRef.GraphId].get<Graph>();

  CxtRef.Line =
      std::string(reinterpret_cast<const char *>(Tensor.Tensor.data()),
                  Tensor.Tensor.size());

  if (GraphRef.Config->JsonInput) {
    simdjson::dom::parser Parser;
    simdjson::dom::element Doc;
    simdjson::padded_string const PaddedInput(CxtRef.Line.value());

    if (Parser.parse(PaddedInput).get(Doc) != simdjson::SUCCESS) {
      spdlog::error("[WASI-NN] Piper backend: Failed to parse JSON input."sv);
      return WASINN::ErrNo::InvalidArgument;
    }

    simdjson::dom::object JsonObj;
    if (Doc.get(JsonObj) != simdjson::SUCCESS) {
      spdlog::error("[WASI-NN] Piper backend: JSON input is not an object."sv);
      return WASINN::ErrNo::InvalidArgument;
    }

    SynthesisConfig NewConfig;
    if (auto Err = parseSynthesisConfig(NewConfig, JsonObj);
        Err != WASINN::ErrNo::Success) {
      return Err;
    }
    CxtRef.JsonInputSynthesisConfig =
        std::make_unique<std::optional<SynthesisConfig>>(NewConfig);
  }
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

  std::string TextToSpeak = CxtRef.Line.value();

  if (GraphRef.Config->JsonInput) {
    simdjson::dom::parser Parser;
    simdjson::dom::element Doc;
    simdjson::padded_string const PaddedInput(TextToSpeak);

    if (Parser.parse(PaddedInput).get(Doc) != simdjson::SUCCESS) {
      spdlog::error("[WASI-NN] Piper backend: Failed to parse JSON input."sv);
      return WASINN::ErrNo::InvalidArgument;
    }

    simdjson::dom::object JsonObj;
    if (Doc.get(JsonObj) != simdjson::SUCCESS) {
      spdlog::error("[WASI-NN] Piper backend: JSON input is not an object."sv);
      return WASINN::ErrNo::InvalidArgument;
    }

    std::string_view TmpText;
    if (JsonObj["text"].get(TmpText) != simdjson::SUCCESS) {
      spdlog::error(
          "[WASI-NN] Piper backend: JSON input must contain 'text' field."sv);
      return WASINN::ErrNo::InvalidArgument;
    }
    TextToSpeak = std::string(TmpText);

    SynthesisConfig NewConfig;
    if (parseSynthesisConfig(NewConfig, JsonObj) == WASINN::ErrNo::Success) {
      CxtRef.JsonInputSynthesisConfig =
          std::make_unique<std::optional<SynthesisConfig>>(NewConfig);
    }
  }

  piper_synthesize_options Options =
      piper_default_synthesize_options(GraphRef.Synth.get());
  updatePiperOptions(GraphRef.Config->DefaultSynthesisConfig, Options);

  auto OutputType = SynthesisConfigOutputType::OUTPUT_WAV;
  if (GraphRef.Config->DefaultSynthesisConfig.OutputType) {
    OutputType = GraphRef.Config->DefaultSynthesisConfig.OutputType.value();
  }

  if (CxtRef.JsonInputSynthesisConfig &&
      CxtRef.JsonInputSynthesisConfig->has_value()) {
    updatePiperOptions(CxtRef.JsonInputSynthesisConfig->value(), Options);
    if (CxtRef.JsonInputSynthesisConfig->value().OutputType) {
      OutputType = CxtRef.JsonInputSynthesisConfig->value().OutputType.value();
    }
  }

  int const Res = piper_synthesize_start(GraphRef.Synth.get(),
                                         TextToSpeak.c_str(), &Options);
  if (Res != PIPER_OK) {
    spdlog::error("[WASI-NN] Piper backend: piper_synthesize_start failed."sv);
    return WASINN::ErrNo::RuntimeError;
  }

  std::vector<int16_t> AudioBuffer;
  piper_audio_chunk Chunk;
  int SampleRate = 0;
  constexpr float MaxWavValue = 32767.0f;

  while (piper_synthesize_next(GraphRef.Synth.get(), &Chunk) != PIPER_DONE) {
    if (Chunk.num_samples == 0) {
      continue;
    }
    SampleRate = Chunk.sample_rate;
    size_t OriginalSize = AudioBuffer.size();
    AudioBuffer.resize(OriginalSize + Chunk.num_samples);

    for (size_t I = 0; I < Chunk.num_samples; I++) {
      float Sample = Chunk.samples[I];
      Sample = std::clamp(Sample, -1.0f, 1.0f);
      AudioBuffer[OriginalSize + I] =
          static_cast<int16_t>(Sample * MaxWavValue);
    }
  }

  CxtRef.Output.emplace();
  constexpr int DefaultPiperSampleRate = 22050;
  if (OutputType == SynthesisConfigOutputType::OUTPUT_WAV) {
    if (SampleRate == 0) {
      SampleRate = DefaultPiperSampleRate;
    }
    size_t const TotalSize = 44 + (AudioBuffer.size() * sizeof(int16_t));
    CxtRef.Output->reserve(TotalSize);

    writeWavHeader(SampleRate, 1, static_cast<int32_t>(AudioBuffer.size()),
                   *CxtRef.Output);

    const uint8_t *RawData =
        reinterpret_cast<const uint8_t *>(AudioBuffer.data());
    CxtRef.Output->insert(CxtRef.Output->end(), RawData,
                          RawData + (AudioBuffer.size() * sizeof(int16_t)));

  } else {
    size_t const TotalSize = AudioBuffer.size() * sizeof(int16_t);
    CxtRef.Output->resize(TotalSize);
    const uint8_t *RawData =
        reinterpret_cast<const uint8_t *>(AudioBuffer.data());
    std::copy_n(RawData, TotalSize, CxtRef.Output->data());
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
