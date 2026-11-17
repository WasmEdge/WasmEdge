// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "inference_manager.h"
#include "GGML/core/ggml_core.h"

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_GGML
#include <algorithm>
#include <exception>
#include <fmt/ranges.h>
#endif

namespace WasmEdge::Host::WASINN::GGML {
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_GGML
uint32_t getBatchSize(const Graph &GraphRef) noexcept {
  const auto RequestedSize = static_cast<uint32_t>(GraphRef.Params.n_batch);
  const auto ContextSize = llama_n_batch(GraphRef.LlamaContext.get());
  if (RequestedSize > ContextSize) {
    LOG_WARN(
        "getBatchSize: requested batch-size {} exceeds the current context's "sv
        "n_batch {}; using {}. Increasing batch-size via set_input does not "sv
        "rebuild the context. Reload the model with the desired batch-size."sv,
        RequestedSize, ContextSize, ContextSize)
  }
  return std::min(RequestedSize, ContextSize);
}

namespace {

// Evaluate tokens. Construct the batch from tokens and decode.
ErrNo evaluateTokens(Span<const llama_token> Tokens, Graph &GraphRef, int &NPos,
                     bool IsLogits = false) noexcept {
  // End the inference if the context is full.
  uint32_t NCtx = llama_n_ctx(GraphRef.LlamaContext.get());
  if (NPos + static_cast<uint32_t>(Tokens.size()) > NCtx) {
    LOG_INFO(
        GraphRef.EnableLog,
        "evaluateTokens: the context if full ({} / {} tokens). Please increase your "sv
        "context size."sv,
        NPos + static_cast<uint32_t>(Tokens.size()), NCtx)
    return ErrNo::ContextFull;
  }

  try {
    llama_batch_ext_ptr Batch(
        llama_batch_ext_init(GraphRef.LlamaContext.get()));
    const auto BatchSize = getBatchSize(GraphRef);
    if (BatchSize == 0) {
      RET_ERROR(ErrNo::RuntimeError, "evaluateTokens: batch size is zero."sv)
    }
    for (size_t I = 0; I < Tokens.size();) {
      const size_t NEval =
          std::min(Tokens.size() - I, static_cast<size_t>(BatchSize));
      llama_batch_ext_clear(Batch.get());
      for (size_t J = 0; J < NEval; ++J) {
        const int32_t Idx =
            llama_batch_ext_add_token(Batch.get(), 0, Tokens[I + J]);
        const llama_pos Pos = NPos + static_cast<llama_pos>(J);
        if (Idx < 0 || !llama_batch_ext_set_pos(Batch.get(), Idx, &Pos)) {
          RET_ERROR(ErrNo::RuntimeError,
                    "evaluateTokens: failed to add token to batch."sv)
        }
      }
      if (IsLogits && I + NEval == Tokens.size() &&
          !llama_batch_ext_set_output_logits(
              Batch.get(), static_cast<int32_t>(NEval - 1), true)) {
        RET_ERROR(ErrNo::RuntimeError,
                  "evaluateTokens: failed to set batch output."sv)
      }
      NPos += static_cast<int>(NEval);
      const auto Status = llama_process(GraphRef.LlamaContext.get(),
                                        LLAMA_PROCESS_TYPE_DECODE, Batch.get());
      if (Status == 1) {
        RET_ERROR(
            ErrNo::RuntimeError,
            "evaluateTokens: failed to process batch: try reducing the size of "sv
            "the batch or increasing the size of context."sv)
      }
      if (Status != 0) {
        RET_ERROR(ErrNo::RuntimeError,
                  "evaluateTokens: failed to process batch: {}."sv, Status)
      }
      I += NEval;
    }
  } catch (const std::exception &E) {
    RET_ERROR(ErrNo::RuntimeError,
              "evaluateTokens: failed to process batch: {}"sv, E.what())
  }

  return ErrNo::Success;
}
// Generate output embedding.
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
      fmt::format(R"({{"n_embedding": {}, )"
                  R"("embedding": [{:.10}]}})"sv,
                  NEmbd, fmt::join(Embeddings, Embeddings + NEmbd, ","sv));
}
} // namespace

