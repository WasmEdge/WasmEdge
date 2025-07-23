#include "wasinn_bitnet.h"
#include "wasinnenv.h"
#include "simdjson.h"

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_BITNET
#include <common.h>
#include <llama.h>
#include <sampling.h>
#include <log.h>
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

namespace {
ErrNo parseMetadata(common_params &Params, bool &EnableLog, const std::string &Metadata) noexcept {
  simdjson::dom::parser Parser;
  simdjson::dom::element Doc;
  if (Parser.parse(Metadata).get(Doc)) {
    spdlog::error("[WASI-NN] BitNet: Failed to parse metadata JSON.");
    return WASINN::ErrNo::InvalidArgument;
  }

  if (Doc.at_key("enable-log").error() == simdjson::SUCCESS) {
      if(Doc["enable-log"].get<bool>().get(EnableLog) != simdjson::SUCCESS) {
        spdlog::warn("[WASI-NN] BitNet: Failed to parse 'enable-log', using default.");
      }
  }

  if (Doc.at_key("n-gpu-layers").error() == simdjson::SUCCESS) {
    int64_t Val;
    if (Doc["n-gpu-layers"].get(Val) == simdjson::SUCCESS) Params.n_gpu_layers = Val;
  }

  if (Doc.at_key("ctx-size").error() == simdjson::SUCCESS) {
    uint64_t Val;
    if (Doc["ctx-size"].get(Val) == simdjson::SUCCESS) Params.n_ctx = Val;
  }
  if (Doc.at_key("batch-size").error() == simdjson::SUCCESS) {
    uint64_t Val;
    if (Doc["batch-size"].get(Val) == simdjson::SUCCESS) Params.n_batch = Val;
  }

  // Sampling parameters
  auto &SParams = Params.sparams;
  if (Doc.at_key("n-predict").error() == simdjson::SUCCESS) {
    int64_t Val;
    if (Doc["n-predict"].get(Val) == simdjson::SUCCESS) Params.n_predict = Val;
  }
  if (Doc.at_key("temperature").error() == simdjson::SUCCESS) {
    double Val;
    if (Doc["temperature"].get(Val) == simdjson::SUCCESS) SParams.temp = Val;
  }
  if (Doc.at_key("top-k").error() == simdjson::SUCCESS) {
    int64_t Val;
    if (Doc["top-k"].get(Val) == simdjson::SUCCESS) SParams.top_k = Val;
  }
  if (Doc.at_key("top-p").error() == simdjson::SUCCESS) {
    double Val;
    if (Doc["top-p"].get(Val) == simdjson::SUCCESS) SParams.top_p = Val;
  }
  if (Doc.at_key("repeat-penalty").error() == simdjson::SUCCESS) {
    double Val;
    if (Doc["repeat-penalty"].get(Val) == simdjson::SUCCESS) SParams.penalty_repeat = Val;
  }
  if (Doc.at_key("repeat-last-n").error() == simdjson::SUCCESS) {
    int64_t Val;
    if (Doc["repeat-last-n"].get(Val) == simdjson::SUCCESS) SParams.penalty_last_n = Val;
  }
  if (Doc.at_key("penalize-nl").error() == simdjson::SUCCESS) {
    bool Val;
    if (Doc["penalize-nl"].get(Val) == simdjson::SUCCESS) SParams.penalize_nl = Val;
  }
  if (Doc.at_key("seed").error() == simdjson::SUCCESS) {
    uint64_t Val;
    if (Doc["seed"].get(Val) == simdjson::SUCCESS) SParams.seed = Val;
  }
  if (Doc.at_key("grammar").error() == simdjson::SUCCESS) {
    std::string_view Val;
    if (Doc["grammar"].get(Val) == simdjson::SUCCESS) SParams.grammar = Val;
  }

  return ErrNo::Success;
}

void llamaLogCallback(ggml_log_level, const char *LogText, void *) {
  std::string Text(LogText);
  Text = Text.erase(Text.find_last_not_of("\n") + 1);
  if (Text.empty() || Text == ".") return;
  spdlog::info("[WASI-NN] BitNet (llama.cpp): {}"sv, Text);
}
} // namespace

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
  
  GraphRef.Params = common_params{};
  bool EnableLog = false; 
  
  if (Builders.size() > 1) {
    const std::string Metadata(
        reinterpret_cast<const char *>(Builders[1].data()),
        Builders[1].size());
    if (auto Res = parseMetadata(GraphRef.Params, EnableLog, Metadata); Res != ErrNo::Success) {
        Env.deleteGraph(GId);
        return Res;
    }
  }

  if  (GraphRef.Params.cpuparams.n_threads <= 0) {
    GraphRef.Params.cpuparams.n_threads = std::thread::hardware_concurrency();
  }
  if (GraphRef.Params.cpuparams_batch.n_threads <= 0) {
    // Default batch threads to generation threads if not specified
    GraphRef.Params.cpuparams_batch.n_threads = GraphRef.Params.cpuparams.n_threads;
  }
  // Set up logging
  if (EnableLog) {
      llama_log_set(llamaLogCallback, nullptr);
  } else {
      common_log_pause(common_log_main());
  }
  
  const auto &Weight = Builders[0];
  const std::string_view ModelPathStr(reinterpret_cast<const char *>(Weight.data()),
                                      Weight.size());
                                      
  if (ModelPathStr.substr(0, 8) == "preload:"sv) {
    GraphRef.Params.model = ModelPathStr.substr(8);
  } else {
    spdlog::warn("[WASI-NN] BitNet: Input doesn't have 'preload:' prefix. "
                 "Assuming raw model bytes and writing to 'bitnet-model.bin'.");
    GraphRef.Params.model = "bitnet-model.bin";
    std::ofstream TempFile(GraphRef.Params.model, std::ios::binary | std::ios::trunc);
    if (!TempFile) {
        spdlog::error("[WASI-NN] BitNet: Failed to create fallback model file '{}'.", GraphRef.Params.model);
        Env.deleteGraph(GId);
        return WASINN::ErrNo::RuntimeError;
    }
    TempFile.write(reinterpret_cast<const char*>(Weight.data()), Weight.size());
    TempFile.close();
  }
  
  llama_backend_init();
  llama_numa_init(GraphRef.Params.numa);

  // A. Load the model.
  auto MParams = common_model_params_to_llama(GraphRef.Params);
  llama_model* ModelPtr = llama_load_model_from_file(GraphRef.Params.model.c_str(), MParams);

  if (ModelPtr == nullptr) {
    spdlog::error("[WASI-NN] BitNet: Failed to load model from '{}'.", GraphRef.Params.model);
    llama_backend_free();
    Env.deleteGraph(GId);
    return WASINN::ErrNo::InvalidArgument;
  }
  GraphRef.LlamaModel.reset(ModelPtr);


  if (GraphRef.Params.n_ctx == 0) {
      GraphRef.Params.n_ctx = llama_n_ctx_train(GraphRef.LlamaModel.get());
      // If the model itself doesn't specify a context size, fall back to a reasonable default.
      if (GraphRef.Params.n_ctx == 0) {
          spdlog::warn("[WASI-NN] BitNet: Model does not specify context size. Falling back to 1024.");
          GraphRef.Params.n_ctx = 1024;
      }
  }

 
  auto CParams = common_context_params_to_llama(GraphRef.Params);
  llama_context* ContextPtr = llama_new_context_with_model(GraphRef.LlamaModel.get(), CParams);

  if (ContextPtr == nullptr) {
    spdlog::error("[WASI-NN] BitNet: Failed to create llama_context.");
    llama_backend_free();
    Env.deleteGraph(GId);
    return WASINN::ErrNo::InvalidArgument;
  }
  GraphRef.LlamaContext.reset(ContextPtr);

  Env.NNGraph[GId].setReady();
  GraphId = GId;
  return WASINN::ErrNo::Success;
}

