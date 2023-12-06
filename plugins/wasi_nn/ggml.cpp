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
Expect<ErrNo> parseMetadata(Graph &GraphRef, const std::string &Metadata,
                            bool *IsCxtUpdated = nullptr,
                            bool *IsModelUpdated = nullptr) noexcept {
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
  CxtParams.n_ctx = GraphRef.CtxSize;
  CxtParams.n_batch = GraphRef.BatchSize;

  // The plugin parameters.
  if (Doc.at_key("enable-log").error() == simdjson::SUCCESS) {
    auto Err = Doc["enable-log"].get<bool>().get(GraphRef.EnableLog);
    if (Err) {
      spdlog::error(
          "[WASI-NN] GGML backend: Unable to retrieve the enable-log option."sv);
      return ErrNo::InvalidArgument;
    }
    llama_log_set(nullptr, &GraphRef.EnableLog);
  }
  if (Doc.at_key("stream-stdout").error() == simdjson::SUCCESS) {
    auto Err = Doc["stream-stdout"].get<bool>().get(GraphRef.StreamStdout);
    if (Err) {
      spdlog::error(
          "[WASI-NN] GGML backend: Unable to retrieve the stream-stdout option."sv);
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

  // The model parameters.
  if (Doc.at_key("n-gpu-layers").error() == simdjson::SUCCESS) {
    auto Err = Doc["n-gpu-layers"].get<int64_t>().get(GraphRef.NGPULayers);
    if (Err) {
      spdlog::error(
          "[WASI-NN] GGML backend: Unable to retrieve the n-gpu-layers option."sv);
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
  if (Doc.at_key("repeat-penalty").error() == simdjson::SUCCESS) {
    auto Err = Doc["repeat-penalty"].get<double>().get(GraphRef.RepeatPenalty);
    if (Err) {
      spdlog::error(
          "[WASI-NN] GGML backend: Unable to retrieve the repeat-penalty option."sv);
      return ErrNo::InvalidArgument;
    }
  }

  // Check if the model is updated.
  if (IsModelUpdated && ModelParams.n_gpu_layers != GraphRef.NGPULayers) {
    *IsModelUpdated = true;
  }

  // Check if the context is updated.
  if (IsCxtUpdated && (CxtParams.n_ctx != GraphRef.CtxSize ||
                       CxtParams.n_batch != GraphRef.BatchSize)) {
    *IsCxtUpdated = true;
  }

  return ErrNo::Success;
}

Expect<ErrNo> parseModelConfig(Graph &GraphRef,
                               std::string ModelFilePathWithConfig,
                               std::string &ModelFilePath) noexcept {
  std::vector<std::string> Configs;
  std::string Delimiter = ",";
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
      Configs.emplace_back(Token);
      ModelFilePathWithConfig.erase(0, Pos + Delimiter.length());
    }
    Configs.emplace_back(ModelFilePathWithConfig);
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
      return ErrNo::InvalidArgument;
    } catch (const std::out_of_range &e) {
      spdlog::error(
          "[WASI-NN] GGML backend: parse parameter failed: out_of_range {}"sv,
          e.what());
      return ErrNo::InvalidArgument;
    }
  }

  return ErrNo::Success;
}

} // namespace details