// Evaluate the input tokens. Clear all inputs on success.
ErrNo evaluateInput(Graph &GraphRef, Context &CxtRef,
                    std::string_view LogPrefix) noexcept {
  // Check if the input is set before setting up the context.
  if (CxtRef.LlamaInputs.size() == 0) {
    RET_ERROR(ErrNo::InvalidArgument, "{}: llama input is not set!"sv,
              LogPrefix)
  }

  // Get the context size.
  const uint64_t NCtx = llama_n_ctx(GraphRef.LlamaContext.get());
  // Minus 4 for the special tokens. (Such as <BOS>, <EOS>, ... tokens.)
  const uint64_t MaxTokensListSize = NCtx - 4;
  // Return value.
  auto ReturnCode = ErrNo::Success;

  // Check if the input is too long.
  if (static_cast<uint64_t>(CxtRef.LlamaInputs.size()) > MaxTokensListSize) {
    RET_ERROR(ErrNo::PromptTooLong,
              "{}: the prompt is too long. Your input has {} tokens. "sv
              "Please reduce it to {} tokens."sv,
              LogPrefix, CxtRef.LlamaInputs.size(), MaxTokensListSize)
  }

  // Evaluate input tokens.
  ReturnCode =
      evaluateTokens(Span<const llama_token>(CxtRef.LlamaInputs.begin(),
                                             CxtRef.LlamaInputs.size()),
                     GraphRef, CxtRef.NPos, true);
  if (ReturnCode != ErrNo::Success) {
    RET_ERROR(ReturnCode, "{}: failed to evaluate input tokens."sv, LogPrefix)
  }

  return ErrNo::Success;
}

// Clear the context and reset the sampler.
void clearContext(Graph &GraphRef, Context &CxtRef) noexcept {
  LOG_DEBUG(GraphRef.EnableDebugLog, "{}: clearContext"sv)
  llama_memory_clear(llama_get_memory(GraphRef.LlamaContext.get()), true);
  common_sampler_reset(CxtRef.LlamaSampler);
  CxtRef.NPos = 0;
  CxtRef.LlamaOutputs.clear();
  CxtRef.LlamaOutputTokens.clear();
  LOG_DEBUG(GraphRef.EnableDebugLog, "{}: clearContext...Done"sv)
}

// TODO: Merge into compute.
Expect<ErrNo> getEmbedding(Graph &GraphRef, Context &CxtRef) noexcept {
  LOG_DEBUG(GraphRef.EnableDebugLog, "getEmbedding"sv)

  const llama_vocab *Vocab = llama_model_get_vocab(GraphRef.LlamaModel.get());
  // Add SEP if not present.
  if (CxtRef.LlamaInputs.size() > 0 &&
      CxtRef.LlamaInputs.back() != llama_vocab_sep(Vocab)) {
    LOG_WARN(
        "getEmbedding: last token in the prompt is not SEP, "sv
        "'tokenizer.ggml.add_eos_token' should be set to 'true' in the GGUF "sv
        "header."sv)
  }

  const auto BatchSize = std::min(getBatchSize(GraphRef),
                                  llama_n_ubatch(GraphRef.LlamaContext.get()));
  // Check if the input is too long.
  if (CxtRef.LlamaInputs.size() > BatchSize) {
    RET_ERROR(
        ErrNo::PromptTooLong,
        "getEmbedding: the prompt has {} tokens, exceeding the effective "sv
        "limit of {} (requested batch-size: {}, context n_batch: {}, "sv
        "context n_ubatch: {}). Reduce the input size or reload the model "sv
        "with larger batch-size and ubatch-size; changing these settings via "sv
        "set_input alone does not rebuild the context."sv,
        CxtRef.LlamaInputs.size(), BatchSize, GraphRef.Params.n_batch,
        llama_n_batch(GraphRef.LlamaContext.get()),
        llama_n_ubatch(GraphRef.LlamaContext.get()))
  }

  // Evaluate the input tokens.
  auto ReturnCode = evaluateInput(GraphRef, CxtRef, "getEmbedding"sv);
  if (ReturnCode != ErrNo::Success) {
    return ReturnCode;
  }

  // Main prediction loop.
  const int32_t NEmbd = llama_model_n_embd(GraphRef.LlamaModel.get());
  std::vector<float> Embeddings(NEmbd);

  const int32_t LastIndex = static_cast<int32_t>(CxtRef.LlamaInputs.size() - 1);
  auto *Embd = llama_get_embeddings_seq(GraphRef.LlamaContext.get(), 0);
  if (Embd == nullptr) {
    Embd = llama_get_embeddings_ith(GraphRef.LlamaContext.get(), LastIndex);
  }
  if (Embd != nullptr) {
    common_embd_normalize(Embd, Embeddings.data(), NEmbd,
                          static_cast<int32_t>(CxtRef.Conf.EmbdNormalize));
  } else {
    LOG_ERROR("getEmbedding: failed to get embeddings for token {}"sv,
              LastIndex);
  }

  std::string EmbeddingString;
  buildOutputEmbedding(EmbeddingString, NEmbd, Embeddings.data());
  CxtRef.LlamaOutputs =
      std::vector<uint8_t>(EmbeddingString.begin(), EmbeddingString.end());

  if (GraphRef.EnableLog) {
    common_perf_print(GraphRef.LlamaContext.get(), /* Sampler */ nullptr);
  }

  LOG_DEBUG(GraphRef.EnableDebugLog, "getEmbedding...Done"sv)
  return ErrNo::Success;
}

