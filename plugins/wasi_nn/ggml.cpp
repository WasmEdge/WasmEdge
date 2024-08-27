// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "ggml.h"
#include "wasinnenv.h"

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_GGML
#include "simdjson.h"
#include <algorithm>
#include <base64.hpp>
#include <clip.h>
#include <common.h>
#include <cstdlib>
#include <filesystem>
#include <fmt/ranges.h>
#include <json-schema-to-grammar.h>
#include <json.hpp>
#include <llama.h>
#include <llava.h>
#include <sstream>
#endif

namespace WasmEdge::Host::WASINN::GGML {
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_GGML

namespace {

void LlamaLogCallback(ggml_log_level LogLevel, const char *LogText,
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

Expect<ErrNo> parseMetadata(Graph &GraphRef, const std::string &Metadata,
                            bool *IsModelUpdated = nullptr) noexcept {
  simdjson::dom::parser Parser;
  simdjson::dom::element Doc;
  auto ParseError = Parser.parse(Metadata).get(Doc);
  if (ParseError) {
    spdlog::error("[WASI-NN] GGML backend: Parse metadata error"sv);
    return ErrNo::InvalidEncoding;
  }

  // Get metadata from the json.

  // Currently supported metadata:
  // Plugin parameters (used by this plugin):
  //   enable-log: bool
  //   enable-debug-log: bool
  //   stream-stdout: bool
  //   embedding: bool
  //   n-predict: uint64_t
  //   reverse-prompt: string
  //   mmproj: string
  //   image: string
  //   use-mmap: bool
  // Model parameters (need to reload the model if updated):
  //   n-gpu-layers: int64_t
  //   main-gpu: int64_t
  //   tensor-split: string, comma-separated floating number list
  //   use-mmap: use mmap
  // Context parameters (used by the llama context):
  //   ctx-size: uint64_t
  //   batch-size: uint64_t
  //   ubatch-size: uint64_t
  //   threads: uint64_t
  // Sampling parameters (used by the llama sampling context).
  //   temp: double
  //   top-p: double
  //   repeat-penalty: double
  //   presence-penalty: double
  //   frequency-penalty: double
  //   grammar: string

  // Get the current llama parameters.
  llama_model_params ModelParams = llama_model_default_params();
  ModelParams.n_gpu_layers = static_cast<int32_t>(GraphRef.NGPULayers);
  ModelParams.main_gpu = static_cast<int32_t>(GraphRef.MainGPU);
  ModelParams.tensor_split = GraphRef.TensorSplit.data();
  ModelParams.use_mmap = GraphRef.UseMMap;

  // The plugin parameters.
  if (Doc.at_key("enable-log").error() == simdjson::SUCCESS) {
    auto Err = Doc["enable-log"].get<bool>().get(GraphRef.EnableLog);
    if (Err) {
      spdlog::error(
          "[WASI-NN] GGML backend: Unable to retrieve the enable-log option."sv);
      return ErrNo::InvalidArgument;
    }
  }
  if (Doc.at_key("enable-debug-log").error() == simdjson::SUCCESS) {
    auto Err = Doc["enable-debug-log"].get<bool>().get(GraphRef.EnableDebugLog);
    if (Err) {
      spdlog::error(
          "[WASI-NN] GGML backend: Unable to retrieve the enable-debug-log option."sv);
      return ErrNo::InvalidArgument;
    }
  }
  if (Doc.at_key("stream-stdout").error() == simdjson::SUCCESS) {
    auto Err = Doc["stream-stdout"].get<bool>().get(GraphRef.StreamStdout);
    if (Err) {
      spdlog::error(
          "[WASI-NN] GGML backend: Unable to retrieve the stream-stdout option."sv);
      return ErrNo::InvalidArgument;
    }
  }
  if (Doc.at_key("embedding").error() == simdjson::SUCCESS) {
    auto Err = Doc["embedding"].get<bool>().get(GraphRef.Embedding);
    if (Err) {
      spdlog::error(
          "[WASI-NN] GGML backend: Unable to retrieve the embedding option."sv);
      return ErrNo::InvalidArgument;
    }
  }
  if (Doc.at_key("n-predict").error() == simdjson::SUCCESS) {
    auto Err = Doc["n-predict"].get<uint64_t>().get(GraphRef.NPredict);
    if (Err) {
      spdlog::error(
          "[WASI-NN] GGML backend: Unable to retrieve the n-predict option."sv);
      return ErrNo::InvalidArgument;
    }
  }
  if (Doc.at_key("reverse-prompt").error() == simdjson::SUCCESS) {
    std::string_view ReversePrompt;
    auto Err = Doc["reverse-prompt"].get<std::string_view>().get(ReversePrompt);
    if (Err) {
      spdlog::error(
          "[WASI-NN] GGML backend: Unable to retrieve the reverse-prompt option."sv);
      return ErrNo::InvalidArgument;
    }
    GraphRef.ReversePrompt = ReversePrompt;
  }
  if (Doc.at_key("mmproj").error() == simdjson::SUCCESS) {
    std::string_view MMProjModelPath;
    auto Err = Doc["mmproj"].get<std::string_view>().get(MMProjModelPath);
    if (Err) {
      spdlog::error(
          "[WASI-NN] GGML backend: Unable to retrieve the mmproj option."sv);
      return ErrNo::InvalidArgument;
    }
    GraphRef.MMProjModelPath = MMProjModelPath;
  }
  if (Doc.at_key("image").error() == simdjson::SUCCESS) {
    std::string_view ImagePath;
    auto Err = Doc["image"].get<std::string_view>().get(ImagePath);
    if (Err) {
      spdlog::error(
          "[WASI-NN] GGML backend: Unable to retrieve the image option."sv);
      return ErrNo::InvalidArgument;
    }
    GraphRef.ImagePath = ImagePath;
  }

  // The model parameters.
  if (Doc.at_key("n-gpu-layers").error() == simdjson::SUCCESS) {
    auto Err = Doc["n-gpu-layers"].get<int64_t>().get(GraphRef.NGPULayers);
    if (Err) {
      spdlog::error(
          "[WASI-NN] GGML backend: Unable to retrieve the n-gpu-layers option."sv);
      return ErrNo::InvalidArgument;
    }
  }
  if (Doc.at_key("main-gpu").error() == simdjson::SUCCESS) {
    auto Err = Doc["main-gpu"].get<int64_t>().get(GraphRef.MainGPU);
    if (Err) {
      spdlog::error(
          "[WASI-NN] GGML backend: Unable to retrieve the main-gpu option."sv);
      return ErrNo::InvalidArgument;
    }
  }
  if (Doc.at_key("tensor-split").error() == simdjson::SUCCESS) {
    // The TensorSplit is a comma-separated list of non-negative values.
    // E.g., "3,2" presents 60% of the data to GPU 0 and 40% to GPU 1.
    std::string_view TSV;
    auto Err = Doc["tensor-split"].get<std::string_view>().get(TSV);
    if (Err) {
      spdlog::error(
          "[WASI-NN] GGML backend: Unable to retrieve the tensor-split option."sv);
      return ErrNo::InvalidArgument;
    }
    std::string TS(TSV);
    std::replace(TS.begin(), TS.end(), ',', ' ');
    std::stringstream SS(TS);
    GraphRef.TensorSplit.clear();
    while (SS.good()) {
      float TmpTensor;
      SS >> TmpTensor;
      GraphRef.TensorSplit.push_back(TmpTensor);
    }
    size_t NDevices = llama_max_devices();
    if (GraphRef.TensorSplit.size() > NDevices) {
      spdlog::error(
          "[WASI-NN] GGML backend: Number of Tensor-Split is larger "
          "than MaxDevices, please reduce the size of tensor-split."sv);
      return ErrNo::InvalidArgument;
    }
    for (size_t Idx = GraphRef.TensorSplit.size(); Idx < NDevices; Idx++) {
      GraphRef.TensorSplit.push_back(0.0f);
    }
  }
  if (Doc.at_key("use-mmap").error() == simdjson::SUCCESS) {
    auto Err = Doc["use-mmap"].get<bool>().get(GraphRef.UseMMap);
    if (Err) {
      spdlog::error(
          "[WASI-NN] GGML backend: Unable to retrieve the use-mmap option."sv);
      return ErrNo::InvalidArgument;
    }
  }

  // The context parameters.
  if (Doc.at_key("ctx-size").error() == simdjson::SUCCESS) {
    auto Err = Doc["ctx-size"].get<uint64_t>().get(GraphRef.CtxSize);
    if (Err) {
      spdlog::error(
          "[WASI-NN] GGML backend: Unable to retrieve the ctx-size option."sv);
      return ErrNo::InvalidArgument;
    }
  }
  if (Doc.at_key("batch-size").error() == simdjson::SUCCESS) {
    auto Err = Doc["batch-size"].get<uint64_t>().get(GraphRef.BatchSize);
    if (Err) {
      spdlog::error(
          "[WASI-NN] GGML backend: Unable to retrieve the batch-size option."sv);
      return ErrNo::InvalidArgument;
    }
  }
  if (Doc.at_key("ubatch-size").error() == simdjson::SUCCESS) {
    auto Err = Doc["ubatch-size"].get<uint64_t>().get(GraphRef.UBatchSize);
    if (Err) {
      spdlog::error(
          "[WASI-NN] GGML backend: Unable to retrieve the ubatch-size option."sv);
      return ErrNo::InvalidArgument;
    }
  }
  if (Doc.at_key("threads").error() == simdjson::SUCCESS) {
    auto Err = Doc["threads"].get<uint64_t>().get(GraphRef.Threads);
    if (Err) {
      spdlog::error(
          "[WASI-NN] GGML backend: Unable to retrieve the threads option."sv);
      return ErrNo::InvalidArgument;
    }
  }

  // The sampling parameters.
  if (Doc.at_key("temp").error() == simdjson::SUCCESS) {
    auto Err = Doc["temp"].get<double>().get(GraphRef.Temp);
    if (Err) {
      spdlog::error(
          "[WASI-NN] GGML backend: Unable to retrieve the temp option."sv);
      return ErrNo::InvalidArgument;
    }
    GraphRef.Temp = std::max(0.0, GraphRef.Temp);
  }
  if (Doc.at_key("top-p").error() == simdjson::SUCCESS) {
    auto Err = Doc["top-p"].get<double>().get(GraphRef.TopP);
    if (Err) {
      spdlog::error(
          "[WASI-NN] GGML backend: Unable to retrieve the top-p option."sv);
      return ErrNo::InvalidArgument;
    }
  }
  if (Doc.at_key("repeat-penalty").error() == simdjson::SUCCESS) {
    auto Err = Doc["repeat-penalty"].get<double>().get(GraphRef.RepeatPenalty);
    if (Err) {
      spdlog::error(
          "[WASI-NN] GGML backend: Unable to retrieve the repeat-penalty option."sv);
      return ErrNo::InvalidArgument;
    }
  }
  if (Doc.at_key("presence-penalty").error() == simdjson::SUCCESS) {
    auto Err =
        Doc["presence-penalty"].get<double>().get(GraphRef.PresencePenalty);
    if (Err) {
      spdlog::error(
          "[WASI-NN] GGML backend: Unable to retrieve the presence-penalty option."sv);
      return ErrNo::InvalidArgument;
    }
  }
  if (Doc.at_key("frequency-penalty").error() == simdjson::SUCCESS) {
    auto Err =
        Doc["frequency-penalty"].get<double>().get(GraphRef.FrequencyPenalty);
    if (Err) {
      spdlog::error(
          "[WASI-NN] GGML backend: Unable to retrieve the frequency-penalty option."sv);
      return ErrNo::InvalidArgument;
    }
  }
  if (Doc.at_key("grammar").error() == simdjson::SUCCESS) {
    std::string_view Grammar;
    auto Err = Doc["grammar"].get<std::string_view>().get(Grammar);
    if (Err) {
      spdlog::error(
          "[WASI-NN] GGML backend: Unable to retrieve the grammar option."sv);
      return ErrNo::InvalidArgument;
    }
    GraphRef.Grammar = Grammar;
  }
  if (Doc.at_key("json-schema").error() == simdjson::SUCCESS) {
    std::string_view JsonSchema;
    auto Err = Doc["json-schema"].get<std::string_view>().get(JsonSchema);
    if (Err) {
      spdlog::error(
          "[WASI-NN] GGML backend: Unable to retrieve the json-schema option."sv);
      return ErrNo::InvalidArgument;
    }
    GraphRef.Grammar =
        json_schema_to_grammar(nlohmann::ordered_json::parse(JsonSchema));
  }

  // Check if the model is updated.
  if (IsModelUpdated && ModelParams.n_gpu_layers != GraphRef.NGPULayers) {
    *IsModelUpdated = true;
  }

  return ErrNo::Success;
}

Expect<ErrNo> setupGPTParam(Graph &GraphRef, gpt_params &GPTParams) {
  GPTParams.sparams.temp = static_cast<float>(GraphRef.Temp);
  GPTParams.sparams.top_p = static_cast<float>(GraphRef.TopP);
  GPTParams.sparams.penalty_repeat = static_cast<float>(GraphRef.RepeatPenalty);
  GPTParams.sparams.penalty_present =
      static_cast<float>(GraphRef.PresencePenalty);
  GPTParams.sparams.grammar = GraphRef.Grammar;
  return ErrNo::Success;
}

Expect<ErrNo> setupContextParam(Graph &GraphRef,
                                llama_context_params &ContextParams) {
  ContextParams.n_ctx = static_cast<uint32_t>(GraphRef.CtxSize);
  ContextParams.n_batch = static_cast<uint32_t>(GraphRef.BatchSize);
  ContextParams.n_ubatch = static_cast<uint32_t>(GraphRef.UBatchSize);
  ContextParams.n_threads = static_cast<uint32_t>(GraphRef.Threads);
  ContextParams.n_threads_batch = static_cast<uint32_t>(GraphRef.Threads);
  ContextParams.embeddings = GraphRef.Embedding;
  return ErrNo::Success;
}

Expect<ErrNo> buildOutputMetadata(Context &CxtRef,
                                  std::string &Metadata) noexcept {
  Metadata = fmt::format(R"({{"input_tokens": {}, )"
                         R"("output_tokens": {}, )"
                         R"("llama_build_number": {}, )"
                         R"("llama_commit": "{}"}})"sv,
                         CxtRef.LlamaNInputs, CxtRef.LlamaOutputTokens.size(),
                         LLAMA_BUILD_NUMBER, LLAMA_COMMIT);
  return ErrNo::Success;
}

void buildOutputEmbedding(std::string &Embedding, int32_t NEmbd,
                          const float *Embeddings) noexcept {
  // Embedding vector format
  // | Content                             |
  // | ----------------------------------- |
  // | '{"number_embedding": '             |
  // | n_embedding                         |
  // | ', "embedding": '                   |
  // | '['                                 |
  // | n_embedding*(embedding value %.10f) |
  // | (n_embedding-1)*(',')               |
  // | ']'                                 |
  // | '}'                                 |
  Embedding =
      fmt::format(R"({{"n_embedding": {:.10}, )"
                  R"("embedding": [{:.10}]}})"sv,
                  NEmbd, fmt::join(Embeddings, Embeddings + NEmbd, ","sv));
}

ErrNo evaluateTokens(Graph &GraphRef, struct llama_context *LlamaContext,
                     std::vector<llama_token> Tokens, int &NPast) noexcept {
  uint32_t NCtx = llama_n_ctx(LlamaContext);

  // End the inference if the context is full.
  if (NPast + static_cast<uint32_t>(Tokens.size()) > NCtx) {
    if (GraphRef.EnableLog) {
      spdlog::info(
          "[WASI-NN] GGML backend: the context if full ({} / {} tokens). Please increase your context size."sv,
          NPast + static_cast<uint32_t>(Tokens.size()), NCtx);
    }
    return ErrNo::ContextFull;
  }

  for (int I = 0; I < static_cast<int>(Tokens.size());
       I += static_cast<int>(GraphRef.BatchSize)) {
    int NEval = static_cast<int>(Tokens.size()) - I;
    if (NEval > static_cast<int>(GraphRef.BatchSize)) {
      NEval = static_cast<int>(GraphRef.BatchSize);
    }
    // llama_batch_get_one(*token, n_tokens, position, sequence_id)
    // This will return batch for single sequence of tokens starting at
    // position.
    const llama_seq_id SequenceId = 0;
    auto Status =
        llama_decode(LlamaContext,
                     llama_batch_get_one(&Tokens[I], NEval, NPast, SequenceId));
    if (Status == 1) {
      spdlog::error(
          "[WASI-NN] GGML backend: failed to llama_decode: try reducing the size of the batch or increasing the size of context"sv);
      return ErrNo::RuntimeError;
    } else if (Status < 0) {
      spdlog::error(
          "[WASI-NN] GGML backend: failed to llama_decode: internal fatal error. Please open an issue on GitHub"sv);
      return ErrNo::RuntimeError;
    }
    NPast += NEval;
  }

  return ErrNo::Success;
}

void batchAddSeq(llama_batch &Batch, const std::vector<llama_token> &Tokens,
                 llama_seq_id SequenceId) noexcept {
  for (int I = 0; I < static_cast<int>(Tokens.size()); I++) {
    // llama_batch_add_seq(llama_batch, llama_token, llama_pos,
    // std::vector<llama_seq_id>, logits);
    llama_batch_add(Batch, Tokens[I], I, {SequenceId},
                    I == static_cast<int>(Tokens.size()) - 1);
  }
}

ErrNo batchDecode(llama_context *LlamaContext, llama_batch &Batch,
                  float *Output, int NEmbd) noexcept {
  // Clear previous kv_cache values (irrelevant for embeddings)
  llama_kv_cache_clear(LlamaContext);

  // Decode the batch.
  auto Status = llama_decode(LlamaContext, Batch);
  if (Status == 1) {
    spdlog::error(
        "[WASI-NN] GGML backend: failed to llama_decode: try reducing the size of the batch or increasing the size of context"sv);
    return ErrNo::RuntimeError;
  } else if (Status < 0) {
    spdlog::error(
        "[WASI-NN] GGML backend: failed to llama_decode: internal fatal error. Please open an issue on GitHub"sv);
    return ErrNo::RuntimeError;
  }

  for (int I = 0; I < Batch.n_tokens; I++) {
    if (!Batch.logits[I]) {
      continue;
    }

    // Try to get sequence embeddings.
    auto *Embd = llama_get_embeddings_seq(LlamaContext, Batch.seq_id[I][0]);
    if (Embd == nullptr) {
      Embd = llama_get_embeddings_ith(LlamaContext, I);
      if (Embd == nullptr) {
        spdlog::error(
            "[WASI-NN] GGML backend: failed to get embeddings for token {}"sv,
            I);
        continue;
      }
    }

    // Normalize the embeddings.
    llama_embd_normalize(Embd, Output, NEmbd);
  }

  return ErrNo::Success;
}

Expect<ErrNo> getEmbedding(WasiNNEnvironment &Env,
                           uint32_t ContextId) noexcept {
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  auto &GraphRef = Env.NNGraph[CxtRef.GraphId].get<Graph>();
  if (GraphRef.EnableDebugLog) {
    spdlog::info("[WASI-NN][Debug] GGML backend: getEmbedding"sv);
  }

  if (CxtRef.LlamaInputs.size() == 0) {
    spdlog::error("[WASI-NN] GGML backend: Llama input is not set!"sv);
    return ErrNo::InvalidArgument;
  }

  // Clear the outputs.
  if (GraphRef.EnableDebugLog) {
    spdlog::info(
        "[WASI-NN][Debug] GGML backend: clear the previous output and tokens"sv);
  }
  CxtRef.LlamaOutputs.clear();
  CxtRef.LlamaOutputTokens.clear();
  if (GraphRef.EnableDebugLog) {
    spdlog::info(
        "[WASI-NN][Debug] GGML backend: clear the previous output and tokens...Done"sv);
  }

  // Main predict loop.
  if (GraphRef.EnableDebugLog) {
    spdlog::info("[WASI-NN][Debug] GGML backend: handle embedding"sv);
  }
  // Initialize the llama context.
  llama_context_params ContextParams = llama_context_default_params();
  setupContextParam(GraphRef, ContextParams);
  // For non-causal models, batch size must be equal to ubatch size
  ContextParams.n_ubatch = ContextParams.n_batch;
  auto *LlamaContext =
      llama_new_context_with_model(GraphRef.LlamaModel, ContextParams);

  // Use the const sequence id here.
  const llama_seq_id SequenceId = 0;
  // Return value.
  auto ReturnCode = ErrNo::Success;

  // Add SEP if not present.
  if (CxtRef.LlamaInputs.back() != llama_token_sep(GraphRef.LlamaModel)) {
    CxtRef.LlamaInputs.push_back(llama_token_sep(GraphRef.LlamaModel));
  }

  // Check if the input is too long.
  if (static_cast<uint64_t>(CxtRef.LlamaInputs.size()) >
      ContextParams.n_batch) {
    if (GraphRef.EnableLog) {
      spdlog::info(
          "[WASI-NN] GGML backend: the prompt is too long. "
          "Your input has {} tokens exceeds batch size {}. "
          "Please reduce the input size or increase your batch-size."sv,
          CxtRef.LlamaInputs.size(), ContextParams.n_batch);
    }
    return ErrNo::PromptTooLong;
  }

  const int32_t NEmbd = llama_n_embd(GraphRef.LlamaModel);
  struct llama_batch Batch = llama_batch_init(
      /* n_tokens_alloc */ static_cast<int32_t>(GraphRef.BatchSize),
      /* embd */ 0,
      /* n_seq_max */ 1);
  std::vector<float> Embeddings(NEmbd);
  batchAddSeq(Batch, CxtRef.LlamaInputs, SequenceId);
  ReturnCode = batchDecode(LlamaContext, Batch, Embeddings.data(), NEmbd);
  if (ReturnCode != ErrNo::Success) {
    spdlog::error("[WASI-NN] GGML backend: failed to evaluate input tokens."sv);
    return ReturnCode;
  }

  buildOutputEmbedding(CxtRef.LlamaOutputs, NEmbd, Embeddings.data());

  if (GraphRef.EnableDebugLog) {
    spdlog::info(
        "[WASI-NN][Debug] GGML backend: enter embedding loop...Done"sv);
  }

  if (GraphRef.EnableLog) {
    llama_print_timings(LlamaContext);
  }

  // We free the contexts here to keep the ggml plugin stateless.
  // Users could fully control the contexts by themselves via their prompt.
  llama_free(LlamaContext);
  llama_batch_free(Batch);

  if (GraphRef.EnableDebugLog) {
    spdlog::info("[WASI-NN][Debug] GGML backend: getEmbedding...Done"sv);
  }

  return ErrNo::Success;
}

const std::string_view Base64ImageTagPrefix = "<img src=\"data:image/"sv;
const std::string_view Base64ImageBytesPrefix = ";base64,"sv;
const std::string_view Base64ImageTagSuffix = "\">"sv;
const std::string_view PromptImagePlaceholder = "<image>"sv;

bool containsBase64Image(Graph &GraphRef, std::string_view Prompt) noexcept {
  // Check if the prompt contains a base64 image.
  // Follow this link for the supported image formats:
  // https://github.com/ggerganov/llama.cpp/blob/master/common/stb_image.h

  auto Base64ImageTagBeginPos = Prompt.find(Base64ImageTagPrefix);
  if (Base64ImageTagBeginPos == std::string::npos) {
    if (GraphRef.EnableDebugLog) {
      spdlog::info(
          "[WASI-NN][Debug] GGML backend: No base64 image tag found in the prompt."sv);
    }
    return false;
  }
  auto Base64ImageTagEndPos =
      Prompt.find(Base64ImageTagSuffix, Base64ImageTagBeginPos);
  if (Base64ImageTagEndPos == std::string::npos) {
    if (GraphRef.EnableDebugLog) {
      spdlog::info(
          "[WASI-NN][Debug] GGML backend: Found an unclosed base64 image tag."sv);
    }
    return false;
  }
  return true;
}

struct llava_image_embed *
loadBase64ImageFromPrompt(Graph &GraphRef, clip_ctx *ClipContext,
                          std::string_view Prompt) noexcept {
  // Load the base64 image from the prompt.
  // Follow this link for the supported image formats:
  // https://github.com/ggerganov/llama.cpp/blob/master/common/stb_image.h

  if (GraphRef.EnableDebugLog) {
    spdlog::info("[WASI-NN][Debug] GGML backend: loadBase64ImageFromPrompt"sv);
  }

  // Find `<img src="data:image/`
  auto Base64ImageTagBeginPos = Prompt.find(Base64ImageTagPrefix);
  if (Base64ImageTagBeginPos == std::string::npos) {
    return nullptr;
  }

  // Find `;base64,` (skip the image type part)
  auto Base64ImageBytesBeginPos =
      Prompt.find(Base64ImageBytesPrefix, Base64ImageTagBeginPos);
  if (Base64ImageTagBeginPos == std::string::npos) {
    return nullptr;
  }

  // Find `">`
  auto Base64ImageTagEndPos =
      Prompt.find(Base64ImageTagSuffix, Base64ImageBytesBeginPos);
  if (Base64ImageTagEndPos == std::string::npos) {
    return nullptr;
  }

  auto Base64Str =
      Prompt.substr(Base64ImageBytesBeginPos + Base64ImageBytesPrefix.size(),
                    Base64ImageTagEndPos - Base64ImageBytesBeginPos -
                        Base64ImageBytesPrefix.size());

  // Decode the base64 image.
  auto RequiredBytes = base64::required_encode_size(Base64Str.size());
  auto ImageBytes = std::vector<unsigned char>(RequiredBytes);
  try {
    base64::decode(Base64Str.begin(), Base64Str.end(), ImageBytes.begin());
  } catch (const base64_error &E) {
    spdlog::error("[WASI-NN] GGML backend: Error when base64::decode: {}"sv,
                  E.what());
    return nullptr;
  }

  if (GraphRef.EnableDebugLog) {
    spdlog::info(
        "[WASI-NN][Debug] GGML backend: loadBase64ImageFromPrompt...Done"sv);
  }

  return llava_image_embed_make_with_bytes(
      ClipContext, static_cast<int>(GraphRef.Threads), ImageBytes.data(),
      static_cast<int>(ImageBytes.size()));
}

ErrNo replaceBase64ImagePlaceholderInPrompt(std::string &Prompt) noexcept {
  // Replace the base64 image in the prompt with a placeholder.

  // Find `<img src="data:image/`
  auto Base64ImageTagBeginPos = Prompt.find(Base64ImageTagPrefix);
  if (Base64ImageTagBeginPos == std::string::npos) {
    return ErrNo::InvalidArgument;
  }

  // Find `">`
  auto Base64ImageTagEndPos =
      Prompt.find(Base64ImageTagSuffix, Base64ImageTagBeginPos);
  if (Base64ImageTagEndPos == std::string::npos) {
    return ErrNo::InvalidArgument;
  }

  auto Base64ImageTagLength = Base64ImageTagEndPos - Base64ImageTagBeginPos +
                              Base64ImageTagSuffix.size();
  Prompt.replace(Base64ImageTagBeginPos, Base64ImageTagLength,
                 PromptImagePlaceholder);

  return ErrNo::Success;
}

} // namespace

Expect<ErrNo> load(WasiNNEnvironment &Env, Span<const Span<uint8_t>> Builders,
                   [[maybe_unused]] Device Device, uint32_t &GraphId) noexcept {
  // Add a new graph.
  Env.NNGraph.emplace_back(Backend::GGML);
  auto &GraphRef = Env.NNGraph.back().get<Graph>();

  // Initialize the plugin parameters.
  auto ContextDefault = llama_context_default_params();
  GraphRef.EnableLog = false;
  GraphRef.EnableDebugLog = false;
  GraphRef.StreamStdout = false;
  GraphRef.NPredict = ContextDefault.n_ctx;
  GraphRef.ReversePrompt = ""sv;
  GraphRef.MMProjModelPath = ""sv;
  GraphRef.ImagePath = ""sv;
  // Initialize the model parameters.
  GraphRef.NGPULayers = 0;
  // Initialize the context parameters.
  GraphRef.CtxSize = ContextDefault.n_ctx;
  GraphRef.BatchSize = ContextDefault.n_batch;
  GraphRef.Threads = ContextDefault.n_threads;
  // Initialize the sampling parameters.
  const llama_sampling_params SamplingDefault;
  GraphRef.Temp = SamplingDefault.temp;
  GraphRef.TopP = SamplingDefault.top_p;
  GraphRef.RepeatPenalty = SamplingDefault.penalty_repeat;
  GraphRef.PresencePenalty = SamplingDefault.penalty_present;
  GraphRef.FrequencyPenalty = SamplingDefault.penalty_freq;
  GraphRef.Grammar = SamplingDefault.grammar;

  // Set llama log callback.
  llama_log_set(LlamaLogCallback, &GraphRef);

  // If the graph builder length > 1, the data of builder[1] is the metadata.
  if (Builders.size() > 1) {
    const std::string Metadata(reinterpret_cast<char *>(Builders[1].data()),
                               Builders[1].size());
    // Ignore context or model updates when initializing the graph.
    auto Res = parseMetadata(GraphRef, Metadata);
    if (Res != ErrNo::Success) {
      spdlog::error("[WASI-NN] GGML backend: Failed to parse metadata."sv);
      Env.NNGraph.pop_back();
      return Res;
    }
  }
  if (GraphRef.EnableLog) {
    spdlog::info("[WASI-NN] GGML backend: LLAMA_COMMIT {}"sv, LLAMA_COMMIT);
    spdlog::info("[WASI-NN] GGML backend: LLAMA_BUILD_NUMBER {}"sv,
                 LLAMA_BUILD_NUMBER);
  }

  if (GraphRef.EnableDebugLog) {
    spdlog::info("[WASI-NN][Debug] GGML backend: Handling model path."sv);
  }
  // Handle the model path.
  auto Weight = Builders[0];
  const std::string_view BinModel(reinterpret_cast<char *>(Weight.data()),
                                  Weight.size());
  std::string ModelFilePath;
  if (BinModel.substr(0, 8) == "preload:"sv) {
    ModelFilePath = BinModel.substr(8);
  } else {
    if (GraphRef.EnableDebugLog) {
      spdlog::info(
          "[WASI-NN][Debug] GGML backend: Model path not found in nn-preload, "
          "write model into a tmpfile."sv);
    }
    // TODO: pass the model directly to ggml
    // Write ggml model to file.
    ModelFilePath = "ggml-model.bin"sv;
    std::ofstream TempFile(ModelFilePath, std::ios::out | std::ios::binary);
    if (!TempFile) {
      spdlog::error(
          "[WASI-NN] GGML backend: Failed to create the temporary file. "
          "Currently, our workaround involves creating a temporary model "
          "file named \"ggml-model.bin\" and passing this filename as a "
          "parameter to the ggml llama library."sv);
      Env.NNGraph.pop_back();
      return ErrNo::InvalidArgument;
    }
    TempFile.write(BinModel.data(), BinModel.size());
    TempFile.close();
    if (GraphRef.EnableDebugLog) {
      spdlog::info(
          "[WASI-NN][Debug] GGML backend: Write model into a tmpfile...Done"sv);
    }
  }
  if (GraphRef.EnableDebugLog) {
    spdlog::info(
        "[WASI-NN][Debug] GGML backend: Finished handling model path."sv);
  }
  // Check if the model exists.
  if (!std::filesystem::exists(std::filesystem::u8path(ModelFilePath))) {
    spdlog::error("[WASI-NN] GGML backend: Model file not found."sv);
    Env.NNGraph.pop_back();
    return ErrNo::ModelNotFound;
  }

  if (GraphRef.EnableDebugLog) {
    spdlog::info(
        "[WASI-NN][Debug] GGML backend: Initialize ggml model with given parameters"sv);
  }
  // Initialize ggml model with model parameters.
  GraphRef.ModelFilePath = ModelFilePath;
  llama_model_params ModelParams = llama_model_default_params();
  ModelParams.n_gpu_layers = static_cast<int32_t>(GraphRef.NGPULayers);
  ModelParams.main_gpu = static_cast<int32_t>(GraphRef.MainGPU);
  ModelParams.tensor_split = GraphRef.TensorSplit.data();
  ModelParams.use_mmap = GraphRef.UseMMap;
  GraphRef.LlamaModel =
      llama_load_model_from_file(GraphRef.ModelFilePath.c_str(), ModelParams);
  if (GraphRef.LlamaModel == nullptr) {
    spdlog::error("[WASI-NN] GGML backend: Error: unable to init model."sv);
    Env.NNGraph.pop_back();
    return ErrNo::InvalidArgument;
  }
  if (GraphRef.EnableDebugLog) {
    spdlog::info(
        "[WASI-NN][Debug] GGML backend: Initialize ggml model with given parameters...Done"sv);
  }

  // Store the loaded graph.
  GraphId = static_cast<uint32_t>(Env.NNGraph.size() - 1);

  // Disable llama log by default.
  log_disable();

  return ErrNo::Success;
}

Expect<ErrNo> initExecCtx(WasiNNEnvironment &Env, uint32_t GraphId,
                          uint32_t &ContextId) noexcept {
  auto &GraphRef = Env.NNGraph[GraphId].get<Graph>();
  if (GraphRef.EnableDebugLog) {
    spdlog::info("[WASI-NN][Debug] GGML backend: initExecCtx"sv);
  }
  Env.NNContext.emplace_back(GraphId, Env.NNGraph[GraphId]);
  ContextId = static_cast<uint32_t>(Env.NNContext.size() - 1);
  if (GraphRef.EnableLog) {
    spdlog::info("[WASI-NN] GGML backend: llama_system_info: {}"sv,
                 llama_print_system_info());
  }
  if (GraphRef.EnableDebugLog) {
    spdlog::info("[WASI-NN][Debug] GGML backend: initExecCtx...Done"sv);
  }
  return ErrNo::Success;
}

Expect<ErrNo> setInput(WasiNNEnvironment &Env, uint32_t ContextId,
                       uint32_t Index, const TensorData &Tensor) noexcept {
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  auto &GraphRef = Env.NNGraph[CxtRef.GraphId].get<Graph>();
  if (GraphRef.EnableDebugLog) {
    spdlog::info("[WASI-NN][Debug] GGML backend: setInput"sv);
  }

  bool IsModelParamsUpdated = false;
  // Use index 1 for metadata.
  if (Index == 1) {
    if (GraphRef.EnableDebugLog) {
      spdlog::info(
          "[WASI-NN][Debug] GGML backend: found Metadata, processing"sv);
    }
    const std::string Metadata(reinterpret_cast<char *>(Tensor.Tensor.data()),
                               Tensor.Tensor.size());
    auto Res = parseMetadata(GraphRef, Metadata, &IsModelParamsUpdated);

    if (Res != ErrNo::Success) {
      spdlog::error("[WASI-NN] GGML backend: Failed to parse metadata."sv);
      return Res;
    }

#ifndef __APPLE__
    // XXX: Due to the limitation of WASI-NN proposal,
    // this is a workaround for non-macOS devices.
    // However, if the model params is updated in Config stage,
    // then, we doesn't encourage to use this to avoid the model
    // reloading.
    {
      if (IsModelParamsUpdated) {
        llama_model_params ModelParams = llama_model_default_params();
        ModelParams.n_gpu_layers = static_cast<int32_t>(GraphRef.NGPULayers);
        llama_free_model(GraphRef.LlamaModel);
        GraphRef.LlamaModel = llama_load_model_from_file(
            GraphRef.ModelFilePath.c_str(), ModelParams);
        if (GraphRef.LlamaModel == nullptr) {
          spdlog::error(
              "[WASI-NN] GGML backend: Error: unable to init model."sv);
          Env.NNGraph.pop_back();
          return ErrNo::InvalidArgument;
        }
      }
    }
#endif

    if (GraphRef.EnableDebugLog) {
      spdlog::info(
          "[WASI-NN][Debug] GGML backend: found Metadata, processing...Done"sv);
    }
    return ErrNo::Success;
  }

  // Initialize the llama context.
  if (GraphRef.EnableDebugLog) {
    spdlog::info("[WASI-NN][Debug] GGML backend: init llama context"sv);
  }
  llama_context_params ContextParams = llama_context_default_params();
  setupContextParam(GraphRef, ContextParams);
  auto LlamaContext =
      llama_new_context_with_model(GraphRef.LlamaModel, ContextParams);
  if (GraphRef.EnableDebugLog) {
    spdlog::info("[WASI-NN][Debug] GGML backend: init llama context...Done"sv);
  }

  // Set the input.
  if (GraphRef.EnableDebugLog) {
    spdlog::info("[WASI-NN][Debug] GGML backend: set the input"sv);
  }
  const bool AddSpecial = true;
  const bool ParseSpecial = true;
  std::string Prompt(reinterpret_cast<char *>(Tensor.Tensor.data()),
                     Tensor.Tensor.size());
  CxtRef.LlamaInputs.clear();
  if (GraphRef.MMProjModelPath == ""sv) {
    // Text only prompt.
    if (GraphRef.EnableDebugLog) {
      spdlog::info("[WASI-NN][Debug] GGML backend: tokenize text prompt"sv);
    }
    CxtRef.LlamaInputs =
        llama_tokenize(LlamaContext, Prompt, AddSpecial, ParseSpecial);
    if (GraphRef.EnableDebugLog) {
      spdlog::info(
          "[WASI-NN][Debug] GGML backend: tokenize text prompt...Done"sv);
    }
  } else {
    // Handle llava format prompt.
    if (GraphRef.EnableDebugLog) {
      spdlog::info(
          "[WASI-NN][Debug] GGML backend: handle llava format prompt"sv);
    }
    // Check if the prompt contains a base64 image.
    bool ContainsBase64Image = containsBase64Image(GraphRef, Prompt);
    if (GraphRef.ImagePath == ""sv && ContainsBase64Image == false) {
      spdlog::error(
          "[WASI-NN] GGML backend: Error: when using llava model, "
          "you need to specify the image path or have the base64 encoded "
          "image in the prompt."sv);
      return ErrNo::InvalidArgument;
    }

    // Show some warnings.
    if (GraphRef.EnableLog) {
      if (GraphRef.CtxSize < 4096) {
        spdlog::info(
            "[WASI-NN] GGML backend: Context size is {}, "
            "we recommend context size >= 2048 when using llava-v1.5 "
            "and context size >= 4096 when using llava-v1.6 for better results."sv,
            GraphRef.CtxSize);
      }
    }

    // Load image for llava.
    int LlavaVerbosity = 0;
    if (GraphRef.EnableLog) {
      LlavaVerbosity = 1;
    }
    auto ClipContext =
        clip_model_load(GraphRef.MMProjModelPath.c_str(), LlavaVerbosity);
    if (ContainsBase64Image) {
      // Load the base64 image from the prompt.
      CxtRef.LlavaImageEmbd =
          loadBase64ImageFromPrompt(GraphRef, ClipContext, Prompt);
      // Replace the base64 image in the prompt with a placeholder.
      auto Res = replaceBase64ImagePlaceholderInPrompt(Prompt);
      if (Res != ErrNo::Success) {
        spdlog::error(
            "[WASI-NN] GGML backend: Error: unable to replace the base64 image in the prompt."sv);
        clip_free(ClipContext);
        return Res;
      }
    } else {
      // Load the image from the file.
      CxtRef.LlavaImageEmbd = llava_image_embed_make_with_filename(
          ClipContext, static_cast<int>(GraphRef.Threads),
          GraphRef.ImagePath.c_str());
    }
    clip_free(ClipContext);
    if (CxtRef.LlavaImageEmbd == nullptr) {
      spdlog::error(
          "[WASI-NN] GGML backend: Error: unable to load the image."sv);
      return ErrNo::InvalidArgument;
    }

    // We split prompt by <image> as placeholder and save the position.
    auto PlaceholderPosition = Prompt.find(PromptImagePlaceholder);
    if (PlaceholderPosition == std::string::npos) {
      spdlog::error(
          "[WASI-NN] GGML backend: Error: unable to find the placeholder in the llava prompt."sv);
      return ErrNo::InvalidArgument;
    }
    std::string PromptBeforeImage = Prompt.substr(0, PlaceholderPosition);
    std::string PromptAfterImage =
        Prompt.substr(PlaceholderPosition + PromptImagePlaceholder.length());
    std::vector<llama_token> EmbdInputBeforeImage = llama_tokenize(
        LlamaContext, PromptBeforeImage, AddSpecial, ParseSpecial);
    // Do not add special token (such as <BOS>, <EOS>, ... tokens.) to the
    // tokens after the image.
    std::vector<llama_token> EmbdInputAfterImage =
        llama_tokenize(LlamaContext, PromptAfterImage, false, ParseSpecial);
    CxtRef.LlavaImagePosition = EmbdInputBeforeImage.size();
    CxtRef.LlamaInputs.reserve(EmbdInputBeforeImage.size() +
                               EmbdInputAfterImage.size());
    CxtRef.LlamaInputs.insert(CxtRef.LlamaInputs.end(),
                              EmbdInputBeforeImage.begin(),
                              EmbdInputBeforeImage.end());
    CxtRef.LlamaInputs.insert(CxtRef.LlamaInputs.end(),
                              EmbdInputAfterImage.begin(),
                              EmbdInputAfterImage.end());
    if (GraphRef.EnableDebugLog) {
      spdlog::info(
          "[WASI-NN][Debug] GGML backend: handle llava format prompt...Done"sv);
    }
  }
  CxtRef.LlamaNInputs = CxtRef.LlamaInputs.size();
  if (GraphRef.EnableDebugLog) {
    spdlog::info("[WASI-NN][Debug] GGML backend: set the input...Done"sv);
  }

  // Delete the llama context.
  if (GraphRef.EnableDebugLog) {
    spdlog::info(
        "[WASI-NN][Debug] GGML backend: delete llama context to make it stateless"sv);
  }
  llama_free(LlamaContext);
  if (GraphRef.EnableDebugLog) {
    spdlog::info(
        "[WASI-NN][Debug] GGML backend: delete llama context to make it stateless...Done"sv);
  }

  if (GraphRef.EnableDebugLog) {
    spdlog::info("[WASI-NN][Debug] GGML backend: setInput...Done"sv);
  }
  return ErrNo::Success;
}

Expect<ErrNo> getOutput(WasiNNEnvironment &Env, uint32_t ContextId,
                        uint32_t Index, Span<uint8_t> OutBuffer,
                        uint32_t &BytesWritten) noexcept {
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  auto &GraphRef = Env.NNGraph[CxtRef.GraphId].get<Graph>();
  if (GraphRef.EnableDebugLog) {
    spdlog::info("[WASI-NN][Debug] GGML backend: getOutput with Index {}"sv,
                 Index);
  }
  // Index 1 is for the metadata of the outputs.
  if (Index == 1) {
    std::string Metadata;
    auto Res = buildOutputMetadata(CxtRef, Metadata);
    if (Res != ErrNo::Success) {
      spdlog::error(
          "[WASI-NN] GGML backend: Failed to build output metadata."sv);
      return Res;
    }
    std::copy_n(Metadata.data(), Metadata.length(), OutBuffer.data());
    BytesWritten = static_cast<uint32_t>(Metadata.length());
    if (GraphRef.EnableDebugLog) {
      spdlog::info(
          "[WASI-NN][Debug] GGML backend: getOutput with Index {}...Done"sv,
          Index);
    }
    return ErrNo::Success;
  }

  std::copy_n(CxtRef.LlamaOutputs.data(), CxtRef.LlamaOutputs.length(),
              OutBuffer.data());
  BytesWritten = static_cast<uint32_t>(CxtRef.LlamaOutputs.length());
  if (GraphRef.EnableDebugLog) {
    spdlog::info(
        "[WASI-NN][Debug] GGML backend: getOutput with Index {}...Done"sv,
        Index);
  }
  return ErrNo::Success;
}

Expect<ErrNo> compute(WasiNNEnvironment &Env, uint32_t ContextId) noexcept {
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  auto &GraphRef = Env.NNGraph[CxtRef.GraphId].get<Graph>();
  if (GraphRef.EnableDebugLog) {
    spdlog::info("[WASI-NN][Debug] GGML backend: compute"sv);
  }

  if (GraphRef.Embedding) {
    return getEmbedding(Env, ContextId);
  }

  if (CxtRef.LlamaInputs.size() == 0) {
    spdlog::error("[WASI-NN] GGML backend: Llama input is not set!"sv);
    return ErrNo::InvalidArgument;
  }

  // Clear the outputs.
  if (GraphRef.EnableDebugLog) {
    spdlog::info(
        "[WASI-NN][Debug] GGML backend: clear the previous output and tokens"sv);
  }
  CxtRef.LlamaOutputs.clear();
  CxtRef.LlamaOutputTokens.clear();
  if (GraphRef.EnableDebugLog) {
    spdlog::info(
        "[WASI-NN][Debug] GGML backend: clear the previous output and tokens...Done"sv);
  }

  // Initialize the llama context.
  gpt_params GPTParams;
  llama_context_params ContextParams = llama_context_default_params();
  setupGPTParam(GraphRef, GPTParams);
  setupContextParam(GraphRef, ContextParams);
  auto LlamaContext =
      llama_new_context_with_model(GraphRef.LlamaModel, ContextParams);
  struct llama_sampling_context *CtxSampling =
      llama_sampling_init(GPTParams.sparams);
  // Prepare variables;
  int32_t NPast = 0;
  uint64_t NRemain = GraphRef.NPredict;
  // Get the context size.
  const uint64_t NCtx = llama_n_ctx(LlamaContext);
  // Minus 4 for the special tokens. (Such as <BOS>, <EOS>, ... tokens.)
  const uint64_t MaxTokensListSize = NCtx - 4;
  // Return value.
  auto ReturnCode = ErrNo::Success;

  // Check if the input is too long.
  if (static_cast<uint64_t>(CxtRef.LlamaInputs.size()) > MaxTokensListSize) {
    if (GraphRef.EnableLog) {
      spdlog::info("[WASI-NN] GGML backend: the prompt is too long. Your input "
                   "has {} tokens. Please reduce it to {} tokens."sv,
                   CxtRef.LlamaInputs.size(), MaxTokensListSize);
    }
    return ErrNo::PromptTooLong;
  }

  // Evaluate input tokens.
  if (CxtRef.LlavaImageEmbd == nullptr) {
    // Text only prompt.
    ReturnCode = evaluateTokens(GraphRef, LlamaContext,
                                std::move(CxtRef.LlamaInputs), NPast);
    if (ReturnCode != ErrNo::Success) {
      spdlog::error(
          "[WASI-NN] GGML backend: failed to evaluate input tokens."sv);
      return ReturnCode;
    }
  } else {
    // Llava format prompt with image data.
    std::vector<llama_token> EmbdInputBeforeImage(
        CxtRef.LlamaInputs.begin(),
        CxtRef.LlamaInputs.begin() + CxtRef.LlavaImagePosition);
    std::vector<llama_token> EmbdInputAfterImage(CxtRef.LlamaInputs.begin() +
                                                     CxtRef.LlavaImagePosition,
                                                 CxtRef.LlamaInputs.end());
    ReturnCode = evaluateTokens(GraphRef, LlamaContext,
                                std::move(EmbdInputBeforeImage), NPast);
    if (ReturnCode != ErrNo::Success) {
      spdlog::error(
          "[WASI-NN] GGML backend: failed to evaluate input tokens before image."sv);
      return ReturnCode;
    }
    bool EvalImageStatus =
        llava_eval_image_embed(LlamaContext, CxtRef.LlavaImageEmbd,
                               static_cast<int>(GraphRef.BatchSize), &NPast);
    if (!EvalImageStatus) {
      spdlog::error(
          "[WASI-NN] GGML backend: failed to evaluate embed image tokens."sv);
      return ErrNo::RuntimeError;
    }
    ReturnCode = evaluateTokens(GraphRef, LlamaContext,
                                std::move(EmbdInputAfterImage), NPast);
    if (ReturnCode != ErrNo::Success) {
      spdlog::error(
          "[WASI-NN] GGML backend: failed to evaluate input tokens after image."sv);
      return ReturnCode;
    }
  }

  // Main predict loop.
  if (GraphRef.EnableDebugLog) {
    spdlog::info("[WASI-NN][Debug] GGML backend: enter main predict loop"sv);
  }
  while (NRemain > 0) {
    const llama_token Id =
        llama_sampling_sample(CtxSampling, LlamaContext, nullptr);
    llama_sampling_accept(CtxSampling, LlamaContext, Id, true);
    --NRemain;

    // Save the output token.
    CxtRef.LlamaOutputTokens.emplace_back(Id);
    CxtRef.LlamaOutputs += llama_token_to_piece(LlamaContext, Id);
    // When setting StreamStdout, we print the output to stdout.
    if (GraphRef.StreamStdout) {
      fmt::print("{}"sv, llama_token_to_piece(LlamaContext, Id));
      std::fflush(stdout);
    }
    // Break if reverse prompt is found.
    if (!GraphRef.ReversePrompt.empty() &&
        CxtRef.LlamaOutputs.find(GraphRef.ReversePrompt) != std::string::npos) {
      if (GraphRef.EnableLog) {
        spdlog::info("[WASI-NN] GGML backend: reverse prompt found"sv);
      }
      break;
    }
    // Deal with end of text token.
    if (llama_token_is_eog(GraphRef.LlamaModel,
                           llama_sampling_last(CtxSampling))) {
      if (GraphRef.EnableLog) {
        spdlog::info("[WASI-NN] GGML backend: EOS token found"sv);
      }
      break;
    }
    // Evaluate the output token.
    ReturnCode = evaluateTokens(GraphRef, LlamaContext, {Id}, NPast);
    if (ReturnCode != ErrNo::Success) {
      break;
    }
  }
  if (GraphRef.EnableDebugLog) {
    spdlog::info(
        "[WASI-NN][Debug] GGML backend: enter main predict loop...Done"sv);
  }
  // End of main predict loop.

  if (GraphRef.EnableLog) {
    llama_print_timings(LlamaContext);
  }

  // We free the contexts here to keep the ggml plugin stateless.
  // Users could fully control the contexts by themselves via their prompt.
  if (GraphRef.EnableDebugLog) {
    spdlog::info(
        "[WASI-NN][Debug] GGML backend: delete llama context to make it stateless"sv);
  }
  llama_sampling_free(CtxSampling);
  llama_free(LlamaContext);
  if (CxtRef.LlavaImageEmbd != nullptr) {
    llava_image_embed_free(CxtRef.LlavaImageEmbd);
    CxtRef.LlavaImageEmbd = nullptr;
  }
  if (GraphRef.EnableDebugLog) {
    spdlog::info(
        "[WASI-NN][Debug] GGML backend: delete llama context to make it stateless...Done"sv);
  }

  if (GraphRef.EnableDebugLog) {
    spdlog::info("[WASI-NN][Debug] GGML backend: compute...Done"sv);
  }

  return ReturnCode;
}

Expect<ErrNo> getOutputSingle(WasiNNEnvironment &Env, uint32_t ContextId,
                              uint32_t Index, Span<uint8_t> OutBuffer,
                              uint32_t &BytesWritten) noexcept {
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  auto &GraphRef = Env.NNGraph[CxtRef.GraphId].get<Graph>();
  if (GraphRef.EnableDebugLog) {
    spdlog::info(
        "[WASI-NN][Debug] GGML backend: getOutputSingle with Index {}"sv,
        Index);
  }
  // Index 1 is for the metadata of the outputs.
  if (Index == 1) {
    std::string Metadata;
    auto Res = buildOutputMetadata(CxtRef, Metadata);
    if (Res != ErrNo::Success) {
      spdlog::error(
          "[WASI-NN] GGML backend: Failed to build output metadata."sv);
      return Res;
    }
    std::copy_n(Metadata.data(), Metadata.length(), OutBuffer.data());
    BytesWritten = static_cast<uint32_t>(Metadata.length());
    if (GraphRef.EnableDebugLog) {
      spdlog::info(
          "[WASI-NN][Debug] GGML backend: getOutputSingle with Index {}...Done"sv,
          Index);
    }
    return ErrNo::Success;
  }
  std::string LastToken = llama_token_to_piece(CxtRef.LlamaContext,
                                               CxtRef.LlamaOutputTokens.back());
  std::copy_n(LastToken.data(), LastToken.length(), OutBuffer.data());
  BytesWritten = static_cast<uint32_t>(LastToken.length());
  if (GraphRef.EnableDebugLog) {
    spdlog::info(
        "[WASI-NN][Debug] GGML backend: getOutputSingle with Index {}...Done"sv,
        Index);
  }
  return ErrNo::Success;
}

Expect<ErrNo> computeSingle(WasiNNEnvironment &Env,
                            uint32_t ContextId) noexcept {
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  auto &GraphRef = Env.NNGraph[CxtRef.GraphId].get<Graph>();

  // Logging.
  if (GraphRef.EnableDebugLog) {
    spdlog::info("[WASI-NN][Debug] GGML backend: computeSingleToken"sv);
  }

  // New compute single token context.
  if (CxtRef.LlamaContext == nullptr) {
    // Check if the input is set before setting up the context.
    if (CxtRef.LlamaInputs.size() == 0) {
      spdlog::error("[WASI-NN] GGML backend: Llama input is not set!"sv);
      return ErrNo::InvalidArgument;
    }

    // Clear the outputs.
    if (GraphRef.EnableDebugLog) {
      spdlog::info(
          "[WASI-NN][Debug] GGML backend: clear the previous output and tokens"sv);
    }
    CxtRef.LlamaOutputs.clear();
    CxtRef.LlamaOutputTokens.clear();
    if (GraphRef.EnableDebugLog) {
      spdlog::info(
          "[WASI-NN][Debug] GGML backend: clear the previous output and tokens...Done"sv);
    }

    // Initialize the llama context.
    gpt_params GPTParams;
    llama_context_params ContextParams = llama_context_default_params();
    setupGPTParam(GraphRef, GPTParams);
    setupContextParam(GraphRef, ContextParams);
    CxtRef.LlamaContext =
        llama_new_context_with_model(GraphRef.LlamaModel, ContextParams);
    CxtRef.LlamaSampling = llama_sampling_init(GPTParams.sparams);
    CxtRef.LlamaNPast = 0;

    // Get the context size.
    const uint64_t NCtx = llama_n_ctx(CxtRef.LlamaContext);
    // Minus 4 for the special tokens. (Such as <BOS>, <EOS>, ... tokens.)
    const uint64_t MaxTokensListSize = NCtx - 4;
    // Return value.
    auto ReturnCode = ErrNo::Success;

    // Check if the input is too long.
    if (static_cast<uint64_t>(CxtRef.LlamaInputs.size()) > MaxTokensListSize) {
      if (GraphRef.EnableLog) {
        spdlog::info(
            "[WASI-NN] GGML backend: the prompt is too long. Your input has {} tokens. Please reduce it to {} tokens."sv,
            CxtRef.LlamaInputs.size(), MaxTokensListSize);
      }
      return ErrNo::PromptTooLong;
    }

    // Evaluate input tokens.
    if (CxtRef.LlavaImageEmbd == nullptr) {
      // Text only prompt.
      ReturnCode =
          evaluateTokens(GraphRef, CxtRef.LlamaContext,
                         std::move(CxtRef.LlamaInputs), CxtRef.LlamaNPast);
      if (ReturnCode != ErrNo::Success) {
        spdlog::error(
            "[WASI-NN] GGML backend: failed to evaluate input tokens."sv);
        return ReturnCode;
      }
    } else {
      // Llava format prompt with image data.
      std::vector<llama_token> EmbdInputBeforeImage(
          CxtRef.LlamaInputs.begin(),
          CxtRef.LlamaInputs.begin() + CxtRef.LlavaImagePosition);
      std::vector<llama_token> EmbdInputAfterImage(
          CxtRef.LlamaInputs.begin() + CxtRef.LlavaImagePosition,
          CxtRef.LlamaInputs.end());
      ReturnCode =
          evaluateTokens(GraphRef, CxtRef.LlamaContext,
                         std::move(EmbdInputBeforeImage), CxtRef.LlamaNPast);
      if (ReturnCode != ErrNo::Success) {
        spdlog::error(
            "[WASI-NN] GGML backend: failed to evaluate input tokens before image."sv);
        return ReturnCode;
      }
      bool EvalImageStatus = llava_eval_image_embed(
          CxtRef.LlamaContext, CxtRef.LlavaImageEmbd,
          static_cast<int>(GraphRef.BatchSize), &CxtRef.LlamaNPast);
      if (!EvalImageStatus) {
        spdlog::error(
            "[WASI-NN] GGML backend: failed to evaluate embed image tokens."sv);
        return ErrNo::RuntimeError;
      }
      ReturnCode =
          evaluateTokens(GraphRef, CxtRef.LlamaContext,
                         std::move(EmbdInputAfterImage), CxtRef.LlamaNPast);
      if (ReturnCode != ErrNo::Success) {
        spdlog::error(
            "[WASI-NN] GGML backend: failed to evaluate input tokens after image."sv);
        return ReturnCode;
      }
    }
  }

  // Main predict process.
  if (GraphRef.EnableDebugLog) {
    spdlog::info("[WASI-NN][Debug] GGML backend: enter main predict process"sv);
  }
  auto ReturnCode = ErrNo::Success;
  const llama_token Id =
      llama_sampling_sample(CxtRef.LlamaSampling, CxtRef.LlamaContext, nullptr);
  llama_sampling_accept(CxtRef.LlamaSampling, CxtRef.LlamaContext, Id, true);

  // Save the output token.
  // In single token mode, we do not handle StreamStdout and ReversePrompt.
  CxtRef.LlamaOutputTokens.emplace_back(Id);
  CxtRef.LlamaOutputs += llama_token_to_piece(CxtRef.LlamaContext, Id);
  // Deal with end of text token.
  if (llama_token_is_eog(GraphRef.LlamaModel,
                         llama_sampling_last(CxtRef.LlamaSampling))) {
    ReturnCode = ErrNo::EndOfSequence;
    if (GraphRef.EnableLog) {
      spdlog::info("[WASI-NN] GGML backend: EOS token found"sv);
    }
  }
  // Evaluate the output token if not EOS.
  if (ReturnCode != ErrNo::EndOfSequence) {
    ReturnCode =
        evaluateTokens(GraphRef, CxtRef.LlamaContext, {Id}, CxtRef.LlamaNPast);
  }
  if (GraphRef.EnableDebugLog) {
    spdlog::info(
        "[WASI-NN][Debug] GGML backend: enter main predict process...Done"sv);
  }
  // End of main predict process.

  if (GraphRef.EnableDebugLog) {
    spdlog::info("[WASI-NN][Debug] GGML backend: computeSingleToken...Done"sv);
  }

  return ReturnCode;
}

Expect<ErrNo> finiSingle(WasiNNEnvironment &Env, uint32_t ContextId) noexcept {
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  auto &GraphRef = Env.NNGraph[CxtRef.GraphId].get<Graph>();

  if (GraphRef.EnableDebugLog) {
    spdlog::info("[WASI-NN][Debug] GGML backend: finiSingle"sv);
  }

  // Logging for the llama timings.
  if (GraphRef.EnableLog) {
    llama_print_timings(CxtRef.LlamaContext);
  }

  // Clear the outputs.
  if (GraphRef.EnableDebugLog) {
    spdlog::info(
        "[WASI-NN][Debug] GGML backend: finiSingle: clear the previous output and tokens"sv);
  }
  CxtRef.LlamaOutputs.clear();
  CxtRef.LlamaOutputTokens.clear();
  if (GraphRef.EnableDebugLog) {
    spdlog::info(
        "[WASI-NN][Debug] GGML backend: finiSingle: clear the previous output and tokens...Done"sv);
  }

  // Delete the llama context.
  if (GraphRef.EnableDebugLog) {
    spdlog::info(
        "[WASI-NN][Debug] GGML backend: finiSingle: free the llama context"sv);
  }
  llama_sampling_free(CxtRef.LlamaSampling);
  llama_free(CxtRef.LlamaContext);
  CxtRef.LlamaSampling = nullptr;
  CxtRef.LlamaContext = nullptr;
  if (CxtRef.LlavaImageEmbd != nullptr) {
    llava_image_embed_free(CxtRef.LlavaImageEmbd);
    CxtRef.LlavaImageEmbd = nullptr;
  }
  if (GraphRef.EnableDebugLog) {
    spdlog::info(
        "[WASI-NN][Debug] GGML backend: finiSingle: free the llama context...Done"sv);
  }

  // Reset the context variables.
  CxtRef.LlamaNPast = 0;

  if (GraphRef.EnableDebugLog) {
    spdlog::info("[WASI-NN][Debug] GGML backend: finiSingle...Done"sv);
  }

  return ErrNo::Success;
}

Expect<ErrNo> unload(WasiNNEnvironment &Env, uint32_t GraphId) noexcept {
  auto &GraphRef = Env.NNGraph[GraphId].get<Graph>();
  if (GraphRef.EnableDebugLog) {
    spdlog::info("[WASI-NN][Debug] GGML backend: unload"sv);
  }
  if (GraphRef.LlamaModel != nullptr) {
    if (GraphRef.EnableDebugLog) {
      spdlog::info("[WASI-NN][Debug] GGML backend: unload: free llama model"sv);
    }
    llama_free_model(GraphRef.LlamaModel);
    GraphRef.LlamaModel = nullptr;
    if (GraphRef.EnableDebugLog) {
      spdlog::info(
          "[WASI-NN][Debug] GGML backend: unload: free llama model...Done"sv);
    }
  }
  Env.NNGraph.erase(Env.NNGraph.begin() + GraphId);
  Env.mdRemoveById(GraphId);
  if (GraphRef.EnableDebugLog) {
    spdlog::info("[WASI-NN][Debug] GGML backend: unload...Done"sv);
  }
  return ErrNo::Success;
}

#else
namespace {
Expect<ErrNo> reportBackendNotSupported() noexcept {
  spdlog::error("[WASI-NN] ggml backend is not built. use "
                "-WASMEDGE_PLUGIN_WASI_NN_BACKEND=\"ggml\" to build it."sv);
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
Expect<ErrNo> getOutputSingle(WasiNNEnvironment &, uint32_t, uint32_t,
                              Span<uint8_t>, uint32_t &) noexcept {
  return reportBackendNotSupported();
}
Expect<ErrNo> computeSingle(WasiNNEnvironment &, uint32_t) noexcept {
  return reportBackendNotSupported();
}
Expect<ErrNo> finiSingle(WasiNNEnvironment &, uint32_t) noexcept {
  return reportBackendNotSupported();
}
Expect<ErrNo> unload(WasiNNEnvironment &, uint32_t) noexcept {
  return reportBackendNotSupported();
}

#endif
} // namespace WasmEdge::Host::WASINN::GGML
