// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "ggml.h"
#include "wasinnenv.h"

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_GGML
#include "simdjson.h"
#include <common.h>
#include <cstdlib>
#include <llama.h>
#endif

namespace WasmEdge::Host::WASINN::GGML {
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_GGML

namespace details {
Expect<ErrNo> getMetadata(Context &CxtRef, Graph &GraphRef,
                          const TensorData &Tensor, bool &IsCxtUpdated,
                          bool &IsModelUpdated) noexcept {
  // Decode metadata.
  const std::string Metadata(reinterpret_cast<char *>(Tensor.Tensor.data()),
                             Tensor.Tensor.size());
  simdjson::dom::parser Parser;
  simdjson::dom::element Doc;
  auto ParseError = Parser.parse(Metadata).get(Doc);
  if (ParseError) {
    spdlog::error("[WASI-NN] GGML backend: Parse metadata error"sv);
    return ErrNo::InvalidEncoding;
  }

  // Get metadata from the json.
  // Need to update Model:
  // * n_gpu_layers
  // Need to update Context:
  // * ctx-size
  // * batch-size

  // Get the current llama parameters.
  llama_model_params ModelParams = llama_model_default_params();
  ModelParams.n_gpu_layers = GraphRef.NGPULayers;
  llama_context_params CxtParams = llama_context_default_params();
  CxtParams.n_ctx = CxtRef.CtxSize;
  CxtParams.n_batch = CxtRef.BatchSize;

  if (Doc.at_key("enable-log").error() == simdjson::SUCCESS) {
    auto Err = Doc["enable-log"].get<bool>().get(CxtRef.EnableLog);
    if (Err) {
      spdlog::error(
          "[WASI-NN] GGML backend: Unable to retrieve the enable-log option."sv);
      return ErrNo::InvalidArgument;
    }
    llama_log_set(nullptr, &CxtRef.EnableLog);
  }
  if (Doc.at_key("stream-stdout").error() == simdjson::SUCCESS) {
    auto Err = Doc["stream-stdout"].get<bool>().get(CxtRef.StreamStdout);
    if (Err) {
      spdlog::error(
          "[WASI-NN] GGML backend: Unable to retrieve the stream-stdout option."sv);
      return ErrNo::InvalidArgument;
    }
  }
  if (Doc.at_key("ctx-size").error() == simdjson::SUCCESS) {
    auto Err = Doc["ctx-size"].get<uint64_t>().get(CxtRef.CtxSize);
    if (Err) {
      spdlog::error(
          "[WASI-NN] GGML backend: Unable to retrieve the ctx-size option."sv);
      return ErrNo::InvalidArgument;
    }
  }
  if (Doc.at_key("n-predict").error() == simdjson::SUCCESS) {
    auto Err = Doc["n-predict"].get<uint64_t>().get(CxtRef.NPredict);
    if (Err) {
      spdlog::error(
          "[WASI-NN] GGML backend: Unable to retrieve the n-predict option."sv);
      return ErrNo::InvalidArgument;
    }
  }
  if (Doc.at_key("n-gpu-layers").error() == simdjson::SUCCESS) {
    auto Err = Doc["n-gpu-layers"].get<int64_t>().get(GraphRef.NGPULayers);
    if (Err) {
      spdlog::error(
          "[WASI-NN] GGML backend: Unable to retrieve the n-gpu-layers option."sv);
      return ErrNo::InvalidArgument;
    }
  }
  if (Doc.at_key("batch-size").error() == simdjson::SUCCESS) {
    auto Err = Doc["batch-size"].get<uint64_t>().get(CxtRef.BatchSize);
    if (Err) {
      spdlog::error(
          "[WASI-NN] GGML backend: Unable to retrieve the batch-size option."sv);
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
    CxtRef.ReversePrompt = ReversePrompt;
  }

  // Check if the model is updated.
  if (ModelParams.n_gpu_layers != GraphRef.NGPULayers) {
    IsModelUpdated = true;
  }

  // Check if the context is updated.
  if (CxtParams.n_ctx != CxtRef.CtxSize ||
      CxtParams.n_batch != CxtRef.BatchSize) {
    IsCxtUpdated = true;
  }

  return ErrNo::Success;
}
} // namespace details

Expect<ErrNo> load(WasiNNEnvironment &Env, Span<const Span<uint8_t>> Builders,
                   [[maybe_unused]] Device Device, uint32_t &GraphId) noexcept {
  // The graph builder length must be 1.
  if (Builders.size() != 1) {
    spdlog::error(
        "[WASI-NN] GGML backend: Wrong GraphBuilder Length {:d}, expect 1"sv,
        Builders.size());
    return ErrNo::InvalidArgument;
  }

  // Add a new graph.
  Env.NNGraph.emplace_back(Backend::GGML);
  auto &GraphRef = Env.NNGraph.back().get<Graph>();

  // Initialize the model parameters.
  GraphRef.NGPULayers = 0;

  // Handle the model path.
  auto Weight = Builders[0];
  std::string BinModel(reinterpret_cast<char *>(Weight.data()), Weight.size());
  std::string ModelFilePath;
  if (BinModel.substr(0, 8) == "preload:") {
    // If BinModel starts with 'preload:', it means that the model name passed
    // in as the --nn-preload parameter may have a config separated by ',' at
    // the end. For example, "preload:./model.bin,n_gpu_layers=99"
    std::vector<std::string> Configs;
    std::string Delimiter = ",";
    std::string ModelFilePathWithConfig = BinModel.substr(8);
    if (ModelFilePathWithConfig.find(Delimiter) == std::string::npos) {
      ModelFilePath = ModelFilePathWithConfig;
    } else {
      // Handle model path with config.
      size_t Pos = 0;
      std::string Token;
      Pos = ModelFilePathWithConfig.find(Delimiter);
      ModelFilePath = ModelFilePathWithConfig.substr(0, Pos);
      ModelFilePathWithConfig.erase(0, Pos + Delimiter.length());
      while ((Pos = ModelFilePathWithConfig.find(Delimiter)) !=
             std::string::npos) {
        Token = ModelFilePathWithConfig.substr(0, Pos);
        Configs.push_back(Token);
        ModelFilePathWithConfig.erase(0, Pos + Delimiter.length());
      }
      Configs.push_back(ModelFilePathWithConfig);
    }
    // Parse the configs.
    for (const auto &Config : Configs) {
      std::string Delimiter = "=";
      size_t Pos = 0;
      std::string Token;
      Pos = Config.find(Delimiter);
      Token = Config.substr(0, Pos);
      try {
        if (Token == "n_gpu_layers" || Token == "ngl") {
          GraphRef.NGPULayers =
              std::stoi(Config.substr(Pos + Delimiter.length()));
        }
      } catch (const std::invalid_argument &e) {
        spdlog::error(
            "[WASI-NN] GGML backend: parse model parameter failed: invalid_argument {}"sv,
            e.what());
        Env.NNGraph.pop_back();
        return ErrNo::InvalidArgument;
      } catch (const std::out_of_range &e) {
        spdlog::error(
            "[WASI-NN] GGML backend: parse parameter failed: out_of_range {}"sv,
            e.what());
        Env.NNGraph.pop_back();
        return ErrNo::InvalidArgument;
      }
    }
  } else {
    // TODO: pass the model directly to ggml
    // Write ggml model to file.
    std::istringstream BinRead(BinModel);
    ModelFilePath = "ggml-model.bin"sv;
    std::ofstream TempFile(ModelFilePath);
    if (!TempFile) {
      spdlog::error(
          "[WASI-NN] GGML backend: Failed to create the temporary file. "
          "Currently, our workaround involves creating a temporary model "
          "file named \"ggml-model.bin\" and passing this filename as a "
          "parameter to the ggml llama library."sv);
      Env.NNGraph.pop_back();
      return ErrNo::InvalidArgument;
    }
    TempFile << BinModel;
    TempFile.close();
  }

  // Initialize ggml model with model parameters.
  gpt_params Params;
  llama_backend_init(Params.numa);
  GraphRef.ModelFilePath = ModelFilePath;
  llama_model_params ModelParams = llama_model_default_params();
  ModelParams.n_gpu_layers = GraphRef.NGPULayers;
  GraphRef.LlamaModel =
      llama_load_model_from_file(GraphRef.ModelFilePath.c_str(), ModelParams);
  if (GraphRef.LlamaModel == nullptr) {
    spdlog::error("[WASI-NN] GGML backend: Error: unable to init model."sv);
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

  // Set the default context options.
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  auto ContextDefault = llama_context_default_params();
  CxtRef.EnableLog = false;
  CxtRef.StreamStdout = false;
  CxtRef.CtxSize = ContextDefault.n_ctx;
  CxtRef.NPredict = ContextDefault.n_ctx;
  CxtRef.BatchSize = ContextDefault.n_batch;
  CxtRef.ReversePrompt = ""sv;

  return ErrNo::Success;
}

Expect<ErrNo> setInput(WasiNNEnvironment &Env, uint32_t ContextId,
                       uint32_t Index, const TensorData &Tensor) noexcept {
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  auto &GraphRef = Env.NNGraph[CxtRef.GraphId].get<Graph>();

  bool IsCxtParamsUpdated = false;
  bool IsModelParamsUpdated = false;
  // Use index 1 for metadata.
  if (Index == 1) {
    return details::getMetadata(CxtRef, GraphRef, Tensor, IsCxtParamsUpdated,
                                IsModelParamsUpdated);
  }

  // XXX: Due to the limitation of WASI-NN proposal,
  // we have no way to pass the metadata before the setInput phase
  // when we want to do some configurations in the load phase.
  // That's why we have this hack.
#ifndef __APPLE__
  {
    if (IsModelParamsUpdated) {
      llama_model_params ModelParams = llama_model_default_params();
      ModelParams.n_gpu_layers = GraphRef.NGPULayers;
      llama_free_model(GraphRef.LlamaModel);
      GraphRef.LlamaModel = llama_load_model_from_file(
          GraphRef.ModelFilePath.c_str(), ModelParams);
      if (GraphRef.LlamaModel == nullptr) {
        spdlog::error("[WASI-NN] GGML backend: Error: unable to init model."sv);
        Env.NNGraph.pop_back();
        return ErrNo::InvalidArgument;
      }
    }
  }
#endif

  // Initialize the llama context.
  if (GraphRef.LlamaContext == nullptr || IsCxtParamsUpdated) {
    llama_context_params ContextParams = llama_context_default_params();
    ContextParams.n_ctx = CxtRef.CtxSize;
    ContextParams.n_batch = CxtRef.BatchSize;
    if (GraphRef.LlamaContext != nullptr) {
      llama_free(GraphRef.LlamaContext);
    }
    GraphRef.LlamaContext =
        llama_new_context_with_model(GraphRef.LlamaModel, ContextParams);
  }

  // Set the input.
  std::string Prompt(reinterpret_cast<char *>(Tensor.Tensor.data()),
                     Tensor.Tensor.size());
  CxtRef.LlamaInputs = llama_tokenize(GraphRef.LlamaContext, Prompt, true);
  const uint32_t MaxContextSize = llama_n_ctx(GraphRef.LlamaContext);
  // Minus 4 for the special tokens.
  const uint32_t MaxTokensListSize = MaxContextSize - 4;
  if (CxtRef.LlamaInputs.size() > MaxTokensListSize) {
    spdlog::error(
        "[WASI-NN] GGML backend: Error: prompt too long ({} tokens, max {})"sv,
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
    spdlog::error("[WASI-NN] GGML backend: Llama input is not set!"sv);
    return ErrNo::InvalidArgument;
  }

  if (CxtRef.EnableLog) {
    spdlog::info("[WASI-NN] GGML backend: llama_system_info: {}"sv,
                 llama_print_system_info());
  }

  // Clear the outputs.
  CxtRef.LlamaOutputs = ""sv;

  // Main predict loop.
  // TODO: recompute a compressed context based on previous tokens once the
  // cache is full.
  const int MaxContextSize = llama_n_ctx(GraphRef.LlamaContext);
  // NPredict is the number of tokens to predict. Same as -n, --n-predict in
  // llama.cpp.
  int NPredict = CxtRef.NPredict;

  // Evaluate the initial prompt.
  llama_batch LlamaBatch = llama_batch_init(CxtRef.BatchSize, 0);
  LlamaBatch.n_tokens = CxtRef.LlamaInputs.size();
  for (int32_t I = 0; I < LlamaBatch.n_tokens; I++) {
    LlamaBatch.token[I] = CxtRef.LlamaInputs[I];
    LlamaBatch.pos[I] = I;
    LlamaBatch.seq_id[I] = 0;
    LlamaBatch.logits[I] = false;
  }

  // llama_decode will output logits only for the last token of the prompt
  LlamaBatch.logits[LlamaBatch.n_tokens - 1] = true;
  if (llama_decode(GraphRef.LlamaContext, LlamaBatch) != 0) {
    spdlog::info("[WASI-NN] GGML backend: llama_decode() failed"sv);
    return ErrNo::RuntimeError;
  }

  int NCur = LlamaBatch.n_tokens;
  while (NCur < MaxContextSize && NCur < NPredict) {
    // Sample the next token
    auto NVocab = llama_n_vocab(GraphRef.LlamaModel);
    auto *Logits =
        llama_get_logits_ith(GraphRef.LlamaContext, LlamaBatch.n_tokens - 1);

    std::vector<llama_token_data> Candidates;
    Candidates.reserve(NVocab);
    for (llama_token TokenId = 0; TokenId < NVocab; TokenId++) {
      Candidates.emplace_back(llama_token_data{TokenId, Logits[TokenId], 0.0f});
    }
    llama_token_data_array CandidatesP = {Candidates.data(), Candidates.size(),
                                          false};

    // Sample the most likely token
    const llama_token NewTokenId =
        llama_sample_token_greedy(GraphRef.LlamaContext, &CandidatesP);

    // Is it an end of stream?
    if (NewTokenId == llama_token_eos(GraphRef.LlamaContext) ||
        NCur == MaxContextSize || NCur == NPredict) {
      break;
    }

    std::string NextToken =
        llama_token_to_piece(GraphRef.LlamaContext, NewTokenId);

    // When setting StreamStdout, we print the output to stdout.
    if (CxtRef.StreamStdout) {
      std::cout << NextToken << std::flush;
    }

    // Append the new token.
    CxtRef.LlamaOutputs += NextToken;

    // Prepare the next batch
    LlamaBatch.n_tokens = 0;

    // Push this new token for next evaluation
    LlamaBatch.token[LlamaBatch.n_tokens] = NewTokenId;
    LlamaBatch.pos[LlamaBatch.n_tokens] = NCur;
    LlamaBatch.seq_id[LlamaBatch.n_tokens] = 0;
    LlamaBatch.logits[LlamaBatch.n_tokens] = true;
    LlamaBatch.n_tokens += 1;
    NCur += 1;

    // Evaluate the current batch with the transformer model
    if (llama_decode(GraphRef.LlamaContext, LlamaBatch)) {
      spdlog::error("[WASI-NN] GGML backend: failed to llama_decode"sv);
      return ErrNo::RuntimeError;
    }

    // Break if reverse prompt is found.
    if (!CxtRef.ReversePrompt.empty() &&
        CxtRef.LlamaOutputs.find(CxtRef.ReversePrompt) != std::string::npos) {
      if (CxtRef.EnableLog) {
        spdlog::info("[WASI-NN] GGML backend: reverse prompt found"sv);
      }
      break;
    }
  }

  if (CxtRef.EnableLog) {
    llama_log_set(nullptr, &CxtRef.EnableLog);
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
