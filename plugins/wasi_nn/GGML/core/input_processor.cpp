// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "GGML/metadata/metadata_parser.h"
#include "GGML/tts/tts_core.h"
#include "GGML/utils.h"
#include "ggml_core.h"

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_GGML
#include <mtmd-helper.h>
#include <mtmd.h>
#endif

namespace WasmEdge::Host::WASINN::GGML {
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_GGML

Expect<ErrNo> setInput(WasiNNEnvironment &Env, uint32_t ContextId,
                       uint32_t Index, const TensorData &Tensor) noexcept {
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  auto &GraphRef = Env.NNGraph[CxtRef.GraphId].get<Graph>();
  LOG_DEBUG(GraphRef.EnableDebugLog, "setInput"sv)

  // Use index 1 for metadata.
  if (Index == 1) {
    LOG_DEBUG(GraphRef.EnableDebugLog, "setInput: found Metadata, processing"sv)
    bool IsModelParamsUpdated = false;
    bool IsContextParamsUpdated = false;
    bool IsSamplerParamsUpdated = false;
    const std::string Metadata(reinterpret_cast<char *>(Tensor.Tensor.data()),
                               Tensor.Tensor.size());
    auto Res =
        parseMetadata(GraphRef, CxtRef.Conf, Metadata, &IsModelParamsUpdated,
                      &IsContextParamsUpdated, &IsSamplerParamsUpdated);
    if (Res != ErrNo::Success) {
      RET_ERROR(Res, "setInput: failed to parse metadata."sv)
    }

#ifndef __APPLE__
    // XXX: Due to the limitation of WASI-NN proposal, this is a workaround
    // for non-macOS devices. However, if the model params is updated in
    // Config stage, then, we don't encourage to use this to avoid the model
    // reloading.
    {
      if (IsModelParamsUpdated || GraphRef.LlamaModel == nullptr) {
        // The llama model may be nullptr if set_input with updated model params
        // last time. Therefore besides the model params updated, we should
        // reload the llama model if the model is nullptr.
        LOG_INFO(GraphRef.EnableLog,
                 "setInput: Reload model due to parameters change."sv)
        llama_model_params ModelParams = llama_model_default_params();
        ModelParams.n_gpu_layers =
            static_cast<int32_t>(GraphRef.Params.n_gpu_layers);
        GraphRef.LlamaModel.reset();
        // Due to the model change, the context and sampler should also be
        // reloaded. The new context and sampler will be created in the next
        // block.
        GraphRef.LlamaContext.reset();
        if (CxtRef.LlamaSampler) {
          // TODO: Trigger the sampler in other contexts to reallocate.
          common_sampler_free(CxtRef.LlamaSampler);
          CxtRef.LlamaSampler = nullptr;
        }
        GraphRef.LlamaModel = llama_model_ptr(llama_model_load_from_file(
            GraphRef.Params.model.path.c_str(), ModelParams));
        if (GraphRef.LlamaModel == nullptr) {
          Env.NNGraph[CxtRef.GraphId].setInvalid();
          RET_ERROR(ErrNo::InvalidArgument, "setInput: unable to init model."sv)
        }
      }
    }
#endif

    // Some changes of context parameters will require the context to be
    // reloaded.
    if (IsContextParamsUpdated || GraphRef.LlamaContext == nullptr) {
      LOG_INFO(GraphRef.EnableLog,
               "setInput: Reload llama context due to parameters change."sv)
      GraphRef.LlamaContext.reset();
      GraphRef.LlamaContext = llama_context_ptr(llama_init_from_model(
          GraphRef.LlamaModel.get(),
          common_context_params_to_llama(GraphRef.Params)));
      if (GraphRef.LlamaContext == nullptr) {
        Env.NNGraph[CxtRef.GraphId].setInvalid();
        RET_ERROR(ErrNo::InvalidArgument, "setInput: unable to init context."sv)
      }
    }

    // Some changes of sampling parameters will require the sampler to be
    // reallocated.
    if (IsSamplerParamsUpdated || CxtRef.LlamaSampler == nullptr) {
      LOG_INFO(GraphRef.EnableLog,
               "setInput: Reallocate llama sampler due to parameters change."sv)
      if (CxtRef.LlamaSampler) {
        common_sampler_free(CxtRef.LlamaSampler);
      }
      CxtRef.LlamaSampler = common_sampler_init(GraphRef.LlamaModel.get(),
                                                GraphRef.Params.sampling);
      if (GraphRef.LlamaContext == nullptr) {
        Env.NNGraph[CxtRef.GraphId].setInvalid();
        RET_ERROR(ErrNo::InvalidArgument, "setInput: unable to init sampler."sv)
      }
    }

    // Check that is batch size changed.
    if (CxtRef.CurrentBatchSize != GraphRef.Params.n_batch) {
      llama_batch_free(CxtRef.LlamaBatch);
      CxtRef.LlamaBatch = allocBatch(GraphRef.Params.n_batch);
      CxtRef.CurrentBatchSize = GraphRef.Params.n_batch;
    }

    Env.NNGraph[CxtRef.GraphId].setReady();
    LOG_DEBUG(GraphRef.EnableDebugLog,
              "setInput: found Metadata, processing...Done"sv)
    return ErrNo::Success;
  }

  // Check the graph is valid after reloading during previous set_input.
  if (!Env.NNGraph[CxtRef.GraphId].isReady()) {
    RET_ERROR(
        ErrNo::InvalidArgument,
        "setInput: Graph is invalid. Please reload again by passing metadata "sv
        "in set_input or unload graph."sv)
  }

  // Clear the llama context.
  LOG_DEBUG(GraphRef.EnableDebugLog, "setInput: clear llama context"sv)
  llama_memory_clear(llama_get_memory(GraphRef.LlamaContext.get()), true);
  LOG_DEBUG(GraphRef.EnableDebugLog, "setInput: clear llama context...Done"sv)

  // Set the input.
  const bool AddSpecial = true;
  const bool ParseSpecial = true;
  std::string Prompt(reinterpret_cast<char *>(Tensor.Tensor.data()),
                     Tensor.Tensor.size());
  CxtRef.LlamaInputs.clear();

  auto Base64ImagePos = findBase64ImagePayload(Prompt);

  if (Base64ImagePos.has_value() || CxtRef.Conf.ImagePath != ""sv) {
    // First check the projection model is given.
    if (GraphRef.Params.mmproj.path == ""sv) {
      RET_ERROR(
          ErrNo::InvalidArgument,
          "setInput: the given model does not support image input, so a projection model is required."sv)
    }

    // Make sure the projection model is loaded.
    if (GraphRef.VisionContext == nullptr) {
      LOG_DEBUG(GraphRef.EnableDebugLog, "setInput: initialize mtmd context."sv)
      // Initialize the mtmd context.
      mtmd_context_params VisionContextParams = mtmd_context_params_default();
      std::string VisionPromptImagePlaceholderStr(VisionPromptImagePlaceholder);
      VisionContextParams.media_marker =
          VisionPromptImagePlaceholderStr.c_str();
      VisionContextParams.use_gpu = GraphRef.Params.mmproj_use_gpu;
      VisionContextParams.n_threads = GraphRef.Params.cpuparams.n_threads;
      VisionContextParams.print_timings =
          GraphRef.EnableLog || GraphRef.EnableDebugLog;
      if (GraphRef.EnableDebugLog) {
        VisionContextParams.verbosity = GGML_LOG_LEVEL_DEBUG;
      } else if (GraphRef.EnableLog) {
        VisionContextParams.verbosity = GGML_LOG_LEVEL_INFO;
      } else {
        VisionContextParams.verbosity = GGML_LOG_LEVEL_NONE;
      }
      GraphRef.VisionContext.reset(
          mtmd_init_from_file(GraphRef.Params.mmproj.path.c_str(),
                              GraphRef.LlamaModel.get(), VisionContextParams));
      if (GraphRef.VisionContext == nullptr) {
        RET_ERROR(ErrNo::InvalidArgument,
                  "setInput: unable to load the mmproj model {}."sv,
                  GraphRef.Params.mmproj.path)
      }
      LOG_DEBUG(GraphRef.EnableDebugLog,
                "setInput: initialize mtmd context...Done"sv)
    }

    // Show some warnings for context size.
    if (GraphRef.Params.n_ctx < 4096) {
      LOG_INFO(
          GraphRef.EnableLog,
          "setInput: Context size is {}, we recommend context size >= 4096 when using multimodal models for better results"sv,
          GraphRef.Params.n_ctx)
    }

    // Get the image bitmaps.
    // Follow this link for the supported image formats:
    // https://github.com/ggml-org/llama.cpp/blob/master/common/stb_image.h
    mtmd::bitmaps Bitmaps;
    if (GraphRef.VisionContext != nullptr) {
      if (Base64ImagePos.has_value()) {
        // Load the image bitmap from the base64 image.
        LOG_DEBUG(GraphRef.EnableDebugLog,
                  "setInput: load the image bitmap from the base64 image."sv)
        // Extract the payload and image type from the prompt.
        std::optional<std::pair<std::vector<uint8_t>, std::string>> Payload =
            extractBase64ImagePayload(Prompt, *Base64ImagePos,
                                      VisionPromptImagePlaceholder);
        if (Payload.has_value()) {
          // Create the new image bitmap.
          mtmd::bitmap Bitmap(mtmd_helper_bitmap_init_from_buf(
              GraphRef.VisionContext.get(), Payload->first.data(),
              Payload->first.size()));
          if (Bitmap.ptr == nullptr) {
            RET_ERROR(
                ErrNo::InvalidArgument,
                "setInput: unable to load the image from base64 paylaod."sv)
          }
          Bitmaps.entries.push_back(std::move(Bitmap));
        }
        LOG_DEBUG(GraphRef.EnableDebugLog,
                  "setInput: Compute image embd from the base64 image...Done"sv)
      } else {
        // Load the image from the file.
        LOG_DEBUG(GraphRef.EnableDebugLog,
                  "setInput: load the image bitmap from file: {}"sv,
                  CxtRef.Conf.ImagePath)
        mtmd::bitmap Bitmap(mtmd_helper_bitmap_init_from_file(
            GraphRef.VisionContext.get(), CxtRef.Conf.ImagePath.c_str()));
        if (Bitmap.ptr == nullptr) {
          RET_ERROR(
              ErrNo::InvalidArgument,
              "setInput: unable to load the image bitmap from file: {}."sv,
              CxtRef.Conf.ImagePath)
        }
        Bitmaps.entries.push_back(std::move(Bitmap));
        LOG_DEBUG(GraphRef.EnableDebugLog,
                  "setInput: load the image bitmap from file: {}...Done"sv,
                  CxtRef.Conf.ImagePath)
      }
    }

    // Tokenize the prompt.
    LOG_DEBUG(GraphRef.EnableDebugLog, "setInput: tokenize the mtmd prompt"sv)
    GraphRef.VisionInputChunks.reset(mtmd_input_chunks_init());
    mtmd_input_text MtmdText;
    MtmdText.text = Prompt.c_str();
    MtmdText.add_special = AddSpecial;
    MtmdText.parse_special = ParseSpecial;
    std::vector<const mtmd_bitmap *> BitmapsPtr = Bitmaps.c_ptr();
    int32_t Res = mtmd_tokenize(GraphRef.VisionContext.get(),
                                GraphRef.VisionInputChunks.get(), &MtmdText,
                                BitmapsPtr.data(), BitmapsPtr.size());
    if (Res != 0) {
      RET_ERROR(ErrNo::InvalidArgument,
                "setInput: unable to tokenize the mtmd prompt."sv)
    }
    LOG_DEBUG(GraphRef.EnableDebugLog,
              "setInput: tokenize the mtmd prompt...Done"sv)

    // Get the number of input tokens (for the metadata).
    CxtRef.LlamaNInputs = 0;
    for (size_t ChunkIndex = 0;
         ChunkIndex < mtmd_input_chunks_size(GraphRef.VisionInputChunks.get());
         ++ChunkIndex) {
      size_t NTokens = 0;
      const mtmd_input_chunk *Chunk =
          mtmd_input_chunks_get(GraphRef.VisionInputChunks.get(), ChunkIndex);
      mtmd_input_chunk_get_tokens_text(Chunk, &NTokens);
      CxtRef.LlamaNInputs += NTokens;
    }
  } else if (GraphRef.TextToSpeech == true) {
    // TTS prompt.
    LOG_DEBUG(GraphRef.EnableDebugLog, "setInput: tokenize tts prompt"sv)
    CxtRef.LlamaInputs = processTTSPrompt(Env, GraphRef, Prompt);
    if (CxtRef.LlamaInputs.empty()) {
      RET_ERROR(ErrNo::InvalidArgument,
                "setInput: failed to tokenize tts prompt."sv)
    }
    LOG_DEBUG(GraphRef.EnableDebugLog, "setInput: tokenize tts prompt...Done"sv)

    // Get the number of input tokens (for the metadata).
    CxtRef.LlamaNInputs = CxtRef.LlamaInputs.size();
  } else {
    // Text only prompt.
    LOG_DEBUG(GraphRef.EnableDebugLog, "setInput: tokenize text prompt"sv)
    CxtRef.LlamaInputs = common_tokenize(GraphRef.LlamaContext.get(), Prompt,
                                         AddSpecial, ParseSpecial);
    LOG_DEBUG(GraphRef.EnableDebugLog,
              "setInput: tokenize text prompt...Done"sv)

    // Get the number of input tokens (for the metadata).
    CxtRef.LlamaNInputs = CxtRef.LlamaInputs.size();
  }

  // Maybe currently in the compute_single mode. Reset the computing.
  CxtRef.ComputeSingleStarted = false;

  LOG_DEBUG(GraphRef.EnableDebugLog, "setInput...Done"sv)
  return ErrNo::Success;
}

#endif
} // namespace WasmEdge::Host::WASINN::GGML
