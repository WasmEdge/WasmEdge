// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "whispercpp.h"
#include "wasinnenv.h"

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_WHISPER
#define DR_WAV_IMPLEMENTATION
#include "simdjson.h"
#include <examples/dr_wav.h>

#include <algorithm>
#endif

namespace WasmEdge::Host::WASINN::Whisper {
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_WHISPER

namespace {

bool checkAudioRIFF(const std::string_view Buf, const std::string_view Format) {
  if (Buf.size() < 12 || Buf.substr(0, 4) != "RIFF"sv) {
    return false;
  }
  if (Buf.substr(8, 4) != Format) {
    return false;
  }
  uint32_t ChunkSize = *reinterpret_cast<const uint32_t *>(Buf.data() + 4);
  if (ChunkSize + 8 != Buf.size()) {
    return false;
  }
  return true;
}

bool loadWAV(Span<const uint8_t> Buf, std::vector<float> &PCMF32) {
  // Not to use the helper function in examples of whisper.cpp to prevent from
  // copy.
  drwav WAV;
  const uint32_t ConstSampleRate = 16000;

  if (!drwav_init_memory(&WAV, Buf.data(), Buf.size(), nullptr)) {
    spdlog::error("[WASI-NN] Whisper backend: load WAV failed."sv);
    return false;
  }

  if (WAV.channels != 1 && WAV.channels != 2) {
    spdlog::error("[WASI-NN] Whisper backend: WAV must be mono or stereo."sv);
    drwav_uninit(&WAV);
    return false;
  }

  if (WAV.sampleRate != ConstSampleRate) {
    spdlog::error("[WASI-NN] Whisper backend: WAV must be {} kHz."sv,
                  ConstSampleRate / 1000);
    drwav_uninit(&WAV);
    return false;
  }

  if (WAV.bitsPerSample != 16) {
    spdlog::error("[WASI-NN] Whisper backend: WAV must be 16-bit."sv);
    drwav_uninit(&WAV);
    return false;
  }

  const uint32_t N = WAV.totalPCMFrameCount;
  std::vector<int16_t> PCM16(N * WAV.channels);
  drwav_read_pcm_frames_s16(&WAV, N, PCM16.data());
  drwav_uninit(&WAV);

  PCMF32.resize(N);
  if (WAV.channels == 1) {
    for (uint64_t I = 0; I < N; I++) {
      PCMF32[I] = static_cast<float>(PCM16[I]) / 32768.0f;
    }
  } else {
    for (uint64_t I = 0; I < N; I++) {
      PCMF32[I] =
          static_cast<float>(PCM16[2 * I] + PCM16[2 * I + 1]) / 65536.0f;
    }
  }
  return true;
}

void WhisperLogCallback(ggml_log_level LogLevel, const char *LogText,
                        void *UserData) {
  const Graph &GraphRef = *reinterpret_cast<Graph *>(UserData);
  if (!GraphRef.WhisperConfig.EnableLog) {
    return;
  }
  std::string Text(LogText);
  // Remove the trailing newlines.
  Text = Text.erase(Text.find_last_not_of("\n") + 1);
  // Skip for "."
  if (Text == ".") {
    return;
  }
  if (LogLevel == GGML_LOG_LEVEL_ERROR) {
    spdlog::error("[WASI-NN] whisper.cpp: {}"sv, Text);
  } else if (LogLevel == GGML_LOG_LEVEL_WARN) {
    spdlog::warn("[WASI-NN] whisper.cpp: {}"sv, Text);
  } else if (LogLevel == GGML_LOG_LEVEL_INFO) {
    spdlog::info("[WASI-NN] whisper.cpp: {}"sv, Text);
  } else if (LogLevel == GGML_LOG_LEVEL_DEBUG) {
    spdlog::debug("[WASI-NN] whisper.cpp: {}"sv, Text);
  }
}

void WhisperOutputSegmentCallback(struct whisper_context *WhisperCtx,
                                  struct whisper_state * /* state */, int NewN,
                                  void *UserData) {
  auto &CxtRef = *reinterpret_cast<Context *>(UserData);
  const int SegN = whisper_full_n_segments(WhisperCtx);

  auto ToTimeStr = [](int64_t T) -> std::string {
    T *= 10;
    uint32_t HR = static_cast<uint32_t>(T / (1000 * 60 * 60));
    T %= 1000 * 60 * 60;
    uint32_t M = static_cast<uint32_t>(T / (1000 * 60));
    T %= 1000 * 60;
    uint32_t S = static_cast<uint32_t>(T / 1000);
    uint32_t MS = static_cast<uint32_t>(T % 1000);
    char Buf[32];
    snprintf(Buf, sizeof(Buf), "%02d:%02d:%02d.%03d", HR, M, S, MS);
    return std::string(Buf);
  };

  // Output the last new N segments.
  for (int I = SegN - NewN; I < SegN; I++) {
    int64_t T0 = whisper_full_get_segment_t0(WhisperCtx, I);
    int64_t T1 = whisper_full_get_segment_t1(WhisperCtx, I);
    // TODO: Add the print timestamp config.
    CxtRef.Outputs += "[";
    CxtRef.Outputs += ToTimeStr(T0);
    CxtRef.Outputs += " --> ";
    CxtRef.Outputs += ToTimeStr(T1);
    CxtRef.Outputs += "] ";
    CxtRef.Outputs += whisper_full_get_segment_text(WhisperCtx, I);
    CxtRef.Outputs += "\n";
  }
}

void setWhisperParams(Context &CxtRef) noexcept {
  auto &WParam = CxtRef.WhisperParams;
  auto &ConfigRef = CxtRef.WhisperConfig;
  WParam.print_progress = false;
  WParam.thold_pt = ConfigRef.WordThreshold;
  WParam.translate = ConfigRef.Translate;
  WParam.language = ConfigRef.SpokenLanguage.c_str();
  WParam.detect_language = ConfigRef.DetectLanguage;
  WParam.initial_prompt = ConfigRef.InitialPrompt.c_str();
  WParam.temperature_inc = ConfigRef.TemperatureInc;
  WParam.temperature = ConfigRef.Temperature;
  WParam.entropy_thold = ConfigRef.EntropyThreshold;
  WParam.logprob_thold = ConfigRef.LogprobThreshold;
  WParam.grammar_penalty = ConfigRef.GrammarPenalty;
  WParam.new_segment_callback = WhisperOutputSegmentCallback;
  WParam.new_segment_callback_user_data = &CxtRef;
}

Expect<ErrNo> parseMetadata(Config &ConfigRef,
                            const std::string &Metadata) noexcept {
  simdjson::dom::parser Parser;
  simdjson::dom::element Doc;
  auto ParseError = Parser.parse(Metadata).get(Doc);
  if (ParseError) {
    spdlog::error("[WASI-NN] Whisper backend: Parse metadata error."sv);
    return ErrNo::InvalidEncoding;
  }

  // Get metadata from the json.
  // Currently supported metadata:
  // Plugin parameters (used by this plugin):
  //   enable-log: bool
  //   enable-debug-log: bool
  //   translate: bool
  //   language: string
  //   detect-language: bool
  //   prompt: string

  // The plugin parameters.
  if (Doc.at_key("enable-log").error() == simdjson::SUCCESS) {
    auto Err = Doc["enable-log"].get<bool>().get(ConfigRef.EnableLog);
    if (Err) {
      spdlog::error(
          "[WASI-NN] Whisper backend: Unable to retrieve the enable-log "
          "option."sv);
      return ErrNo::InvalidArgument;
    }
  }
  if (Doc.at_key("enable-debug-log").error() == simdjson::SUCCESS) {
    auto Err =
        Doc["enable-debug-log"].get<bool>().get(ConfigRef.EnableDebugLog);
    if (Err) {
      spdlog::error(
          "[WASI-NN] Whisper backend: Unable to retrieve the enable-debug-log "
          "option."sv);
      return ErrNo::InvalidArgument;
    }
  }
  if (Doc.at_key("translate").error() == simdjson::SUCCESS) {
    auto Err = Doc["translate"].get<bool>().get(ConfigRef.Translate);
    if (Err) {
      spdlog::error(
          "[WASI-NN] Whisper backend: Unable to retrieve the translate "
          "option."sv);
      return ErrNo::InvalidArgument;
    }
  }
  if (Doc.at_key("language").error() == simdjson::SUCCESS) {
    std::string_view Language;
    auto Err = Doc["language"].get<std::string_view>().get(Language);
    if (Err) {
      spdlog::error(
          "[WASI-NN] Whisper backend: Unable to retrieve the language "
          "option."sv);
      return ErrNo::InvalidArgument;
    }
    ConfigRef.SpokenLanguage = Language;
  }
  if (Doc.at_key("detect-language").error() == simdjson::SUCCESS) {
    auto Err = Doc["detect-language"].get<bool>().get(ConfigRef.DetectLanguage);
    if (Err) {
      spdlog::error(
          "[WASI-NN] Whisper backend: Unable to retrieve the detect-language "
          "option."sv);
      return ErrNo::InvalidArgument;
    }
  }
  if (Doc.at_key("prompt").error() == simdjson::SUCCESS) {
    std::string_view Prompt;
    auto Err = Doc["prompt"].get<std::string_view>().get(Prompt);
    if (Err) {
      spdlog::error(
          "[WASI-NN] Whisper backend: Unable to retrieve the prompt option."sv);
      return ErrNo::InvalidArgument;
    }
    ConfigRef.InitialPrompt = Prompt;
  }
  return ErrNo::Success;
}

Expect<ErrNo> handleTranslationConfig(whisper_context *WhisperCtx,
                                      Config &ConfigRef) noexcept {
  assuming(WhisperCtx);

  // Check the language.
  if (ConfigRef.SpokenLanguage != "auto"sv &&
      whisper_lang_id(ConfigRef.SpokenLanguage.c_str()) == -1) {
    spdlog::error("[WASI-NN] Whisper backend: Error: unknown language {}."sv,
                  ConfigRef.SpokenLanguage);
    return ErrNo::InvalidArgument;
  }

  // Check the translate option.
  if (!whisper_is_multilingual(WhisperCtx)) {
    if (ConfigRef.SpokenLanguage != "en"sv || ConfigRef.Translate) {
      ConfigRef.SpokenLanguage = "en"sv;
      ConfigRef.Translate = false;
      if (ConfigRef.EnableLog) {
        spdlog::info(
            "[WASI-NN] Whisper backend: Model is not multilingual. Ignoring "
            "language and translation options"sv);
      }
    }
  }
  if (ConfigRef.DetectLanguage) {
    ConfigRef.SpokenLanguage = "auto"sv;
  }
  return ErrNo::Success;
}

} // namespace

Expect<ErrNo> load(WasiNNEnvironment &Env, Span<const Span<uint8_t>> Builders,
                   [[maybe_unused]] Device Device, uint32_t &GraphId) noexcept {
  // Add a new graph.
  Env.NNGraph.emplace_back(Backend::Whisper);
  auto &GraphRef = Env.NNGraph.back().get<Graph>();

  // Initialize the parameters.
  auto CParam = whisper_context_default_params();
  GraphRef.ModelFilePath = ""sv;
  GraphRef.WhisperConfig.SpokenLanguage = "en"sv;
  GraphRef.UseGPU = CParam.use_gpu;
  GraphRef.MainGPU = CParam.gpu_device;

  // Set whisper log callback.
  whisper_log_set(WhisperLogCallback, &GraphRef);

  // If the graph builder length > 1, the data of builder[1] is the metadata.
  if (Builders.size() > 1) {
    const std::string Metadata(reinterpret_cast<char *>(Builders[1].data()),
                               Builders[1].size());
    // Ignore context or model updates when initializing the graph.
    auto Res = parseMetadata(GraphRef.WhisperConfig, Metadata);
    if (Res != ErrNo::Success) {
      spdlog::error("[WASI-NN] Whisper backend: Failed to parse metadata."sv);
      Env.NNGraph.pop_back();
      return Res;
    }
  }

  // Handle the model path.
  if (GraphRef.WhisperConfig.EnableDebugLog) {
    spdlog::info("[WASI-NN][Debug] Whisper backend: Handling model path."sv);
  }
  auto Weight = Builders[0];
  const std::string_view BinModel(reinterpret_cast<char *>(Weight.data()),
                                  Weight.size());
  if (BinModel.substr(0, 8) == "preload:"sv) {
    GraphRef.ModelFilePath = BinModel.substr(8);
  }

  // Initialize whisper context from model file with parameters.
  if (GraphRef.WhisperConfig.EnableDebugLog) {
    spdlog::info(
        "[WASI-NN][Debug] Whisper backend: Initialize whisper context with "
        "given parameters"sv);
  }
  if (GraphRef.ModelFilePath == ""sv) {
    GraphRef.WhisperCtx = whisper_init_from_buffer_with_params(
        Weight.data(), Weight.size(), CParam);
  } else {
    GraphRef.WhisperCtx = whisper_init_from_file_with_params(
        GraphRef.ModelFilePath.c_str(), CParam);
  }
  if (GraphRef.WhisperCtx == nullptr) {
    spdlog::error(
        "[WASI-NN] Whisper backend: Error: unable to init whisper context from "
        "model."sv);
    Env.NNGraph.pop_back();
    return ErrNo::InvalidArgument;
  }
  if (GraphRef.WhisperConfig.EnableDebugLog) {
    spdlog::info(
        "[WASI-NN][Debug] Whisper backend: Initialize whisper context with "
        "given parameters...Done"sv);
  }

  auto ResTranslateConfig =
      handleTranslationConfig(GraphRef.WhisperCtx, GraphRef.WhisperConfig);
  if (ResTranslateConfig != ErrNo::Success) {
    Env.NNGraph.pop_back();
    return ResTranslateConfig;
  }

  // Store the loaded graph.
  GraphId = Env.NNGraph.size() - 1;

  return ErrNo::Success;
}

Expect<ErrNo> initExecCtx(WasiNNEnvironment &Env, uint32_t GraphId,
                          uint32_t &ContextId) noexcept {
  auto &GraphRef = Env.NNGraph[GraphId].get<Graph>();
  if (GraphRef.WhisperConfig.EnableDebugLog) {
    spdlog::info("[WASI-NN][Debug] Whisper backend: initExecCtx"sv);
  }
  Env.NNContext.emplace_back(GraphId, Env.NNGraph[GraphId]);
  ContextId = Env.NNContext.size() - 1;
  setWhisperParams(Env.NNContext[ContextId].get<Context>());
  if (GraphRef.WhisperConfig.EnableLog) {
    spdlog::info("[WASI-NN] Whisper backend: whisper_system_info: {}"sv,
                 whisper_print_system_info());
  }
  if (GraphRef.WhisperConfig.EnableDebugLog) {
    spdlog::info("[WASI-NN][Debug] Whisper backend: initExecCtx...Done"sv);
  }
  return ErrNo::Success;
}

Expect<ErrNo> setInput(WasiNNEnvironment &Env, uint32_t ContextId,
                       uint32_t Index [[maybe_unused]],
                       const TensorData &Tensor) noexcept {
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  if (CxtRef.WhisperConfig.EnableDebugLog) {
    spdlog::info("[WASI-NN][Debug] Whisper backend: setInput"sv);
  }

  // Use index 1 for metadata.
  if (Index == 1) {
    if (CxtRef.WhisperConfig.EnableDebugLog) {
      spdlog::info(
          "[WASI-NN][Debug] Whisper backend: found Metadata, processing"sv);
    }
    const std::string Metadata(reinterpret_cast<char *>(Tensor.Tensor.data()),
                               Tensor.Tensor.size());
    auto Res = parseMetadata(CxtRef.WhisperConfig, Metadata);
    if (Res != ErrNo::Success) {
      spdlog::error("[WASI-NN] Whisper backend: Failed to parse metadata."sv);
      return Res;
    }
    auto &GraphRef = Env.NNGraph[CxtRef.GraphId].get<Graph>();
    Res = handleTranslationConfig(GraphRef.WhisperCtx, CxtRef.WhisperConfig);
    if (Res != ErrNo::Success) {
      return Res;
    }
    setWhisperParams(CxtRef);
    if (CxtRef.WhisperConfig.EnableDebugLog) {
      spdlog::info(
          "[WASI-NN][Debug] Whisper backend: found Metadata, processing...Done"sv);
    }
    return ErrNo::Success;
  }

  if (Tensor.Dimension.size() != 2) {
    spdlog::error("[WASI-NN] Tensor dimension is out of range, expect 2-dim, "
                  "but got {}-dim.",
                  Tensor.Dimension.size());
    return WASINN::ErrNo::InvalidArgument;
  }
  if (Tensor.Dimension[0] != 1) {
    spdlog::error("[WASI-NN] Only 1 channel supported for now.");
    return WASINN::ErrNo::InvalidArgument;
  }

  // Tensor type not used here. Not to check this.

  // Check the input audio file format and load. Currently WAV supported.
  if (!checkAudioRIFF(
          std::string_view(reinterpret_cast<char *>(Tensor.Tensor.data()),
                           Tensor.Tensor.size()),
          "WAVE"sv)) {
    spdlog::error("[WASI-NN] Only WAV format supported now."sv);
    return WASINN::ErrNo::InvalidArgument;
  }
  if (!loadWAV(Tensor.Tensor, CxtRef.InputPCM)) {
    return WASINN::ErrNo::InvalidArgument;
  }

  if (CxtRef.WhisperConfig.EnableDebugLog) {
    spdlog::info("[WASI-NN][Debug] Whisper backend: setInput...Done"sv);
  }
  return ErrNo::Success;
}

Expect<ErrNo> getOutput(WasiNNEnvironment &Env, uint32_t ContextId,
                        uint32_t Index, Span<uint8_t> OutBuffer,
                        uint32_t &BytesWritten) noexcept {
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  if (CxtRef.WhisperConfig.EnableDebugLog) {
    spdlog::info("[WASI-NN][Debug] Whisper backend: getOutput with Index {}"sv,
                 Index);
  }

  // Check out buffer max size.
  if (OutBuffer.size() < CxtRef.Outputs.length()) {
    spdlog::error("[WASI-NN] Expect out buffer max size {}, but got {}",
                  CxtRef.Outputs.length(), OutBuffer.size());
    return WASINN::ErrNo::InvalidArgument;
  }

  std::copy_n(CxtRef.Outputs.data(), CxtRef.Outputs.length(), OutBuffer.data());
  BytesWritten = CxtRef.Outputs.length();
  if (CxtRef.WhisperConfig.EnableDebugLog) {
    spdlog::info(
        "[WASI-NN][Debug] Whisper backend: getOutput with Index {}...Done"sv,
        Index);
  }
  return ErrNo::Success;
}

Expect<ErrNo> compute(WasiNNEnvironment &Env, uint32_t ContextId) noexcept {
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  auto &GraphRef = Env.NNGraph[CxtRef.GraphId].get<Graph>();
  if (CxtRef.WhisperConfig.EnableDebugLog) {
    spdlog::info("[WASI-NN][Debug] Whisper backend: compute"sv);
  }

  CxtRef.Outputs.clear();
  if (whisper_full(GraphRef.WhisperCtx, CxtRef.WhisperParams,
                   CxtRef.InputPCM.data(), CxtRef.InputPCM.size()) != 0) {
    spdlog::error(
        "[WASI-NN] Whisper backend: Error: failed to process audio."sv);
    return ErrNo::RuntimeError;
  }

  if (CxtRef.WhisperConfig.EnableDebugLog) {
    spdlog::info("[WASI-NN][Debug] Whisper backend: compute...Done"sv);
  }
  return ErrNo::Success;
}

#else

namespace {
Expect<ErrNo> reportBackendNotSupported() noexcept {
  spdlog::error("[WASI-NN] Whisper backend is not built. use "
                "-WASMEDGE_PLUGIN_WASI_NN_BACKEND=\"whisper\" to build it."sv);
  return ErrNo::InvalidArgument;
}
} // namespace

Expect<ErrNo> load(WasiNNEnvironment &, Span<const Span<uint8_t>>, Device,
                   uint32_t &) noexcept {
  return reportBackendNotSupported();
}
Expect<ErrNo> initExecCtx(WasiNNEnvironment &, uint32_t, uint32_t &) noexcept {
  return reportBackendNotSupported();
}
Expect<ErrNo> setInput(WasiNNEnvironment &, uint32_t, uint32_t,
                       const TensorData &) noexcept {
  return reportBackendNotSupported();
}
Expect<ErrNo> getOutput(WasiNNEnvironment &, uint32_t, uint32_t, Span<uint8_t>,
                        uint32_t &) noexcept {
  return reportBackendNotSupported();
}
Expect<ErrNo> compute(WasiNNEnvironment &, uint32_t) noexcept {
  return reportBackendNotSupported();
}

#endif
} // namespace WasmEdge::Host::WASINN::Whisper
