// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "ggml.h"
#include "wasinnenv.h"

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_GGML
#include <common.h>
#include <llama.h>
#endif

namespace WasmEdge::Host::WASINN::Ggml {
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_GGML
Expect<ErrNo> load(WasiNNEnvironment &Env, Span<const Span<uint8_t>> Builders,
                   Device Device, uint32_t &GraphId) noexcept {
  // The graph builder length must be 1.
  if (Builders.size() != 1) {
    spdlog::error("[WASI-NN] Wrong GraphBuilder Length {:d}, expect 1",
                  Builders.size());
    return ErrNo::InvalidArgument;
  }

  // Add a new graph.
  Env.NNGraph.emplace_back(Backend::Ggml);
  auto &GraphRef = Env.NNGraph.back().get<Graph>();

  // Setup Graph Device
  if (Device != Device::CPU) {
    spdlog::error("[WASI-NN] ggml backend only support CPU target currently.");
    return ErrNo::InvalidArgument;
  }

  auto Weight = Builders[0];
  std::string BinModel(reinterpret_cast<char *>(Weight.data()), Weight.size());
  std::istringstream BinRead(BinModel);

  if (BinModel.substr(0, 4) != "tjgg") {
    spdlog::error("[WASI-NN] Invalid ggml model.");
    return ErrNo::InvalidArgument;
  }

  // TODO: pass the model directly to ggml
  // Write ggml model to file.
  std::string modelfilepath("ggml-model.bin");
  std::ofstream tempFile(modelfilepath);
  if (!tempFile) {
    spdlog::error("[WASI-NN] Failed to create the temporary file.");
    return ErrNo::InvalidArgument;
  }
  tempFile << BinModel;
  tempFile.close();

  // Initialize ggml model.
  gpt_params params;
  params.model = modelfilepath;
  llama_backend_init(params.numa);
  std::tie(GraphRef.LlamaModel, GraphRef.LlamaContext) =
      llama_init_from_gpt_params(params);
  if (GraphRef.LlamaModel == nullptr) {
    spdlog::error("[WASI-NN] Error: unable to load model.");
    return ErrNo::InvalidArgument;
  }

  // Store the loaded graph.
  GraphId = Env.NNGraph.size() - 1;
  return ErrNo::Success;
}

Expect<ErrNo> initExecCtx(WasiNNEnvironment &Env, uint32_t GraphId,
                          uint32_t &ContextId) noexcept {
  Env.NNContext.emplace_back(GraphId, Env.NNGraph[GraphId]);

  ContextId = Env.NNContext.size() - 1;
  return ErrNo::Success;
}

Expect<ErrNo> setInput(WasiNNEnvironment &Env, uint32_t ContextId,
                       __attribute__((unused)) uint32_t Index,
                       const TensorData &Tensor) noexcept {
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  auto &GraphRef = Env.NNGraph[CxtRef.GraphId].get<Graph>();
  std::string prompt(reinterpret_cast<char *>(Tensor.Tensor.data()),
                     Tensor.Tensor.size());
  CxtRef.LlamaInputs = llama_tokenize(GraphRef.LlamaContext, prompt, true);
  const uint32_t max_context_size = llama_n_ctx(GraphRef.LlamaContext);
  const uint32_t max_tokens_list_size = max_context_size - 4;
  if (CxtRef.LlamaInputs.size() > max_tokens_list_size) {
    spdlog::error("[WASI-NN]: Error: prompt too long ({} tokens, max %{})",
                  CxtRef.LlamaInputs.size(), max_tokens_list_size);
    return ErrNo::InvalidArgument;
  }
  return ErrNo::Success;
}

Expect<ErrNo> getOutput(WasiNNEnvironment &Env, uint32_t ContextId,
                        __attribute__((unused)) uint32_t Index,
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
    spdlog::error("[WASI-NN] Llama input is not set!");
    return ErrNo::InvalidArgument;
  }

  // Output start from prompt.
  for (auto id : CxtRef.LlamaInputs) {
    CxtRef.LlamaOutputs += llama_token_to_str(GraphRef.LlamaContext, id);
  }

  // Main predict loop.
  // TODO: recompute a compressed context based on previous tokens once the
  // cache is full.
  const int max_context_size = llama_n_ctx(GraphRef.LlamaContext);
  while (llama_get_kv_cache_token_count(GraphRef.LlamaContext) <
         max_context_size) {
    if (llama_eval(GraphRef.LlamaContext, CxtRef.LlamaInputs.data(),
                   int(CxtRef.LlamaInputs.size()),
                   llama_get_kv_cache_token_count(GraphRef.LlamaContext),
                   get_num_physical_cores())) {
      spdlog::error("[WASI-NN] Llama failed to eval.");
      return ErrNo::InvalidArgument;
    }
    CxtRef.LlamaInputs.clear();

    // Select the best prediction.
    llama_token new_token_id = 0;
    auto logits = llama_get_logits(GraphRef.LlamaContext);
    auto n_vocab = llama_n_vocab(GraphRef.LlamaContext);
    std::vector<llama_token_data> candidates;
    candidates.reserve(n_vocab);
    for (llama_token token_id = 0; token_id < n_vocab; token_id++) {
      candidates.emplace_back(
          llama_token_data{token_id, logits[token_id], 0.0f});
    }
    llama_token_data_array candidates_p = {candidates.data(), candidates.size(),
                                           false};
    new_token_id =
        llama_sample_token_greedy(GraphRef.LlamaContext, &candidates_p);

    if (new_token_id == llama_token_eos()) {
      CxtRef.LlamaOutputs += "[end of text]";
      break;
    }

    // Append the new token.
    CxtRef.LlamaOutputs +=
        llama_token_to_str(GraphRef.LlamaContext, new_token_id);

    // Push this new token for next evaluation.
    CxtRef.LlamaInputs.push_back(new_token_id);
  }

  return ErrNo::Success;
}
#else
namespace {
Expect<ErrNo> reportBackendNotSupported() noexcept {
  spdlog::error("[WASI-NN] ggml backend is not built. use "
                "-WASMEDGE_PLUGIN_WASI_NN_BACKEND=\"ggml\" to build it.");
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
} // namespace WasmEdge::Host::WASINN::Ggml
