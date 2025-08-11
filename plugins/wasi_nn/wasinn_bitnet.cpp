#include "wasinn_bitnet.h"
#include "wasinnenv.h"

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_BITNET
#include "simdjson.h"
#include <algorithm>
#include <common.h>
#include <filesystem>
#include <fstream>
#include <llama.h>
#include <sampling.h>
#include <sstream>
#endif

namespace WasmEdge::Host::WASINN::BitNet {
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_BITNET

namespace {

// Macro for logging debug message.
#define LOG_DEBUG(Debug, ...)                                                  \
  if (Debug) {                                                                 \
    spdlog::info("[WASI-NN][Debug] BitNet backend: "sv __VA_ARGS__);           \
  }

// Macro for logging info message.
#define LOG_INFO(Info, ...)                                                    \
  if (Info) {                                                                  \
    spdlog::info("[WASI-NN] BitNet backend: "sv __VA_ARGS__);                  \
  }

// Macro for logging warning message.
#define LOG_WARN(...) spdlog::warn("[WASI-NN] BitNet backend: "sv __VA_ARGS__);

// Macro for logging error message.
#define LOG_ERROR(...)                                                         \
  spdlog::error("[WASI-NN] BitNet backend: "sv __VA_ARGS__);

// Macro for logging error message and return.
#define RET_ERROR(Error, ...)                                                  \
  do {                                                                         \
    spdlog::error("[WASI-NN] BitNet backend: "sv __VA_ARGS__);                 \
    return Error;                                                              \
  } while (0)

// Llama logging callback.
void llamaLogCallback(ggml_log_level LogLevel, const char *LogText,
                      void *UserData) {
  Graph &GraphRef = *reinterpret_cast<Graph *>(UserData);
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
    spdlog::error("[WASI-NN] llama.cpp: {}"sv, Text);
  } else if (LogLevel == GGML_LOG_LEVEL_WARN) {
    spdlog::warn("[WASI-NN] llama.cpp: {}"sv, Text);
  } else if (LogLevel == GGML_LOG_LEVEL_INFO) {
    spdlog::info("[WASI-NN] llama.cpp: {}"sv, Text);
  } else if (LogLevel == GGML_LOG_LEVEL_DEBUG) {
    spdlog::debug("[WASI-NN] llama.cpp: {}"sv, Text);
  }
}

// Parse metadata from json.
ErrNo parseMetadata(Graph &GraphRef, LocalConfig &ConfRef,
                    const std::string &Metadata) noexcept {
  simdjson::dom::parser Parser;
  simdjson::dom::element Doc;
  if (Parser.parse(Metadata).get(Doc)) {
    RET_ERROR(ErrNo::InvalidEncoding, "Failed to parse metadata JSON.");
  }

  if (Doc.at_key("enable-log").error() == simdjson::SUCCESS) {
    if (auto Err = Doc["enable-log"].get<bool>().get(GraphRef.EnableLog); Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the enable-log option.");
    }
  }
  if (Doc.at_key("enable-debug-log").error() == simdjson::SUCCESS) {
    if (auto Err =
            Doc["enable-debug-log"].get<bool>().get(GraphRef.EnableDebugLog);
        Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the enable-debug-log option.");
    }
  }
  if (Doc.at_key("n-gpu-layers").error() == simdjson::SUCCESS) {
    int64_t NGPULayers;
    if (auto Err = Doc["n-gpu-layers"].get<int64_t>().get(NGPULayers); Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the n-gpu-layers option.");
    }
    GraphRef.Params.n_gpu_layers = static_cast<int32_t>(NGPULayers);
  }

  if (Doc.at_key("ctx-size").error() == simdjson::SUCCESS) {
    uint64_t CtxSize;
    if (auto Err = Doc["ctx-size"].get<uint64_t>().get(CtxSize); Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the ctx-size option.");
    }
    GraphRef.Params.n_ctx = static_cast<int32_t>(CtxSize);
  }
  if (Doc.at_key("batch-size").error() == simdjson::SUCCESS) {
    uint64_t BatchSize;
    if (auto Err = Doc["batch-size"].get<uint64_t>().get(BatchSize); Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the batch-size option.");
    }
    GraphRef.Params.n_batch = static_cast<int32_t>(BatchSize);
  }

  // Sampling parameters
  auto &SParams = GraphRef.Params.sparams;
  if (Doc.at_key("n-predict").error() == simdjson::SUCCESS) {
    if (auto Err = Doc["n-predict"].get<int64_t>().get(ConfRef.NPredict); Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the n-predict option.");
    }
  }
  if (Doc.at_key("temperature").error() == simdjson::SUCCESS) {
    double Temp;
    if (auto Err = Doc["temperature"].get<double>().get(Temp); Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the temperature option.");
    }
    SParams.temp = static_cast<float>(Temp);
  }
  if (Doc.at_key("top-k").error() == simdjson::SUCCESS) {
    int64_t TopK;
    if (auto Err = Doc["top-k"].get<int64_t>().get(TopK); Err) {
      RET_ERROR(ErrNo::InvalidArgument, "Unable to retrieve the top-k option.");
    }
    SParams.top_k = static_cast<int32_t>(TopK);
  }
  if (Doc.at_key("top-p").error() == simdjson::SUCCESS) {
    double TopP;
    if (auto Err = Doc["top-p"].get<double>().get(TopP); Err) {
      RET_ERROR(ErrNo::InvalidArgument, "Unable to retrieve the top-p option.");
    }
    SParams.top_p = static_cast<float>(TopP);
  }
  if (Doc.at_key("repeat-penalty").error() == simdjson::SUCCESS) {
    double RepeatPenalty;
    if (auto Err = Doc["repeat-penalty"].get<double>().get(RepeatPenalty);
        Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the repeat-penalty option.");
    }
    SParams.penalty_repeat = static_cast<float>(RepeatPenalty);
  }
  if (Doc.at_key("repeat-last-n").error() == simdjson::SUCCESS) {
    int64_t RepeatLastN;
    if (auto Err = Doc["repeat-last-n"].get<int64_t>().get(RepeatLastN); Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the repeat-last-n option.");
    }
    SParams.penalty_last_n = static_cast<int32_t>(RepeatLastN);
  }
  if (Doc.at_key("penalize-nl").error() == simdjson::SUCCESS) {
    if (auto Err = Doc["penalize-nl"].get<bool>().get(SParams.penalize_nl);
        Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the penalize-nl option.");
    }
  }
  if (Doc.at_key("seed").error() == simdjson::SUCCESS) {
    uint64_t Seed;
    if (auto Err = Doc["seed"].get<uint64_t>().get(Seed); Err) {
      RET_ERROR(ErrNo::InvalidArgument, "Unable to retrieve the seed option.");
    }
    SParams.seed = static_cast<uint32_t>(Seed);
  }
  if (Doc.at_key("grammar").error() == simdjson::SUCCESS) {
    std::string_view Val;
    if (auto Err = Doc["grammar"].get<std::string_view>().get(Val); Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the grammar option.");
    }
    SParams.grammar = Val;
  }

  return ErrNo::Success;
}

void clearContext(Graph &GraphRef, Context &CxtRef) noexcept {
  LOG_DEBUG(GraphRef.EnableDebugLog, "clearContext");
  llama_kv_cache_clear(GraphRef.LlamaContext.get());
  common_sampler_reset(CxtRef.LlamaSampler.get());
  CxtRef.NPos = 0;
  CxtRef.LlamaOutputTokens.clear();
  CxtRef.LlamaOutputs.clear();
  LOG_DEBUG(GraphRef.EnableDebugLog, "clearContext...Done");
}

ErrNo evaluateTokens(Graph &GraphRef, Context &CxtRef,
                     Span<const llama_token> Tokens, bool IsLogit) noexcept {
  const int32_t NCtx = llama_n_ctx(GraphRef.LlamaContext.get());
  const int32_t BatchSize = GraphRef.Params.n_batch;

  if (CxtRef.NPos + static_cast<int32_t>(Tokens.size()) > NCtx) {
    RET_ERROR(ErrNo::ContextFull,
              "evaluateTokens: context is full ({} + {} > {}).", CxtRef.NPos,
              Tokens.size(), NCtx);
  }

  for (size_t I = 0; I < Tokens.size(); I += BatchSize) {
    const size_t NTokensToProcess =
        std::min(static_cast<size_t>(BatchSize), Tokens.size() - I);

    common_batch_clear(CxtRef.LlamaBatch);
    for (size_t J = 0; J < NTokensToProcess; ++J) {
      common_batch_add(CxtRef.LlamaBatch, Tokens[I + J], CxtRef.NPos, {0},
                       false);
      CxtRef.NPos++;
    }

    if (IsLogit && (I + NTokensToProcess == Tokens.size())) {
      CxtRef.LlamaBatch.logits[CxtRef.LlamaBatch.n_tokens - 1] = true;
    }

    if (llama_decode(GraphRef.LlamaContext.get(), CxtRef.LlamaBatch) != 0) {
      RET_ERROR(ErrNo::RuntimeError,
                "evaluateTokens: llama_decode failed during processing.");
    }
  }

  return ErrNo::Success;
}

ErrNo evaluateInput(Graph &GraphRef, Context &CxtRef,
                    std::string_view LogPrefix) noexcept {
  if (CxtRef.LlamaInputs.empty()) {
    RET_ERROR(ErrNo::InvalidArgument, "{}: Input tensor not set.", LogPrefix);
  }

  const int32_t NCtx = llama_n_ctx(GraphRef.LlamaContext.get());
  if (CxtRef.LlamaInputs.size() >= static_cast<size_t>(NCtx)) {
    RET_ERROR(ErrNo::PromptTooLong,
              "{}: Prompt is too long ({} tokens), but context size is {}.",
              LogPrefix, CxtRef.LlamaInputs.size(), NCtx);
  }

  for (const auto &Token : CxtRef.LlamaInputs) {
    common_sampler_accept(CxtRef.LlamaSampler.get(), Token, false);
  }

  return evaluateTokens(GraphRef, CxtRef, CxtRef.LlamaInputs, true);
}

ErrNo sampleOutput(Graph &GraphRef, Context &CxtRef,
                   bool IsSingleTokenMode = false) noexcept {
  const int32_t NCtx = llama_n_ctx(GraphRef.LlamaContext.get());
  if (CxtRef.NPos >= NCtx) {
    return ErrNo::ContextFull;
  }

  llama_token NewTokenId = common_sampler_sample(
      CxtRef.LlamaSampler.get(), GraphRef.LlamaContext.get(), -1);
  common_sampler_accept(CxtRef.LlamaSampler.get(), NewTokenId, true);

  if (NewTokenId == llama_token_eos(GraphRef.LlamaModel.get())) {
    return ErrNo::EndOfSequence;
  }

  CxtRef.LlamaOutputTokens.push_back(NewTokenId);
  std::string OutputString =
      common_token_to_piece(GraphRef.LlamaContext.get(), NewTokenId);

  if (!IsSingleTokenMode) {
    CxtRef.LlamaOutputs.insert(CxtRef.LlamaOutputs.end(), OutputString.begin(),
                               OutputString.end());
  }

  return evaluateTokens(GraphRef, CxtRef,
                        Span<const llama_token>(&NewTokenId, 1), true);
}

} // namespace

