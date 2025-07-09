#pragma once

#include "wasinntypes.h"
#include "plugin/plugin.h"

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_BITNET
#include <llama.h>
#include <common.h>
#include <string>
#include <vector>
#include <memory>
#endif

namespace WasmEdge::Host::WASINN {
struct WasiNNEnvironment;
}

namespace WasmEdge::Host::WASINN::BitNet {

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_BITNET

struct LlamaModelDeleter {
  void operator()(llama_model *ptr) const {
    if (ptr)
      llama_free_model(ptr);
  }
};
struct LlamaContextDeleter {
  void operator()(llama_context *ptr) const {
    if (ptr)
      llama_free(ptr);
  }
};
struct LlamaSamplerDeleter {
  void operator()(llama_sampler *ptr) const {
    if (ptr)
      llama_sampler_free(ptr);
  }
};

struct LlamaBatchDeleter {
    void operator()(llama_batch* ptr) const {
        if (ptr) {
            llama_batch_free(*ptr);
            delete ptr;
        }
    }
};

using llama_model_ptr = std::unique_ptr<llama_model, LlamaModelDeleter>;
using llama_context_ptr = std::unique_ptr<llama_context, LlamaContextDeleter>;
using llama_sampler_ptr = std::unique_ptr<llama_sampler, LlamaSamplerDeleter>;
using llama_batch_ptr = std::unique_ptr<llama_batch, LlamaBatchDeleter>;


struct Graph {
  bool EnableLog = false;
  llama_model_params MParams;
  llama_context_params CParams;
  llama_model_ptr LlamaModel = nullptr;
  llama_context_ptr LlamaContext = nullptr;
};


struct Context {
  const uint32_t GraphId;
  Graph &GraphRef;


  int32_t NPos = 0;
  std::vector<llama_token> LlamaInputs;
  std::vector<llama_token> LlamaOutputTokens;
  
  
  llama_sampler_ptr LlamaSampler = nullptr;
  llama_batch_ptr LlamaBatch = nullptr;

  
  uint32_t NPredict = 512;

  Context(uint32_t GId, Graph &G) noexcept : GraphId(GId), GraphRef(G) {
  }
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

Expect<WASINN::ErrNo> finalizeExecCtx(WASINN::WasiNNEnvironment &Env,
                                      uint32_t ContextId) noexcept;

Expect<WASINN::ErrNo> unload(WASINN::WasiNNEnvironment &Env,
                                      uint32_t GraphId) noexcept;
        

} // namespace WasmEdge::Host::WASINN::BitNet