// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "whispercpp.h"
#include "wasinnenv.h"

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_WHISPER
#define DR_WAV_IMPLEMENTATION
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
  if (!GraphRef.EnableLog) {
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

} // namespace

Expect<ErrNo> load(WasiNNEnvironment &Env, Span<const Span<uint8_t>> Builders,
                   [[maybe_unused]] Device Device, uint32_t &GraphId) noexcept {
  // Add a new graph.
  Env.NNGraph.emplace_back(Backend::Whisper);
  auto &GraphRef = Env.NNGraph.back().get<Graph>();

  // Initialize the parameters.
  auto CParam = whisper_context_default_params();
  GraphRef.EnableLog = false;
  GraphRef.EnableDebugLog = false;
  GraphRef.UseGPU = CParam.use_gpu;
  GraphRef.MainGPU = CParam.gpu_device;
  GraphRef.ModelFilePath = ""sv;
  GraphRef.ModelLanguage = "en"sv;

  // Set whisper log callback.
  whisper_log_set(WhisperLogCallback, &GraphRef);

  // TODO: Use the metadata to pass data.

  // Handle the model path.
  if (GraphRef.EnableDebugLog) {
    spdlog::info("[WASI-NN][Debug] Whisper backend: Handling model path."sv);
  }
  auto Weight = Builders[0];
  const std::string_view BinModel(reinterpret_cast<char *>(Weight.data()),
                                  Weight.size());
  if (BinModel.substr(0, 8) == "preload:"sv) {
    GraphRef.ModelFilePath = BinModel.substr(8);
  }

  // Initialize whisper context from model file with parameters.
  if (GraphRef.EnableDebugLog) {
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
  if (GraphRef.EnableDebugLog) {
    spdlog::info(
        "[WASI-NN][Debug] Whisper backend: Initialize whisper context with "
        "given parameters...Done"sv);
  }

  // Store the loaded graph.
  GraphId = Env.NNGraph.size() - 1;

  return ErrNo::Success;
}

Expect<ErrNo> initExecCtx(WasiNNEnvironment &Env, uint32_t GraphId,
                          uint32_t &ContextId) noexcept {
  auto &GraphRef = Env.NNGraph[GraphId].get<Graph>();
  if (GraphRef.EnableDebugLog) {
    spdlog::info("[WASI-NN][Debug] Whisper backend: initExecCtx"sv);
  }
  Env.NNContext.emplace_back(GraphId, Env.NNGraph[GraphId]);
  ContextId = Env.NNContext.size() - 1;
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  auto &WParam = CxtRef.WhisperParams;
  WParam.print_progress = false;
  WParam.thold_pt = GraphRef.WordThreshold;
  WParam.language = GraphRef.ModelLanguage.c_str();
  WParam.temperature_inc = GraphRef.TemperatureInc;
  WParam.temperature = GraphRef.Temperature;
  WParam.entropy_thold = GraphRef.EntropyThreshold;
  WParam.logprob_thold = GraphRef.LogprobThreshold;
  WParam.grammar_penalty = GraphRef.GrammarPenalty;
  WParam.new_segment_callback = WhisperOutputSegmentCallback;
  WParam.new_segment_callback_user_data = &CxtRef;
  if (GraphRef.EnableLog) {
    spdlog::info("[WASI-NN] Whisper backend: whisper_system_info: {}"sv,
                 whisper_print_system_info());
  }
  if (GraphRef.EnableDebugLog) {
    spdlog::info("[WASI-NN][Debug] Whisper backend: initExecCtx...Done"sv);
  }
  return ErrNo::Success;
}

Expect<ErrNo> setInput(WasiNNEnvironment &Env, uint32_t ContextId,
                       uint32_t Index [[maybe_unused]],
                       const TensorData &Tensor) noexcept {
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  auto &GraphRef = Env.NNGraph[CxtRef.GraphId].get<Graph>();
  if (GraphRef.EnableDebugLog) {
    spdlog::info("[WASI-NN][Debug] Whisper backend: setInput"sv);
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

  if (GraphRef.EnableDebugLog) {
    spdlog::info("[WASI-NN][Debug] Whisper backend: setInput...Done"sv);
  }
  return ErrNo::Success;
}

Expect<ErrNo> getOutput(WasiNNEnvironment &Env, uint32_t ContextId,
                        uint32_t Index, Span<uint8_t> OutBuffer,
                        uint32_t &BytesWritten) noexcept {
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  auto &GraphRef = Env.NNGraph[CxtRef.GraphId].get<Graph>();
  if (GraphRef.EnableDebugLog) {
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
  if (GraphRef.EnableDebugLog) {
    spdlog::info(
        "[WASI-NN][Debug] Whisper backend: getOutput with Index {}...Done"sv,
        Index);
  }
  return ErrNo::Success;
}

Expect<ErrNo> compute(WasiNNEnvironment &Env, uint32_t ContextId) noexcept {
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  auto &GraphRef = Env.NNGraph[CxtRef.GraphId].get<Graph>();
  if (GraphRef.EnableDebugLog) {
    spdlog::info("[WASI-NN][Debug] Whisper backend: compute"sv);
  }

  CxtRef.Outputs.clear();
  if (whisper_full(GraphRef.WhisperCtx, CxtRef.WhisperParams,
                   CxtRef.InputPCM.data(), CxtRef.InputPCM.size()) != 0) {
    spdlog::error(
        "[WASI-NN] Whisper backend: Error: failed to process audio."sv);
    return ErrNo::RuntimeError;
  }

  if (GraphRef.EnableDebugLog) {
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