Expect<WASINN::ErrNo> load(WASINN::WasiNNEnvironment &Env,
                           Span<const Span<uint8_t>> Builders,
                           [[maybe_unused]] WASINN::Device Device,
                           uint32_t &GraphId) noexcept {
  if (Builders.empty()) {
    RET_ERROR(WASINN::ErrNo::InvalidArgument,
              "Invalid builders size, builders size must be > 0.");
  }

  const uint32_t GId = Env.newGraph(Backend::BitNet);
  auto &GraphRef = Env.NNGraph[GId].get<Graph>();
  LOG_DEBUG(GraphRef.EnableDebugLog, "load");

  GraphRef.Params = common_params{};

  if (Builders.size() > 1) {
    const std::string Metadata(
        reinterpret_cast<const char *>(Builders[1].data()), Builders[1].size());
    if (auto Res = parseMetadata(GraphRef, GraphRef.Conf, Metadata);
        Res != ErrNo::Success) {
      Env.deleteGraph(GId);
      RET_ERROR(Res, "Failed to parse metadata.");
    }
  }

  if (GraphRef.Params.cpuparams.n_threads <= 0) {
    GraphRef.Params.cpuparams.n_threads = std::thread::hardware_concurrency();
  }
  if (GraphRef.Params.cpuparams_batch.n_threads <= 0) {
    // Default batch threads to generation threads if not specified
    GraphRef.Params.cpuparams_batch.n_threads =
        GraphRef.Params.cpuparams.n_threads;
  }
  llama_log_set(llamaLogCallback, &GraphRef);

  const auto &Weight = Builders[0];
  const std::string_view ModelPathStr(
      reinterpret_cast<const char *>(Weight.data()), Weight.size());

  if (ModelPathStr.substr(0, 8) == "preload:"sv) {
    GraphRef.Params.model = ModelPathStr.substr(8);
  } else {
    LOG_WARN("Model path does not have 'preload:' prefix. Assuming it is a raw "
             "model and writing to 'bitnet-model.bin'.");
    GraphRef.Params.model = "bitnet-model.bin";
    std::ofstream TempFile(GraphRef.Params.model,
                           std::ios::binary | std::ios::trunc);
    if (!TempFile) {
      Env.deleteGraph(GId);
      RET_ERROR(WASINN::ErrNo::RuntimeError,
                "Failed to create temp model file.");
    }
    TempFile.write(reinterpret_cast<const char *>(Weight.data()),
                   Weight.size());
    TempFile.close();
  }

  LOG_INFO(GraphRef.EnableLog, "Loading model from '{}'.",
           GraphRef.Params.model);

  llama_backend_init();
  llama_numa_init(GraphRef.Params.numa);

  // A. Load the model.
  auto MParams = common_model_params_to_llama(GraphRef.Params);
  GraphRef.LlamaModel.reset(
      llama_load_model_from_file(GraphRef.Params.model.c_str(), MParams));

  if (GraphRef.LlamaModel == nullptr) {
    llama_backend_free();
    Env.deleteGraph(GId);
    RET_ERROR(WASINN::ErrNo::InvalidArgument, "Failed to load model from '{}'.",
              GraphRef.Params.model);
  }

  if (GraphRef.Params.n_ctx == 0) {
    GraphRef.Params.n_ctx = llama_n_ctx_train(GraphRef.LlamaModel.get());
    // If the model itself doesn't specify a context size, fall back to a
    // reasonable default.
    if (GraphRef.Params.n_ctx == 0) {
      LOG_WARN("Model does not specify n_ctx, falling back to 2048.");
      GraphRef.Params.n_ctx = 2048;
    }
  }

  auto CParams = common_context_params_to_llama(GraphRef.Params);
  GraphRef.LlamaContext.reset(
      llama_new_context_with_model(GraphRef.LlamaModel.get(), CParams));

  if (GraphRef.LlamaContext == nullptr) {
    llama_backend_free();
    Env.deleteGraph(GId);
    RET_ERROR(WASINN::ErrNo::InvalidArgument,
              "Failed to create llama context.");
  }

  Env.NNGraph[GId].setReady();
  GraphId = GId;
  LOG_DEBUG(GraphRef.EnableDebugLog, "load...Done");
  return WASINN::ErrNo::Success;
}