Expect<WASINN::ErrNo> initExecCtx(WASINN::WasiNNEnvironment &Env,
                                  uint32_t GraphId,
                                  uint32_t &ContextId) noexcept {
  auto &GraphRef = Env.NNGraph[GraphId].get<Graph>();
  ContextId = Env.newContext(GraphId, Env.NNGraph[GraphId]);
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();

  CxtRef.LlamaSampler.reset(common_sampler_init(GraphRef.LlamaModel.get(), CxtRef.SParams));
  if (!CxtRef.LlamaSampler) {
    spdlog::error("[WASI-NN] BitNet: Failed to initialize sampler.");
    Env.deleteContext(ContextId);
    return WASINN::ErrNo::RuntimeError;
  }

  CxtRef.LlamaBatch = llama_batch_init(GraphRef.Params.n_batch, 0, 1);
  if (!CxtRef.LlamaBatch.token) {
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
    spdlog::error("[WASI-NN] BitNet: Only one input tensor (the prompt) is supported at index 0.");
    return WASINN::ErrNo::InvalidArgument;
  }

  if (Tensor.RType != WASINN::TensorType::U8) {
    spdlog::error("[WASI-NN] BitNet: Input tensor must be a UTF-8 string (U8).");
    return WASINN::ErrNo::InvalidArgument;
  }

  // Reset state for a new generation sequence.
  llama_kv_cache_clear(GraphRef.LlamaContext.get());
  common_sampler_reset(CxtRef.LlamaSampler.get());
  CxtRef.NPos = 0;
  CxtRef.LlamaInputs.clear();
  CxtRef.LlamaOutputTokens.clear();

  const std::string Prompt(reinterpret_cast<const char *>(Tensor.Tensor.data()), Tensor.Tensor.size());

  CxtRef.LlamaInputs = common_tokenize(GraphRef.LlamaContext.get(), Prompt, llama_add_bos_token(GraphRef.LlamaModel.get()), false);
  if (CxtRef.LlamaInputs.size() >= static_cast<size_t>(GraphRef.Params.n_ctx)) {
    spdlog::error("[WASI-NN] BitNet: Prompt is too long ({} tokens), context size is {}.", CxtRef.LlamaInputs.size(), GraphRef.Params.n_ctx);
    return WASINN::ErrNo::PromptTooLong;
  }

  for (const auto& token : CxtRef.LlamaInputs) {
      common_sampler_accept(CxtRef.LlamaSampler.get(), token, false);
  }

  return WASINN::ErrNo::Success;
}

