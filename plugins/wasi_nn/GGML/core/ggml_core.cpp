// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "ggml_core.h"
#include "GGML/utils.h"
#include "common/types.h"
#include "host/wasi/vfs_io.h"
#include "wasinnenv.h"
#include <cstdint>

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_GGML
#include "GGML/metadata/metadata_parser.h"
#include <base64.hpp>
#include <common.h>
#include <cstdlib>
#include <fmt/ranges.h>
#include <json-partial.h>
#include <json-schema-to-grammar.h>
#include <llama.h>
#include <mtmd-helper.h>
#include <mtmd.h>
#include <sampling.h>

#include <filesystem>
#include <math.h>
#endif

namespace WasmEdge::Host::WASINN::GGML {
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_GGML
namespace {

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
} // namespace

Expect<ErrNo> load(WasiNNEnvironment &Env, WASINN::Graph &G,
                   Span<const Span<uint8_t>> Builders, Device) noexcept {
  auto &GraphRef = G.get<Graph>();

  // Initialize the plugin parameters.
  GraphRef.EnableLog = false;
  GraphRef.EnableDebugLog = false;
  common_params CommonParamsDefault;
  CommonParamsDefault.lr.init();
  GraphRef.Params = CommonParamsDefault;
  GraphRef.Params.n_keep = 0;
  GraphRef.Params.n_chunks = -1;
  GraphRef.Params.n_parallel = 1;
  GraphRef.Params.grp_attn_n = 1;
  GraphRef.Params.grp_attn_w = 512;
  GraphRef.Params.n_print = -1;
  GraphRef.Params.split_mode = llama_split_mode::LLAMA_SPLIT_MODE_LAYER;
  // Initialize the model parameters.
  llama_model_params ModelParamsDefault = llama_model_default_params();
  GraphRef.Params.n_gpu_layers = ModelParamsDefault.n_gpu_layers;
  GraphRef.Params.mmproj.path = ""sv;
  GraphRef.Params.warmup = false;

  // Initialize the sampling parameters.
  const common_params_sampling SamplerParamsDefault;
  GraphRef.Params.sampling = SamplerParamsDefault;
  // Initialize the config parameters.
  GraphRef.Conf.StreamStdout = false;
  GraphRef.Conf.EmbdNormalize =
      static_cast<EmbdNormalizeType>(CommonParamsDefault.embd_normalize);
  GraphRef.Conf.NPredict = GraphRef.Params.n_predict;
  GraphRef.Conf.ReversePrompt = ""sv;
  GraphRef.Conf.ImagePath = ""sv;

  // Set llama log callback.
  llama_log_set(llamaLogCallback, &GraphRef);
  mtmd_helper_log_set(llamaLogCallback, &GraphRef);

  // If the graph builder length is greater than 1, builder[1] contains the
  // metadata.
  if (Builders.size() > 1) {
    const std::string Metadata(reinterpret_cast<char *>(Builders[1].data()),
                               Builders[1].size());
    // Ignore context or model updates when initializing the graph.
    auto Res = parseMetadata(GraphRef, GraphRef.Conf, Metadata);
    if (Res != ErrNo::Success) {
      RET_ERROR(Res, "load: Failed to parse metadata."sv)
    }
  }

  // Logging.
  LOG_DEBUG(GraphRef.EnableDebugLog, "load"sv)
  LOG_INFO(GraphRef.EnableLog, "LLAMA_COMMIT {}"sv, LLAMA_COMMIT)
  LOG_INFO(GraphRef.EnableLog, "LLAMA_BUILD_NUMBER {}"sv, LLAMA_BUILD_NUMBER)

  // Handle the model path.
  LOG_DEBUG(GraphRef.EnableDebugLog, "load: handling model path."sv)
  auto Weight = Builders[0];
  const std::string_view BinModel(reinterpret_cast<char *>(Weight.data()),
                                  Weight.size());
  if (BinModel.substr(0, 8) == "preload:"sv) {
    GraphRef.Params.model.path = BinModel.substr(8);
  } else {
    LOG_DEBUG(GraphRef.EnableDebugLog,
              "load: Model path not found in nn-preload, write model into "sv
              "a tmpfile."sv)
    // TODO: pass the model directly to ggml.
    // Write ggml model to file.
    GraphRef.Params.model.path = "ggml-model.bin"sv;
    WasmEdge::FStream::OFStream TempFile(
        GraphRef.Params.model.path, std::ios_base::out | std::ios_base::binary,
        Env.getEnv());
    if (!TempFile) {
      RET_ERROR(ErrNo::InvalidArgument,
                "load: Failed to create the temporary file. Currently, our "sv
                "workaround involves creating a temporary model file named "sv
                "\"ggml-model.bin\" and passing this filename as a "sv
                "parameter to the ggml llama library."sv)
    }
    TempFile.write(BinModel.data(), BinModel.size());
    TempFile.close();
    LOG_DEBUG(GraphRef.EnableDebugLog,
              "load: Write model into a tmpfile...Done"sv)
  }
  LOG_DEBUG(GraphRef.EnableDebugLog, "load: handling model path...Done"sv)

  // Check if the model exists.
  if (!std::filesystem::exists(
          std::filesystem::u8path(GraphRef.Params.model.path))) {
    RET_ERROR(ErrNo::ModelNotFound, "load: model file not found."sv)
  }
  GraphRef.Params.model = GraphRef.Params.model;

  // Initialize ggml parameters.
  LOG_DEBUG(GraphRef.EnableDebugLog,
            "load: initialize ggml model with given parameters."sv)

  common_params Params = GraphRef.Params;
  Params.cpuparams.n_threads =
      static_cast<int32_t>(GraphRef.Params.cpuparams.n_threads);
  Params.cpuparams_batch.n_threads =
      static_cast<int32_t>(GraphRef.Params.cpuparams.n_threads);
  llama_backend_init();
  llama_numa_init(Params.numa);

  // Initialize the llama model and context.
  llama_model_params ModelParams = common_model_params_to_llama(Params);
  GraphRef.LlamaModel = llama_model_ptr(
      llama_model_load_from_file(Params.model.path.c_str(), ModelParams));
  if (GraphRef.LlamaModel == nullptr) {
    RET_ERROR(ErrNo::InvalidArgument, "load: unable to init model."sv)
  }
  GraphRef.LlamaContext = llama_context_ptr(llama_init_from_model(
      GraphRef.LlamaModel.get(), common_context_params_to_llama(Params)));
  if (GraphRef.LlamaContext == nullptr) {
    RET_ERROR(ErrNo::InvalidArgument, "load: unable to init context."sv)
  }
  LOG_DEBUG(GraphRef.EnableDebugLog,
            "load: initialize ggml model with given parameters...Done"sv)

  // Initialize the TTS related model and context.
  if (GraphRef.TextToSpeech) {
    LOG_DEBUG(GraphRef.EnableDebugLog, "load: initialize TTS model."sv)
    Params.model = GraphRef.Params.vocoder.model;
    Params.embedding = true;
    llama_model_params TTSModelParams = common_model_params_to_llama(Params);
    GraphRef.TTSModel = llama_model_ptr(
        llama_model_load_from_file(Params.model.path.c_str(), TTSModelParams));
    if (GraphRef.TTSModel == nullptr) {
      RET_ERROR(ErrNo::InvalidArgument, "load: unable to init TTS model."sv)
    }
    GraphRef.TTSContext = llama_context_ptr(llama_init_from_model(
        GraphRef.TTSModel.get(), common_context_params_to_llama(Params)));
    if (GraphRef.TTSContext == nullptr) {
      RET_ERROR(ErrNo::InvalidArgument, "load: unable to init TTS context."sv)
    }
    LOG_DEBUG(GraphRef.EnableDebugLog, "load: initialize TTS model...Done"sv)
  }

  LOG_DEBUG(GraphRef.EnableDebugLog, "load...Done"sv)
  return ErrNo::Success;
}

Expect<ErrNo> initExecCtx(WasiNNEnvironment &, WASINN::Graph &G,
                          WASINN::Context &C) noexcept {
  auto &GraphRef = G.get<Graph>();
  LOG_DEBUG(GraphRef.EnableDebugLog, "initExecCtx"sv)
  LOG_INFO(GraphRef.EnableLog, "llama_system_info: {}"sv,
           llama_print_system_info())

  auto &CxtRef = C.get<Context>();
  // Allocate the batch for input string prompt tokens.
  CxtRef.LlamaBatch = allocBatch(GraphRef.Params.n_batch);
  CxtRef.CurrentBatchSize = GraphRef.Params.n_batch;

  // Allocate the batch for output sampling. The batch size is always 1.
  CxtRef.OutputBatch = allocBatch(1);

  // Allocate sampler.
  CxtRef.LlamaSampler =
      common_sampler_init(GraphRef.LlamaModel.get(), GraphRef.Params.sampling);

  LOG_DEBUG(GraphRef.EnableDebugLog, "initExecCtx...Done"sv)
  return ErrNo::Success;
}

Expect<ErrNo> finiSingle(WasiNNEnvironment &, WASINN::Graph &G,
                         WASINN::Context &C) noexcept {
  auto &CxtRef = C.get<Context>();
  auto &GraphRef = G.get<Graph>();
  LOG_DEBUG(GraphRef.EnableDebugLog, "finiSingle"sv)

  // Logging for the llama timings.
  if (GraphRef.EnableLog) {
    common_perf_print(GraphRef.LlamaContext.get(), CxtRef.LlamaSampler);
  }

  // Clear the outputs.
  LOG_DEBUG(GraphRef.EnableDebugLog,
            "finiSingle: clear the previous output and tokens"sv)
  CxtRef.LlamaOutputs.clear();
  CxtRef.LlamaOutputTokens.clear();
  LOG_DEBUG(GraphRef.EnableDebugLog,
            "finiSingle: clear the previous output and tokens...Done"sv)

  // Reset the llama sampler.
  common_sampler_reset(CxtRef.LlamaSampler);
  CxtRef.ComputeSingleStarted = false;
  CxtRef.NPos = 0;

  LOG_DEBUG(GraphRef.EnableDebugLog, "finiSingle...Done"sv)
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

Expect<ErrNo> load(WasiNNEnvironment &, WASINN::Graph &,
                   Span<const Span<uint8_t>>, Device) noexcept {
  return reportBackendNotSupported();
}
Expect<ErrNo> initExecCtx(WasiNNEnvironment &, WASINN::Graph &,
                          WASINN::Context &) noexcept {
  return reportBackendNotSupported();
}
Expect<ErrNo> setInput(WasiNNEnvironment &, WASINN::Graph &, WASINN::Context &,
                       uint32_t, const TensorData &) noexcept {
  return reportBackendNotSupported();
}
Expect<ErrNo> getOutput(WasiNNEnvironment &, WASINN::Graph &, WASINN::Context &,
                        uint32_t, Span<uint8_t>, uint32_t &) noexcept {
  return reportBackendNotSupported();
}
Expect<ErrNo> compute(WasiNNEnvironment &, WASINN::Graph &,
                      WASINN::Context &) noexcept {
  return reportBackendNotSupported();
}
Expect<ErrNo> getOutputSingle(WasiNNEnvironment &, WASINN::Graph &,
                              WASINN::Context &, uint32_t, Span<uint8_t>,
                              uint32_t &) noexcept {
  return reportBackendNotSupported();
}
Expect<ErrNo> computeSingle(WasiNNEnvironment &, WASINN::Graph &,
                            WASINN::Context &) noexcept {
  return reportBackendNotSupported();
}
Expect<ErrNo> finiSingle(WasiNNEnvironment &, WASINN::Graph &,
                         WASINN::Context &) noexcept {
  return reportBackendNotSupported();
}
#endif
} // namespace WasmEdge::Host::WASINN::GGML