Expect<WASINN::ErrNo> initExecCtx(WASINN::WasiNNEnvironment &Env,
                                  uint32_t GraphId,
                                  uint32_t &ContextId) noexcept {
  auto &GraphRef = Env.NNGraph[GraphId].get<Graph>();
  LOG_DEBUG(GraphRef.EnableDebugLog, "initExecCtx");
  ContextId = Env.newContext(GraphId, Env.NNGraph[GraphId]);
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();

  CxtRef.LlamaSampler.reset(
      common_sampler_init(GraphRef.LlamaModel.get(), GraphRef.Params.sparams));
  if (!CxtRef.LlamaSampler) {
    Env.deleteContext(ContextId);
    RET_ERROR(WASINN::ErrNo::RuntimeError, "Failed to initialize sampler.");
  }

  CxtRef.LlamaBatch = llama_batch_init(GraphRef.Params.n_batch, 0, 1);
  if (!CxtRef.LlamaBatch.token) {
    Env.deleteContext(ContextId);
    RET_ERROR(WASINN::ErrNo::RuntimeError, "Failed to initialize llama_batch.");
  }

  Env.NNContext[ContextId].setReady();
  LOG_INFO(GraphRef.EnableLog, "llama_system_info: {}"sv,
           llama_print_system_info());
  LOG_DEBUG(GraphRef.EnableDebugLog, "initExecCtx...Done");
  return WASINN::ErrNo::Success;
}