// Sample and get the output token.
ErrNo sampleOutput(Graph &GraphRef, Context &CxtRef,
                   bool IsSingleTokenMode) noexcept {
  // Use idx = -1 to sample the next token.
  const llama_token Id = common_sampler_sample(
      CxtRef.LlamaSampler, GraphRef.LlamaContext.get(), /* idx */ -1);
  common_sampler_accept(CxtRef.LlamaSampler, Id, /* accept_grammar */ true);

  // Save the output token.
  CxtRef.LlamaOutputTokens.emplace_back(Id);
  std::string OutputString =
      common_token_to_piece(GraphRef.LlamaContext.get(), Id);
  CxtRef.LlamaOutputs.insert(CxtRef.LlamaOutputs.end(), OutputString.begin(),
                             OutputString.end());
  // In single token mode, we do not handle StreamStdout and ReversePrompt.
  if (!IsSingleTokenMode) {
    // When setting StreamStdout, we print the output to stdout.
    if (CxtRef.Conf.StreamStdout) {
      fmt::print("{}"sv,
                 common_token_to_piece(GraphRef.LlamaContext.get(), Id));
      std::fflush(stdout);
    }
    // Break if reverse prompt is found.
    if (!CxtRef.Conf.ReversePrompt.empty() &&
        std::string(CxtRef.LlamaOutputs.begin(), CxtRef.LlamaOutputs.end())
                .find(CxtRef.Conf.ReversePrompt) != std::string::npos) {
      LOG_INFO(GraphRef.EnableLog, "sampleOutput: reverse prompt found."sv)
      return ErrNo::EndOfSequence;
    }
  }
  // Deal with end of text token.
  const llama_vocab *Vocab = llama_model_get_vocab(GraphRef.LlamaModel.get());
  // Only stop on EOS if GraphRef.Params.sampling.ignore_eos is false.
  if (!GraphRef.Params.sampling.ignore_eos &&
      llama_vocab_is_eog(Vocab, common_sampler_last(CxtRef.LlamaSampler))) {
    LOG_INFO(GraphRef.EnableLog, "sampleOutput: EOS token found."sv)
    return ErrNo::EndOfSequence;
  }
  // Evaluate the output token.
  return evaluateTokens(Span<const llama_token>(&Id, 1), GraphRef, CxtRef.NPos,
                        true);
}

#endif
} // namespace WasmEdge::Host::WASINN::GGML
