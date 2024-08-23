#pragma once

#include "plugin/plugin.h"
#include "types.h"

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_MLX
#include "MLX/model/transformer.h"
#include "llama.h"
#include "transformer.h"
#include <mlx/mlx.h>
#include <tokenizers_cpp.h>
#endif

namespace WasmEdge::Host::WASINN {
struct WasiNNEnvironment;
}

namespace WasmEdge::Host::WASINN::MLX {
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_MLX
struct Graph {
  std::string ModelType = "tiny_llama_1.1B_chat_v1.0";
  std::unique_ptr<tokenizers::Tokenizer> Tok;
  Transformer *Model;
  inline static int GraphNumber = 0;
  double Temp = 0.0;
  bool EnableDebugLog = true;
  int MaxToken = 1024;
  BasePrompt Prmopt;
};
struct Context {
  Context(size_t Gid, Graph &) noexcept : GraphId(Gid) {}
  size_t GraphId;
  std::string Inputs;
  std::string Outputs;
};
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