Expect<WASINN::ErrNo> compute(WASINN::WasiNNEnvironment &Env,
                              uint32_t ContextId) noexcept {
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  auto &GraphRef = CxtRef.GraphRef;
  const int32_t NCtx = llama_n_ctx(GraphRef.LlamaContext.get());
  
  if (CxtRef.LlamaInputs.empty()) {
    spdlog::error("[WASI-NN] BitNet: Input tensor not set before compute.");
    return WASINN::ErrNo::InvalidArgument;
  }

  for (size_t i = 0; i < CxtRef.LlamaInputs.size(); i += GraphRef.Params.n_batch) {
    const size_t n_tokens_to_process = std::min((size_t)GraphRef.Params.n_batch, CxtRef.LlamaInputs.size() - i);
    
    common_batch_clear(CxtRef.LlamaBatch);
    for (size_t j = 0; j < n_tokens_to_process; ++j) {
        common_batch_add(CxtRef.LlamaBatch, CxtRef.LlamaInputs[i + j], CxtRef.NPos, {0}, false);
        CxtRef.NPos++;
    }

    if (i + n_tokens_to_process == CxtRef.LlamaInputs.size()) {
        CxtRef.LlamaBatch.logits[CxtRef.LlamaBatch.n_tokens - 1] = true;
    }

    if (llama_decode(GraphRef.LlamaContext.get(), CxtRef.LlamaBatch) != 0) {
      spdlog::error("[WASI-NN] BitNet: llama_decode failed during prompt processing.");
      return WASINN::ErrNo::RuntimeError;
    }
  }

  int n_remain = CxtRef.NPredict;
  if (n_remain < 0) n_remain = INT32_MAX;

  while(n_remain > 0) {
    llama_token new_token_id = common_sampler_sample(CxtRef.LlamaSampler.get(), GraphRef.LlamaContext.get(), -1);
    common_sampler_accept(CxtRef.LlamaSampler.get(), new_token_id, true);

    if (new_token_id == llama_token_eos(GraphRef.LlamaModel.get()) || CxtRef.NPos >= NCtx) {
      break;
    }
    
    CxtRef.LlamaOutputTokens.push_back(new_token_id);
    n_remain--;

    common_batch_clear(CxtRef.LlamaBatch);
    common_batch_add(CxtRef.LlamaBatch, new_token_id, CxtRef.NPos, {0}, true);
    CxtRef.NPos++;

    if (llama_decode(GraphRef.LlamaContext.get(), CxtRef.LlamaBatch) != 0) {
      spdlog::error("[WASI-NN] BitNet: llama_decode failed during token generation.");
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
    spdlog::error("[WASI-NN] BitNet: Only one output tensor (at index 0) is supported.");
    return WASINN::ErrNo::InvalidArgument;
  }

  std::string output_text;
  for(const auto& token : CxtRef.LlamaOutputTokens) {
      output_text += common_token_to_piece(GraphRef.LlamaContext.get(), token);
  }

  const size_t len = std::min(static_cast<size_t>(OutBuffer.size()), output_text.length());
  std::copy_n(output_text.data(), len, OutBuffer.data());
  BytesWritten = output_text.length();

  return WASINN::ErrNo::Success;
}

Expect<WASINN::ErrNo> finalizeExecCtx(WASINN::WasiNNEnvironment &Env,
                                      uint32_t ContextId) noexcept {
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  llama_batch_free(CxtRef.LlamaBatch);
  Env.deleteContext(ContextId);
  return WASINN::ErrNo::Success;
}

Expect<WASINN::ErrNo> unload(WASINN::WasiNNEnvironment &Env, uint32_t GraphId) noexcept {
  if (GraphId < Env.NNGraph.size()) {
      Env.deleteGraph(GraphId);
      llama_backend_free();
  }
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