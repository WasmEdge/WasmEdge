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

  // Get the current llama parameters.
  llama_model_params ModelParams = llama_model_default_params();
  ModelParams.n_gpu_layers = GraphRef.NGPULayers;

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
#ifdef __APPLE__
  // Whatever the `n-gpu-layers` is given, we will always set the ngl to 1 on
  // macOS to forcely enabled Metal.
  GraphRef.NGPULayers = 1; // Force enabled Metal on macOS
#endif

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

  return ErrNo::Success;
}

Expect<ErrNo> buildOutputMetadata(Context &CxtRef,
                                  std::string &Metadata) noexcept {
  std::string MetadataTemplate = R"({"input_tokens": %d, "output_tokens": %d})";

  // The 20 bytes are reserved to accommodate two %d placeholders in the
  // MetadataTemplate. This allows for a decimal integer value up to a
  // 12-digit number of input/output tokens.
  char Buffer[MetadataTemplate.size() + 20];
  snprintf(Buffer, sizeof(Buffer), MetadataTemplate.c_str(),
           CxtRef.LlamaInputs.size(), CxtRef.LlamaOutputTokens.size());
  Metadata = std::string(Buffer);

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
#ifdef __APPLE__
  // We will always set the ngl to 1 on macOS to enable Metal.
  GraphRef.NGPULayers = 1;
#endif
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
  std::string BinModel(reinterpret_cast<char *>(Weight.data()), Weight.size());
  std::string ModelFilePath;
  if (BinModel.substr(0, 8) == "preload:") {
    ModelFilePath = BinModel.substr(8);
  } else {
    if (GraphRef.EnableDebugLog) {
      spdlog::info(
          "[WASI-NN][Debug] GGML backend: Model path not found in nn-preload, write model into a tmpfile."sv);
    }
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
    if (GraphRef.EnableDebugLog) {
      spdlog::info(
          "[WASI-NN][Debug] GGML backend: Write model into a tmpfile...Done"sv);
    }
  }
  if (GraphRef.EnableDebugLog) {
    spdlog::info(
        "[WASI-NN][Debug] GGML backend: Finished handling model path."sv);
  }

  if (GraphRef.EnableDebugLog) {
    spdlog::info(
        "[WASI-NN][Debug] GGML backend: Initialize ggml model with given parameters"sv);
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
  if (GraphRef.EnableDebugLog) {
    spdlog::info(
        "[WASI-NN][Debug] GGML backend: Initialize ggml model with given parameters...Done"sv);
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
  auto &GraphRef = Env.NNGraph[GraphId].get<Graph>();
  if (GraphRef.EnableLog) {
    spdlog::info("[WASI-NN] GGML backend: llama_system_info: {}"sv,
                 llama_print_system_info());
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
    auto Res =
        details::parseMetadata(GraphRef, Metadata, &IsModelParamsUpdated);

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
        ModelParams.n_gpu_layers = GraphRef.NGPULayers;
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
  ContextParams.n_ctx = GraphRef.CtxSize;
  ContextParams.n_batch = GraphRef.BatchSize;
  auto LlamaContext =
      llama_new_context_with_model(GraphRef.LlamaModel, ContextParams);
  if (GraphRef.EnableDebugLog) {
    spdlog::info("[WASI-NN][Debug] GGML backend: init llama context...Done"sv);
  }

  // Set the input.
  if (GraphRef.EnableDebugLog) {
    spdlog::info("[WASI-NN][Debug] GGML backend: set the input"sv);
  }
  const bool AddBos = llama_should_add_bos_token(GraphRef.LlamaModel);
  std::string Prompt(reinterpret_cast<char *>(Tensor.Tensor.data()),
                     Tensor.Tensor.size());
  CxtRef.LlamaInputs = llama_tokenize(LlamaContext, Prompt, AddBos, true);
  const uint32_t MaxContextSize = llama_n_ctx(LlamaContext);
  // Minus 4 for the special tokens.
  const uint32_t MaxTokensListSize = MaxContextSize - 4;
  if (CxtRef.LlamaInputs.size() > MaxTokensListSize) {
    spdlog::error(
        "[WASI-NN] GGML backend: Error: The prompt is too long. Your input has {} tokens. Please reduce it to {} tokens."sv,
        CxtRef.LlamaInputs.size(), MaxTokensListSize);
    return ErrNo::InvalidArgument;
  }
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
  // Index 1 is for the metadata of the outputs.
  if (Index == 1) {
    std::string Metadata;
    auto Res = details::buildOutputMetadata(CxtRef, Metadata);
    if (Res != ErrNo::Success) {
      spdlog::error(
          "[WASI-NN] GGML backend: Failed to build output metadata."sv);
      return Res;
    }
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
  if (GraphRef.EnableDebugLog) {
    spdlog::info("[WASI-NN][Debug] GGML backend: compute"sv);
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
    spdlog::info("[WASI-NN][Debug] GGML backend: enter main predict loop"sv);
  }
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
  auto LlamaContext =
      llama_new_context_with_model(GraphRef.LlamaModel, ContextParams);
  int NCtx = llama_n_ctx(LlamaContext);
  // Minus 4 for the special tokens. (Such as <BOS>, <EOS>, ... tokens.)
  const int MaxTokensListSize = NCtx - 4;
  // Use the const sequence id here.
  const int SequenceId = 0;
  // Return value.
  auto ReturnCode = ErrNo::Success;
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
        ReturnCode = ErrNo::ContextFull;
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
        auto Status =
            llama_decode(LlamaContext, llama_batch_get_one(&Embd[I], NEval,
                                                           NPast, SequenceId));
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
    }

    Embd.clear();

    if (static_cast<int>(CxtRef.LlamaInputs.size()) <= NConsumed) {
      const llama_token Id =
          llama_sampling_sample(CtxSampling, LlamaContext, nullptr);
      llama_sampling_accept(CtxSampling, LlamaContext, Id, true);
      Embd.emplace_back(Id);
      --NRemain;
      // Save the output token.
      CxtRef.LlamaOutputTokens.emplace_back(Id);
      CxtRef.LlamaOutputs += llama_token_to_piece(LlamaContext, Id);
      // When setting StreamStdout, we print the output to stdout.
      if (GraphRef.StreamStdout) {
        std::cout << llama_token_to_piece(LlamaContext, Id) << std::flush;
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
        llama_sampling_accept(CtxSampling, LlamaContext,
                              CxtRef.LlamaInputs[NConsumed], false);
        ++NConsumed;
        if (Embd.size() >= GraphRef.BatchSize) {
          break;
        }
      }
    }
  }
  if (GraphRef.EnableDebugLog) {
    spdlog::info(
        "[WASI-NN][Debug] GGML backend: enter main predict loop...Done"sv);
  }

  if (GraphRef.EnableLog) {
    llama_print_timings(LlamaContext);
  }

  // We free the contexts here to keep the ggml plugin stateless.
  // Users could fully control the contexts by themselves via their prompt.
  llama_sampling_free(CtxSampling);
  llama_free(LlamaContext);

  if (GraphRef.EnableDebugLog) {
    spdlog::info("[WASI-NN][Debug] GGML backend: compute...Done"sv);
  }

  return ReturnCode;
}

Expect<ErrNo> getOutputSingle(WasiNNEnvironment &Env, uint32_t ContextId,
                              uint32_t Index, Span<uint8_t> OutBuffer,
                              uint32_t &BytesWritten) noexcept {
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  // Index 1 is for the metadata of the outputs.
  if (Index == 1) {
    std::string Metadata;
    auto Res = details::buildOutputMetadata(CxtRef, Metadata);
    if (Res != ErrNo::Success) {
      spdlog::error(
          "[WASI-NN] GGML backend: Failed to build output metadata."sv);
      return Res;
    }
    std::copy_n(Metadata.data(), Metadata.length(), OutBuffer.data());
    BytesWritten = Metadata.length();
    return ErrNo::Success;
  }
  std::string LastToken = llama_token_to_piece(CxtRef.LlamaContext,
                                               CxtRef.LlamaOutputTokens.back());
  std::copy_n(LastToken.data(), LastToken.length(), OutBuffer.data());
  BytesWritten = LastToken.length();
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
  if (CxtRef.LlamaInputs.size() == 0) {
    spdlog::error("[WASI-NN] GGML backend: Llama input is not set!"sv);
    return ErrNo::InvalidArgument;
  }

  // New compute single token context.
  if (CxtRef.LlamaContext == nullptr) {
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
    GPTParams.sparams.temp = GraphRef.Temp;
    GPTParams.sparams.penalty_repeat = GraphRef.RepeatPenalty;
    CxtRef.LlamaSampling = llama_sampling_init(GPTParams.sparams);
    llama_context_params ContextParams = llama_context_default_params();
    ContextParams.n_ctx = GraphRef.CtxSize;
    ContextParams.n_batch = GraphRef.BatchSize;
    CxtRef.LlamaContext =
        llama_new_context_with_model(GraphRef.LlamaModel, ContextParams);
    CxtRef.LlamaEmbd.clear();
    CxtRef.LlamaNPast = 0;
    CxtRef.LlamaNConsumed = 0;
  }

  const int NCtx = llama_n_ctx(CxtRef.LlamaContext);
  // Minus 4 for the special tokens. (Such as <BOS>, <EOS>, ... tokens.)
  const int MaxTokensListSize = NCtx - 4;
  // Use the const sequence id here.
  const int SequenceId = 0;

  while (true) {
    if (!CxtRef.LlamaEmbd.empty()) {
      // Input too long.
      if (static_cast<int>(CxtRef.LlamaEmbd.size()) > MaxTokensListSize) {
        spdlog::error(
            "[WASI-NN] GGML backend: Error: The prompt is too long. Your input has {} tokens. Please reduce it to {} tokens."sv,
            CxtRef.LlamaEmbd.size(), MaxTokensListSize);
        return ErrNo::RuntimeError;
      }

      // We do not swap context here. End the inference if the context is full.
      if (CxtRef.LlamaNPast + static_cast<int>(CxtRef.LlamaEmbd.size()) >
          NCtx) {
        if (GraphRef.EnableLog) {
          spdlog::info(
              "[WASI-NN] GGML backend: the context if full ({} / {} tokens)"sv,
              CxtRef.LlamaNPast + static_cast<int>(CxtRef.LlamaEmbd.size()),
              NCtx);
        }
        return ErrNo::ContextFull;
      }

      // Evaluate tokens in batches.
      for (int I = 0; I < static_cast<int>(CxtRef.LlamaEmbd.size());
           I += GraphRef.BatchSize) {
        int NEval = static_cast<int>(CxtRef.LlamaEmbd.size()) - I;
        if (NEval > static_cast<int>(GraphRef.BatchSize)) {
          NEval = GraphRef.BatchSize;
        }
        // llama_batch_get_one(*token, n_tokens, position, sequence_id)
        // This will return batch for single sequence of tokens starting at
        // position.
        auto Status =
            llama_decode(CxtRef.LlamaContext,
                         llama_batch_get_one(&CxtRef.LlamaEmbd[I], NEval,
                                             CxtRef.LlamaNPast, SequenceId));
        if (Status == 1) {
          spdlog::error(
              "[WASI-NN] GGML backend: failed to llama_decode: try reducing the size of the batch or increasing the size of context"sv);
          return ErrNo::RuntimeError;
        } else if (Status < 0) {
          spdlog::error(
              "[WASI-NN] GGML backend: failed to llama_decode: internal fatal error. Please open an issue on GitHub"sv);
          return ErrNo::RuntimeError;
        }

        CxtRef.LlamaNPast += NEval;
      }
    }

    CxtRef.LlamaEmbd.clear();

    if (static_cast<int>(CxtRef.LlamaInputs.size()) <= CxtRef.LlamaNConsumed) {
      const llama_token Id = llama_sampling_sample(
          CxtRef.LlamaSampling, CxtRef.LlamaContext, nullptr);
      llama_sampling_accept(CxtRef.LlamaSampling, CxtRef.LlamaContext, Id,
                            true);
      CxtRef.LlamaEmbd.emplace_back(Id);
      // Save the output token.
      CxtRef.LlamaOutputTokens.emplace_back(Id);
      CxtRef.LlamaOutputs += llama_token_to_piece(CxtRef.LlamaContext, Id);
      // Deal with end of text token.
      if (llama_sampling_last(CxtRef.LlamaSampling) ==
          llama_token_eos(GraphRef.LlamaModel)) {
        if (GraphRef.EnableLog) {
          spdlog::info("[WASI-NN] GGML backend: EOS token found"sv);
        }
        return ErrNo::EndOfSequence;
      }
      return ErrNo::Success;
    } else {
      while (static_cast<int>(CxtRef.LlamaInputs.size()) >
             CxtRef.LlamaNConsumed) {
        CxtRef.LlamaEmbd.push_back(CxtRef.LlamaInputs[CxtRef.LlamaNConsumed]);
        // Push the prompt in the sampling context.
        llama_sampling_accept(CxtRef.LlamaSampling, CxtRef.LlamaContext,
                              CxtRef.LlamaInputs[CxtRef.LlamaNConsumed], false);
        ++CxtRef.LlamaNConsumed;
        if (CxtRef.LlamaEmbd.size() >= GraphRef.BatchSize) {
          break;
        }
      }
    }
  }

  return ErrNo::Success;
}

Expect<ErrNo> finiSingle(WasiNNEnvironment &Env, uint32_t ContextId) noexcept {
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  auto &GraphRef = Env.NNGraph[CxtRef.GraphId].get<Graph>();

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
  if (GraphRef.EnableDebugLog) {
    spdlog::info(
        "[WASI-NN][Debug] GGML backend: finiSingle: free the llama context...Done"sv);
  }

  // Reset the context variables.
  CxtRef.LlamaEmbd.clear();
  CxtRef.LlamaNPast = 0;
  CxtRef.LlamaNConsumed = 0;

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