Expect<ErrNo> load(WasiNNEnvironment &Env, Span<const Span<uint8_t>> Builders,
                   [[maybe_unused]] Device Device, uint32_t &GraphId) noexcept {
  // Add a new graph.
  Env.NNGraph.emplace_back(Backend::GGML);
  auto &GraphRef = Env.NNGraph.back().get<Graph>();

  // Initialize the plugin parameters.
  auto ContextDefault = llama_context_default_params();
  GraphRef.EnableLog = false;
  GraphRef.StreamStdout = false;
  GraphRef.ReversePrompt = ""sv;
  GraphRef.NPredict = ContextDefault.n_ctx;
  // Initialize the model parameters.
  GraphRef.NGPULayers = 0;
  // Initialize the context parameters.
  GraphRef.CtxSize = ContextDefault.n_ctx;
  GraphRef.BatchSize = ContextDefault.n_batch;
  // Initialize the sampling parameters.
  llama_sampling_params SamplingDefault;
  GraphRef.Temp = SamplingDefault.temp;
  GraphRef.RepeatPenalty = SamplingDefault.penalty_repeat;

  // If the graph builder length > 1, the data of builder[1] is the metadata.
  if (Builders.size() > 1) {
    std::string Metadata(reinterpret_cast<char *>(Builders[1].data()),
                         Builders[1].size());
    // Ignore context or model updates when initializing the graph.
    auto Res = details::parseMetadata(GraphRef, Metadata);
    if (Res != ErrNo::Success) {
      spdlog::error("[WASI-NN] GGML backend: Failed to parse metadata."sv);
      Env.NNGraph.pop_back();
      return Res;
    }
  }

  // Handle the model path.
  auto Weight = Builders[0];
  std::string BinModel(reinterpret_cast<char *>(Weight.data()), Weight.size());
  std::string ModelFilePath;
  if (BinModel.substr(0, 8) == "preload:") {
    // If BinModel starts with 'preload:', it means that the model name passed
    // in as the --nn-preload parameter may have a config separated by ',' at
    // the end. For example, "preload:./model.bin,n_gpu_layers=99"
    auto Res =
        details::parseModelConfig(GraphRef, BinModel.substr(8), ModelFilePath);
    if (Res != ErrNo::Success) {
      spdlog::error("[WASI-NN] GGML backend: Failed to parse model config."sv);
      Env.NNGraph.pop_back();
      return Res;
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
    const std::string Metadata(reinterpret_cast<char *>(Tensor.Tensor.data()),
                               Tensor.Tensor.size());
    return details::parseMetadata(GraphRef, Metadata, &IsCxtParamsUpdated,
                                  &IsModelParamsUpdated);
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
  if (CxtRef.LlamaContext == nullptr || IsCxtParamsUpdated) {
    llama_context_params ContextParams = llama_context_default_params();
    ContextParams.n_ctx = GraphRef.CtxSize;
    ContextParams.n_batch = GraphRef.BatchSize;
    if (CxtRef.LlamaContext != nullptr) {
      llama_free(CxtRef.LlamaContext);
    }
    CxtRef.LlamaContext =
        llama_new_context_with_model(GraphRef.LlamaModel, ContextParams);
  }

  // Set the input.
  const bool AddBos = llama_should_add_bos_token(GraphRef.LlamaModel);
  std::string Prompt(reinterpret_cast<char *>(Tensor.Tensor.data()),
                     Tensor.Tensor.size());
  CxtRef.LlamaInputs =
      llama_tokenize(CxtRef.LlamaContext, Prompt, AddBos, true);
  const uint32_t MaxContextSize = llama_n_ctx(CxtRef.LlamaContext);
  // Minus 4 for the special tokens.
  const uint32_t MaxTokensListSize = MaxContextSize - 4;
  if (CxtRef.LlamaInputs.size() > MaxTokensListSize) {
    spdlog::error(
        "[WASI-NN] GGML backend: Error: The prompt is too long. Your input has {} tokens. Please reduce it to {} tokens."sv,
        CxtRef.LlamaInputs.size(), MaxTokensListSize);
    return ErrNo::InvalidArgument;
  }
  return ErrNo::Success;
}

Expect<ErrNo> getOutput(WasiNNEnvironment &Env, uint32_t ContextId,
                        uint32_t Index, Span<uint8_t> OutBuffer,
                        uint32_t &BytesWritten) noexcept {
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  // Index 1 is for the metadata of the outputs.
  if (Index == 1) {
    std::string MetadataTemplate =
        R"({"input_tokens": %d, "output_tokens": %d})";
    // The 20 bytes are reserved to accommodate two %d placeholders in the
    // MetadataTemplate. This allows for a decimal integer value up to a
    // 12-digit number of input/output tokens.
    char Buffer[MetadataTemplate.size() + 20];
    snprintf(Buffer, sizeof(Buffer), MetadataTemplate.c_str(),
             CxtRef.LlamaInputs.size(), CxtRef.LlamaOutputTokens.size());
    std::string Metadata(Buffer);
    std::copy_n(Metadata.data(), Metadata.length(), OutBuffer.data());
    BytesWritten = Metadata.length();
    return ErrNo::Success;
  }

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

  if (GraphRef.EnableLog) {
    spdlog::info("[WASI-NN] GGML backend: llama_system_info: {}"sv,
                 llama_print_system_info());
  }

  // Clear the outputs.
  CxtRef.LlamaOutputs.clear();
  CxtRef.LlamaOutputTokens.clear();

  // Main predict loop.
  gpt_params GPTParams;
  GPTParams.sparams.temp = GraphRef.Temp;
  GPTParams.sparams.penalty_repeat = GraphRef.RepeatPenalty;
  struct llama_sampling_context *CtxSampling =
      llama_sampling_init(GPTParams.sparams);
  std::vector<llama_token> Embd;
  int NPast = 0;
  int NConsumed = 0;
  int NRemain = GraphRef.NPredict;
  // Initialize the llama context.
  llama_context_params ContextParams = llama_context_default_params();
  ContextParams.n_ctx = GraphRef.CtxSize;
  ContextParams.n_batch = GraphRef.BatchSize;
  CxtRef.LlamaContext =
      llama_new_context_with_model(GraphRef.LlamaModel, ContextParams);
  int NCtx = llama_n_ctx(CxtRef.LlamaContext);
  // Minus 4 for the special tokens. (Such as <BOS>, <EOS>, ... tokens.)
  const int MaxTokensListSize = NCtx - 4;
  // Use the const sequence id here.
  const int SequenceId = 0;
  while (NRemain >= 0) {
    // Preidct
    if (!Embd.empty()) {
      // Input too long.
      if (static_cast<int>(Embd.size()) > MaxTokensListSize) {
        spdlog::error(
            "[WASI-NN] GGML backend: Error: The prompt is too long. Your input has {} tokens. Please reduce it to {} tokens."sv,
            Embd.size(), MaxTokensListSize);
        return ErrNo::RuntimeError;
      }

      // We do not swap context here. End the inference if the context is full.
      if (NPast + static_cast<int>(Embd.size()) > NCtx) {
        if (GraphRef.EnableLog) {
          spdlog::info(
              "[WASI-NN] GGML backend: the context if full ({} / {} tokens)"sv,
              NPast + static_cast<int>(Embd.size()), NCtx);
        }
        break;
      }

      // Evaluate tokens in batches.
      for (int I = 0; I < static_cast<int>(Embd.size());
           I += GraphRef.BatchSize) {
        int NEval = static_cast<int>(Embd.size()) - I;
        if (NEval > static_cast<int>(GraphRef.BatchSize)) {
          NEval = GraphRef.BatchSize;
        }
        // llama_batch_get_one(*token, n_tokens, position, sequence_id)
        // This will return batch for single sequence of tokens starting at
        // position.
        if (llama_decode(
                CxtRef.LlamaContext,
                llama_batch_get_one(&Embd[I], NEval, NPast, SequenceId))) {
          spdlog::error("[WASI-NN] GGML backend: failed to llama_decode"sv);
          return ErrNo::RuntimeError;
        }

        NPast += NEval;
      }
    }

    Embd.clear();

    if (static_cast<int>(CxtRef.LlamaInputs.size()) <= NConsumed) {
      const llama_token Id =
          llama_sampling_sample(CtxSampling, CxtRef.LlamaContext, nullptr);
      llama_sampling_accept(CtxSampling, CxtRef.LlamaContext, Id, true);
      Embd.emplace_back(Id);
      --NRemain;
      // Save the output token.
      CxtRef.LlamaOutputTokens.emplace_back(Id);
      CxtRef.LlamaOutputs += llama_token_to_piece(CxtRef.LlamaContext, Id);
      // When setting StreamStdout, we print the output to stdout.
      if (GraphRef.StreamStdout) {
        std::cout << llama_token_to_piece(CxtRef.LlamaContext, Id)
                  << std::flush;
      }
      // Break if reverse prompt is found.
      if (!GraphRef.ReversePrompt.empty() &&
          CxtRef.LlamaOutputs.find(GraphRef.ReversePrompt) !=
              std::string::npos) {
        if (GraphRef.EnableLog) {
          spdlog::info("[WASI-NN] GGML backend: reverse prompt found"sv);
        }
        break;
      }
      // Deal with end of text token.
      if (llama_sampling_last(CtxSampling) ==
          llama_token_eos(GraphRef.LlamaModel)) {
        if (GraphRef.EnableLog) {
          spdlog::info("[WASI-NN] GGML backend: EOS token found"sv);
        }
        break;
      }
    } else {
      while (static_cast<int>(CxtRef.LlamaInputs.size()) > NConsumed) {
        Embd.push_back(CxtRef.LlamaInputs[NConsumed]);
        // Push the prompt in the sampling context.
        llama_sampling_accept(CtxSampling, CxtRef.LlamaContext,
                              CxtRef.LlamaInputs[NConsumed], false);
        ++NConsumed;
        if (Embd.size() >= GraphRef.BatchSize) {
          break;
        }
      }
    }
  }

  if (GraphRef.EnableLog) {
    llama_print_timings(CxtRef.LlamaContext);
  }

  // We free the contexts here to keep the ggml plugin stateless.
  // Users could fully control the contexts by themselves via their prompt.
  llama_sampling_free(CtxSampling);
  llama_free(CxtRef.LlamaContext);

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