Expect<WASINN::ErrNo> setInput(WASINN::WasiNNEnvironment &Env,
                               uint32_t ContextId, uint32_t Index,
                               const TensorData &Tensor) noexcept {
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  auto &GraphRef = Env.NNGraph[CxtRef.GraphId];
  LOG_DEBUG(GraphRef.get<Graph>().EnableDebugLog, "setInput");

  if (Index == 1) {
    LOG_DEBUG(GraphRef.get<Graph>().EnableDebugLog,
              "setInput: found Metadata, processing");
    const std::string Metadata(
        reinterpret_cast<const char *>(Tensor.Tensor.data()),
        Tensor.Tensor.size());
    if (auto Res = parseMetadata(GraphRef.get<Graph>(), CxtRef.Conf, Metadata);
        Res != ErrNo::Success) {
      RET_ERROR(Res, "setInput: failed to parse metadata.");
    }
    // Re-initialize the sampler with new parameters
    CxtRef.LlamaSampler.reset(
        common_sampler_init(GraphRef.get<Graph>().LlamaModel.get(),
                            GraphRef.get<Graph>().Params.sparams));
    if (!CxtRef.LlamaSampler) {
      RET_ERROR(WASINN::ErrNo::RuntimeError,
                "Failed to re-initialize sampler with new options.");
    }
    LOG_DEBUG(GraphRef.get<Graph>().EnableDebugLog,
              "setInput: found Metadata, processing...Done");
    return WASINN::ErrNo::Success;
  }

  if (Index != 0) {
    RET_ERROR(WASINN::ErrNo::InvalidArgument,
              "Only one input tensor (the prompt) is supported at index 0.");
  }

  if (Tensor.RType != WASINN::TensorType::U8) {
    RET_ERROR(WASINN::ErrNo::InvalidArgument,
              "Input tensor must be a UTF-8 string (U8).");
  }

  const std::string Prompt(reinterpret_cast<const char *>(Tensor.Tensor.data()),
                           Tensor.Tensor.size());

  CxtRef.LlamaInputs = common_tokenize(
      GraphRef.get<Graph>().LlamaContext.get(), Prompt,
      llama_add_bos_token(GraphRef.get<Graph>().LlamaModel.get()), false);

  CxtRef.ComputeSingleStarted = false;
  LOG_DEBUG(GraphRef.get<Graph>().EnableDebugLog, "setInput...Done");
  return WASINN::ErrNo::Success;
}

