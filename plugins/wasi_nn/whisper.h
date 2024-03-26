// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#pragma once

#include "plugin/plugin.h"
#include "types.h"

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_WHISPER
#include <common.h>
#include <whisper.h>
#endif

namespace WasmEdge::Host::WASINN {
struct WasiNNEnvironment;
}

namespace WasmEdge::Host::WASINN::WHISPER {
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_WHISPER
struct Graph {
  whisper_model *WhisperModel = nullptr;
  std::string ModelFilePath;
  // Plugin parameters:
  bool EnableLog = false;
  bool EnableDebugLog = false;
  bool StreamStdout = false;
  // Context parameters:
  // bool UseGPU = true;
  // int64_t GPUDevice = 0;
  // // DTW parameters[Experimental]:
  // bool DTWTokenTimestamps = false;
  // int DTWAheadsPreset = 0;
  // int DTWNTop = -1;
  // size_t DTWMemorySize = 1024 * 1024 * 128;
  // //decoding parameters
  // float temperature = 0.0f;
  // bool translate=True;
  // std::string language= “en”;
  // float suppress_tokens= -1.0f;
  // float logprob_threshold= -1.0f;
  // float no_speech_threshold= 0.6f;
  // bool condition_on_previous_text= true;
  // float compression_ratio_threshold= 2.4;
  // float temperature_increment_on_fallback= 0.2f;
};
struct Context {
  Context(size_t, Graph &)
};
// struct WhisperContext {
//     WhisperContext(size_t GId, Graph &) noexcept : GraphId(GId) {}
//     size_t GraphId;
//     std::vector<std::string> WhisperInputs;
//     uint64_t WhisperNInputs = 0;
//     std::string WhisperOutputs;
//     std::vector<std::string> WhisperOutputSegments;
// };

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

} // namespace WasmEdge::Host::WASINN::WHISPER