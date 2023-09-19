// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "ggml.h"
#include "wasinnenv.h"

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_GGML
#include <common.h>
#include <cstdlib>
#include <llama.h>
#endif

namespace WasmEdge::Host::WASINN::GGML {
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_GGML
Expect<ErrNo> load(WasiNNEnvironment &Env, Span<const Span<uint8_t>> Builders,
                   Device Device, uint32_t &GraphId) noexcept {
  // The graph builder length must be 1.
  if (Builders.size() != 1) {
    spdlog::error("[WASI-NN] Wrong GraphBuilder Length {:d}, expect 1"sv,
                  Builders.size());
    return ErrNo::InvalidArgument;
  }

  // Setup Graph Device
  if (Device != Device::CPU) {
    spdlog::error(
        "[WASI-NN] ggml backend only support CPU target currently."sv);
    return ErrNo::InvalidArgument;
  }

  auto Weight = Builders[0];
  std::string BinModel(reinterpret_cast<char *>(Weight.data()), Weight.size());
  std::istringstream BinRead(BinModel);

  // TODO: pass the model directly to ggml
  // Write ggml model to file.
  std::string ModelFilePath("ggml-model.bin"sv);
  std::ofstream TempFile(ModelFilePath);
  if (!TempFile) {
    spdlog::error("[WASI-NN] Failed to create the temporary file. Currently, "
                  "our workaround involves creating a temporary model file "
                  "named \"ggml-model.bin\" and passing this filename as a "
                  "parameter to the ggml llama library."sv);
    return ErrNo::InvalidArgument;
  }
  TempFile << BinModel;
  TempFile.close();

  // Add a new graph.
  Env.NNGraph.emplace_back(Backend::GGML);
  auto &GraphRef = Env.NNGraph.back().get<Graph>();

  // Initialize ggml model.
  gpt_params Params;
  Params.model = ModelFilePath;
  llama_backend_init(Params.numa);
  std::tie(GraphRef.LlamaModel, GraphRef.LlamaContext) =
      llama_init_from_gpt_params(Params);
  if (GraphRef.LlamaModel == nullptr) {
    spdlog::error("[WASI-NN] Error: unable to init model."sv);
    Env.NNGraph.pop_back();
    return ErrNo::InvalidArgument;
  }

  // Store the loaded graph.
  GraphId = Env.NNGraph.size() - 1;

  // Disable llama log by default.
  log_disable();

  return ErrNo::Success;
}

Expect<ErrNo> initExecCtx(WasiNNEnvironment &Env, uint32_t GraphId,
                          uint32_t &ContextId) noexcept {
  Env.NNContext.emplace_back(GraphId, Env.NNGraph[GraphId]);

  ContextId = Env.NNContext.size() - 1;
  return ErrNo::Success;
}

Expect<ErrNo> setInput(WasiNNEnvironment &Env, uint32_t ContextId,
                       [[maybe_unused]] uint32_t Index,
                       const TensorData &Tensor) noexcept {
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  auto &GraphRef = Env.NNGraph[CxtRef.GraphId].get<Graph>();
  std::string Prompt(reinterpret_cast<char *>(Tensor.Tensor.data()),
                     Tensor.Tensor.size());
  CxtRef.LlamaInputs = llama_tokenize(GraphRef.LlamaContext, Prompt, true);
  const uint32_t MaxContextSize = llama_n_ctx(GraphRef.LlamaContext);
  // Minus 4 for the special tokens.
  const uint32_t MaxTokensListSize = MaxContextSize - 4;
  if (CxtRef.LlamaInputs.size() > MaxTokensListSize) {
    spdlog::error("[WASI-NN]: Error: prompt too long ({} tokens, max %{})"sv,
                  CxtRef.LlamaInputs.size(), MaxTokensListSize);
    return ErrNo::InvalidArgument;
  }
  return ErrNo::Success;
}

Expect<ErrNo> getOutput(WasiNNEnvironment &Env, uint32_t ContextId,
                        [[maybe_unused]] uint32_t Index,
                        Span<uint8_t> OutBuffer,
                        uint32_t &BytesWritten) noexcept {
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  std::copy_n(CxtRef.LlamaOutputs.data(), CxtRef.LlamaOutputs.length(),
              OutBuffer.data());
  BytesWritten = CxtRef.LlamaOutputs.length();
  return ErrNo::Success;
}

Expect<ErrNo> compute(WasiNNEnvironment &Env, uint32_t ContextId) noexcept {
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  auto &GraphRef = Env.NNGraph[CxtRef.GraphId].get<Graph>();
  if (CxtRef.LlamaInputs.size() == 0) {
    spdlog::error("[WASI-NN] Llama input is not set!"sv);
    return ErrNo::InvalidArgument;
  }

  // Use env LLAMA_LOG=1 to enable llama log.
  const char *LlamaLogEnv = std::getenv("LLAMA_LOG");
  if (LlamaLogEnv != nullptr) {
    spdlog::info("llama_system_info: {}"sv, llama_print_system_info());
  }

  // Clear the outputs.
  CxtRef.LlamaOutputs = ""sv;

  // Main predict loop.
  // TODO: recompute a compressed context based on previous tokens once the
  // cache is full.
  const int MaxContextSize = llama_n_ctx(GraphRef.LlamaContext);
  while (llama_get_kv_cache_token_count(GraphRef.LlamaContext) <
         MaxContextSize) {
    if (llama_eval(GraphRef.LlamaContext, CxtRef.LlamaInputs.data(),
                   int(CxtRef.LlamaInputs.size()),
                   llama_get_kv_cache_token_count(GraphRef.LlamaContext),
                   get_num_physical_cores())) {
      spdlog::error("[WASI-NN] Llama failed to eval."sv);
      return ErrNo::InvalidArgument;
    }
    CxtRef.LlamaInputs.clear();

    // Select the best prediction.
    llama_token NewTokenId = 0;
    auto Logits = llama_get_logits(GraphRef.LlamaContext);
    auto NVocab = llama_n_vocab(GraphRef.LlamaContext);
    std::vector<llama_token_data> Candidates;
    Candidates.reserve(NVocab);
    for (llama_token TokenId = 0; TokenId < NVocab; TokenId++) {
      Candidates.emplace_back(llama_token_data{TokenId, Logits[TokenId], 0.0f});
    }
    llama_token_data_array CandidatesP = {Candidates.data(), Candidates.size(),
                                          false};
    NewTokenId = llama_sample_token_greedy(GraphRef.LlamaContext, &CandidatesP);

    if (NewTokenId == llama_token_eos(GraphRef.LlamaContext)) {
      CxtRef.LlamaOutputs += "[end of text]"sv;
      break;
    }

    // Append the new token.
    CxtRef.LlamaOutputs +=
        llama_token_to_piece(GraphRef.LlamaContext, NewTokenId);

    // Push this new token for next evaluation.
    CxtRef.LlamaInputs.push_back(NewTokenId);
  }

  if (LlamaLogEnv != nullptr) {
    llama_print_timings(GraphRef.LlamaContext);
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

#endif
} // namespace WasmEdge::Host::WASINN::GGML