Expect<WASINN::ErrNo> compute(WASINN::WasiNNEnvironment &Env,
                              uint32_t ContextId) noexcept {
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  auto &GraphRef = Env.NNGraph[CxtRef.GraphId].get<Graph>();
  LOG_DEBUG(GraphRef.EnableDebugLog, "compute");

  clearContext(GraphRef, CxtRef);

  auto ReturnCode = evaluateInput(GraphRef, CxtRef, "compute"sv);
  if (ReturnCode != ErrNo::Success) {
    return ReturnCode;
  }

  int NRemain = CxtRef.Conf.NPredict;
  if (NRemain < 0) {
    NRemain = INT32_MAX;
  }

  while (NRemain > 0) {
    ReturnCode = sampleOutput(GraphRef, CxtRef);
    if (ReturnCode != ErrNo::Success) {
      break;
    }
    NRemain--;
  }

  if (ReturnCode == ErrNo::EndOfSequence || ReturnCode == ErrNo::ContextFull) {
    LOG_INFO(
        GraphRef.EnableLog, "compute finished with status: {}.",
        static_cast<uint32_t>(ReturnCode)); // <--- FIX: Cast enum for logging
    return WASINN::ErrNo::Success;
  }

  LOG_DEBUG(GraphRef.EnableDebugLog, "compute...Done");
  return ReturnCode;
}

