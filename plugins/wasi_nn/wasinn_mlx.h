// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "wasinntypes.h"

#include "plugin/plugin.h"

#include <memory>

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_MLX
#include "MLX/mlx/base.h"
#include "MLX/mlx/transformer.h"
#include "MLX/model/llm/transformer.h"
#include "MLX/prompt/prompt.h"

#include <mlx/mlx.h>
#include <tokenizers_cpp.h>
#endif

namespace WasmEdge::Host::WASINN {
struct WasiNNEnvironment;
}

namespace WasmEdge::Host::WASINN::MLX {
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
  std::variant<LLMInput, VLMInput> Inputs;
  std::variant<LLMOutput, VLMOutput> Outputs;
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
Expect<WASINN::ErrNo> unload(WASINN::WasiNNEnvironment &Env,
                             uint32_t GraphId) noexcept;
} // namespace WasmEdge::Host::WASINN::MLX
