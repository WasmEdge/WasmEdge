// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "wasinn_whisper.h"
#include "api/vfs_io.h"
#include "wasinnenv.h"
#include <cstdint>
#include <vector>

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_WHISPER
#define DR_WAV_IMPLEMENTATION
#include "simdjson.h"
#include <examples/dr_wav.h>

#include <algorithm>
#endif

using namespace std::literals;

namespace WasmEdge::Host::WASINN::Whisper {
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_WHISPER

namespace {
int timestampToSample(int64_t T, int NSamples, int WhisperSampleRate) {
  return std::max(0, std::min(static_cast<int>(NSamples) - 1,
                              static_cast<int>((T * WhisperSampleRate) / 100)));
}
std::string toTimestamp(int64_t T, bool Comma) {
  int64_t Msec = T * 10;
  int64_t Hr = Msec / (1000 * 60 * 60);
  Msec = Msec - Hr * (1000 * 60 * 60);
  int64_t Min = Msec / (1000 * 60);
  Msec = Msec - Min * (1000 * 60);
  int64_t Sec = Msec / 1000;
  Msec = Msec - Sec * 1000;

  char Buf[32] = {};
  snprintf(Buf, sizeof(Buf), "%02d:%02d:%02d%s%03d", static_cast<int>(Hr),
           static_cast<int>(Min), static_cast<int>(Sec), Comma ? "," : ".",
           static_cast<int>(Msec));

  return std::string(Buf);
}

std::string
estimateDiarizationSpeaker(const std::vector<std::vector<float>> PCMF32s,
                           int64_t T0, int64_t T1, bool IdOnly = false) {
  std::string Speaker = "";
  const int64_t NSamples = PCMF32s[0].size();

  const int64_t Is0 = timestampToSample(T0, NSamples, WHISPER_SAMPLE_RATE);
  const int64_t Is1 = timestampToSample(T1, NSamples, WHISPER_SAMPLE_RATE);

  double Energy0 = 0.0f;
  double Energy1 = 0.0f;

  for (int64_t I = Is0; I < Is1; I++) {
    Energy0 += fabs(PCMF32s[0][I]);
    Energy1 += fabs(PCMF32s[1][I]);
  }

  if (Energy0 > 1.1 * Energy1) {
    Speaker = "0";
  } else if (Energy1 > 1.1 * Energy0) {
    Speaker = "1";
  } else {
    Speaker = "?";
  }

  if (!IdOnly) {
    Speaker.insert(0, "(speaker ");
    Speaker.append(")");
  }

  return Speaker;
}

bool outputSrt(WasiNNEnvironment &Env, whisper_context *Ctx,
               const std::string &Fname, const Config &Params,
               const std::vector<std::vector<float>> &PCMF32s) {
  WasmEdge::Host::API::WasmEdgeOfstream Fout(Env.getEnv(), Fname);
  if (!Fout.is_open()) {
    spdlog::error("[WASI-NN] Whisper backend: failed to open {} for writing."sv,
                  Fname);
    return false;
  }
  spdlog::info("[WASI-NN] Whisper backend: saving srt output to {}."sv, Fname);

  const int NSegments = whisper_full_n_segments(Ctx);
  for (int I = 0; I < NSegments; ++I) {
    const std::string &Text = whisper_full_get_segment_text(Ctx, I);
    const int64_t T0 = whisper_full_get_segment_t0(Ctx, I);
    const int64_t T1 = whisper_full_get_segment_t1(Ctx, I);
    std::string Speaker = "";

    if (Params.Diarize && PCMF32s.size() == 2) {
      Speaker = estimateDiarizationSpeaker(PCMF32s, T0, T1);
    }

    Fout << I + 1 + Params.OffsetN << "\n";
    Fout << toTimestamp(T0, true) << " --> " << toTimestamp(T1, true) << "\n";
    Fout << Speaker << Text << "\n\n";
  }
  return true;
}

static bool outputLrc(WasiNNEnvironment &Env, whisper_context *Ctx,
                      const std::string &Fname, const Config &Params,
                      const std::vector<std::vector<float>> &PCMF32s) {
  WasmEdge::Host::API::WasmEdgeOfstream Fout(Env.getEnv(), Fname);
  if (!Fout.is_open()) {
    spdlog::error("[WASI-NN] Whisper backend: failed to open {} for writing."sv,
                  Fname);
    return false;
  }

  spdlog::info("[WASI-NN] Whisper backend: saving lrc output to {}."sv, Fname);

  Fout << "[by:whisper.cpp]\n";

  const int NSegments = whisper_full_n_segments(Ctx);
  for (int I = 0; I < NSegments; ++I) {
    const std::string &text = whisper_full_get_segment_text(Ctx, I);
    const int64_t T = whisper_full_get_segment_t0(Ctx, I);

    int64_t Msec = T * 10;
    int64_t Min = Msec / (1000 * 60);
    Msec = Msec - Min * (1000 * 60);
    int64_t Sec = Msec / 1000;
    Msec = Msec - Sec * 1000;

    char Buf[16];
    snprintf(Buf, sizeof(Buf), "%02d:%02d.%02d", static_cast<int>(Min),
             static_cast<int>(Sec), static_cast<int>((Msec / 10)));
    std::string TimestampLrc = std::string(Buf);
    std::string Speaker = "";

    if (Params.Diarize && PCMF32s.size() == 2) {
      const int64_t t0 = whisper_full_get_segment_t0(Ctx, I);
      const int64_t t1 = whisper_full_get_segment_t1(Ctx, I);
      Speaker = estimateDiarizationSpeaker(PCMF32s, t0, t1);
    }

    Fout << '[' << TimestampLrc << ']' << Speaker << text << "\n";
  }

  return true;
}

std::string escapeDoubleQuotesAndBackslashes(const std::string &Str) {
  std::string Escaped;
  for (auto W : Str) {
    if (W == '"' || W == '\\') {
      Escaped += '\\';
    }
    Escaped += W;
  }
  return Escaped;
}

bool outputJson(WasiNNEnvironment &Env, whisper_context *Ctx,
                const std::string &Fname, const Config &Params,
                const std::vector<std::vector<float>> &PCMF32s, bool Full) {
  WasmEdge::Host::API::WasmEdgeOfstream Fout(Env.getEnv(), Fname);
  int Indent = 0;

  auto Doindent = [&]() {
    for (int i = 0; i < Indent; i++)
      Fout << "\t";
  };

  auto StartArr = [&](const char *Name) {
    Doindent();
    Fout << "\"" << Name << "\": [\n";
    Indent++;
  };

  auto EndArr = [&](bool End) {
    Indent--;
    Doindent();
    Fout << (End ? "]\n" : "],\n");
  };

  auto StartObj = [&](const char *Name) {
    Doindent();
    if (Name) {
      Fout << "\"" << Name << "\": {\n";
    } else {
      Fout << "{\n";
    }
    Indent++;
  };

  auto EndObj = [&](bool End) {
    Indent--;
    Doindent();
    Fout << (End ? "}\n" : "},\n");
  };

  auto StartValue = [&](const char *Name) {
    Doindent();
    Fout << "\"" << Name << "\": ";
  };

  auto ValueS = [&](const char *Name, const std::string &Val, bool End) {
    StartValue(Name);
    std::string ValEscaped = escapeDoubleQuotesAndBackslashes(Val);
    Fout << "\"" << ValEscaped << (End ? "\"\n" : "\",\n");
  };

  auto EndValue = [&](bool End) { Fout << (End ? "\n" : ",\n"); };

  auto ValueI = [&](const char *Name, const int64_t Val, bool End) {
    StartValue(Name);
    Fout << Val;
    EndValue(End);
  };

  auto ValueF = [&](const char *Name, const float Val, bool End) {
    StartValue(Name);
    Fout << Val;
    EndValue(End);
  };

  auto ValueB = [&](const char *Name, const bool Val, bool End) {
    StartValue(Name);
    Fout << (Val ? "true" : "false");
    EndValue(End);
  };

  auto TimesO = [&](int64_t T0, int64_t T1, bool End) {
    StartObj("timestamps");
    ValueS("from", toTimestamp(T0, true), false);
    ValueS("to", toTimestamp(T1, true), true);
    EndObj(false);
    StartObj("offsets");
    ValueI("from", T0 * 10, false);
    ValueI("to", T1 * 10, true);
    EndObj(End);
  };

  if (!Fout.is_open()) {
    spdlog::error("[WASI-NN] Whisper backend: failed to open {} for writing."sv,
                  Fname);
    return false;
  }

  spdlog::info("[WASI-NN] Whisper backend: saving json output to {}."sv, Fname);

  StartObj(nullptr);
  ValueS("systeminfo", whisper_print_system_info(), false);
  StartObj("model");
  ValueS("type", whisper_model_type_readable(Ctx), false);
  ValueB("multilingual", whisper_is_multilingual(Ctx), false);
  ValueI("vocab", whisper_model_n_vocab(Ctx), false);
  StartObj("audio");
  ValueI("ctx", whisper_model_n_audio_ctx(Ctx), false);
  ValueI("state", whisper_model_n_audio_state(Ctx), false);
  ValueI("head", whisper_model_n_audio_head(Ctx), false);
  ValueI("layer", whisper_model_n_audio_layer(Ctx), true);
  EndObj(false);
  StartObj("text");
  ValueI("ctx", whisper_model_n_text_ctx(Ctx), false);
  ValueI("state", whisper_model_n_text_state(Ctx), false);
  ValueI("head", whisper_model_n_text_head(Ctx), false);
  ValueI("layer", whisper_model_n_text_layer(Ctx), true);
  EndObj(false);
  ValueI("mels", whisper_model_n_mels(Ctx), false);
  ValueI("ftype", whisper_model_ftype(Ctx), true);
  EndObj(false);
  StartObj("params");
  ValueS("model", "Wasi-nn preload", false);
  ValueS("language", Params.SpokenLanguage, false);
  ValueB("translate", Params.Translate, true);
  EndObj(false);
  StartObj("result");
  ValueS("language", whisper_lang_str(whisper_full_lang_id(Ctx)), true);
  EndObj(false);
  StartArr("transcription");

  const int NSegments = whisper_full_n_segments(Ctx);
  for (int I = 0; I < NSegments; ++I) {
    const std::string &Text = whisper_full_get_segment_text(Ctx, I);

    const int64_t T0 = whisper_full_get_segment_t0(Ctx, I);
    const int64_t T1 = whisper_full_get_segment_t1(Ctx, I);

    StartObj(nullptr);
    TimesO(T0, T1, false);
    ValueS("text", Text, !Params.Diarize && !Params.TinyDiarize && !Full);

    if (Full) {
      StartArr("tokens");
      const int n = whisper_full_n_tokens(Ctx, I);
      for (int j = 0; j < n; ++j) {
        auto token = whisper_full_get_token_data(Ctx, I, j);
        StartObj(nullptr);
        ValueS("text", whisper_token_to_str(Ctx, token.id), false);
        if (token.t0 > -1 && token.t1 > -1) {
          // If we have per-token timestamps, write them out
          TimesO(token.t0, token.t1, false);
        }
        ValueI("id", token.id, false);
        ValueF("p", token.p, false);
        ValueF("t_dtw", token.t_dtw, true);
        EndObj(j == (n - 1));
      }
      EndArr(!Params.Diarize && !Params.TinyDiarize);
    }

    if (Params.Diarize && PCMF32s.size() == 2) {
      ValueS("speaker", estimateDiarizationSpeaker(PCMF32s, T0, T1, true),
             true);
    }

    if (Params.TinyDiarize) {
      ValueB("speaker_turn_next",
             whisper_full_get_segment_speaker_turn_next(Ctx, I), true);
    }
    EndObj(I == (NSegments - 1));
  }

  EndArr(true);
  EndObj(true);
  return true;
}

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

bool loadWAV(Span<const uint8_t> Buf, std::vector<float> &PCMF32,
             std::vector<std::vector<float>> &PCMF32s, bool Stereo) {
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
  if (Stereo) {
    PCMF32s.resize(2);

    PCMF32s[0].resize(N);
    PCMF32s[1].resize(N);
    for (uint64_t I = 0; I < N; I++) {
      PCMF32s[0][I] = float(PCM16[2 * I]) / 32768.0f;
      PCMF32s[1][I] = float(PCM16[2 * I + 1]) / 32768.0f;
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

  std::string Speaker = "";
  // Output the last new N segments.
  for (int I = SegN - NewN; I < SegN; I++) {
    int64_t T0 = 0;
    int64_t T1 = 0;
    if (!CxtRef.WhisperConfig.NoTimestamps) {
      T0 = whisper_full_get_segment_t0(WhisperCtx, I);
      T1 = whisper_full_get_segment_t1(WhisperCtx, I);
      CxtRef.Outputs += "[";
      CxtRef.Outputs += toTimestamp(T0, false);
      CxtRef.Outputs += " --> ";
      CxtRef.Outputs += toTimestamp(T1, false);
      CxtRef.Outputs += "] ";
    }
    if (CxtRef.WhisperConfig.Diarize && CxtRef.InputPCMs.size() == 2) {
      Speaker = estimateDiarizationSpeaker(CxtRef.InputPCMs, T0, T1);
    }
    CxtRef.Outputs += Speaker + whisper_full_get_segment_text(WhisperCtx, I);
    if (!CxtRef.WhisperConfig.NoTimestamps || CxtRef.WhisperConfig.Diarize) {
      CxtRef.Outputs += "\n";
    }
  }
}

void setWhisperParams(Context &CxtRef) noexcept {
  auto &WParam = CxtRef.WhisperParams;
  auto &ConfigRef = CxtRef.WhisperConfig;
  WParam.n_threads = ConfigRef.ThreadsNum;
  WParam.n_max_text_ctx = ConfigRef.MaxTokenContext;
  WParam.offset_ms = ConfigRef.TimeOffsetMS;
  WParam.duration_ms = ConfigRef.DurationMS;
  WParam.print_progress = false;
  WParam.thold_pt = ConfigRef.WordThreshold;
  WParam.max_len = ConfigRef.MaxSegmentLength;
  WParam.token_timestamps = (WParam.max_len > 0);
  WParam.split_on_word = ConfigRef.SplitOnWord;
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
  WParam.greedy.best_of = ConfigRef.BestOf;
  WParam.print_timestamps = !ConfigRef.NoTimestamps;
  WParam.no_timestamps = ConfigRef.NoTimestamps;
  WParam.audio_ctx = ConfigRef.AudioCtx;
  WParam.strategy =
      (ConfigRef.BeamSize > 1)
          ? whisper_sampling_strategy::WHISPER_SAMPLING_BEAM_SEARCH
          : whisper_sampling_strategy::WHISPER_SAMPLING_GREEDY;
  WParam.beam_search.beam_size = ConfigRef.BeamSize;

  if (ConfigRef.EnableDebugLog) {
    spdlog::info("[WASI-NN][Debug] Whisper backend: Config: threads: {}"sv,
                 ConfigRef.ThreadsNum);
    spdlog::info("[WASI-NN][Debug] Whisper backend: Config: processors: {}"sv,
                 ConfigRef.ProcessorsNum);
    spdlog::info("[WASI-NN][Debug] Whisper backend: Config: max-context: {}"sv,
                 ConfigRef.MaxTokenContext);
    spdlog::info("[WASI-NN][Debug] Whisper backend: Config: offset-t: {}"sv,
                 ConfigRef.TimeOffsetMS);
    spdlog::info("[WASI-NN][Debug] Whisper backend: Config: duration: {}"sv,
                 ConfigRef.DurationMS);
    spdlog::info("[WASI-NN][Debug] Whisper backend: Config: max-len: {}"sv,
                 ConfigRef.MaxSegmentLength);
    spdlog::info(
        "[WASI-NN][Debug] Whisper backend: Config: split-on-word : {}"sv,
        ConfigRef.SplitOnWord);
    spdlog::info("[WASI-NN][Debug] Whisper backend: Config: translate: {}"sv,
                 ConfigRef.Translate);
    spdlog::info("[WASI-NN][Debug] Whisper backend: Config: language: \"{}\""sv,
                 ConfigRef.SpokenLanguage);
    spdlog::info(
        "[WASI-NN][Debug] Whisper backend: Config: detect-language: {}"sv,
        ConfigRef.DetectLanguage);
    spdlog::info("[WASI-NN][Debug] Whisper backend: Config: temperature: {}"sv,
                 ConfigRef.Temperature);
    spdlog::info("[WASI-NN][Debug] Whisper backend: Config: prompt: \"{}\""sv,
                 ConfigRef.InitialPrompt);
  }
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

  auto PrintParsedOption = [&](std::string_view Name, const auto &Val) {
    if (ConfigRef.EnableDebugLog) {
      spdlog::info(
          "[WASI-NN][Debug] Whisper backend: Parsed metadata -- {}:{}"sv, Name,
          Val);
    }
  };

  // Get metadata from the json.
  // Currently supported metadata:
  // Plugin parameters (used by this plugin):
  //   enable-log: bool
  //   enable-debug-log: bool
  //   threads: uint32_t
  //   processors: uint32_t
  //   offset-t: uint32_t
  //   duration: uint32_t
  //   max-context: uint32_t
  //   max-len: uint32_t
  //   split-on-word: bool
  //   translate: bool
  //   language: string
  //   detect-language: bool
  //   temperature: float
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
  if (Doc.at_key("threads").error() == simdjson::SUCCESS) {
    auto Err = Doc["threads"].get<uint64_t>().get(ConfigRef.ThreadsNum);
    if (Err) {
      spdlog::error(
          "[WASI-NN] Whisper backend: Unable to retrieve the threads option."sv);
      return ErrNo::InvalidArgument;
    }
    PrintParsedOption("threads"sv, ConfigRef.ThreadsNum);
  }
  if (Doc.at_key("processors").error() == simdjson::SUCCESS) {
    auto Err = Doc["processors"].get<uint64_t>().get(ConfigRef.ProcessorsNum);
    if (Err) {
      spdlog::error(
          "[WASI-NN] Whisper backend: Unable to retrieve the processors option."sv);
      return ErrNo::InvalidArgument;
    }
    PrintParsedOption("processors"sv, ConfigRef.ProcessorsNum);
  }
  if (Doc.at_key("offset-t").error() == simdjson::SUCCESS) {
    auto Err = Doc["offset-t"].get<uint64_t>().get(ConfigRef.TimeOffsetMS);
    if (Err) {
      spdlog::error(
          "[WASI-NN] Whisper backend: Unable to retrieve the offset-t option."sv);
      return ErrNo::InvalidArgument;
    }
    PrintParsedOption("offset-t"sv, ConfigRef.TimeOffsetMS);
  }
  if (Doc.at_key("duration").error() == simdjson::SUCCESS) {
    auto Err = Doc["duration"].get<uint64_t>().get(ConfigRef.DurationMS);
    if (Err) {
      spdlog::error(
          "[WASI-NN] Whisper backend: Unable to retrieve the duration option."sv);
      return ErrNo::InvalidArgument;
    }
    PrintParsedOption("duration"sv, ConfigRef.DurationMS);
  }
  if (Doc.at_key("max-context").error() == simdjson::SUCCESS) {
    int64_t MaxContext = 0;
    auto Err = Doc["max-context"].get<int64_t>().get(MaxContext);
    if (Err) {
      spdlog::error(
          "[WASI-NN] Whisper backend: Unable to retrieve the max-context option."sv);
      return ErrNo::InvalidArgument;
    }
    if (MaxContext >= 0) {
      ConfigRef.MaxTokenContext = static_cast<uint64_t>(MaxContext);
      PrintParsedOption("max-context"sv, ConfigRef.MaxTokenContext);
    }
  }
  if (Doc.at_key("max-len").error() == simdjson::SUCCESS) {
    auto Err = Doc["max-len"].get<uint64_t>().get(ConfigRef.MaxSegmentLength);
    if (Err) {
      spdlog::error(
          "[WASI-NN] Whisper backend: Unable to retrieve the max-len option."sv);
      return ErrNo::InvalidArgument;
    }
    PrintParsedOption("max-len"sv, ConfigRef.MaxSegmentLength);
  }
  if (Doc.at_key("split-on-word").error() == simdjson::SUCCESS) {
    auto Err = Doc["split-on-word"].get<bool>().get(ConfigRef.SplitOnWord);
    if (Err) {
      spdlog::error(
          "[WASI-NN] Whisper backend: Unable to retrieve the split-on-word "
          "option."sv);
      return ErrNo::InvalidArgument;
    }
    PrintParsedOption("split-on-word"sv, ConfigRef.SplitOnWord);
  }
  if (Doc.at_key("translate").error() == simdjson::SUCCESS) {
    auto Err = Doc["translate"].get<bool>().get(ConfigRef.Translate);
    if (Err) {
      spdlog::error(
          "[WASI-NN] Whisper backend: Unable to retrieve the translate "
          "option."sv);
      return ErrNo::InvalidArgument;
    }
    PrintParsedOption("translate"sv, ConfigRef.Translate);
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
    PrintParsedOption("language"sv, ConfigRef.SpokenLanguage);
  }
  if (Doc.at_key("detect-language").error() == simdjson::SUCCESS) {
    auto Err = Doc["detect-language"].get<bool>().get(ConfigRef.DetectLanguage);
    if (Err) {
      spdlog::error(
          "[WASI-NN] Whisper backend: Unable to retrieve the detect-language "
          "option."sv);
      return ErrNo::InvalidArgument;
    }
    PrintParsedOption("detect-language"sv, ConfigRef.DetectLanguage);
  }
  if (Doc.at_key("temperature").error() == simdjson::SUCCESS) {
    double Temperature;
    auto Err = Doc["temperature"].get<double>().get(Temperature);
    if (Err) {
      spdlog::error(
          "[WASI-NN] Whisper backend: Unable to retrieve the temperature option."sv);
      return ErrNo::InvalidArgument;
    }
    ConfigRef.Temperature = static_cast<float>(Temperature);
    PrintParsedOption("temperature"sv, ConfigRef.Temperature);
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
    PrintParsedOption("prompt"sv, ConfigRef.InitialPrompt);
  }
  if (Doc.at_key("best-of").error() == simdjson::SUCCESS) {
    auto Err = Doc["best-of"].get<uint64_t>().get(ConfigRef.BestOf);
    if (Err) {
      spdlog::error(
          "[WASI-NN] Whisper backend: Unable to retrieve the best-of option."sv);
      return ErrNo::InvalidArgument;
    }
    PrintParsedOption("best-of"sv, ConfigRef.BestOf);
  }
  if (Doc.at_key("beam-size").error() == simdjson::SUCCESS) {
    auto Err = Doc["beam-size"].get<uint64_t>().get(ConfigRef.BeamSize);
    if (Err) {
      spdlog::error(
          "[WASI-NN] Whisper backend: Unable to retrieve the beam-size option."sv);
      return ErrNo::InvalidArgument;
    }
    PrintParsedOption("beam-size"sv, ConfigRef.BeamSize);
  }
  if (Doc.at_key("output-srt").error() == simdjson::SUCCESS) {
    auto Err = Doc["output-srt"].get<bool>().get(ConfigRef.OutputSrt);
    if (Err) {
      spdlog::error(
          "[WASI-NN] Whisper backend: Unable to retrieve the output-srt "
          "option."sv);
      return ErrNo::InvalidArgument;
    }
    PrintParsedOption("output-srt"sv, ConfigRef.OutputLrc);
  }
  if (Doc.at_key("output-lrc").error() == simdjson::SUCCESS) {
    auto Err = Doc["output-lrc"].get<bool>().get(ConfigRef.OutputLrc);
    if (Err) {
      spdlog::error(
          "[WASI-NN] Whisper backend: Unable to retrieve the output-lrc"
          "option."sv);
      return ErrNo::InvalidArgument;
    }
    PrintParsedOption("output-lrc"sv, ConfigRef.OutputLrc);
  }
  if (Doc.at_key("output-json").error() == simdjson::SUCCESS) {
    auto Err = Doc["output-json"].get<bool>().get(ConfigRef.OutputJson);
    if (Err) {
      spdlog::error(
          "[WASI-NN] Whisper backend: Unable to retrieve the output-json "
          "option."sv);
      return ErrNo::InvalidArgument;
    }
    PrintParsedOption("output-json"sv, ConfigRef.OutputJson);
  }
  if (Doc.at_key("output-json-full").error() == simdjson::SUCCESS) {
    auto Err =
        Doc["output-json-full"].get<bool>().get(ConfigRef.OutputJsonFull);
    if (Err) {
      spdlog::error(
          "[WASI-NN] Whisper backend: Unable to retrieve the output-json-full "
          "option."sv);
      return ErrNo::InvalidArgument;
    }
    PrintParsedOption("output-json-full"sv, ConfigRef.OutputJsonFull);
  }
  if (Doc.at_key("no-timestamps").error() == simdjson::SUCCESS) {
    auto Err = Doc["no-timestamps"].get<bool>().get(ConfigRef.NoTimestamps);
    if (Err) {
      spdlog::error(
          "[WASI-NN] Whisper backend: Unable to retrieve the no-timestamps "
          "option."sv);
      return ErrNo::InvalidArgument;
    }
    PrintParsedOption("no-timestamps"sv, ConfigRef.NoTimestamps);
  }
  if (Doc.at_key("output-file").error() == simdjson::SUCCESS) {
    std::string_view FileName;
    auto Err = Doc["output-file"].get<std::string_view>().get(FileName);
    if (Err) {
      spdlog::error(
          "[WASI-NN] Whisper backend: Unable to retrieve the output file"
          "option."sv);
      return ErrNo::InvalidArgument;
    }
    ConfigRef.FileName = FileName;
    PrintParsedOption("output-file"sv, ConfigRef.FileName);
  }
  if (Doc.at_key("audio-ctx").error() == simdjson::SUCCESS) {
    auto Err = Doc["audio-ctx"].get<uint64_t>().get(ConfigRef.AudioCtx);
    if (Err) {
      spdlog::error(
          "[WASI-NN] Whisper backend: Unable to retrieve the audio-ctx "
          "option."sv);
      return ErrNo::InvalidArgument;
    }
    PrintParsedOption("audio-ctx"sv, ConfigRef.AudioCtx);
  }
  if (Doc.at_key("diarize").error() == simdjson::SUCCESS) {
    auto Err = Doc["diarize"].get<bool>().get(ConfigRef.Diarize);
    if (Err) {
      spdlog::error("[WASI-NN] Whisper backend: Unable to retrieve the diarize "
                    "option."sv);
      return ErrNo::InvalidArgument;
    }
    PrintParsedOption("diarize"sv, ConfigRef.Diarize);
  }
  if (Doc.at_key("offset-n").error() == simdjson::SUCCESS) {
    auto Err = Doc["offset-n"].get<uint64_t>().get(ConfigRef.OffsetN);
    if (Err) {
      spdlog::error(
          "[WASI-NN] Whisper backend: Unable to retrieve the offset-n "
          "option."sv);
      return ErrNo::InvalidArgument;
    }
    PrintParsedOption("offset-n"sv, ConfigRef.OffsetN);
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

} // Namespace

Expect<ErrNo> load(WasiNNEnvironment &Env, Span<const Span<uint8_t>> Builders,
                   [[maybe_unused]] Device Device, uint32_t &GraphId) noexcept {
  // Add a new graph.
  uint32_t GId = Env.newGraph(Backend::Whisper);
  auto &GraphRef = Env.NNGraph[GId].get<Graph>();

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
      Env.deleteGraph(GId);
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
    Env.deleteGraph(GId);
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
    Env.deleteGraph(GId);
    return ResTranslateConfig;
  }

  // Store the loaded graph.
  GraphId = GId;
  Env.NNGraph[GId].setReady();

  return ErrNo::Success;
}

Expect<ErrNo> initExecCtx(WasiNNEnvironment &Env, uint32_t GraphId,
                          uint32_t &ContextId) noexcept {
  auto &GraphRef = Env.NNGraph[GraphId].get<Graph>();
  if (GraphRef.WhisperConfig.EnableDebugLog) {
    spdlog::info("[WASI-NN][Debug] Whisper backend: initExecCtx"sv);
  }
  ContextId = Env.newContext(GraphId, Env.NNGraph[GraphId]);
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  CxtRef.WhisperParams = whisper_full_default_params(
      whisper_sampling_strategy::WHISPER_SAMPLING_BEAM_SEARCH);
  setWhisperParams(CxtRef);
  if (GraphRef.WhisperConfig.EnableLog) {
    spdlog::info("[WASI-NN] Whisper backend: whisper_system_info: {}"sv,
                 whisper_print_system_info());
  }
  Env.NNContext[ContextId].setReady();
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
    // Set the whisper config of this context as the graph default first.
    // This will reset the config and inherit settings from the graph metadata.
    auto &GraphRef = Env.NNGraph[CxtRef.GraphId].get<Graph>();
    CxtRef.WhisperConfig = GraphRef.WhisperConfig;
    const std::string Metadata(reinterpret_cast<char *>(Tensor.Tensor.data()),
                               Tensor.Tensor.size());
    auto Res = parseMetadata(CxtRef.WhisperConfig, Metadata);
    if (Res != ErrNo::Success) {
      spdlog::error("[WASI-NN] Whisper backend: Failed to parse metadata."sv);
      return Res;
    }
    Res = handleTranslationConfig(GraphRef.WhisperCtx, CxtRef.WhisperConfig);
    if (Res != ErrNo::Success) {
      return Res;
    }
    setWhisperParams(CxtRef);
    if (CxtRef.WhisperConfig.EnableDebugLog) {
      spdlog::info("[WASI-NN][Debug] Whisper backend: found Metadata, "
                   "processing...Done"sv);
    }
    return ErrNo::Success;
  }

  if (Tensor.Dimension.size() != 2) {
    spdlog::error("[WASI-NN] Tensor dimension is out of range, expect 2-dim, "
                  "but got {}-dim."sv,
                  Tensor.Dimension.size());
    return WASINN::ErrNo::InvalidArgument;
  }
  if (Tensor.Dimension[0] != 1) {
    spdlog::error("[WASI-NN] Only 1 channel supported for now."sv);
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
  if (!loadWAV(Tensor.Tensor, CxtRef.InputPCM, CxtRef.InputPCMs,
               CxtRef.WhisperConfig.Diarize)) {
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
  auto &GraphRef = Env.NNGraph[CxtRef.GraphId].get<Graph>();
  if (CxtRef.WhisperConfig.EnableDebugLog) {
    spdlog::info("[WASI-NN][Debug] Whisper backend: getOutput with Index {}"sv,
                 Index);
  }

  // Check out buffer max size.
  if (OutBuffer.size() < CxtRef.Outputs.length()) {
    spdlog::error("[WASI-NN] Expect out buffer max size {}, but got {}"sv,
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

  if (CxtRef.WhisperConfig.OutputSrt) {
    const auto Fname = CxtRef.WhisperConfig.FileName + ".srt";
    outputSrt(Env, GraphRef.WhisperCtx, Fname, CxtRef.WhisperConfig,
              CxtRef.InputPCMs);
  }

  if (CxtRef.WhisperConfig.OutputLrc) {
    const auto Fname = CxtRef.WhisperConfig.FileName + ".lrc";
    outputLrc(Env, GraphRef.WhisperCtx, Fname, CxtRef.WhisperConfig,
              CxtRef.InputPCMs);
  }

  if (CxtRef.WhisperConfig.OutputJson) {
    const auto Fname = CxtRef.WhisperConfig.FileName + ".json";
    outputJson(Env, GraphRef.WhisperCtx, Fname, CxtRef.WhisperConfig,
               CxtRef.InputPCMs, CxtRef.WhisperConfig.OutputJsonFull);
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
  if (whisper_full_parallel(GraphRef.WhisperCtx, CxtRef.WhisperParams,
                            CxtRef.InputPCM.data(), CxtRef.InputPCM.size(),
                            CxtRef.WhisperConfig.ProcessorsNum) != 0) {
    spdlog::error(
        "[WASI-NN] Whisper backend: Error: failed to process audio."sv);
    return ErrNo::RuntimeError;
  }

  if (CxtRef.WhisperConfig.EnableDebugLog) {
    spdlog::info("[WASI-NN][Debug] Whisper backend: compute...Done"sv);
  }
  return ErrNo::Success;
}

Expect<ErrNo> unload(WasiNNEnvironment &Env, uint32_t GraphId) noexcept {
  auto &GraphRef = Env.NNGraph[GraphId].get<Graph>();
  const bool IsDebugLog = GraphRef.WhisperConfig.EnableDebugLog;
  if (IsDebugLog) {
    spdlog::info("[WASI-NN][Debug] Whisper backend: unload"sv);
  }
  if (GraphRef.WhisperCtx != nullptr) {
    if (IsDebugLog) {
      spdlog::info(
          "[WASI-NN][Debug] Whisper backend: unload: free whisper context"sv);
    }
    whisper_free(GraphRef.WhisperCtx);
    GraphRef.WhisperCtx = nullptr;
    if (IsDebugLog) {
      spdlog::info(
          "[WASI-NN][Debug] Whisper backend: unload: free whisper context...Done"sv);
    }
  }
  Env.deleteGraph(GraphId);
  Env.mdRemoveById(GraphId);
  if (IsDebugLog) {
    spdlog::info("[WASI-NN][Debug] Whisper backend: unload...Done"sv);
  }
  return ErrNo::Success;
}

Expect<ErrNo> finalizeExecCtx(WasiNNEnvironment &Env,
                              uint32_t ContextId) noexcept {
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  const bool IsDebugLog = CxtRef.WhisperConfig.EnableDebugLog;
  if (IsDebugLog) {
    spdlog::info(
        "[WASI-NN][Debug] Whisper backend: finalize_execution_context"sv);
  }
  // TODO: Free resources
  Env.deleteContext(ContextId);
  if (IsDebugLog) {
    spdlog::info(
        "[WASI-NN][Debug] Whisper backend: finalize_execution_context...Done"sv);
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
} // Namespace

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
Expect<ErrNo> unload(WasiNNEnvironment &, uint32_t) noexcept {
  return reportBackendNotSupported();
}
Expect<ErrNo> finalizeExecCtx(WasiNNEnvironment &, uint32_t) noexcept {
  return reportBackendNotSupported();
}

#endif
} // Namespace WasmEdge::Host::WASINN::Whisper