Expect<WASINN::ErrNo> getOutput(WASINN::WasiNNEnvironment &Env,
                                uint32_t ContextId, uint32_t Index,
                                Span<uint8_t> OutBuffer,
                                uint32_t &BytesWritten) noexcept {
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  auto &GraphRef = Env.NNGraph[CxtRef.GraphId].get<Graph>();
  LOG_DEBUG(GraphRef.EnableDebugLog, "getOutput: with Index {}", Index);

  if (Index != 0) {
    RET_ERROR(WASINN::ErrNo::InvalidArgument,
              "Only one output tensor (at index 0) is supported.");
  }

  const size_t Len = std::min(static_cast<size_t>(OutBuffer.size()),
                              CxtRef.LlamaOutputs.size());
  std::copy_n(CxtRef.LlamaOutputs.data(), Len, OutBuffer.data());
  BytesWritten = CxtRef.LlamaOutputs.size();

  LOG_DEBUG(GraphRef.EnableDebugLog, "getOutput: with Index {}...Done", Index);
  return WASINN::ErrNo::Success;
}

Expect<WASINN::ErrNo> computeSingle(WASINN::WasiNNEnvironment &Env,
                                    uint32_t ContextId) noexcept {
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  auto &GraphRef = Env.NNGraph[CxtRef.GraphId].get<Graph>();
  LOG_DEBUG(GraphRef.EnableDebugLog, "computeSingle");

  auto ReturnCode = ErrNo::Success;
  if (!CxtRef.ComputeSingleStarted) {
    clearContext(GraphRef, CxtRef);
    ReturnCode = evaluateInput(GraphRef, CxtRef, "computeSingle"sv);
    if (ReturnCode != ErrNo::Success) {
      return ReturnCode;
    }
    CxtRef.ComputeSingleStarted = true;
  }

  ReturnCode = sampleOutput(GraphRef, CxtRef, /* IsSingleTokenMode */ true);
  if (ReturnCode != ErrNo::Success) {
    CxtRef.ComputeSingleStarted = false;
  }

  LOG_DEBUG(GraphRef.EnableDebugLog, "computeSingle...Done");
  return ReturnCode;
}

Expect<WASINN::ErrNo> getOutputSingle(WASINN::WasiNNEnvironment &Env,
                                      uint32_t ContextId, uint32_t Index,
                                      Span<uint8_t> OutBuffer,
                                      uint32_t &BytesWritten) noexcept {
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  auto &GraphRef = Env.NNGraph[CxtRef.GraphId].get<Graph>();
  LOG_DEBUG(GraphRef.EnableDebugLog, "getOutputSingle: with Index {}", Index);

  if (Index != 0) {
    RET_ERROR(WASINN::ErrNo::InvalidArgument,
              "Only one output tensor (at index 0) is supported.");
  }

  if (CxtRef.LlamaOutputTokens.empty()) {
    BytesWritten = 0;
    return WASINN::ErrNo::Success;
  }

  llama_token LastTokenId = CxtRef.LlamaOutputTokens.back();
  std::string LastTokenStr =
      common_token_to_piece(GraphRef.LlamaContext.get(), LastTokenId);

  const size_t Len =
      std::min(static_cast<size_t>(OutBuffer.size()), LastTokenStr.length());
  std::copy_n(LastTokenStr.data(), Len, OutBuffer.data());
  BytesWritten = LastTokenStr.length();

  LOG_DEBUG(GraphRef.EnableDebugLog, "getOutputSingle: with Index {}...Done",
            Index);
  return WASINN::ErrNo::Success;
}

