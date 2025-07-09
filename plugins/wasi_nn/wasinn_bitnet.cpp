#include "wasinn_bitnet.h"
#include "wasinnenv.h"
#include "simdjson.h"

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_BITNET
#include <common.h>
#include <llama.h>
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#endif

namespace WasmEdge::Host::WASINN::BitNet {

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_BITNET


Expect<WASINN::ErrNo> load(WASINN::WasiNNEnvironment &Env,
                           Span<const Span<uint8_t>> Builders,
                           [[maybe_unused]] WASINN::Device Device,
                           uint32_t &GraphId) noexcept {
  if (Builders.empty()) {
    spdlog::error("[WASI-NN] BitNet: Expects at least one builder (the model).");
    return WASINN::ErrNo::InvalidArgument;
  }

  const uint32_t GId = Env.newGraph(Backend::BitNet);
  auto &GraphRef = Env.NNGraph[GId].get<Graph>();

 
  GraphRef.MParams = llama_model_default_params();
  GraphRef.CParams = llama_context_default_params();

 
  if (Builders.size() > 1) {
    const std::string Metadata(
        reinterpret_cast<const char *>(Builders[1].data()),
        Builders[1].size());
    simdjson::dom::parser Parser;
    simdjson::dom::element Doc;
    if (Parser.parse(Metadata).get(Doc)) {
      spdlog::error("[WASI-NN] BitNet: Failed to parse metadata JSON.");
      Env.deleteGraph(GId);
      return WASINN::ErrNo::InvalidArgument;
    }
    int64_t NGpuLayers;
    if (Doc["n-gpu-layers"].get(NGpuLayers) == simdjson::SUCCESS) {
      GraphRef.MParams.n_gpu_layers = static_cast<int32_t>(NGpuLayers);
    }
    uint64_t CtxSize;
    if (Doc["ctx-size"].get(CtxSize) == simdjson::SUCCESS) {
      GraphRef.CParams.n_ctx = static_cast<uint32_t>(CtxSize);
    }
    bool EnableLog;
    if (Doc["enable-log"].get(EnableLog) == simdjson::SUCCESS) {
      GraphRef.EnableLog = EnableLog;
    }
  }

  llama_backend_init();


  //// Handle the model path similar to ggml
  std::string ModelPath;
  const auto &Weight = Builders[0];
  const std::string_view ModelPathStr(reinterpret_cast<const char *>(Weight.data()),
                                      Weight.size());
                                      
  if (ModelPathStr.substr(0, 8) == "preload:"sv) {
    ModelPath = ModelPathStr.substr(8);
  } else {
    spdlog::warn("[WASI-NN] BitNet: Input doesn't have 'preload:' prefix. "
                 "Assuming raw model bytes and writing to 'bitnet-model.bin'.");
    ModelPath = "bitnet-model.bin";
    std::ofstream TempFile(ModelPath, std::ios::binary | std::ios::trunc);
    if (!TempFile) {
        spdlog::error("[WASI-NN] BitNet: Failed to create fallback model file '{}'.", ModelPath);
        Env.deleteGraph(GId);
        return WASINN::ErrNo::RuntimeError;
    }
    TempFile.write(reinterpret_cast<const char*>(Weight.data()), Weight.size());
    TempFile.close();
  }

  if (GraphRef.LlamaModel == nullptr) {
    spdlog::error("[WASI-NN] BitNet: Failed to load model.");
    Env.deleteGraph(GId);
    return WASINN::ErrNo::InvalidArgument;
  }

 
  if (GraphRef.CParams.n_ctx == 0) {
    GraphRef.CParams.n_ctx = llama_n_ctx_train(GraphRef.LlamaModel.get());
  }
  
  GraphRef.CParams.n_batch = std::min(GraphRef.CParams.n_ctx, (uint32_t)2048);

  GraphRef.LlamaContext.reset(llama_new_context_with_model(
      GraphRef.LlamaModel.get(), GraphRef.CParams));
  if (GraphRef.LlamaContext == nullptr) {
    spdlog::error("[WASI-NN] BitNet: Failed to create llama_context.");
    Env.deleteGraph(GId);
    return WASINN::ErrNo::InvalidArgument;
  }

  Env.NNGraph[GId].setReady();
  GraphId = GId;
  return WASINN::ErrNo::Success;
}

Expect<WASINN::ErrNo> initExecCtx(WASINN::WasiNNEnvironment &Env,
                                  uint32_t GraphId,
                                  uint32_t &ContextId) noexcept {
  ContextId = Env.newContext(GraphId, Env.NNGraph[GraphId]);
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  auto &GraphRef = CxtRef.GraphRef;

  //TODO: see if we can use common_sampler_init
  CxtRef.LlamaSampler.reset(llama_sampler_init_greedy());
  if (!CxtRef.LlamaSampler) {
    spdlog::error("[WASI-NN] BitNet: Failed to initialize sampler.");
    Env.deleteContext(ContextId);
    return WASINN::ErrNo::RuntimeError;
  }

  // Use a smart pointer for the batch
  auto NewBatch = new llama_batch(llama_batch_init(GraphRef.CParams.n_batch, 0, 1));
  CxtRef.LlamaBatch.reset(NewBatch);
  if (!CxtRef.LlamaBatch) {
    spdlog::error("[WASI-NN] BitNet: Failed to initialize llama_batch.");
    Env.deleteContext(ContextId);
    return WASINN::ErrNo::RuntimeError;
  }

  Env.NNContext[ContextId].setReady();
  return WASINN::ErrNo::Success;
}

Expect<WASINN::ErrNo> setInput(WASINN::WasiNNEnvironment &Env,
                               uint32_t ContextId, uint32_t Index,
                               const TensorData &Tensor) noexcept {
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  auto &GraphRef = CxtRef.GraphRef;

  if (Index != 0) {
    spdlog::error(
        "[WASI-NN] BitNet: Only one input tensor (the prompt) is supported at index 0.");
    return WASINN::ErrNo::InvalidArgument;
  }

  if (Tensor.RType != WASINN::TensorType::U8) {
    spdlog::error("[WASI-NN] BitNet: Input tensor must be a UTF-8 string (U8).");
    return WASINN::ErrNo::InvalidArgument;
  }

  // Reset state for a new generation sequence.
  llama_kv_cache_clear(GraphRef.LlamaContext.get());
  llama_sampler_reset(CxtRef.LlamaSampler.get());
  CxtRef.NPos = 0;
  CxtRef.LlamaInputs.clear();
  CxtRef.LlamaOutputTokens.clear();

  const std::string Prompt(reinterpret_cast<const char *>(Tensor.Tensor.data()),
                           Tensor.Tensor.size());

  
  CxtRef.LlamaInputs = common_tokenize(
      GraphRef.LlamaModel.get(), Prompt,
      llama_add_bos_token(GraphRef.LlamaModel.get()), false);

  if (static_cast<uint32_t>(CxtRef.LlamaInputs.size()) >=
      GraphRef.CParams.n_ctx) {
    spdlog::error(
        "[WASI-NN] BitNet: Prompt is too long ({} tokens), context size is {}.",
        CxtRef.LlamaInputs.size(), GraphRef.CParams.n_ctx);
    return WASINN::ErrNo::PromptTooLong;
  }

  return WASINN::ErrNo::Success;
}

Expect<WASINN::ErrNo> compute(WASINN::WasiNNEnvironment &Env,
                              uint32_t ContextId) noexcept {
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  auto &GraphRef = CxtRef.GraphRef;

  if (CxtRef.LlamaInputs.empty()) {
    spdlog::error("[WASI-NN] BitNet: Input tensor not set before compute.");
    return WASINN::ErrNo::InvalidArgument;
  }

  for (size_t I = 0; I < CxtRef.LlamaInputs.size();
       I += GraphRef.CParams.n_batch) {
    size_t NTokensToProcess = std::min(
        (size_t)GraphRef.CParams.n_batch, CxtRef.LlamaInputs.size() - I);


    auto &batch = *CxtRef.LlamaBatch;
    batch.n_tokens = 0;
    for (size_t J = 0; J < NTokensToProcess; ++J) {
      const auto n = batch.n_tokens;
      batch.token[n] = CxtRef.LlamaInputs[I + J];
      batch.pos[n] = CxtRef.NPos;
      batch.n_seq_id[n] = 1;
      batch.seq_id[n][0] = 0;
      batch.logits[n] = (I + J == CxtRef.LlamaInputs.size() - 1);
      batch.n_tokens++;
      CxtRef.NPos++;
    }

    if (llama_decode(GraphRef.LlamaContext.get(), batch) != 0) {
      spdlog::error(
          "[WASI-NN] BitNet: llama_decode failed during prompt processing.");
      return WASINN::ErrNo::RuntimeError;
    }
  }

  // Generation Loop
  llama_token NewTokenId = 0;
  for (uint32_t I = 0; I < CxtRef.NPredict; ++I) {
    NewTokenId = llama_sampler_sample(CxtRef.LlamaSampler.get(),
                                      GraphRef.LlamaContext.get(), -1);

    llama_sampler_accept(CxtRef.LlamaSampler.get(), NewTokenId);

    if (NewTokenId == llama_token_eos(GraphRef.LlamaModel.get())) {
      break;
    }

    CxtRef.LlamaOutputTokens.push_back(NewTokenId);

    // Prepare batch for the next token (batch size is 1)
    auto &batch = *CxtRef.LlamaBatch;
    batch.n_tokens = 0;
    batch.token[0] = NewTokenId;
    batch.pos[0] = CxtRef.NPos;
    batch.n_seq_id[0] = 1;
    batch.seq_id[0][0] = 0;
    batch.logits[0] = true;
    batch.n_tokens++;
    CxtRef.NPos++;

    if (llama_decode(GraphRef.LlamaContext.get(), batch) != 0) {
      spdlog::error(
          "[WASI-NN] BitNet: llama_decode failed during token generation.");
      return WASINN::ErrNo::RuntimeError;
    }
  }

  return WASINN::ErrNo::Success;
}

Expect<WASINN::ErrNo> getOutput(WASINN::WasiNNEnvironment &Env,
                                uint32_t ContextId, uint32_t Index,
                                Span<uint8_t> OutBuffer,
                                uint32_t &BytesWritten) noexcept {
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  auto &GraphRef = CxtRef.GraphRef;

  if (Index != 0) {
    spdlog::error(
        "[WASI-NN] BitNet: Only one output tensor (at index 0) is supported.");
    return WASINN::ErrNo::InvalidArgument;
  }

  // Detokenize the generated tokens into the output buffer
  int32_t NChars = llama_detokenize(
      GraphRef.LlamaModel.get(), CxtRef.LlamaOutputTokens.data(),
      CxtRef.LlamaOutputTokens.size(),
      reinterpret_cast<char *>(OutBuffer.data()), OutBuffer.size(),
      false, false);

  if (NChars < 0) {
    // If the buffer is too small, llama_detokenize returns the negative of the required size.
    BytesWritten = static_cast<uint32_t>(-NChars);
    return WASINN::ErrNo::Success;
  }

  BytesWritten = NChars;
  return WASINN::ErrNo::Success;
}

Expect<WASINN::ErrNo> finalizeExecCtx(WASINN::WasiNNEnvironment &Env,
                                      uint32_t ContextId) noexcept {
  Env.deleteContext(ContextId);
  return WASINN::ErrNo::Success;
}

Expect<WASINN::ErrNo> unload(WASINN::WasiNNEnvironment &Env, uint32_t GraphId) noexcept {
  Env.deleteGraph(GraphId);
  return WASINN::ErrNo::Success;
}

#else 
namespace {
Expect<ErrNo> reportBackendNotSupported() noexcept {
  spdlog::error("[WASI-NN] BitNet backend is not built. Please build with "
                "-DWASMEDGE_PLUGIN_WASI_NN_BACKEND_BITNET=ON."sv);
  return WASINN::ErrNo::InvalidArgument;
}
} // namespace

Expect<WASINN::ErrNo> load(WASINN::WasiNNEnvironment &,
                           Span<const Span<uint8_t>>, WASINN::Device,
                           uint32_t &) noexcept {
  return reportBackendNotSupported();
}
Expect<ErrNo> initExecCtx(WASINN::WasiNNEnvironment &, uint32_t,
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
Expect<WASINN::ErrNo> finalizeExecCtx(WASINN::WasiNNEnvironment &Env,
                                      uint32_t ContextId) noexcept {
  Env.deleteContext(ContextId);
  return reportBackendNotSupported();
}
Expect<WASINN::ErrNo> unload(WASINN::WasiNNEnvironment &Env, uint32_t GraphId) noexcept {
  Env.deleteGraph(GraphId);
  return WASINN::ErrNo::Success;
}

#endif 
} // namespace WasmEdge::Host::WASINN::BitNet