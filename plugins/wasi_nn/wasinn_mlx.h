// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "wasinntypes.h"

#include "plugin/plugin.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_MLX
#include "MLX/mlx/base.h"
#include "MLX/mlx/transformer.h"
#include "MLX/model/llm/transformer.h"
#include "MLX/prompt/prompt.h"
#include <model/whisper_transcribe.h>

#include <mlx/mlx.h>
#include <tokenizers_cpp.h>
#endif

namespace WasmEdge::Host::WASINN {
struct WasiNNEnvironment;
class Graph;
class Context;
} // namespace WasmEdge::Host::WASINN

namespace WasmEdge::Host::WASINN::MLX {

// A raw-bytes load materializes each model builder as a temporary safetensors
// file before conversion. The path stays relative so it resolves under the
// same WASI preopen the writer and the reader share, and a process-wide serial
// keeps concurrent builds from colliding on one file.
inline std::string uniqueModelFileName(std::size_t Idx) noexcept {
  static std::atomic<std::uint64_t> Serial{0};
  const std::uint64_t Ticket = Serial.fetch_add(1, std::memory_order_relaxed);
  return "MLX" + std::to_string(Ticket) + "-" + std::to_string(Idx) +
         ".safetensors";
}

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_MLX
struct LLMInput {
  std::string Prompt = {};
};
struct LLMOutput {
  std::string Answer = {};
};
struct VLMInput {
  mx::array Prompt = mx::array({});
  mx::array Pixel = mx::array({});
  mx::array Mask = mx::array({});
};
struct VLMOutput {
  mx::array Answer = mx::array({});
};
struct WhisperInput {
  mx::array Audio = mx::array({});
};
struct Graph {
  std::string ModelType;
  std::string ModelArch;
  std::unique_ptr<tokenizers::Tokenizer> Tok = nullptr;
  std::shared_ptr<nn::Module> Model;
  double Temp = 0.0;
  bool EnableDebugLog = false;
  bool IsQuantized = false;
  uint64_t MaxToken = 1024;
  uint64_t QBits = 0;
  uint64_t GroupSize = 0;
  BasePrompt Prmopt;
};
struct Context {
  Context(uint32_t Gid, Graph &) noexcept : GraphId(Gid) {}
  uint32_t GraphId;
  std::variant<LLMInput, VLMInput, WhisperInput> Inputs;
  std::variant<LLMOutput, VLMOutput, whisper::TranscribeResult> Outputs;
};
#else
struct Graph {};
struct Context {
  Context(uint32_t, Graph &) noexcept {}
};
#endif

struct Environ {};

Expect<WASINN::ErrNo> load(WASINN::WasiNNEnvironment &Env, WASINN::Graph &G,
                           Span<const Span<uint8_t>> Builders,
                           WASINN::Device Device) noexcept;
Expect<WASINN::ErrNo> initExecCtx(WASINN::WasiNNEnvironment &Env,
                                  WASINN::Graph &G,
                                  WASINN::Context &C) noexcept;
Expect<WASINN::ErrNo> setInput(WASINN::WasiNNEnvironment &Env, WASINN::Graph &G,
                               WASINN::Context &C, uint32_t Index,
                               const TensorData &Tensor) noexcept;
Expect<WASINN::ErrNo> getOutput(WASINN::WasiNNEnvironment &Env,
                                WASINN::Graph &G, WASINN::Context &C,
                                uint32_t Index, Span<uint8_t> OutBuffer,
                                uint32_t &BytesWritten) noexcept;
Expect<WASINN::ErrNo> compute(WASINN::WasiNNEnvironment &Env, WASINN::Graph &G,
                              WASINN::Context &C) noexcept;
} // namespace WasmEdge::Host::WASINN::MLX