Expect<WASINN::ErrNo> finiSingle(WASINN::WasiNNEnvironment &Env,
                                 uint32_t ContextId) noexcept {
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  auto &GraphRef = Env.NNGraph[CxtRef.GraphId].get<Graph>();
  LOG_DEBUG(GraphRef.EnableDebugLog, "finiSingle");

  clearContext(GraphRef, CxtRef);
  CxtRef.ComputeSingleStarted = false;

  LOG_DEBUG(GraphRef.EnableDebugLog, "finiSingle...Done");
  return WASINN::ErrNo::Success;
}

Expect<WASINN::ErrNo> unload(WASINN::WasiNNEnvironment &Env,
                             uint32_t GraphId) noexcept {
  const bool IsDebugLog = (GraphId < Env.NNGraph.size())
                              ? Env.NNGraph[GraphId].get<Graph>().EnableDebugLog
                              : false;
  LOG_DEBUG(IsDebugLog, "unload");

  if (GraphId < Env.NNGraph.size()) {
    Env.deleteGraph(GraphId);
  }
  llama_backend_free();

  LOG_DEBUG(IsDebugLog, "unload...Done");
  return WASINN::ErrNo::Success;
}

Expect<WASINN::ErrNo> finalizeExecCtx(WASINN::WasiNNEnvironment &Env,
                                      uint32_t ContextId) noexcept {
  auto &GraphRef =
      Env.NNGraph[Env.NNContext[ContextId].get<Context>().GraphId].get<Graph>();
  LOG_DEBUG(GraphRef.EnableDebugLog, "finalizeExecCtx");

  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  CxtRef.LlamaSampler.reset();
  llama_batch_free(CxtRef.LlamaBatch);
  Env.deleteContext(ContextId);

  LOG_DEBUG(GraphRef.EnableDebugLog, "finalizeExecCtx...Done");
  return WASINN::ErrNo::Success;
}

#else
namespace {
Expect<ErrNo> reportBackendNotSupported() noexcept {
  RET_ERROR(WASINN::ErrNo::InvalidArgument,
            "BitNet backend is not built. Please build with "
            "-DWASMEDGE_PLUGIN_WASI_NN_BACKEND_BITNET=ON.");
}
} // namespace

Expect<WASINN::ErrNo> load(WASINN::WasiNNEnvironment &,
                           Span<const Span<uint8_t>>, WASINN::Device,
                           uint32_t &) noexcept {
  return reportBackendNotSupported();
}
Expect<WASINN::ErrNo> initExecCtx(WASINN::WasiNNEnvironment &, uint32_t,
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
Expect<ErrNo> getOutputSingle(WASINN::WasiNNEnvironment &, uint32_t, uint32_t,
                              Span<uint8_t>, uint32_t &) noexcept {
  return reportBackendNotSupported();
}
Expect<ErrNo> computeSingle(WASINN::WasiNNEnvironment &, uint32_t) noexcept {
  return reportBackendNotSupported();
}
Expect<ErrNo> finiSingle(WASINN::WasiNNEnvironment &, uint32_t) noexcept {
  return reportBackendNotSupported();
}
Expect<WASINN::ErrNo> finalizeExecCtx(WASINN::WasiNNEnvironment &Env,
                                      uint32_t ContextId) noexcept {
  Env.deleteContext(ContextId);
  return reportBackendNotSupported();
}
Expect<WASINN::ErrNo> unload(WASINN::WasiNNEnvironment &Env,
                             uint32_t GraphId) noexcept {
  Env.deleteGraph(GraphId);
  return reportBackendNotSupported();
}

#endif
} // namespace WasmEdge::Host::WASINN::BitNet