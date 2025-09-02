// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "wasinn_ggml.h"
#include "common/types.h"
#include "wasinnenv.h"
#include <cstdint>

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_GGML
#include "simdjson.h"
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

#include <algorithm>
#include <filesystem>
#include <math.h>
#include <optional>
#include <regex>
#include <sstream>
#include <tuple>
#endif

namespace WasmEdge::Host::WASINN::GGML {
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_GGML

namespace {

// Macro for logging debug message.
#define LOG_DEBUG(Debug, ...)                                                  \
  if (Debug) {                                                                 \
    spdlog::info("[WASI-NN][Debug] GGML backend: "sv __VA_ARGS__);             \
  }

// Macro for logging info message.
#define LOG_INFO(Info, ...)                                                    \
  if (Info) {                                                                  \
    spdlog::info("[WASI-NN] GGML backend: "sv __VA_ARGS__);                    \
  }

// Macro for logging warning message.
#define LOG_WARN(...) spdlog::warn("[WASI-NN] GGML backend: "sv __VA_ARGS__);

// Macro for logging error message.
#define LOG_ERROR(...) spdlog::error("[WASI-NN] GGML backend: "sv __VA_ARGS__);

// Macro for logging error message and return.
#define RET_ERROR(Error, ...)                                                  \
  spdlog::error("[WASI-NN] GGML backend: "sv __VA_ARGS__);                     \
  return Error;

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

// >>>>>>>> Metadata related functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

// Parse metadata from json.
ErrNo parseMetadata(Graph &GraphRef, LocalConfig &ConfRef,
                    const std::string &Metadata, bool *IsModelUpdated = nullptr,
                    bool *IsContextUpdated = nullptr,
                    bool *IsSamplerUpdated = nullptr) noexcept {
  // Parse metadata from the json.
  simdjson::dom::parser Parser;
  simdjson::dom::element Doc;
  auto ParseError = Parser.parse(Metadata).get(Doc);
  if (ParseError) {
    RET_ERROR(ErrNo::InvalidEncoding, "parse metadata error."sv)
  }

  // Get the current llama parameters.
  int64_t PrevNGPULayers = GraphRef.Params.n_gpu_layers;
  bool PrevEmbedding = GraphRef.Params.embedding;
  // Get the current sampler parameters.
  double PrevTemp = GraphRef.Params.sampling.temp;
  double PrevTopP = GraphRef.Params.sampling.top_p;
  double PrevRepeatPenalty = GraphRef.Params.sampling.penalty_repeat;
  double PrevPresencePenalty = GraphRef.Params.sampling.penalty_present;
  double PrevFrequencyPenalty = GraphRef.Params.sampling.penalty_freq;
  std::string PrevGrammar = GraphRef.Params.sampling.grammar;
  uint64_t PrevSeed = GraphRef.Params.sampling.seed;

  // The plugin parameters.
  if (Doc.at_key("enable-log").error() == simdjson::SUCCESS) {
    auto Err = Doc["enable-log"].get<bool>().get(GraphRef.EnableLog);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the enable-log option."sv)
    }
  }
  if (Doc.at_key("enable-debug-log").error() == simdjson::SUCCESS) {
    auto Err = Doc["enable-debug-log"].get<bool>().get(GraphRef.EnableDebugLog);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the enable-debug-log option."sv)
    }
  }

  // The model parameters.
  if (Doc.at_key("main-gpu").error() == simdjson::SUCCESS) {
    int64_t MainGPU;
    auto Err = Doc["main-gpu"].get<int64_t>().get(MainGPU);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the main-gpu option."sv)
    }
    GraphRef.Params.main_gpu = static_cast<int32_t>(MainGPU);
  }
  if (Doc.at_key("n-gpu-layers").error() == simdjson::SUCCESS) {
    int64_t NGPULayers;
    auto Err = Doc["n-gpu-layers"].get<int64_t>().get(NGPULayers);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the n-gpu-layers option."sv)
    }
    GraphRef.Params.n_gpu_layers = static_cast<int32_t>(NGPULayers);
  }
  if (Doc.at_key("cpu-moe").error() == simdjson::SUCCESS) {
    bool CpuMoe;
    auto Err = Doc["cpu-moe"].get<bool>().get(CpuMoe);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the cpu-moe option."sv)
    }
    if (CpuMoe) {
      GraphRef.TensorBuftOverrides.push_back("\\.ffn_(up|down|gate)_exps");
    }
  }
  if (Doc.at_key("n-cpu-moe").error() == simdjson::SUCCESS) {
    int64_t NCpuMoe;
    auto Err = Doc["n-cpu-moe"].get<int64_t>().get(NCpuMoe);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the n-cpu-moe option."sv)
    }
    if (NCpuMoe < 0) {
      RET_ERROR(ErrNo::InvalidArgument, "Invalid n-cpu-moe value."sv)
    }
    for (int I = 0; I < NCpuMoe; I++) {
      GraphRef.TensorBuftOverrides.push_back(
          string_format("blk\\.%d\\.ffn_(up|down|gate)_exps", I));
    }
  }
  if (Doc.at_key("tensor-split").error() == simdjson::SUCCESS) {
    // The TensorSplit is a comma-separated list of non-negative values.
    // E.g., "3,2" presents 60% of the data to GPU 0 and 40% to GPU 1.
    std::string_view TSV;
    auto Err = Doc["tensor-split"].get<std::string_view>().get(TSV);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the tensor-split option."sv)
    }
    std::string TS(TSV);
    std::replace(TS.begin(), TS.end(), ',', ' ');
    std::stringstream SS(TS);
    std::memset(GraphRef.Params.tensor_split, 0,
                sizeof(GraphRef.Params.tensor_split));
    uint32_t TensorSplitSize = 0;
    while (SS.good()) {
      float TmpTensor;
      SS >> TmpTensor;
      GraphRef.Params.tensor_split[TensorSplitSize++] = TmpTensor;
    }
    size_t NDevices = llama_max_devices();
    if (TensorSplitSize > NDevices) {
      RET_ERROR(
          ErrNo::InvalidArgument,
          "Number of Tensor-Split is larger than MaxDevices, please reduce "sv
          "the size of tensor-split."sv)
    }
    for (size_t Idx = TensorSplitSize; Idx < NDevices; Idx++) {
      GraphRef.Params.tensor_split[TensorSplitSize++] = 0.0f;
    }
  }
  if (Doc.at_key("embedding").error() == simdjson::SUCCESS) {

    auto Err = Doc["embedding"].get<bool>().get(GraphRef.Params.embedding);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the embedding option."sv)
    }
  }
  if (Doc.at_key("split-mode").error() == simdjson::SUCCESS) {
    std::string_view SplitMode;
    auto Err = Doc["split-mode"].get<std::string_view>().get(SplitMode);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the split-mode option."sv)
    }
    if (SplitMode == "none"sv) {
      GraphRef.Params.split_mode = LLAMA_SPLIT_MODE_NONE;
    } else if (SplitMode == "layer"sv) {
      GraphRef.Params.split_mode = LLAMA_SPLIT_MODE_LAYER;
    } else if (SplitMode == "row"sv) {
      GraphRef.Params.split_mode = LLAMA_SPLIT_MODE_ROW;
    } else {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unknown split-mode: {}. Valid: none, layer, row."sv, SplitMode)
    }
  }
  if (Doc.at_key("mmproj").error() == simdjson::SUCCESS) {
    std::string_view MMProjModelPath;
    auto Err = Doc["mmproj"].get<std::string_view>().get(MMProjModelPath);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the mmproj option."sv)
    }
    GraphRef.Params.mmproj.path = MMProjModelPath;
  }

  // The TTS parameters.
  if (Doc.at_key("tts").error() == simdjson::SUCCESS) {
    auto Err = Doc["tts"].get<bool>().get(GraphRef.TextToSpeech);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument, "Unable to retrieve the tts option."sv)
    }
  }
  if (Doc.at_key("model-vocoder").error() == simdjson::SUCCESS) {
    std::string_view VocoderModelPath;
    auto Err =
        Doc["model-vocoder"].get<std::string_view>().get(VocoderModelPath);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the model-vocoder option."sv)
    }
    GraphRef.Params.vocoder.model.path = VocoderModelPath;
  }
  if (Doc.at_key("tts-output-file").error() == simdjson::SUCCESS) {
    std::string_view TTSOutputFilePath;
    auto Err =
        Doc["tts-output-file"].get<std::string_view>().get(TTSOutputFilePath);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the tts-output-file option."sv)
    }
    GraphRef.TTSOutputFilePath = TTSOutputFilePath;
  }
  if (Doc.at_key("tts-speaker-file").error() == simdjson::SUCCESS) {
    std::string_view TTSSpeakerFilePath;
    auto Err =
        Doc["tts-speaker-file"].get<std::string_view>().get(TTSSpeakerFilePath);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the tts-speaker-file option."sv)
    }
    GraphRef.TTSSpeakerFilePath = TTSSpeakerFilePath;
  }

  // The context parameters.
  if (Doc.at_key("ctx-size").error() == simdjson::SUCCESS) {
    int64_t CtxSize;
    auto Err = Doc["ctx-size"].get<int64_t>().get(CtxSize);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the ctx-size option."sv)
    }
    GraphRef.Params.n_ctx = static_cast<int32_t>(CtxSize);
  }
  if (Doc.at_key("batch-size").error() == simdjson::SUCCESS) {
    int64_t BatchSize;
    auto Err = Doc["batch-size"].get<int64_t>().get(BatchSize);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the batch-size option."sv)
    }
    GraphRef.Params.n_batch = static_cast<int32_t>(BatchSize);
  }
  if (Doc.at_key("ubatch-size").error() == simdjson::SUCCESS) {
    int64_t UBatchSize;
    auto Err = Doc["ubatch-size"].get<int64_t>().get(UBatchSize);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the ubatch-size option."sv)
    }
    GraphRef.Params.n_ubatch = static_cast<int32_t>(UBatchSize);
  }
  if (Doc.at_key("n-keep").error() == simdjson::SUCCESS) {
    int64_t NKeep;
    auto Err = Doc["n-keep"].get<int64_t>().get(NKeep);
    if (Err) {
      spdlog::error(
          "[WASI-NN] GGML backend: Unable to retrieve the n-keep option."sv);
      return ErrNo::InvalidArgument;
    }
    GraphRef.Params.n_keep = static_cast<int32_t>(NKeep);
  }
  if (Doc.at_key("n-chunks").error() == simdjson::SUCCESS) {
    int64_t NChunks;
    auto Err = Doc["n-chunks"].get<int64_t>().get(NChunks);
    if (Err) {
      spdlog::error(
          "[WASI-NN] GGML backend: Unable to retrieve the n-chunks option."sv);
      return ErrNo::InvalidArgument;
    }
    GraphRef.Params.n_chunks = static_cast<int32_t>(NChunks);
  }
  if (Doc.at_key("n-parallel").error() == simdjson::SUCCESS) {
    int64_t NParallel;
    auto Err = Doc["n-parallel"].get<int64_t>().get(NParallel);
    if (Err) {
      spdlog::error(
          "[WASI-NN] GGML backend: Unable to retrieve the n-parallel option."sv);
      return ErrNo::InvalidArgument;
    }
    GraphRef.Params.n_parallel = static_cast<int32_t>(NParallel);
  }
  if (Doc.at_key("n-sequences").error() == simdjson::SUCCESS) {
    int64_t NSequences;
    auto Err = Doc["n-sequences"].get<int64_t>().get(NSequences);
    if (Err) {
      spdlog::error(
          "[WASI-NN] GGML backend: Unable to retrieve the n-sequences option."sv);
      return ErrNo::InvalidArgument;
    }
    GraphRef.Params.n_sequences = static_cast<int32_t>(NSequences);
  }
  if (Doc.at_key("grp-attn-n").error() == simdjson::SUCCESS) {
    int64_t GrpAttnN;
    auto Err = Doc["grp-attn-n"].get<int64_t>().get(GrpAttnN);
    if (Err) {
      spdlog::error(
          "[WASI-NN] GGML backend: Unable to retrieve the grp-attn-n option."sv);
      return ErrNo::InvalidArgument;
    }
    GraphRef.Params.grp_attn_n = static_cast<int32_t>(GrpAttnN);
  }
  if (Doc.at_key("grp-attn-w").error() == simdjson::SUCCESS) {
    int64_t GrpAttnW;
    auto Err = Doc["grp-attn-w"].get<int64_t>().get(GrpAttnW);
    if (Err) {
      spdlog::error(
          "[WASI-NN] GGML backend: Unable to retrieve the grp-attn-w option."sv);
      return ErrNo::InvalidArgument;
    }
    GraphRef.Params.grp_attn_w = static_cast<int32_t>(GrpAttnW);
  }
  if (Doc.at_key("n-print").error() == simdjson::SUCCESS) {
    int64_t NPrint;
    auto Err = Doc["n-print"].get<int64_t>().get(NPrint);
    if (Err) {
      spdlog::error(
          "[WASI-NN] GGML backend: Unable to retrieve the n-print option."sv);
      return ErrNo::InvalidArgument;
    }
    GraphRef.Params.n_print = static_cast<int32_t>(NPrint);
  }
  if (Doc.at_key("rope-freq-base").error() == simdjson::SUCCESS) {
    double RopeFreqBase;
    auto Err = Doc["rope-freq-base"].get<double>().get(RopeFreqBase);
    if (Err) {
      spdlog::error(
          "[WASI-NN] GGML backend: Unable to retrieve the rope-freq-base option."sv);
      return ErrNo::InvalidArgument;
    }
    GraphRef.Params.rope_freq_base = static_cast<float>(RopeFreqBase);
  }
  if (Doc.at_key("rope-freq-scale").error() == simdjson::SUCCESS) {
    double RopeFreqScale;
    auto Err = Doc["rope-freq-scale"].get<double>().get(RopeFreqScale);
    if (Err) {
      spdlog::error(
          "[WASI-NN] GGML backend: Unable to retrieve the rope-freq-scale option."sv);
      return ErrNo::InvalidArgument;
    }
    GraphRef.Params.rope_freq_scale = static_cast<float>(RopeFreqScale);
  }
  if (Doc.at_key("yarn-ext-factor").error() == simdjson::SUCCESS) {
    double YarnExtFactor;
    auto Err = Doc["yarn-ext-factor"].get<double>().get(YarnExtFactor);
    if (Err) {
      spdlog::error(
          "[WASI-NN] GGML backend: Unable to retrieve the yarn-ext-factor option."sv);
      return ErrNo::InvalidArgument;
    }
    GraphRef.Params.yarn_ext_factor = static_cast<float>(YarnExtFactor);
  }
  if (Doc.at_key("yarn-attn-factor").error() == simdjson::SUCCESS) {
    double YarnAttnFactor;
    auto Err = Doc["yarn-attn-factor"].get<double>().get(YarnAttnFactor);
    if (Err) {
      spdlog::error(
          "[WASI-NN] GGML backend: Unable to retrieve the yarn-attn-factor option."sv);
      return ErrNo::InvalidArgument;
    }
    GraphRef.Params.yarn_attn_factor = static_cast<float>(YarnAttnFactor);
  }
  if (Doc.at_key("yarn-beta-fast").error() == simdjson::SUCCESS) {
    double YarnBetaFast;
    auto Err = Doc["yarn-beta-fast"].get<double>().get(YarnBetaFast);
    if (Err) {
      spdlog::error(
          "[WASI-NN] GGML backend: Unable to retrieve the yarn-beta-fast option."sv);
      return ErrNo::InvalidArgument;
    }
    GraphRef.Params.yarn_beta_fast = static_cast<float>(YarnBetaFast);
  }
  if (Doc.at_key("yarn-beta-slow").error() == simdjson::SUCCESS) {
    double YarnBetaSlow;
    auto Err = Doc["yarn-beta-slow"].get<double>().get(YarnBetaSlow);
    if (Err) {
      spdlog::error(
          "[WASI-NN] GGML backend: Unable to retrieve the yarn-beta-slow option."sv);
      return ErrNo::InvalidArgument;
    }
    GraphRef.Params.yarn_beta_slow = static_cast<float>(YarnBetaSlow);
  }
  if (Doc.at_key("yarn-orig-ctx").error() == simdjson::SUCCESS) {
    int64_t YarnOrigCtx;
    auto Err = Doc["yarn-orig-ctx"].get<int64_t>().get(YarnOrigCtx);
    if (Err) {
      spdlog::error(
          "[WASI-NN] GGML backend: Unable to retrieve the yarn-orig-ctx option."sv);
      return ErrNo::InvalidArgument;
    }
    GraphRef.Params.yarn_orig_ctx = static_cast<int32_t>(YarnOrigCtx);
  }
  if (Doc.at_key("mask-valid").error() == simdjson::SUCCESS) {
    auto Err =
        Doc["mask-valid"].get<bool>().get(GraphRef.Params.cpuparams.mask_valid);
    if (Err) {
      spdlog::error(
          "[WASI-NN] GGML backend: Unable to retrieve the mask-valid option."sv);
      return ErrNo::InvalidArgument;
    }
  }
  if (Doc.at_key("priority").error() == simdjson::SUCCESS) {
    int64_t Priority;
    auto Err = Doc["priority"].get<int64_t>().get(Priority);
    if (Err) {
      spdlog::error(
          "[WASI-NN] GGML backend: Unable to retrieve the priority option."sv);
      return ErrNo::InvalidArgument;
    }
    GraphRef.Params.cpuparams.priority =
        static_cast<ggml_sched_priority>(Priority);
  }
  if (Doc.at_key("strict-cpu").error() == simdjson::SUCCESS) {
    auto Err =
        Doc["strict-cpu"].get<bool>().get(GraphRef.Params.cpuparams.strict_cpu);
    if (Err) {
      spdlog::error(
          "[WASI-NN] GGML backend: Unable to retrieve the strict-cpu option."sv);
      return ErrNo::InvalidArgument;
    }
  }
  if (Doc.at_key("poll").error() == simdjson::SUCCESS) {
    int64_t Poll;
    auto Err = Doc["poll"].get<int64_t>().get(Poll);
    if (Err) {
      spdlog::error(
          "[WASI-NN] GGML backend: Unable to retrieve the poll option."sv);
      return ErrNo::InvalidArgument;
    }
    GraphRef.Params.cpuparams.poll = static_cast<int32_t>(Poll);
  }
  if (Doc.at_key("mask-valid-batch").error() == simdjson::SUCCESS) {
    auto Err = Doc["mask-valid-batch"].get<bool>().get(
        GraphRef.Params.cpuparams_batch.mask_valid);
    if (Err) {
      spdlog::error(
          "[WASI-NN] GGML backend: Unable to retrieve the mask-valid-batch option."sv);
      return ErrNo::InvalidArgument;
    }
  }
  if (Doc.at_key("priority-batch").error() == simdjson::SUCCESS) {
    int64_t Priority;
    auto Err = Doc["priority-batch"].get<int64_t>().get(Priority);
    if (Err) {
      spdlog::error(
          "[WASI-NN] GGML backend: Unable to retrieve the priority-batch option."sv);
      return ErrNo::InvalidArgument;
    }
    GraphRef.Params.cpuparams_batch.priority =
        static_cast<ggml_sched_priority>(Priority);
  }
  if (Doc.at_key("strict-cpu-batch").error() == simdjson::SUCCESS) {
    auto Err = Doc["strict-cpu-batch"].get<bool>().get(
        GraphRef.Params.cpuparams_batch.strict_cpu);
    if (Err) {
      spdlog::error(
          "[WASI-NN] GGML backend: Unable to retrieve the strict-cpu-batch option."sv);
      return ErrNo::InvalidArgument;
    }
  }
  if (Doc.at_key("poll-batch").error() == simdjson::SUCCESS) {
    int64_t Poll;
    auto Err = Doc["poll-batch"].get<int64_t>().get(Poll);
    if (Err) {
      spdlog::error(
          "[WASI-NN] GGML backend: Unable to retrieve the poll-batch option."sv);
      return ErrNo::InvalidArgument;
    }
    GraphRef.Params.cpuparams_batch.poll = static_cast<int32_t>(Poll);
  }
  if (Doc.at_key("numa").error() == simdjson::SUCCESS) {
    int64_t Numa;
    auto Err = Doc["numa"].get<int64_t>().get(Numa);
    if (Err) {
      spdlog::error(
          "[WASI-NN] GGML backend: Unable to retrieve the numa option."sv);
      return ErrNo::InvalidArgument;
    }
    GraphRef.Params.numa = static_cast<ggml_numa_strategy>(Numa);
  }
  if (Doc.at_key("rope-scaling-type").error() == simdjson::SUCCESS) {
    int64_t RopeScalingType;
    auto Err = Doc["rope-scaling-type"].get<int64_t>().get(RopeScalingType);
    if (Err) {
      spdlog::error(
          "[WASI-NN] GGML backend: Unable to retrieve the rope-scaling-type option."sv);
      return ErrNo::InvalidArgument;
    }
    GraphRef.Params.rope_scaling_type =
        static_cast<llama_rope_scaling_type>(RopeScalingType);
  }
  if (Doc.at_key("pooling-type").error() == simdjson::SUCCESS) {
    int64_t PoolingType;
    auto Err = Doc["pooling-type"].get<int64_t>().get(PoolingType);
    if (Err) {
      spdlog::error(
          "[WASI-NN] GGML backend: Unable to retrieve the pooling-type option."sv);
      return ErrNo::InvalidArgument;
    }
    GraphRef.Params.pooling_type =
        static_cast<enum llama_pooling_type>(PoolingType);
  }
  if (Doc.at_key("attention-type").error() == simdjson::SUCCESS) {
    int64_t AttentionType;
    auto Err = Doc["attention-type"].get<int64_t>().get(AttentionType);
    if (Err) {
      spdlog::error(
          "[WASI-NN] GGML backend: Unable to retrieve the attention-type option."sv);
      return ErrNo::InvalidArgument;
    }
    GraphRef.Params.attention_type =
        static_cast<llama_attention_type>(AttentionType);
  }
  if (Doc.at_key("threads").error() == simdjson::SUCCESS) {
    int64_t NThreads;
    auto Err = Doc["threads"].get<int64_t>().get(NThreads);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the threads option."sv)
    }
    GraphRef.Params.cpuparams.n_threads = static_cast<int32_t>(NThreads);
  }
  if (Doc.at_key("threads-batch").error() == simdjson::SUCCESS) {
    int64_t NThreadsBatch;
    auto Err = Doc["threads-batch"].get<int64_t>().get(NThreadsBatch);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the threads-batch option."sv)
    }
    GraphRef.Params.cpuparams_batch.n_threads =
        static_cast<int32_t>(NThreadsBatch);
  }

  // The sampling parameters.
  if (Doc.at_key("n-prev").error() == simdjson::SUCCESS) {
    int64_t NPrev;
    auto Err = Doc["n-prev"].get<int64_t>().get(NPrev);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the n_prev option."sv)
    }
    GraphRef.Params.sampling.n_prev = static_cast<int32_t>(NPrev);
  }
  if (Doc.at_key("n-probs").error() == simdjson::SUCCESS) {
    int64_t NProbs;
    auto Err = Doc["n-probs"].get<int64_t>().get(NProbs);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the n_probs option."sv)
    }
    GraphRef.Params.sampling.n_probs = static_cast<int32_t>(NProbs);
  }
  if (Doc.at_key("min-keep").error() == simdjson::SUCCESS) {
    int64_t MinKeep;
    auto Err = Doc["min-keep"].get<int64_t>().get(MinKeep);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the min-keep option."sv)
    }
    GraphRef.Params.sampling.min_keep = static_cast<int32_t>(MinKeep);
  }
  if (Doc.at_key("top-k").error() == simdjson::SUCCESS) {
    int64_t TopK;
    auto Err = Doc["top-k"].get<int64_t>().get(TopK);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the top-k option."sv)
    }
    GraphRef.Params.sampling.top_k = static_cast<int32_t>(TopK);
  }
  if (Doc.at_key("min-p").error() == simdjson::SUCCESS) {
    double MinP;
    auto Err = Doc["min-p"].get<double>().get(MinP);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the min-p option."sv)
    }
    GraphRef.Params.sampling.min_p = static_cast<float>(MinP);
  }
  if (Doc.at_key("xtc-probability").error() == simdjson::SUCCESS) {
    double XtcProbability;
    auto Err = Doc["xtc-probability"].get<double>().get(XtcProbability);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the xtc-probability option."sv)
    }
    GraphRef.Params.sampling.xtc_probability =
        static_cast<float>(XtcProbability);
  }
  if (Doc.at_key("xtc-threshold").error() == simdjson::SUCCESS) {
    double XtcThreshold;
    auto Err = Doc["xtc-threshold"].get<double>().get(XtcThreshold);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the xtc-threshold option."sv)
    }
    GraphRef.Params.sampling.xtc_threshold = static_cast<float>(XtcThreshold);
  }
  if (Doc.at_key("typ-p").error() == simdjson::SUCCESS) {
    double TypP;
    auto Err = Doc["typ-p"].get<double>().get(TypP);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the typ-p option."sv)
    }
    GraphRef.Params.sampling.typ_p = static_cast<float>(TypP);
  }
  if (Doc.at_key("dynatemp-range").error() == simdjson::SUCCESS) {
    double DynaTempRange;
    auto Err = Doc["dynatemp-range"].get<double>().get(DynaTempRange);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the dynatemp-range option."sv)
    }
    GraphRef.Params.sampling.dynatemp_range = static_cast<float>(DynaTempRange);
  }
  if (Doc.at_key("dynatemp-exponent").error() == simdjson::SUCCESS) {
    double DynaTempExponent;
    auto Err = Doc["dynatemp-exponent"].get<double>().get(DynaTempExponent);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the dynatemp-exponent option."sv)
    }
    GraphRef.Params.sampling.dynatemp_exponent =
        static_cast<float>(DynaTempExponent);
  }
  if (Doc.at_key("last-n-penalty").error() == simdjson::SUCCESS) {
    int64_t LastNPenalty;
    auto Err = Doc["last-n-penalty"].get<int64_t>().get(LastNPenalty);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the last-n-penalty option."sv)
    }
    GraphRef.Params.sampling.penalty_last_n =
        static_cast<int32_t>(LastNPenalty);
  }
  if (Doc.at_key("temp").error() == simdjson::SUCCESS) {
    double Temp;
    auto Err = Doc["temp"].get<double>().get(Temp);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument, "Unable to retrieve the temp option."sv)
    }
    GraphRef.Params.sampling.temp = static_cast<float>(std::max(0.0, Temp));
  }
  if (Doc.at_key("top-p").error() == simdjson::SUCCESS) {
    double TopP;
    auto Err = Doc["top-p"].get<double>().get(TopP);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the top-p option."sv)
    }
    GraphRef.Params.sampling.top_p = static_cast<float>(std::max(0.0, TopP));
  }
  if (Doc.at_key("repeat-penalty").error() == simdjson::SUCCESS) {
    double RepeatPenalty;
    auto Err = Doc["repeat-penalty"].get<double>().get(RepeatPenalty);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the repeat-penalty option."sv)
    }
    GraphRef.Params.sampling.penalty_repeat =
        static_cast<float>(std::max(0.0, RepeatPenalty));
  }
  if (Doc.at_key("presence-penalty").error() == simdjson::SUCCESS) {
    double PresencePenalty;
    auto Err = Doc["presence-penalty"].get<double>().get(PresencePenalty);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the presence-penalty option."sv)
    }
    GraphRef.Params.sampling.penalty_present =
        static_cast<float>(std::max(0.0, PresencePenalty));
  }
  if (Doc.at_key("frequency-penalty").error() == simdjson::SUCCESS) {
    double FrequencyPenalty;
    auto Err = Doc["frequency-penalty"].get<double>().get(FrequencyPenalty);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the frequency-penalty option."sv)
    }
    GraphRef.Params.sampling.penalty_freq =
        static_cast<float>(std::max(0.0, FrequencyPenalty));
  }
  if (Doc.at_key("dry-multipier").error() == simdjson::SUCCESS) {
    double DryMultiplier;
    auto Err = Doc["dry-multipier"].get<double>().get(DryMultiplier);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the dry-multipier option."sv)
    }
    GraphRef.Params.sampling.dry_multiplier = static_cast<float>(DryMultiplier);
  }
  if (Doc.at_key("dry-base").error() == simdjson::SUCCESS) {
    double DryBase;
    auto Err = Doc["dry-base"].get<double>().get(DryBase);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the dry-base option."sv)
    }
    GraphRef.Params.sampling.dry_base = static_cast<float>(DryBase);
  }
  if (Doc.at_key("dry-allowed-length").error() == simdjson::SUCCESS) {
    int64_t DryAllowedLength;
    auto Err = Doc["dry-allowed-length"].get<int64_t>().get(DryAllowedLength);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the dry-allowed-length option."sv)
    }
    GraphRef.Params.sampling.dry_allowed_length =
        static_cast<int32_t>(DryAllowedLength);
  }
  if (Doc.at_key("dry-last-n-penalty").error() == simdjson::SUCCESS) {
    int64_t DryLastNPenalty;
    auto Err = Doc["dry-last-n-penalty"].get<int64_t>().get(DryLastNPenalty);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the dry-last-n-penalty option."sv)
    }
    GraphRef.Params.sampling.penalty_last_n =
        static_cast<int32_t>(DryLastNPenalty);
  }
  if (Doc.at_key("mirostat").error() == simdjson::SUCCESS) {
    int64_t Mirostat;
    auto Err = Doc["mirostat"].get<int64_t>().get(Mirostat);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the mirostat option."sv)
    }
    GraphRef.Params.sampling.mirostat = static_cast<int32_t>(Mirostat);
  }
  if (Doc.at_key("mirostat-eta").error() == simdjson::SUCCESS) {
    double MirostatEta;
    auto Err = Doc["mirostat-eta"].get<double>().get(MirostatEta);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the mirostat-eta option."sv)
    }
    GraphRef.Params.sampling.mirostat_eta = static_cast<float>(MirostatEta);
  }
  if (Doc.at_key("ignore-eos").error() == simdjson::SUCCESS) {
    auto Err =
        Doc["ignore-eos"].get<bool>().get(GraphRef.Params.sampling.ignore_eos);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the ignore-eos option."sv)
    }
  }
  if (Doc.at_key("no-perf-sampling").error() == simdjson::SUCCESS) {
    auto Err = Doc["no-perf-sampling"].get<bool>().get(
        GraphRef.Params.sampling.no_perf);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the no-perf-sampling option."sv)
    }
  }
  if (Doc.at_key("timing-per-token").error() == simdjson::SUCCESS) {
    auto Err = Doc["timing-per-token"].get<bool>().get(
        GraphRef.Params.sampling.timing_per_token);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the timing-per-token option."sv)
    }
  }
  if (Doc.at_key("grammar").error() == simdjson::SUCCESS) {
    std::string_view Grammar;
    auto Err = Doc["grammar"].get<std::string_view>().get(Grammar);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the grammar option."sv)
    }
    GraphRef.Params.sampling.grammar = Grammar;
  }
  if (Doc.at_key("json-schema").error() == simdjson::SUCCESS) {
    std::string_view JsonSchema;
    auto Err = Doc["json-schema"].get<std::string_view>().get(JsonSchema);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the json-schema option."sv)
    }
    GraphRef.Params.sampling.grammar =
        json_schema_to_grammar(nlohmann::ordered_json::parse(JsonSchema));
  }
  if (Doc.at_key("seed").error() == simdjson::SUCCESS) {
    uint64_t Seed;
    auto Err = Doc["seed"].get<uint64_t>().get(Seed);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument, "Unable to retrieve the seed option."sv)
    }
    GraphRef.Params.sampling.seed = static_cast<int32_t>(Seed);
  }
  // The speculative parameters.
  if (Doc.at_key("n-ctx-speculative").error() == simdjson::SUCCESS) {
    int64_t NCtxSpeculative;
    auto Err = Doc["n-ctx-speculative"].get<int64_t>().get(NCtxSpeculative);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the n-ctx-speculative option."sv)
    }
    GraphRef.Params.speculative.n_ctx = static_cast<int32_t>(NCtxSpeculative);
  }
  if (Doc.at_key("n-max-speculative").error() == simdjson::SUCCESS) {
    int64_t NMaxSpeculative;
    auto Err = Doc["n-max-speculative"].get<int64_t>().get(NMaxSpeculative);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the n-max-speculative option."sv)
    }
    GraphRef.Params.speculative.n_max = static_cast<int32_t>(NMaxSpeculative);
  }
  if (Doc.at_key("n-min-speculative").error() == simdjson::SUCCESS) {
    int64_t NMinSpeculative;
    auto Err = Doc["n-min-speculative"].get<int64_t>().get(NMinSpeculative);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the n-min-speculative option."sv)
    }
    GraphRef.Params.speculative.n_min = static_cast<int32_t>(NMinSpeculative);
  }
  if (Doc.at_key("n-gpu-layers-speculative").error() == simdjson::SUCCESS) {
    int64_t NGPULatersinSpeculative;
    auto Err = Doc["n-gpu-layers-speculative"].get<int64_t>().get(
        NGPULatersinSpeculative);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the n-gpu-layers-speculative option."sv)
    }
    GraphRef.Params.speculative.n_gpu_layers =
        static_cast<int32_t>(NGPULatersinSpeculative);
  }
  if (Doc.at_key("p-split-speculative").error() == simdjson::SUCCESS) {
    double PSplitSpeculative;
    auto Err = Doc["p-split-speculative"].get<double>().get(PSplitSpeculative);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the p-split-speculative option."sv)
    }
    GraphRef.Params.speculative.p_split = static_cast<float>(PSplitSpeculative);
  }
  if (Doc.at_key("p-min-speculative").error() == simdjson::SUCCESS) {
    double PMinSpeculative;
    auto Err = Doc["p-min-speculative"].get<double>().get(PMinSpeculative);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the p-min-speculative option."sv)
    }
    GraphRef.Params.speculative.p_min = static_cast<float>(PMinSpeculative);
  }
  // The vocoder parameters.
  if (Doc.at_key("hf-repo-vocoder").error() == simdjson::SUCCESS) {
    std::string_view HfRepo;
    auto Err = Doc["hf-repo-vocoder"].get<std::string_view>().get(HfRepo);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the hf-repo-vocoder option."sv)
    }
    GraphRef.Params.vocoder.model.hf_repo = HfRepo;
  }
  if (Doc.at_key("hf-file-vocoder").error() == simdjson::SUCCESS) {
    std::string_view HfFile;
    auto Err = Doc["hf-file-vocoder"].get<std::string_view>().get(HfFile);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the hf-file-vocoder option."sv)
    }
    GraphRef.Params.vocoder.model.hf_file = HfFile;
  }
  if (Doc.at_key("model-url-vocoder").error() == simdjson::SUCCESS) {
    std::string_view ModelUrlVocoder;
    auto Err =
        Doc["model-url-vocoder"].get<std::string_view>().get(ModelUrlVocoder);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the model-url-vocoder option."sv)
    }
    GraphRef.Params.vocoder.model.url = ModelUrlVocoder;
  }
  // The config parameters.
  if (Doc.at_key("stream-stdout").error() == simdjson::SUCCESS) {
    auto Err = Doc["stream-stdout"].get<bool>().get(ConfRef.StreamStdout);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the stream-stdout option."sv)
    }
  }
  if (Doc.at_key("n-predict").error() == simdjson::SUCCESS) {
    auto Err = Doc["n-predict"].get<int64_t>().get(ConfRef.NPredict);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the n-predict option."sv)
    }
  }
  if (Doc.at_key("reverse-prompt").error() == simdjson::SUCCESS) {
    std::string_view ReversePrompt;
    auto Err = Doc["reverse-prompt"].get<std::string_view>().get(ReversePrompt);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the reverse-prompt option."sv)
    }
    ConfRef.ReversePrompt = ReversePrompt;
  }
  if (Doc.at_key("image").error() == simdjson::SUCCESS) {
    std::string_view ImagePath;
    auto Err = Doc["image"].get<std::string_view>().get(ImagePath);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the image option."sv)
    }
    ConfRef.ImagePath = ImagePath;
  }
  if (Doc.at_key("always-regenerate-image-embd").error() == simdjson::SUCCESS) {
    auto Err = Doc["always-regenerate-image-embd"].get<bool>().get(
        ConfRef.AlwaysRegenerateImageEmbd);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the always-regenerate-image-embd option."sv)
    }
  }
  if (Doc.at_key("model-alias").error() == simdjson::SUCCESS) {
    std::string_view ModelAlias;
    auto Err = Doc["model-alias"].get<std::string_view>().get(ModelAlias);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the model-alias option."sv)
    }
    GraphRef.Params.model_alias = ModelAlias;
  }
  if (Doc.at_key("model-url").error() == simdjson::SUCCESS) {
    std::string_view ModelUrl;
    auto Err = Doc["model-url"].get<std::string_view>().get(ModelUrl);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the model-url option."sv)
    }
    GraphRef.Params.model.url = ModelUrl;
  }
  if (Doc.at_key("hf-token").error() == simdjson::SUCCESS) {
    std::string_view HfToken;
    auto Err = Doc["hf-token"].get<std::string_view>().get(HfToken);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the hf-token option."sv)
    }
    GraphRef.Params.hf_token = HfToken;
  }
  if (Doc.at_key("hf-repo").error() == simdjson::SUCCESS) {
    std::string_view HfRepo;
    auto Err = Doc["hf-repo"].get<std::string_view>().get(HfRepo);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the hf-repo option."sv)
    }
    GraphRef.Params.model.hf_repo = HfRepo;
  }
  if (Doc.at_key("hf-file").error() == simdjson::SUCCESS) {
    std::string_view HfFile;
    auto Err = Doc["hf-file"].get<std::string_view>().get(HfFile);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the hf-file option."sv)
    }
    GraphRef.Params.model.hf_file = HfFile;
  }
  if (Doc.at_key("prompt-file").error() == simdjson::SUCCESS) {
    std::string_view PromptFile;
    auto Err = Doc["prompt-file"].get<std::string_view>().get(PromptFile);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the prompt-file option."sv)
    }
    GraphRef.Params.prompt_file = PromptFile;
  }
  if (Doc.at_key("path-prompt-cache").error() == simdjson::SUCCESS) {
    std::string_view PathPromptCache;
    auto Err =
        Doc["path-prompt-cache"].get<std::string_view>().get(PathPromptCache);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the path-prompt-cache option."sv)
    }
    GraphRef.Params.path_prompt_cache = PathPromptCache;
  }
  if (Doc.at_key("input-prefix").error() == simdjson::SUCCESS) {
    std::string_view InputPrefix;
    auto Err = Doc["input-prefix"].get<std::string_view>().get(InputPrefix);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the input-prefix option."sv)
    }
    GraphRef.Params.input_prefix = InputPrefix;
  }
  if (Doc.at_key("input-suffix").error() == simdjson::SUCCESS) {
    std::string_view InputSuffix;
    auto Err = Doc["input-suffix"].get<std::string_view>().get(InputSuffix);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the input-suffix option."sv)
    }
    GraphRef.Params.input_suffix = InputSuffix;
  }
  if (Doc.at_key("lookup-cache-static").error() == simdjson::SUCCESS) {
    std::string_view LookupCacheStatic;
    auto Err = Doc["lookup-cache-static"].get<std::string_view>().get(
        LookupCacheStatic);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the lookup-cache-static option."sv)
    }
    GraphRef.Params.lookup_cache_static = LookupCacheStatic;
  }
  if (Doc.at_key("lookup-cache-dynamic").error() == simdjson::SUCCESS) {
    std::string_view LookupCacheDynamic;
    auto Err = Doc["lookup-cache-dynamic"].get<std::string_view>().get(
        LookupCacheDynamic);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the lookup-cache-dynamic option."sv)
    }
    GraphRef.Params.lookup_cache_dynamic = LookupCacheDynamic;
  }
  if (Doc.at_key("logits-file").error() == simdjson::SUCCESS) {
    std::string_view LogitsFile;
    auto Err = Doc["logits-file"].get<std::string_view>().get(LogitsFile);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the logits-file option."sv)
    }
    GraphRef.Params.logits_file = LogitsFile;
  }
  if (Doc.at_key("lora-init-without-apply").error() == simdjson::SUCCESS) {
    auto Err = Doc["lora-init-without-apply"].get<bool>().get(
        GraphRef.Params.lora_init_without_apply);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the lora-init-without-apply option."sv)
    }
  }
  if (Doc.at_key("verbosity").error() == simdjson::SUCCESS) {
    int64_t Verbosity;
    auto Err = Doc["verbosity"].get<int64_t>().get(Verbosity);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the verbosity option."sv)
    }
    GraphRef.Params.verbosity = static_cast<int32_t>(Verbosity);
  }
  if (Doc.at_key("control-vector-layer-start").error() == simdjson::SUCCESS) {
    int64_t ControlVectorLayerStart;
    auto Err = Doc["control-vector-layer-start"].get<int64_t>().get(
        ControlVectorLayerStart);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the control-vector-layer-start option."sv)
    }
    GraphRef.Params.control_vector_layer_start =
        static_cast<int32_t>(ControlVectorLayerStart);
  }
  if (Doc.at_key("control-vector-layer-end").error() == simdjson::SUCCESS) {
    int64_t ControlVectorLayerEnd;
    auto Err = Doc["control-vector-layer-end"].get<int64_t>().get(
        ControlVectorLayerEnd);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the control-vector-layer-end option."sv)
    }
    GraphRef.Params.control_vector_layer_end =
        static_cast<int32_t>(ControlVectorLayerEnd);
  }
  if (Doc.at_key("ppl-stride").error() == simdjson::SUCCESS) {
    int64_t PplStride;
    auto Err = Doc["ppl-stride"].get<int64_t>().get(PplStride);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the ppl-stride option."sv)
    }
    GraphRef.Params.ppl_stride = static_cast<int32_t>(PplStride);
  }
  if (Doc.at_key("ppl-output-type").error() == simdjson::SUCCESS) {
    int64_t PplOutputType;
    auto Err = Doc["ppl-output-type"].get<int64_t>().get(PplOutputType);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the ppl-output-type option."sv)
    }
    GraphRef.Params.ppl_output_type = static_cast<int32_t>(PplOutputType);
  }
  if (Doc.at_key("hellaswag").error() == simdjson::SUCCESS) {
    auto Err = Doc["hellaswag"].get<bool>().get(GraphRef.Params.hellaswag);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the hellaswag option."sv)
    }
  }
  if (Doc.at_key("hellaswag-tasks").error() == simdjson::SUCCESS) {
    uint64_t HellaswagTasks;
    auto Err = Doc["hellaswag-tasks"].get<uint64_t>().get(HellaswagTasks);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the hellaswag-tasks option."sv)
    }
    GraphRef.Params.hellaswag_tasks = HellaswagTasks;
  }
  if (Doc.at_key("winogrande").error() == simdjson::SUCCESS) {
    auto Err = Doc["winogrande"].get<bool>().get(GraphRef.Params.winogrande);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the winogrande option."sv)
    }
  }
  if (Doc.at_key("winogrande-tasks").error() == simdjson::SUCCESS) {
    uint64_t WinograndeTasks;
    auto Err = Doc["winogrande-tasks"].get<uint64_t>().get(WinograndeTasks);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the winogrande-tasks option."sv)
    }
    GraphRef.Params.winogrande_tasks = WinograndeTasks;
  }
  if (Doc.at_key("multiple-choice").error() == simdjson::SUCCESS) {
    auto Err =
        Doc["multiple-choice"].get<bool>().get(GraphRef.Params.multiple_choice);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the multiple-choice option."sv)
    }
  }
  if (Doc.at_key("multiple-choice-tasks").error() == simdjson::SUCCESS) {
    uint64_t MultipleChoiceTasks;
    auto Err =
        Doc["multiple-choice-tasks"].get<uint64_t>().get(MultipleChoiceTasks);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the multiple-choice-tasks option."sv)
    }
    GraphRef.Params.multiple_choice_tasks = MultipleChoiceTasks;
  }
  if (Doc.at_key("kl-divergence").error() == simdjson::SUCCESS) {
    auto Err =
        Doc["kl-divergence"].get<bool>().get(GraphRef.Params.kl_divergence);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the kl-divergence option."sv)
    }
  }
  if (Doc.at_key("usage").error() == simdjson::SUCCESS) {
    auto Err = Doc["usage"].get<bool>().get(GraphRef.Params.usage);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the usage option."sv)
    }
  }
  if (Doc.at_key("use-color").error() == simdjson::SUCCESS) {
    auto Err = Doc["use-color"].get<bool>().get(GraphRef.Params.use_color);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the use-color option."sv)
    }
  }
  if (Doc.at_key("special").error() == simdjson::SUCCESS) {
    auto Err = Doc["special"].get<bool>().get(GraphRef.Params.special);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the special option."sv)
    }
  }
  if (Doc.at_key("interactive").error() == simdjson::SUCCESS) {
    auto Err = Doc["interactive"].get<bool>().get(GraphRef.Params.interactive);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the interactive option."sv)
    }
  }
  if (Doc.at_key("interactive-first").error() == simdjson::SUCCESS) {
    auto Err = Doc["interactive-first"].get<bool>().get(
        GraphRef.Params.interactive_first);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the interactive-first option."sv)
    }
  }
  if (Doc.at_key("prompt-cache-all").error() == simdjson::SUCCESS) {
    auto Err = Doc["prompt-cache-all"].get<bool>().get(
        GraphRef.Params.prompt_cache_all);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the prompt-cache-all option."sv)
    }
  }
  if (Doc.at_key("prompt-cache-ro").error() == simdjson::SUCCESS) {
    auto Err =
        Doc["prompt-cache-ro"].get<bool>().get(GraphRef.Params.prompt_cache_ro);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the prompt-cache-ro option."sv)
    }
  }
  if (Doc.at_key("escape").error() == simdjson::SUCCESS) {
    auto Err = Doc["escape"].get<bool>().get(GraphRef.Params.escape);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the escape option."sv)
    }
  }
  if (Doc.at_key("multiline-input").error() == simdjson::SUCCESS) {
    auto Err =
        Doc["multiline-input"].get<bool>().get(GraphRef.Params.multiline_input);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the multiline-input option."sv)
    }
  }
  if (Doc.at_key("simple-io").error() == simdjson::SUCCESS) {
    auto Err = Doc["simple-io"].get<bool>().get(GraphRef.Params.simple_io);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the simple-io option."sv)
    }
  }
  if (Doc.at_key("cont-batching").error() == simdjson::SUCCESS) {
    auto Err =
        Doc["cont-batching"].get<bool>().get(GraphRef.Params.cont_batching);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the cont-batching option."sv)
    }
  }
  if (Doc.at_key("flash-attn").error() == simdjson::SUCCESS) {
    std::string_view FlashAttn;
    auto Err = Doc["flash-attn"].get<std::string_view>().get(FlashAttn);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the flash-attn option."sv)
    }
    if (FlashAttn == "on"sv || FlashAttn == "enabled"sv) {
      GraphRef.Params.flash_attn_type = LLAMA_FLASH_ATTN_TYPE_ENABLED;
    } else if (FlashAttn == "off"sv || FlashAttn == "disabled"sv) {
      GraphRef.Params.flash_attn_type = LLAMA_FLASH_ATTN_TYPE_DISABLED;
    } else if (FlashAttn == "auto"sv) {
      GraphRef.Params.flash_attn_type = LLAMA_FLASH_ATTN_TYPE_AUTO;
    } else {
      RET_ERROR(ErrNo::InvalidArgument,
                "The flash-attn option must be one of: on, off, auto."sv)
    }
  }
  if (Doc.at_key("no-perf").error() == simdjson::SUCCESS) {
    auto Err = Doc["no-perf"].get<bool>().get(GraphRef.Params.no_perf);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the no-perf option."sv)
    }
  }
  if (Doc.at_key("ctx-shift").error() == simdjson::SUCCESS) {
    auto Err = Doc["ctx-shift"].get<bool>().get(GraphRef.Params.ctx_shift);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the ctx-shift option."sv)
    }
  }
  if (Doc.at_key("input-prefix-bos").error() == simdjson::SUCCESS) {
    auto Err = Doc["input-prefix-bos"].get<bool>().get(
        GraphRef.Params.input_prefix_bos);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the input-prefix-bos option."sv)
    }
  }
  if (Doc.at_key("use-mlock").error() == simdjson::SUCCESS) {
    auto Err = Doc["use-mlock"].get<bool>().get(GraphRef.Params.use_mlock);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the use-mlock option."sv)
    }
  }
  if (Doc.at_key("use-mmap").error() == simdjson::SUCCESS) {
    auto Err = Doc["use-mmap"].get<bool>().get(GraphRef.Params.use_mmap);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the use-mmap option."sv)
    }
  }
  if (Doc.at_key("verbose-prompt").error() == simdjson::SUCCESS) {
    auto Err =
        Doc["verbose-prompt"].get<bool>().get(GraphRef.Params.verbose_prompt);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the verbose-prompt option."sv)
    }
  }
  if (Doc.at_key("display-prompt").error() == simdjson::SUCCESS) {
    auto Err =
        Doc["display-prompt"].get<bool>().get(GraphRef.Params.display_prompt);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the display-prompt option."sv)
    }
  }
  if (Doc.at_key("no-kv-offload").error() == simdjson::SUCCESS) {
    auto Err =
        Doc["no-kv-offload"].get<bool>().get(GraphRef.Params.no_kv_offload);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the no-kv-offload option."sv)
    }
  }
  if (Doc.at_key("warmup").error() == simdjson::SUCCESS) {
    auto Err = Doc["warmup"].get<bool>().get(GraphRef.Params.warmup);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the warmup option."sv)
    }
  }
  if (Doc.at_key("check-tensors").error() == simdjson::SUCCESS) {
    auto Err =
        Doc["check-tensors"].get<bool>().get(GraphRef.Params.check_tensors);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the check-tensors option."sv)
    }
  }
  if (Doc.at_key("cache-type-k").error() == simdjson::SUCCESS) {
    int64_t CacheTypeK;
    auto Err = Doc["cache-type-k"].get<int64_t>().get(CacheTypeK);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the cache-type-k option."sv)
    }
    GraphRef.Params.cache_type_k = static_cast<ggml_type>(CacheTypeK);
  }
  if (Doc.at_key("cache-type-v").error() == simdjson::SUCCESS) {
    int64_t CacheTypeV;
    auto Err = Doc["cache-type-v"].get<int64_t>().get(CacheTypeV);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the cache-type-v option."sv)
    }
    GraphRef.Params.cache_type_v = static_cast<ggml_type>(CacheTypeV);
  }
  if (Doc.at_key("embd-normalize").error() == simdjson::SUCCESS) {
    int64_t EmbdNormalize;
    auto Err = Doc["embd-normalize"].get<int64_t>().get(EmbdNormalize);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the embd-normalize option."sv)
    }
    GraphRef.Params.embd_normalize = static_cast<int32_t>(EmbdNormalize);
  }
  if (Doc.at_key("embd-out").error() == simdjson::SUCCESS) {
    std::string_view EmbdOut;
    auto Err = Doc["embd-out"].get<std::string_view>().get(EmbdOut);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the embd-out option."sv)
    }
    GraphRef.Params.embd_out = EmbdOut;
  }
  if (Doc.at_key("embd-sep").error() == simdjson::SUCCESS) {
    std::string_view EmbdSep;
    auto Err = Doc["embd-sep"].get<std::string_view>().get(EmbdSep);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the embd-sep option."sv)
    }
    GraphRef.Params.embd_sep = EmbdSep;
  }
  if (Doc.at_key("reranking").error() == simdjson::SUCCESS) {
    bool Reranking = false;
    auto Err = Doc["reranking"].get<bool>().get(Reranking);
    GraphRef.Params.embedding = true;
    GraphRef.Params.pooling_type = LLAMA_POOLING_TYPE_RANK;
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the reranking option."sv)
    }
  }
  if (Doc.at_key("port").error() == simdjson::SUCCESS) {
    int64_t Port;
    auto Err = Doc["port"].get<int64_t>().get(Port);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument, "Unable to retrieve the port option."sv)
    }
    GraphRef.Params.port = static_cast<int32_t>(Port);
  }
  if (Doc.at_key("timeout-read").error() == simdjson::SUCCESS) {
    int64_t TimeoutRead;
    auto Err = Doc["timeout-read"].get<int64_t>().get(TimeoutRead);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the timeout-read option."sv)
    }
    GraphRef.Params.timeout_read = static_cast<int32_t>(TimeoutRead);
  }
  if (Doc.at_key("timeout-write").error() == simdjson::SUCCESS) {
    int64_t TimeoutWrite;
    auto Err = Doc["timeout-write"].get<int64_t>().get(TimeoutWrite);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the timeout-write option."sv)
    }
    GraphRef.Params.timeout_write = static_cast<int32_t>(TimeoutWrite);
  }
  if (Doc.at_key("n-threads-http").error() == simdjson::SUCCESS) {
    int64_t NThreadsHttp;
    auto Err = Doc["n-threads-http"].get<int64_t>().get(NThreadsHttp);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the n-threads-http option."sv)
    }
    GraphRef.Params.n_threads_http = static_cast<int32_t>(NThreadsHttp);
  }
  if (Doc.at_key("n-cache-reuse").error() == simdjson::SUCCESS) {
    int64_t NCacheReuse;
    auto Err = Doc["n-cache-reuse"].get<int64_t>().get(NCacheReuse);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the n-cache-reuse option."sv)
    }
    GraphRef.Params.n_cache_reuse = static_cast<int32_t>(NCacheReuse);
  }
  if (Doc.at_key("hostname").error() == simdjson::SUCCESS) {
    std::string_view Hostname;
    auto Err = Doc["hostname"].get<std::string_view>().get(Hostname);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the hostname option."sv)
    }
    GraphRef.Params.hostname = Hostname;
  }
  if (Doc.at_key("public-path").error() == simdjson::SUCCESS) {
    std::string_view PublicPath;
    auto Err = Doc["public-path"].get<std::string_view>().get(PublicPath);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the public-path option."sv)
    }
    GraphRef.Params.public_path = PublicPath;
  }
  if (Doc.at_key("chat-template").error() == simdjson::SUCCESS) {
    std::string_view ChatTemplate;
    auto Err = Doc["chat-template"].get<std::string_view>().get(ChatTemplate);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the chat-template option."sv)
    }
    GraphRef.Params.chat_template = ChatTemplate;
  }
  if (Doc.at_key("enable-chat-template").error() == simdjson::SUCCESS) {
    auto Err = Doc["enable-chat-template"].get<bool>().get(
        GraphRef.Params.enable_chat_template);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the enable-chat-template option."sv)
    }
  }
  if (Doc.at_key("ssl-file-key").error() == simdjson::SUCCESS) {
    std::string_view SslFileKey;
    auto Err = Doc["ssl-file-key"].get<std::string_view>().get(SslFileKey);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the ssl-file-key option."sv)
    }
    GraphRef.Params.ssl_file_key = SslFileKey;
  }
  if (Doc.at_key("ssl-file-cert").error() == simdjson::SUCCESS) {
    std::string_view SslFileCert;
    auto Err = Doc["ssl-file-cert"].get<std::string_view>().get(SslFileCert);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the ssl-file-cert option."sv)
    }
    GraphRef.Params.ssl_file_cert = SslFileCert;
  }
  if (Doc.at_key("webui").error() == simdjson::SUCCESS) {
    auto Err = Doc["webui"].get<bool>().get(GraphRef.Params.webui);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the webui option."sv)
    }
  }
  if (Doc.at_key("endpoint-slots").error() == simdjson::SUCCESS) {
    int64_t EndpointSlots;
    auto Err = Doc["endpoint-slots"].get<int64_t>().get(EndpointSlots);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the endpoint-slots option."sv)
    }
    GraphRef.Params.endpoint_slots = static_cast<int32_t>(EndpointSlots);
  }
  if (Doc.at_key("endpoint-props").error() == simdjson::SUCCESS) {
    auto Err =
        Doc["endpoint-props"].get<bool>().get(GraphRef.Params.endpoint_props);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the endpoint-props option."sv)
    }
  }
  if (Doc.at_key("endpoint-metrics").error() == simdjson::SUCCESS) {
    auto Err = Doc["endpoint-metrics"].get<bool>().get(
        GraphRef.Params.endpoint_metrics);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the endpoint-metrics option."sv)
    }
  }
  if (Doc.at_key("log-json").error() == simdjson::SUCCESS) {
    auto Err = Doc["log-json"].get<bool>().get(GraphRef.Params.log_json);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the log-json option."sv)
    }
  }
  if (Doc.at_key("slot-save-path").error() == simdjson::SUCCESS) {
    std::string_view SlotSavePath;
    auto Err = Doc["slot-save-path"].get<std::string_view>().get(SlotSavePath);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the slot-save-path option."sv)
    }
    GraphRef.Params.slot_save_path = SlotSavePath;
  }
  if (Doc.at_key("slot-prompt-similarity").error() == simdjson::SUCCESS) {
    double SlotPromptSimilarity;
    auto Err =
        Doc["slot-prompt-similarity"].get<double>().get(SlotPromptSimilarity);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the slot-prompt-similarity option."sv)
    }
    GraphRef.Params.slot_prompt_similarity =
        static_cast<float>(SlotPromptSimilarity);
  }
  if (Doc.at_key("is-pp-shared").error() == simdjson::SUCCESS) {
    auto Err =
        Doc["is-pp-shared"].get<bool>().get(GraphRef.Params.is_pp_shared);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the is-pp-shared option."sv)
    }
  }
  if (Doc.at_key("n-pp").error() == simdjson::SUCCESS) {
    int64_t NPP;
    auto Err = Doc["n-pp"].get<int64_t>().get(NPP);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument, "Unable to retrieve the n-pp option."sv)
    }
  }
  if (Doc.at_key("n-tg").error() == simdjson::SUCCESS) {
    int64_t NTG;
    auto Err = Doc["n-tg"].get<int64_t>().get(NTG);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument, "Unable to retrieve the n-tg option."sv)
    }
  }
  if (Doc.at_key("n-pl").error() == simdjson::SUCCESS) {
    int64_t NPL;
    auto Err = Doc["n-pl"].get<int64_t>().get(NPL);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument, "Unable to retrieve the n-pl option."sv)
    }
  }
  if (Doc.at_key("context-files").error() == simdjson::SUCCESS) {
    std::string_view ContextFiles;
    auto Err = Doc["context-files"].get<std::string_view>().get(ContextFiles);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the context-files option."sv)
    }
  }
  if (Doc.at_key("chunk-size").error() == simdjson::SUCCESS) {
    int64_t ChunkSize;
    auto Err = Doc["chunk-size"].get<int64_t>().get(ChunkSize);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the chunk-size option."sv)
    }
  }
  if (Doc.at_key("chunk-separator").error() == simdjson::SUCCESS) {
    std::string_view ChunkSeparator;
    auto Err =
        Doc["chunk-separator"].get<std::string_view>().get(ChunkSeparator);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the chunk-separator option."sv)
    }
  }
  if (Doc.at_key("n-junk").error() == simdjson::SUCCESS) {
    int64_t NJunk;
    auto Err = Doc["n-junk"].get<int64_t>().get(NJunk);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the n-junk option."sv)
    }
  }
  if (Doc.at_key("i-pos").error() == simdjson::SUCCESS) {
    int64_t IPos;
    auto Err = Doc["i-pos"].get<int64_t>().get(IPos);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the i-pos option."sv)
    }
  }
  if (Doc.at_key("out-file").error() == simdjson::SUCCESS) {
    std::string_view OutFile;
    auto Err = Doc["out-file"].get<std::string_view>().get(OutFile);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the out-file option."sv)
    }
  }
  if (Doc.at_key("n-out-freq").error() == simdjson::SUCCESS) {
    int64_t NOutFreq;
    auto Err = Doc["n-out-freq"].get<int64_t>().get(NOutFreq);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the n-out-freq option."sv)
    }
  }
  if (Doc.at_key("n-save-freq").error() == simdjson::SUCCESS) {
    int64_t NSaveFreq;
    auto Err = Doc["n-save-freq"].get<int64_t>().get(NSaveFreq);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the n-save-freq option."sv)
    }
  }
  if (Doc.at_key("i-chunk").error() == simdjson::SUCCESS) {
    int64_t IChunk;
    auto Err = Doc["i-chunk"].get<int64_t>().get(IChunk);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the i-chunk option."sv)
    }
  }
  if (Doc.at_key("process-output").error() == simdjson::SUCCESS) {
    auto Err =
        Doc["process-output"].get<bool>().get(GraphRef.Params.process_output);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the process-output option."sv)
    }
  }
  if (Doc.at_key("compute-ppl").error() == simdjson::SUCCESS) {
    auto Err = Doc["compute-ppl"].get<bool>().get(GraphRef.Params.compute_ppl);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the compute-ppl option."sv)
    }
  }
  if (Doc.at_key("n-pca-batch").error() == simdjson::SUCCESS) {
    int64_t NPCABatch;
    auto Err = Doc["n-pca-batch"].get<int64_t>().get(NPCABatch);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the n-pca-batch option."sv)
    }
  }
  if (Doc.at_key("n-pca-iterations").error() == simdjson::SUCCESS) {
    int64_t NPCAIterations;
    auto Err = Doc["n-pca-iterations"].get<int64_t>().get(NPCAIterations);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the n-pca-iterations option."sv)
    }
  }
  if (Doc.at_key("cvector-dimre-method").error() == simdjson::SUCCESS) {
    std::string_view CVectorDimreMethod;
    auto Err = Doc["cvector-dimre-method"].get<std::string_view>().get(
        CVectorDimreMethod);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the cvector-dimre-method option."sv)
    }
  }
  if (Doc.at_key("cvector-outfile").error() == simdjson::SUCCESS) {
    std::string_view CVectorOutfile;
    auto Err =
        Doc["cvector-outfile"].get<std::string_view>().get(CVectorOutfile);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the cvector-outfile option."sv)
    }
  }
  if (Doc.at_key("cvector-positive-file").error() == simdjson::SUCCESS) {
    std::string_view CVectorPositiveFile;
    auto Err = Doc["cvector-positive-file"].get<std::string_view>().get(
        CVectorPositiveFile);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the cvector-positive-file option."sv)
    }
  }
  if (Doc.at_key("cvector-negative-file").error() == simdjson::SUCCESS) {
    std::string_view CVectorNegativeFile;
    auto Err = Doc["cvector-negative-file"].get<std::string_view>().get(
        CVectorNegativeFile);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the cvector-negative-file option."sv)
    }
  }
  if (Doc.at_key("spm-infill").error() == simdjson::SUCCESS) {
    auto Err = Doc["spm-infill"].get<bool>().get(GraphRef.Params.spm_infill);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the spm-infill option."sv)
    }
  }
  if (Doc.at_key("out-file").error() == simdjson::SUCCESS) {
    std::string_view Outfile;
    auto Err = Doc["out-file"].get<std::string_view>().get(Outfile);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the outfile option."sv)
    }
    GraphRef.Params.out_file = Outfile;
  }
  if (Doc.at_key("batched-bench-output-jsonl").error() == simdjson::SUCCESS) {
    auto Err = Doc["batched-bench-output-jsonl"].get<bool>().get(
        GraphRef.Params.batched_bench_output_jsonl);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the batched-bench-output-jsonl option."sv)
    }
  }

  // The tensor buffer overrides should terminated with empty pattern.
  if (!GraphRef.TensorBuftOverrides.empty()) {
    for (const std::string &Override : GraphRef.TensorBuftOverrides) {
      GraphRef.Params.tensor_buft_overrides.push_back(
          {Override.c_str(), ggml_backend_cpu_buffer_type()});
    }
    GraphRef.Params.tensor_buft_overrides.push_back({nullptr, nullptr});
  }

  if (GraphRef.TextToSpeech) {
    GraphRef.Params.sampling.top_k = 4;
    GraphRef.Params.sampling.samplers = {
        COMMON_SAMPLER_TYPE_TOP_K,
    };
  }
  // Check if the model parameters are updated.
  if (IsModelUpdated && PrevNGPULayers != GraphRef.Params.n_gpu_layers) {
    *IsModelUpdated = true;
  }

  // Check if the context parameters are updated.
  if (IsContextUpdated && PrevEmbedding != GraphRef.Params.embedding) {
    *IsContextUpdated = true;
  }

  // Check if the sampler parameters are updated.
  if (IsSamplerUpdated &&
      (PrevTemp != GraphRef.Params.sampling.temp ||
       PrevTopP != GraphRef.Params.sampling.top_p ||
       PrevRepeatPenalty != GraphRef.Params.sampling.penalty_repeat ||
       PrevPresencePenalty != GraphRef.Params.sampling.penalty_present ||
       PrevFrequencyPenalty != GraphRef.Params.sampling.penalty_freq ||
       PrevGrammar != GraphRef.Params.sampling.grammar ||
       PrevSeed != GraphRef.Params.sampling.seed)) {
    *IsSamplerUpdated = true;
  }

  return ErrNo::Success;
}

// <<<<<<<< Metadata related functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> Input related functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

const std::string_view Base64ImageTagPrefix = "<img src=\"data:image/"sv;
const std::string_view Base64ImageBytesPrefix = ";base64,"sv;
const std::string_view Base64ImageTagSuffix = "\">"sv;
const std::string_view VisionPromptImagePlaceholder = "<image>"sv;

// Get base64 image position if found in prompt.
std::optional<std::tuple<size_t, size_t, size_t>>
findBase64ImagePayload(std::string_view Prompt,
                       bool IsDebugLog = false) noexcept {
  // Find `<img src="data:image/`
  auto BeginTagPos = Prompt.find(Base64ImageTagPrefix);
  if (BeginTagPos == std::string::npos) {
    // Not print debug log here because not expect image must occur in every
    // prompt.
    return std::nullopt;
  }
  // Find `;base64,` (skip the image type part)
  auto PayloadPos = Prompt.find(Base64ImageBytesPrefix, BeginTagPos);
  if (PayloadPos == std::string::npos) {
    LOG_DEBUG(IsDebugLog, "base64: Cannot locate the payload."sv)
    return std::nullopt;
  }
  // Find `">`
  auto EndTagPos = Prompt.find(Base64ImageTagSuffix, PayloadPos);
  if (EndTagPos == std::string::npos) {
    LOG_DEBUG(IsDebugLog, "base64: image tag unclosed."sv)
    return std::nullopt;
  }
  return std::make_tuple(BeginTagPos, PayloadPos, EndTagPos);
}

// Extract base64 image payload and image type. Replace it with placeholder.
std::optional<std::pair<std::vector<uint8_t>, std::string>>
extractBase64ImagePayload(std::string &Prompt,
                          std::tuple<size_t, size_t, size_t> ImagePos,
                          const std::string_view Placeholder) noexcept {
  // Locate the payload and image type.
  size_t BeginTagPos = std::get<0>(ImagePos);
  size_t TypePos = std::get<0>(ImagePos) + Base64ImageTagPrefix.size();
  size_t PayloadPos = std::get<1>(ImagePos);
  size_t BeginBytePos = std::get<1>(ImagePos) + Base64ImageBytesPrefix.size();
  size_t EndTagPos = std::get<2>(ImagePos);
  std::string_view Payload =
      std::string_view(Prompt).substr(BeginBytePos, EndTagPos - BeginBytePos);
  std::string ImageType = Prompt.substr(TypePos, PayloadPos - TypePos);

  // Decode the base64 payload.
  auto RequiredBytes = base64::required_encode_size(Payload.size());
  std::vector<uint8_t> ImageBytes(RequiredBytes);
  try {
    base64::decode(Payload.begin(), Payload.end(), ImageBytes.begin());
  } catch (const base64_error &E) {
    RET_ERROR(std::make_pair(std::vector<uint8_t>(), ""),
              "base64: Error when calling base64::decode: {}"sv, E.what())
  }

  // Replace the base64 image with the placeholder.
  Prompt.replace(BeginTagPos,
                 EndTagPos - BeginTagPos + Base64ImageTagSuffix.size(),
                 Placeholder);
  return std::make_pair(ImageBytes, ImageType);
}

// TTS function to process the prompt text.
// clang-format off
const TTSSpeakerProfile TTSDefaultSpeakerProfile = {
  // Speaker profile from edwko/OuteTTS (en_female_1.json).
  "<|text_start|>uhm<|text_sep|>now<|text_sep|>being<|text_sep|>the<|text_sep|>one<|text_sep|>to<|text_sep|>say<|text_sep|>i<|text_sep|>know<|text_sep|>the<|text_sep|>worst<|text_sep|>of<|text_sep|>you<|text_sep|>and<|text_sep|>ive<|text_sep|>been<|text_sep|>directly<|text_sep|>affected<|text_sep|>by<|text_sep|>people<|text_sep|>like<|text_sep|>you<|text_sep|>but<|text_sep|>its<|text_sep|>a<|text_sep|>clean<|text_sep|>slate<|text_sep|>with<|text_sep|>me<|text_sep|>buddy<|text_sep|>you<|text_sep|>know<|text_sep|>like<|text_sep|>thats<|text_sep|>really<|text_sep|>powerful<|text_sep|>in<|text_sep|>and<|text_sep|>of<|text_sep|>itself<|text_sep|>",
  "<|audio_start|>\nuhm<|t_0.36|><|code_start|><|447|><|223|><|967|><|301|><|965|><|827|><|393|><|908|><|764|><|1167|><|711|><|1222|><|324|><|1318|><|806|><|498|><|1198|><|1127|><|1178|><|916|><|1234|><|1411|><|1428|><|706|><|427|><|1605|><|1578|><|code_end|>\nnow<|t_0.36|><|code_start|><|1049|><|327|><|385|><|1070|><|732|><|1480|><|450|><|1025|><|1469|><|174|><|1013|><|1710|><|1674|><|775|><|771|><|251|><|778|><|1400|><|897|><|1487|><|366|><|441|><|1000|><|393|><|271|><|1000|><|768|><|code_end|>\nbeing<|t_0.27|><|code_start|><|926|><|406|><|1457|><|437|><|1231|><|672|><|1785|><|521|><|1179|><|1559|><|198|><|1086|><|733|><|122|><|1344|><|845|><|348|><|1389|><|470|><|1773|><|code_end|>\nthe<|t_0.08|><|code_start|><|1775|><|562|><|768|><|1222|><|768|><|963|><|code_end|>\none<|t_0.21|><|code_start|><|1757|><|744|><|144|><|1610|><|655|><|616|><|1317|><|225|><|1325|><|913|><|1342|><|992|><|1018|><|80|><|1777|><|883|><|code_end|>\nto<|t_0.08|><|code_start|><|487|><|1363|><|1682|><|1426|><|655|><|1483|><|code_end|>\nsay<|t_0.27|><|code_start|><|1644|><|1804|><|731|><|273|><|1592|><|731|><|1523|><|1404|><|984|><|1207|><|430|><|1132|><|1123|><|768|><|1116|><|829|><|1082|><|1095|><|440|><|1162|><|code_end|>\ni<|t_0.33|><|code_start|><|1330|><|335|><|1162|><|1155|><|308|><|1162|><|1150|><|1481|><|612|><|674|><|712|><|1745|><|1188|><|1787|><|1135|><|1275|><|1237|><|1143|><|408|><|1063|><|393|><|927|><|1298|><|132|><|1686|><|code_end|>\nknow<|t_0.27|><|code_start|><|983|><|1677|><|586|><|1528|><|1435|><|835|><|1396|><|706|><|987|><|22|><|1172|><|218|><|1404|><|1001|><|521|><|1389|><|775|><|1416|><|877|><|120|><|code_end|>\nthe<|t_0.16|><|code_start|><|916|><|1756|><|513|><|1245|><|1392|><|89|><|1266|><|12|><|1045|><|1075|><|904|><|35|><|code_end|>\nworst<|t_0.32|><|code_start|><|1607|><|174|><|1231|><|144|><|932|><|490|><|771|><|1504|><|798|><|674|><|364|><|80|><|1314|><|1636|><|449|><|1704|><|713|><|1795|><|968|><|1527|><|1302|><|1529|><|1176|><|795|><|code_end|>\nof<|t_0.12|><|code_start|><|1193|><|1205|><|390|><|1128|><|1091|><|883|><|322|><|377|><|1070|><|code_end|>\nyou<|t_0.17|><|code_start|><|1016|><|1332|><|926|><|281|><|927|><|1368|><|1687|><|918|><|67|><|1638|><|1317|><|1265|><|1770|><|code_end|>\nand<|t_0.28|><|code_start|><|1129|><|1633|><|1373|><|1207|><|405|><|879|><|1030|><|1253|><|1071|><|612|><|724|><|1770|><|665|><|1046|><|1351|><|1450|><|1541|><|1384|><|111|><|1477|><|284|><|code_end|>\nive<|t_0.35|><|code_start|><|674|><|266|><|89|><|1333|><|1183|><|1526|><|1143|><|883|><|1135|><|732|><|827|><|1119|><|594|><|1261|><|1024|><|1347|><|92|><|1392|><|825|><|1710|><|1289|><|1598|><|1070|><|1525|><|1442|><|555|><|code_end|>\nbeen<|t_0.17|><|code_start|><|1461|><|194|><|337|><|1128|><|188|><|892|><|848|><|1280|><|959|><|754|><|231|><|649|><|1304|><|code_end|>\ndirectly<|t_0.87|><|code_start|><|1030|><|353|><|570|><|1331|><|470|><|1832|><|1362|><|1809|><|1383|><|101|><|325|><|1557|><|1242|><|1512|><|180|><|227|><|1242|><|643|><|209|><|464|><|171|><|1219|><|174|><|1723|><|734|><|118|><|1269|><|643|><|209|><|187|><|612|><|1231|><|68|><|567|><|1242|><|505|><|319|><|1268|><|794|><|678|><|40|><|1286|><|470|><|1454|><|199|><|965|><|188|><|300|><|1234|><|1125|><|794|><|1289|><|1224|><|257|><|469|><|1121|><|101|><|823|><|1769|><|1683|><|95|><|255|><|59|><|67|><|832|><|code_end|>\naffected<|t_0.44|><|code_start|><|510|><|873|><|787|><|1228|><|771|><|1428|><|501|><|751|><|696|><|258|><|845|><|1818|><|1112|><|498|><|1111|><|985|><|1073|><|832|><|1427|><|168|><|163|><|447|><|119|><|567|><|1626|><|1820|><|903|><|635|><|1060|><|10|><|1632|><|35|><|1635|><|code_end|>\nby<|t_0.19|><|code_start|><|144|><|144|><|460|><|185|><|1112|><|1044|><|498|><|1192|><|656|><|1333|><|1001|><|1186|><|1186|><|454|><|code_end|>\npeople<|t_0.48|><|code_start|><|1260|><|747|><|351|><|526|><|612|><|1151|><|1262|><|1791|><|344|><|1752|><|1547|><|930|><|1302|><|1703|><|1289|><|92|><|1407|><|1482|><|508|><|1431|><|355|><|1696|><|337|><|199|><|1157|><|223|><|464|><|568|><|845|><|411|><|826|><|718|><|1786|><|545|><|712|><|580|><|code_end|>\nlike<|t_0.32|><|code_start|><|630|><|532|><|526|><|607|><|526|><|839|><|1305|><|660|><|459|><|339|><|717|><|1178|><|1148|><|687|><|149|><|1390|><|229|><|199|><|513|><|712|><|1451|><|731|><|582|><|1551|><|code_end|>\nyou<|t_0.21|><|code_start|><|1389|><|954|><|1781|><|1047|><|1236|><|930|><|809|><|1621|><|1268|><|384|><|242|><|587|><|869|><|816|><|1680|><|405|><|code_end|>\nbut<|t_0.59|><|code_start|><|1089|><|1590|><|908|><|80|><|594|><|1046|><|1706|><|1025|><|1150|><|405|><|548|><|893|><|1285|><|464|><|301|><|939|><|643|><|23|><|285|><|161|><|209|><|453|><|72|><|167|><|417|><|244|><|151|><|643|><|391|><|199|><|651|><|1023|><|337|><|1010|><|54|><|331|><|1167|><|756|><|388|><|934|><|1060|><|18|><|1624|><|1060|><|code_end|>\nits<|t_0.16|><|code_start|><|1102|><|183|><|1199|><|1258|><|1285|><|35|><|659|><|180|><|426|><|1587|><|1733|><|942|><|code_end|>\na<|t_0.04|><|code_start|><|791|><|1012|><|818|><|code_end|>\nclean<|t_0.61|><|code_start|><|1819|><|976|><|163|><|447|><|316|><|223|><|763|><|457|><|1208|><|1808|><|1697|><|1162|><|1660|><|1833|><|1054|><|1734|><|1121|><|1309|><|1643|><|924|><|1677|><|1548|><|869|><|1268|><|223|><|674|><|111|><|792|><|1670|><|912|><|174|><|1554|><|90|><|80|><|1563|><|1621|><|1698|><|1544|><|992|><|988|><|175|><|793|><|1661|><|1026|><|80|><|1761|><|code_end|>\nslate<|t_0.40|><|code_start|><|1802|><|322|><|1689|><|1577|><|1302|><|1552|><|1529|><|1722|><|1580|><|582|><|1642|><|1529|><|1020|><|582|><|1538|><|970|><|437|><|1141|><|1477|><|988|><|335|><|1611|><|922|><|1558|><|1120|><|1189|><|423|><|188|><|171|><|562|><|code_end|>\nwith<|t_0.15|><|code_start|><|963|><|1347|><|1274|><|747|><|1230|><|712|><|1408|><|1290|><|957|><|1279|><|258|><|code_end|>\nme<|t_0.09|><|code_start|><|638|><|1058|><|174|><|1452|><|1038|><|894|><|1571|><|code_end|>\nbuddy<|t_0.32|><|code_start|><|1003|><|130|><|1341|><|938|><|40|><|804|><|167|><|89|><|1456|><|1189|><|1155|><|1171|><|1434|><|1077|><|1029|><|1455|><|1622|><|1037|><|163|><|1411|><|1165|><|1463|><|837|><|1202|><|code_end|>\nyou<|t_0.36|><|code_start|><|1354|><|1165|><|615|><|1588|><|1192|><|1445|><|1033|><|982|><|401|><|1079|><|684|><|1570|><|266|><|31|><|420|><|163|><|893|><|845|><|905|><|1827|><|1804|><|153|><|627|><|243|><|1179|><|298|><|1147|><|code_end|>\nknow<|t_0.19|><|code_start|><|163|><|1542|><|1366|><|698|><|1753|><|206|><|916|><|1499|><|245|><|665|><|600|><|894|><|587|><|1741|><|code_end|>\nlike<|t_0.24|><|code_start|><|1106|><|1280|><|1062|><|1304|><|945|><|809|><|598|><|104|><|1001|><|822|><|965|><|189|><|693|><|1810|><|1293|><|199|><|1277|><|44|><|code_end|>\nthats<|t_0.24|><|code_start|><|121|><|1789|><|1443|><|370|><|1154|><|393|><|1178|><|1200|><|1264|><|424|><|1391|><|381|><|978|><|1346|><|704|><|1808|><|1579|><|1492|><|code_end|>\nreally<|t_0.56|><|code_start|><|1177|><|1761|><|1723|><|1360|><|1413|><|830|><|551|><|193|><|59|><|332|><|598|><|734|><|1684|><|1802|><|60|><|1590|><|353|><|89|><|1636|><|1396|><|893|><|143|><|455|><|1501|><|435|><|1082|><|621|><|1593|><|677|><|474|><|971|><|1513|><|913|><|828|><|1381|><|1148|><|1798|><|1186|><|1443|><|38|><|335|><|883|><|code_end|>\npowerful<|t_0.63|><|code_start|><|1773|><|458|><|1070|><|964|><|826|><|1220|><|1012|><|1738|><|1125|><|669|><|490|><|1169|><|922|><|958|><|1204|><|489|><|1001|><|886|><|1045|><|675|><|1471|><|1652|><|732|><|698|><|1124|><|480|><|897|><|1484|><|1028|><|35|><|594|><|1465|><|505|><|1669|><|436|><|851|><|1288|><|31|><|1501|><|1187|><|394|><|909|><|1541|><|1793|><|1720|><|922|><|840|><|code_end|>\nin<|t_0.16|><|code_start|><|1317|><|523|><|630|><|1343|><|1187|><|719|><|907|><|636|><|111|><|1524|><|188|><|1382|><|code_end|>\nand<|t_0.13|><|code_start|><|1074|><|922|><|1280|><|1496|><|1050|><|832|><|133|><|1435|><|1049|><|1774|><|code_end|>\nof<|t_0.12|><|code_start|><|960|><|1052|><|1192|><|1303|><|1112|><|970|><|417|><|60|><|1155|><|code_end|>\nitself<|t_0.47|><|code_start|><|1682|><|1209|><|1410|><|513|><|1222|><|861|><|167|><|406|><|1551|><|582|><|634|><|1529|><|786|><|1363|><|1578|><|1739|><|873|><|424|><|1041|><|1328|><|955|><|1110|><|1490|><|1424|><|1199|><|988|><|1162|><|1133|><|1193|><|978|><|470|><|832|><|963|><|1251|><|733|><|code_end|>",
};
// clang-format on

const std::map<int, std::string> Ones = {
    {0, "zero"},     {1, "one"},        {2, "two"},       {3, "three"},
    {4, "four"},     {5, "five"},       {6, "six"},       {7, "seven"},
    {8, "eight"},    {9, "nine"},       {10, "ten"},      {11, "eleven"},
    {12, "twelve"},  {13, "thirteen"},  {14, "fourteen"}, {15, "fifteen"},
    {16, "sixteen"}, {17, "seventeen"}, {18, "eighteen"}, {19, "nineteen"}};

const std::map<int, std::string> Tens = {
    {2, "twenty"}, {3, "thirty"},  {4, "forty"},  {5, "fifty"},
    {6, "sixty"},  {7, "seventy"}, {8, "eighty"}, {9, "ninety"}};

// Convert a number less than 1000 to words
std::string convertLessThanThousand(int Num) {
  std::string Result;

  if (Num >= 100) {
    Result += Ones.at(Num / 100) + " hundred ";
    Num %= 100;
  }

  if (Num >= 20) {
    Result += Tens.at(Num / 10);
    if (Num % 10 > 0) {
      Result += "-" + Ones.at(Num % 10);
    }
  } else if (Num > 0) {
    Result += Ones.at(Num);
  }

  return Result;
}

std::string numberToWords(const std::string &NumberStr) {
  try {
    size_t DecimalPos = NumberStr.find('.');
    std::string IntegerPart = NumberStr.substr(0, DecimalPos);

    int IntNumber = std::stoi(IntegerPart);
    std::string Result;

    if (IntNumber == 0) {
      Result = "zero";
    } else {
      if (IntNumber >= 1000000000) {
        int Billions = IntNumber / 1000000000;
        Result += convertLessThanThousand(Billions) + " billion ";
        IntNumber %= 1000000000;
      }

      if (IntNumber >= 1000000) {
        int Millions = IntNumber / 1000000;
        Result += convertLessThanThousand(Millions) + " million ";
        IntNumber %= 1000000;
      }

      if (IntNumber >= 1000) {
        int Thousands = IntNumber / 1000;
        Result += convertLessThanThousand(Thousands) + " thousand ";
        IntNumber %= 1000;
      }

      if (IntNumber > 0) {
        Result += convertLessThanThousand(IntNumber);
      }
    }

    // Handle decimal part
    if (DecimalPos != std::string::npos) {
      Result += " point";
      std::string DecimalPart = NumberStr.substr(DecimalPos + 1);
      for (char Digit : DecimalPart) {
        Result += " " + Ones.at(Digit - '0');
      }
    }

    return Result;
  } catch (const std::exception &) {
    // Skip if fails
    return " ";
  }
}

std::string replaceNumbersWithWords(const std::string &InputText) {
  std::regex NumberPattern(R"(\d+(\.\d+)?)");
  std::string Result;
  auto It =
      std::sregex_iterator(InputText.begin(), InputText.end(), NumberPattern);
  auto End = std::sregex_iterator();

  size_t LastPos = 0;
  for (std::sregex_iterator I = It; I != End; ++I) {
    const std::smatch &Match = *I;
    Result.append(InputText, LastPos, Match.position() - LastPos);
    Result.append(numberToWords(Match.str()));
    LastPos = Match.position() + Match.length();
  }
  Result.append(InputText, LastPos);

  return Result;
}

// Based on:
// https://github.com/edwko/OuteTTS/blob/a613e79c489d8256dd657ea9168d78de75895d82/outetts/version/v1/prompt_processor.py#L39
// https://github.com/ggerganov/llama.cpp/blob/b4488/examples/tts/tts.cpp#L374
std::string processTTSPromptText(const std::string &Text) {
  std::string ProcessedText = replaceNumbersWithWords(Text);

  std::transform(
      ProcessedText.begin(), ProcessedText.end(), ProcessedText.begin(),
      [](unsigned char C) { return static_cast<char>(::tolower(C)); });

  std::regex SpecialChars(R"([-_/,\.\\])");
  ProcessedText = std::regex_replace(ProcessedText, SpecialChars, " ");

  std::regex NonAlpha(R"([^a-z\s])");
  ProcessedText = std::regex_replace(ProcessedText, NonAlpha, "");

  std::regex MultipleSpaces(R"(\s+)");
  ProcessedText = std::regex_replace(ProcessedText, MultipleSpaces, " ");

  ProcessedText =
      std::regex_replace(ProcessedText, std::regex(R"(^\s+|\s+$)"), "");

  ProcessedText =
      std::regex_replace(ProcessedText, std::regex(R"(\s)"), "<|text_sep|>");

  return ProcessedText;
}

std::optional<TTSSpeakerProfile>
getSpeakerProfileFromFile(const std::string &FilePath) {
  std::ifstream JsonFile(FilePath);
  if (!JsonFile.is_open()) {
    return std::nullopt;
  }
  nlohmann::json JsonData;
  JsonFile >> JsonData;
  JsonFile.close();

  // Initialize the outputs
  std::string AudioOutputText = "<|audio_start|>\n";
  std::string TextOutput = "<|text_start|>";

  // Iterate through each word in the JSON data
  for (const auto &WordData : JsonData["words"]) {
    std::string Word = WordData["word"];
    double Duration = WordData["duration"];
    std::vector<int> Codes = WordData["codes"];

    // Create the audio output entry
    std::ostringstream WordEntry;
    WordEntry << Word << "<|t_" << std::fixed << std::setprecision(2)
              << Duration << "|><|code_start|>";
    for (const auto &Code : Codes) {
      WordEntry << "<|" << Code << "|>";
    }
    WordEntry << "<|code_end|>\n";
    AudioOutputText += WordEntry.str();

    // Create the text output entry
    TextOutput += Word + "<|text_sep|>";
  }

  return TTSSpeakerProfile{TextOutput, AudioOutputText};
}

std::vector<llama_token> processTTSPrompt(Graph &GraphRef,
                                          std::string &Prompt) noexcept {
  // Use the custom speaker profile if available.
  TTSSpeakerProfile SpeakerProfile = TTSDefaultSpeakerProfile;
  if (!GraphRef.TTSSpeakerFilePath.empty()) {
    std::optional<TTSSpeakerProfile> SpeakerProfileOpt =
        getSpeakerProfileFromFile(GraphRef.TTSSpeakerFilePath);
    if (SpeakerProfileOpt.has_value()) {
      SpeakerProfile = *SpeakerProfileOpt;
    } else {
      RET_ERROR(
          {},
          "processTTSPrompt: Failed to load speaker profile from file: {}"sv,
          GraphRef.TTSSpeakerFilePath);
    }
  }
  std::string ProcessedPrompt = processTTSPromptText(Prompt);
  std::vector<llama_token> Result, TmpTokens;
  Result = common_tokenize(GraphRef.LlamaContext.get(), "<|im_start|>\n",
                           /* add_special */ true,
                           /* parse_special */ true);
  TmpTokens = common_tokenize(GraphRef.LlamaContext.get(), SpeakerProfile.Text,
                              /* add_special */ false,
                              /* parse_special */ true);
  Result.insert(Result.end(), TmpTokens.begin(), TmpTokens.end());
  TmpTokens = common_tokenize(GraphRef.LlamaContext.get(), ProcessedPrompt,
                              /* add_special */ false,
                              /* parse_special */ true);
  Result.insert(Result.end(), TmpTokens.begin(), TmpTokens.end());
  TmpTokens = common_tokenize(GraphRef.LlamaContext.get(), "<|text_end|>\n",
                              /* add_special */ false,
                              /* parse_special */ true);
  Result.insert(Result.end(), TmpTokens.begin(), TmpTokens.end());
  TmpTokens = common_tokenize(GraphRef.LlamaContext.get(), SpeakerProfile.Data,
                              /* add_special */ false,
                              /* parse_special */ true);
  Result.insert(Result.end(), TmpTokens.begin(), TmpTokens.end());

  return Result;
}

// <<<<<<<< Input related functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> Output related functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

// Generate output metadata.
std::string buildOutputMetadata(Context &CxtRef) noexcept {
  return fmt::format(R"({{"input_tokens": {}, )"
                     R"("output_tokens": {}, )"
                     R"("llama_build_number": {}, )"
                     R"("llama_commit": "{}"}})"sv,
                     CxtRef.LlamaNInputs, CxtRef.LlamaOutputTokens.size(),
                     LLAMA_BUILD_NUMBER, LLAMA_COMMIT);
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

// <<<<<<<< Output related functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> Compute related functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

// Helper to init a llama batch.
struct llama_batch allocBatch(int64_t NTokens, int64_t Embd = 0,
                              int32_t NSeqMax = 1) noexcept {
  struct llama_batch Batch = llama_batch_init(
      /* n_tokens_alloc */ static_cast<int32_t>(NTokens),
      /* embd */ static_cast<int32_t>(Embd),
      /* n_seq_max */ static_cast<int32_t>(NSeqMax));
  std::fill(Batch.n_seq_id, Batch.n_seq_id + NTokens,
            static_cast<int32_t>(NSeqMax));
  for (int64_t I = 0; I < NTokens; I++) {
    std::fill(Batch.seq_id[I], Batch.seq_id[I] + NSeqMax, 0);
  }
  std::fill(Batch.logits, Batch.logits + NTokens, false);
  return Batch;
}

// Fill tokens (smaller than batch size) into a batch with position data.
void fillBatch(Span<const llama_token> Tokens, Graph &GraphRef,
               llama_batch &Batch, int &NPos, bool IsLogit = false) {
  assuming(GraphRef.Params.n_batch >= static_cast<int64_t>(Tokens.size()));
  assuming(Batch.token != nullptr);
  assuming(Batch.pos != nullptr);
  assuming(Batch.logits != nullptr);
  // Fill the batch with pos information.
  Batch.n_tokens = static_cast<int32_t>(Tokens.size());
  for (uint32_t I = 0; I < Tokens.size(); I++) {
    Batch.token[I] = Tokens[I];
    Batch.pos[I] = NPos + I;
    Batch.logits[I] = false;
  }

  // Logits of sampling or end of inputs.
  if (IsLogit) {
    Batch.logits[Tokens.size() - 1] = true;
  }

  // Move the position.
  NPos += static_cast<int>(Tokens.size());
}

// Evaluate tokens. Construct the tokens into batch and decode.
ErrNo evaluateTokens(Span<const llama_token> Tokens, Graph &GraphRef,
                     llama_batch &Batch, int &NPos,
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

  // Loop for decode batch. Split tokens into batch size length.
  for (int I = 0; I < static_cast<int>(Tokens.size());
       I += static_cast<int>(GraphRef.Params.n_batch)) {
    int NEval = static_cast<int>(Tokens.size()) - I;
    if (NEval > static_cast<int>(GraphRef.Params.n_batch)) {
      NEval = static_cast<int>(GraphRef.Params.n_batch);
    }

    // Fill the batch with pos information.
    fillBatch(Span<const llama_token>(Tokens.begin() + I, NEval), GraphRef,
              Batch, NPos,
              IsLogits && I + NEval >= static_cast<int>(Tokens.size()));

    // Decode the batch.
    auto Status = llama_decode(GraphRef.LlamaContext.get(), Batch);
    if (Status == 1) {
      RET_ERROR(
          ErrNo::RuntimeError,
          "evaluateTokens: failed to llama_decode: try reducing the size of the batch "sv
          "or increasing the size of context."sv)
    }
    if (Status < 0) {
      RET_ERROR(
          ErrNo::RuntimeError,
          "evaluateTokens: failed to llama_decode: internal fatal error. Please open "sv
          "an issue on GitHub."sv)
    }
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

// Evaluate the input tokens. Clean all inputs if succeeded.
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
                     GraphRef, CxtRef.LlamaBatch, CxtRef.NPos, true);
  if (ReturnCode != ErrNo::Success) {
    RET_ERROR(ReturnCode, "{}: failed to evaluate input tokens."sv, LogPrefix)
  }

  return ErrNo::Success;
}

// Sample and get the output token.
ErrNo sampleOutput(Graph &GraphRef, Context &CxtRef,
                   bool IsSingleTokenMode = false) noexcept {
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
  return evaluateTokens(Span<const llama_token>(&Id, 1), GraphRef,
                        CxtRef.OutputBatch, CxtRef.NPos, true);
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

  // Check if the input is too long.
  if (static_cast<int64_t>(CxtRef.LlamaInputs.size()) >
      GraphRef.Params.n_batch) {
    RET_ERROR(
        ErrNo::PromptTooLong,
        "getEmbedding: the prompt is too long. Your input has {} tokens exceeds batch "sv
        "size {}. Please reduce the input size or increase your batch-size."sv,
        CxtRef.LlamaInputs.size(), GraphRef.Params.n_batch)
  }

  // Evaluate the input tokens.
  auto ReturnCode = evaluateInput(GraphRef, CxtRef, "getEmbedding"sv);
  if (ReturnCode != ErrNo::Success) {
    return ReturnCode;
  }

  // Main prediction loop.
  const int32_t NEmbd = llama_model_n_embd(GraphRef.LlamaModel.get());
  std::vector<float> Embeddings(NEmbd);

  for (int I = 0; I < CxtRef.LlamaBatch.n_tokens; I++) {
    if (!CxtRef.LlamaBatch.logits[I]) {
      continue;
    }

    // Try to get sequence embeddings.
    auto *Embd = llama_get_embeddings_seq(GraphRef.LlamaContext.get(),
                                          CxtRef.LlamaBatch.seq_id[I][0]);
    if (Embd == nullptr) {
      Embd = llama_get_embeddings_ith(GraphRef.LlamaContext.get(), I);
      if (Embd == nullptr) {
        LOG_ERROR("getEmbedding: failed to get embeddings for token {}"sv, I);
        continue;
      }
    }

    // Normalize the embeddings.
    common_embd_normalize(Embd, Embeddings.data(), NEmbd,
                          static_cast<int32_t>(CxtRef.Conf.EmbdNormalize));
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

// TTS related functions.
void fillHannWindow(int Length, bool Periodic, float *Output) {
  int Offset = -1;
  float Pi = static_cast<float>(std::acos(-1));
  if (Periodic) {
    Offset = 0;
  }
  for (int I = 0; I < Length; I++) {
    double Value =
        0.5 *
        (1.0 - cosf(static_cast<float>((2.0 * Pi * I) / (Length + Offset))));
    Output[I] = static_cast<float>(Value);
  }
}

void twiddle(float *Real, float *Imag, int K, int N) {
  float Pi = static_cast<float>(std::acos(-1));
  float Angle = 2 * Pi * K / N;
  *Real = cos(Angle);
  *Imag = sin(Angle);
}

void irfft(int N, const float *InpCplx, float *OutReal) {
  int NN = N / 2 + 1;

  std::vector<float> RealInput(NN);
  std::vector<float> ImagInput(NN);
  for (int I = 0; I < NN; ++I) {
    RealInput[I] = InpCplx[2 * I];
    ImagInput[I] = InpCplx[2 * I + 1];
  }

  std::vector<float> RealOutput(N);
  std::vector<float> ImagOutput(N);

  for (int K = 0; K < N; ++K) {
    RealOutput[K] = 0.0f;
    ImagOutput[K] = 0.0f;
    for (int M = 0; M < NN; ++M) {
      float TwiddleReal;
      float TwiddleImag;

      twiddle(&TwiddleReal, &TwiddleImag, K * M, N);

      RealOutput[K] += RealInput[M] * TwiddleReal - ImagInput[M] * TwiddleImag;
      ImagOutput[K] += RealInput[M] * TwiddleImag + ImagInput[M] * TwiddleReal;
    }
  }

  for (int I = 0; I < N; ++I) {
    OutReal[I] = RealOutput[I] / NN;
  }
}

void fold(const std::vector<float> &Data, int64_t NOut, int64_t NWin,
          int64_t NHop, int64_t NPad, std::vector<float> &Output) {
  int64_t OutputHeight = NOut;
  int64_t KernelW = NWin;
  int64_t StrideW = NHop;
  int64_t Width = NOut;

  Output.resize(Width, 0.0f);

  int64_t ColIdx = 0;
  for (int64_t WCol = 0; WCol < Width; ++WCol) {
    int64_t Start = WCol * StrideW - NPad;
    int64_t End = Start + KernelW;

    for (int64_t WIm = Start; WIm < End; ++WIm) {
      if (WIm >= 0 && WIm < OutputHeight &&
          ColIdx < static_cast<int64_t>(Data.size())) {
        Output[WIm] += Data[ColIdx];
      }
      ColIdx++;
    }
  }

  Output.resize(NOut - 2 * NPad);
}

std::vector<float> embdToAudio(const float *Embd, const int NCodes,
                               const int NEmbd, const int NThread) {
  const int NFft = 1280;
  const int NHop = 320;
  const int NWin = 1280;
  const int NPad = (NWin - NHop) / 2;
  const int NOut = (NCodes - 1) * NHop + NWin;

  std::vector<float> Hann(NFft);

  fillHannWindow(static_cast<int>(Hann.size()), true, Hann.data());

  int NSpec = NEmbd * NCodes;

  std::vector<float> E(NSpec);
  std::vector<float> S(NSpec);
  std::vector<float> ST(NSpec);

  for (int L = 0; L < NCodes; ++L) {
    for (int K = 0; K < NEmbd; ++K) {
      E[K * NCodes + L] = Embd[L * NEmbd + K];
    }
  }

  for (int K = 0; K < NEmbd / 2; ++K) {
    for (int L = 0; L < NCodes; ++L) {
      float Mag = E[(K)*NCodes + L];
      float Phi = E[(K + NEmbd / 2) * NCodes + L];

      Mag = exp(Mag);

      if (Mag > 1e2) {
        Mag = 1e2;
      }
      S[2 * (K * NCodes + L) + 0] = Mag * cosf(Phi);
      S[2 * (K * NCodes + L) + 1] = Mag * sinf(Phi);
    }
  }

  for (int L = 0; L < NCodes; ++L) {
    for (int K = 0; K < NEmbd / 2; ++K) {
      ST[L * NEmbd + 2 * K + 0] = S[2 * (K * NCodes + L) + 0];
      ST[L * NEmbd + 2 * K + 1] = S[2 * (K * NCodes + L) + 1];
    }
  }

  std::vector<float> Res(NCodes * NFft);
  std::vector<float> Hann2(NCodes * NFft);

  std::vector<std::thread> Workers(NThread);
  for (int I = 0; I < NThread; ++I) {
    Workers[I] = std::thread([&, I]() {
      for (int L = I; L < NCodes; L += NThread) {
        irfft(NFft, ST.data() + L * NEmbd, Res.data() + L * NFft);
        for (int J = 0; J < NFft; ++J) {
          Res[L * NFft + J] *= Hann[J];
          Hann2[L * NFft + J] = Hann[J] * Hann[J];
        }
      }
    });
  }
  for (int I = 0; I < NThread; ++I) {
    Workers[I].join();
  }

  std::vector<float> Audio;
  std::vector<float> Env;

  fold(Res, NOut, NWin, NHop, NPad, Audio);
  fold(Hann2, NOut, NWin, NHop, NPad, Env); // TODO: can be done once

  for (size_t I = 0; I < Audio.size(); ++I) {
    Audio[I] /= Env[I];
  }

  return Audio;
}

struct WavHeader {
  char Riff[4] = {'R', 'I', 'F', 'F'};
  uint32_t ChunkSize;
  char Wave[4] = {'W', 'A', 'V', 'E'};
  char Fmt[4] = {'f', 'm', 't', ' '};
  uint32_t FmtChunkSize = 16;
  uint16_t AudioFormat = 1; // PCM
  uint16_t NumChannels = 1; // Mono
  uint32_t SampleRate;
  uint32_t ByteRate;
  uint16_t BlockAlign;
  uint16_t BitsPerSample = 16;
  char Data[4] = {'d', 'a', 't', 'a'};
  uint32_t DataSize;
};

std::vector<uint8_t> audioDataToWav(const std::vector<float> &Data,
                                    int SampleRate) {
  std::vector<uint8_t> WavData;
  WavHeader Header;
  Header.SampleRate = SampleRate;
  Header.ByteRate =
      Header.SampleRate * Header.NumChannels * (Header.BitsPerSample / 8);
  Header.BlockAlign = Header.NumChannels * (Header.BitsPerSample / 8);
  Header.DataSize =
      static_cast<uint32_t>(Data.size() * (Header.BitsPerSample / 8));
  Header.ChunkSize = 36 + Header.DataSize;

  WavData.insert(WavData.end(), reinterpret_cast<uint8_t *>(&Header),
                 reinterpret_cast<uint8_t *>(&Header) + sizeof(Header));

  for (const auto &Sample : Data) {
    int16_t PCMSample =
        static_cast<int16_t>(std::clamp(Sample * 32767.0, -32768.0, 32767.0));
    WavData.insert(WavData.end(), reinterpret_cast<uint8_t *>(&PCMSample),
                   reinterpret_cast<uint8_t *>(&PCMSample) + sizeof(PCMSample));
  }

  return WavData;
}

// TextToSpeech function, will generate voice data from codes.
ErrNo codesToSpeech(Graph &GraphRef, Context &CxtRef) noexcept {
  // Remove all non-audio tokens.
  CxtRef.LlamaOutputTokens.erase(
      std::remove_if(CxtRef.LlamaOutputTokens.begin(),
                     CxtRef.LlamaOutputTokens.end(),
                     [](llama_token T) { return T < 151672 || T > 155772; }),
      CxtRef.LlamaOutputTokens.end());

  // Adjust the token values for audio data.
  for (llama_token &Token : CxtRef.LlamaOutputTokens) {
    Token -= 151672;
  }

  // Put codes into batch.
  const uint32_t NCodes =
      static_cast<uint32_t>(CxtRef.LlamaOutputTokens.size());
  llama_batch TTSBatch =
      llama_batch_init(NCodes, /* embd */ 0, /* n_seq_max */ 1);
  for (uint32_t I = 0; I < NCodes; ++I) {
    common_batch_add(TTSBatch, CxtRef.LlamaOutputTokens[I], I,
                     /* seq_ids */ {0}, /* logits */ true);
  }
  if (llama_decode(GraphRef.TTSContext.get(), TTSBatch) != 0) {
    RET_ERROR(ErrNo::RuntimeError, "codesToSpeech: fail to eval."sv)
  }
  llama_batch_free(TTSBatch);

  // Get embeddings.
  const int NEmbd = llama_model_n_embd(GraphRef.TTSModel.get());
  const float *Embd = llama_get_embeddings(GraphRef.TTSContext.get());

  // Embeddings to audio.
  std::vector<float> AudioData =
      embdToAudio(Embd, NCodes, NEmbd,
                  static_cast<int>(GraphRef.Params.cpuparams.n_threads));

  // Zero out first 0.25 seconds of audio.
  const uint32_t SamplingRate = 24000;
  for (uint32_t I = 0; I < SamplingRate / 4; ++I) {
    AudioData[I] = 0.0f;
  }

  // Convert audio data to wav and put it into output buffer.
  CxtRef.LlamaOutputs = audioDataToWav(AudioData, SamplingRate);

  // Save .wav file if path is provided.
  if (!GraphRef.TTSOutputFilePath.empty()) {
    std::ofstream File(GraphRef.TTSOutputFilePath, std::ios::binary);
    if (!File) {
      RET_ERROR(ErrNo::RuntimeError,
                "codesToSpeech: Failed to open file '{}' for writing"sv,
                GraphRef.TTSOutputFilePath);
    }
    File.write(reinterpret_cast<const char *>(CxtRef.LlamaOutputs.data()),
               CxtRef.LlamaOutputs.size());
    File.close();
  }

  return ErrNo::Success;
}

// <<<<<<<< Compute related functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

} // namespace

Expect<ErrNo> load(WasiNNEnvironment &Env, Span<const Span<uint8_t>> Builders,
                   [[maybe_unused]] Device Device, uint32_t &GraphId) noexcept {
  // Add a new graph.
  EndianValue<uint32_t> GId = Env.newGraph(Backend::GGML);
  auto &GraphRef = Env.NNGraph[GId.raw()].get<Graph>();

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
  GraphRef.Conf.NPredict = GraphRef.Params.n_ctx;
  GraphRef.Conf.ReversePrompt = ""sv;
  GraphRef.Conf.ImagePath = ""sv;

  // Set llama log callback.
  llama_log_set(llamaLogCallback, &GraphRef);

  // If the graph builder length > 1, the data of builder[1] is the metadata.
  if (Builders.size() > 1) {
    const std::string Metadata(reinterpret_cast<char *>(Builders[1].data()),
                               Builders[1].size());
    // Ignore context or model updates when initializing the graph.
    auto Res = parseMetadata(GraphRef, GraphRef.Conf, Metadata);
    if (Res != ErrNo::Success) {
      Env.deleteGraph(GId.raw());
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
    std::ofstream TempFile(GraphRef.Params.model.path,
                           std::ios::out | std::ios::binary);
    if (!TempFile) {
      Env.deleteGraph(GId.raw());
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
    Env.deleteGraph(GId.raw());
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
  common_init_result LlamaInit = common_init_from_params(Params);
  GraphRef.LlamaModel = std::move(LlamaInit.model);
  GraphRef.LlamaContext = std::move(LlamaInit.context);
  if (GraphRef.LlamaModel == nullptr) {
    Env.deleteGraph(GId.raw());
    RET_ERROR(ErrNo::InvalidArgument, "load: unable to init model."sv)
  }
  if (GraphRef.LlamaContext == nullptr) {
    Env.deleteGraph(GId.raw());
    RET_ERROR(ErrNo::InvalidArgument, "load: unable to init context."sv)
  }
  LOG_DEBUG(GraphRef.EnableDebugLog,
            "load: initialize ggml model with given parameters...Done"sv)

  // Initialize the TTS related model and context.
  if (GraphRef.TextToSpeech) {
    LOG_DEBUG(GraphRef.EnableDebugLog, "load: initialize TTS model."sv)
    Params.model = GraphRef.Params.vocoder.model;
    Params.embedding = true;
    common_init_result TTSInit = common_init_from_params(Params);
    GraphRef.TTSModel = std::move(TTSInit.model);
    GraphRef.TTSContext = std::move(TTSInit.context);
    if (GraphRef.TTSModel == nullptr) {
      Env.deleteGraph(GId.raw());
      RET_ERROR(ErrNo::InvalidArgument, "load: unable to init TTS model."sv)
    }
    if (GraphRef.TTSContext == nullptr) {
      Env.deleteGraph(GId.raw());
      RET_ERROR(ErrNo::InvalidArgument, "load: unable to init TTS context."sv)
    }
    LOG_DEBUG(GraphRef.EnableDebugLog, "load: initialize TTS model...Done"sv)
  }

  // Store the loaded graph.
  GraphId = GId.le();
  Env.NNGraph[GId.raw()].setReady();

  LOG_DEBUG(GraphRef.EnableDebugLog, "load...Done"sv)
  return ErrNo::Success;
}

Expect<ErrNo> initExecCtx(WasiNNEnvironment &Env, uint32_t GraphId,
                          uint32_t &ContextId) noexcept {
  auto &GraphRef = Env.NNGraph[GraphId].get<Graph>();
  LOG_DEBUG(GraphRef.EnableDebugLog, "initExecCtx"sv)
  ContextId = Env.newContext(GraphId, Env.NNGraph[GraphId]);
  LOG_INFO(GraphRef.EnableLog, "llama_system_info: {}"sv,
           llama_print_system_info())

  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  // Allocate the batch for input string prompt tokens.
  CxtRef.LlamaBatch = allocBatch(GraphRef.Params.n_batch);
  CxtRef.CurrentBatchSize = GraphRef.Params.n_batch;

  // Allocate the batch for output sampling. The batch size is always 1.
  CxtRef.OutputBatch = allocBatch(1);

  // Allocate sampler.
  CxtRef.LlamaSampler =
      common_sampler_init(GraphRef.LlamaModel.get(), GraphRef.Params.sampling);

  Env.NNContext[ContextId].setReady();
  ContextId = EndianValue(ContextId).le();
  LOG_DEBUG(GraphRef.EnableDebugLog, "initExecCtx...Done"sv)
  return ErrNo::Success;
}

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
    CxtRef.LlamaInputs = processTTSPrompt(GraphRef, Prompt);
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

Expect<ErrNo> getOutput(WasiNNEnvironment &Env, uint32_t ContextId,
                        uint32_t Index, Span<uint8_t> OutBuffer,
                        uint32_t &BytesWritten) noexcept {
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  auto &GraphRef = Env.NNGraph[CxtRef.GraphId].get<Graph>();
  LOG_DEBUG(GraphRef.EnableDebugLog, "getOutput: with Index {}"sv, Index)

  // Use index 1 for the metadata of the outputs.
  if (Index == 1) {
    std::string Metadata = buildOutputMetadata(CxtRef);
    std::copy_n(Metadata.data(), Metadata.length(), OutBuffer.data());
    BytesWritten = static_cast<uint32_t>(Metadata.length());
    LOG_DEBUG(GraphRef.EnableDebugLog,
              "getOutput: with Index {} a.k.a Metadata ...Done"sv, Index)
    return ErrNo::Success;
  }

  std::copy_n(CxtRef.LlamaOutputs.data(), CxtRef.LlamaOutputs.size(),
              OutBuffer.data());
  BytesWritten =
      EndianValue(static_cast<uint32_t>(CxtRef.LlamaOutputs.size())).le();
  LOG_DEBUG(GraphRef.EnableDebugLog, "getOutput: with Index {}...Done"sv, Index)
  return ErrNo::Success;
}

Expect<ErrNo> compute(WasiNNEnvironment &Env, uint32_t ContextId) noexcept {
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  auto &GraphRef = Env.NNGraph[CxtRef.GraphId].get<Graph>();
  LOG_DEBUG(GraphRef.EnableDebugLog, "compute")

  // Clear the context and reset the sampler.
  clearContext(GraphRef, CxtRef);

  if (GraphRef.Params.embedding) {
    return getEmbedding(GraphRef, CxtRef);
  }

  // Evaluate the input tokens.
  ErrNo ReturnCode = ErrNo::Success;
  if (GraphRef.VisionContext == nullptr) {
    // Text only prompt.
    ReturnCode = evaluateInput(GraphRef, CxtRef, "compute"sv);
    if (ReturnCode != ErrNo::Success) {
      return ReturnCode;
    }
  } else {
    // Multimodal prompt.
    llama_pos NewNPos;
    int32_t Res = mtmd_helper_eval_chunks(
        GraphRef.VisionContext.get(), GraphRef.LlamaContext.get(),
        GraphRef.VisionInputChunks.get(), CxtRef.NPos,
        /* seq_id */ 0, static_cast<int32_t>(CxtRef.CurrentBatchSize),
        /* logits_last */ true, &NewNPos);
    CxtRef.NPos = NewNPos;
    if (Res != 0) {
      RET_ERROR(ErrNo::InvalidArgument,
                "compute: unable to eval the mtmd prompt."sv)
    }
  }

  // Main prediction loop.
  LOG_DEBUG(GraphRef.EnableDebugLog, "compute: enter main prediction loop"sv)
  int64_t NPredict =
      CxtRef.Conf.NPredict < 0 ? INT32_MAX : CxtRef.Conf.NPredict;

  while (NPredict-- > 0) {
    ReturnCode = sampleOutput(GraphRef, CxtRef);
    if (ReturnCode != ErrNo::Success) {
      break;
    }
  }
  if (ReturnCode == ErrNo::EndOfSequence) {
    ReturnCode = ErrNo::Success;
  }
  LOG_DEBUG(GraphRef.EnableDebugLog,
            "compute: enter main prediction loop...Done"sv)
  // End of main prediction loop.

  // TTS: convert output codes to audio file.
  if (GraphRef.TextToSpeech) {
    LOG_DEBUG(GraphRef.EnableDebugLog,
              "compute: convert output codes to audio file."sv)
    ReturnCode = codesToSpeech(GraphRef, CxtRef);
    if (ReturnCode != ErrNo::Success) {
      RET_ERROR(ReturnCode,
                "compute: failed to convert output codes to audio "sv
                "file."sv)
    }
    LOG_DEBUG(GraphRef.EnableDebugLog,
              "compute: convert output codes to audio file...Done"sv)
  }

  if (GraphRef.EnableLog) {
    common_perf_print(GraphRef.LlamaContext.get(), CxtRef.LlamaSampler);
  }

  LOG_DEBUG(GraphRef.EnableDebugLog, "compute...Done"sv)
  return ReturnCode;
}

Expect<ErrNo> getOutputSingle(WasiNNEnvironment &Env, uint32_t ContextId,
                              uint32_t Index, Span<uint8_t> OutBuffer,
                              uint32_t &BytesWritten) noexcept {
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  auto &GraphRef = Env.NNGraph[CxtRef.GraphId].get<Graph>();
  LOG_DEBUG(GraphRef.EnableDebugLog, "getOutputSingle: with Index {}"sv, Index)

  // Use index 1 for the metadata of the outputs.
  if (Index == 1) {
    std::string Metadata = buildOutputMetadata(CxtRef);
    std::copy_n(Metadata.data(), Metadata.length(), OutBuffer.data());
    BytesWritten = static_cast<uint32_t>(Metadata.length());
    LOG_DEBUG(GraphRef.EnableDebugLog,
              "getOutputSingle: with Index {} a.k.a Metadata...Done"sv, Index)
    return ErrNo::Success;
  }

  std::string LastToken = common_token_to_piece(
      GraphRef.LlamaContext.get(), CxtRef.LlamaOutputTokens.back());
  std::copy_n(LastToken.data(), LastToken.length(), OutBuffer.data());
  BytesWritten = EndianValue(static_cast<uint32_t>(LastToken.length())).le();
  LOG_DEBUG(GraphRef.EnableDebugLog, "getOutputSingle: with Index {}...Done"sv,
            Index)
  return ErrNo::Success;
}

Expect<ErrNo> computeSingle(WasiNNEnvironment &Env,
                            uint32_t ContextId) noexcept {
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  auto &GraphRef = Env.NNGraph[CxtRef.GraphId].get<Graph>();
  LOG_DEBUG(GraphRef.EnableDebugLog, "computeSingle"sv)

  // New compute single token context.
  auto ReturnCode = ErrNo::Success;
  if (!CxtRef.ComputeSingleStarted) {
    CxtRef.ComputeSingleStarted = true;

    // Clear the context and reset the sampler.
    clearContext(GraphRef, CxtRef);

    // Evaluate the input tokens.
    if (GraphRef.VisionContext == nullptr) {
      // Text only prompt.
      ReturnCode = evaluateInput(GraphRef, CxtRef, "compute"sv);
      if (ReturnCode != ErrNo::Success) {
        return ReturnCode;
      }
    } else {
      // Multimodal prompt.
      llama_pos NewNPos;
      int32_t Res = mtmd_helper_eval_chunks(
          GraphRef.VisionContext.get(), GraphRef.LlamaContext.get(),
          GraphRef.VisionInputChunks.get(), CxtRef.NPos,
          /* seq_id */ 0, static_cast<int32_t>(CxtRef.CurrentBatchSize),
          /* logits_last */ true, &NewNPos);
      CxtRef.NPos = NewNPos;
      if (Res != 0) {
        RET_ERROR(ErrNo::InvalidArgument,
                  "compute: unable to eval the mtmd prompt."sv)
      }
    }
  }

  // Main prediction process.
  LOG_DEBUG(GraphRef.EnableDebugLog,
            "computeSingle: enter main prediction process"sv)
  ReturnCode = sampleOutput(GraphRef, CxtRef, true);
  if (ReturnCode != ErrNo::Success) {
    CxtRef.ComputeSingleStarted = false;
  }
  LOG_DEBUG(GraphRef.EnableDebugLog,
            "computeSingle: enter main prediction process...Done"sv)
  // End of main predict process.

  LOG_DEBUG(GraphRef.EnableDebugLog, "computeSingle...Done"sv)
  return ReturnCode;
}

Expect<ErrNo> finiSingle(WasiNNEnvironment &Env, uint32_t ContextId) noexcept {
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  auto &GraphRef = Env.NNGraph[CxtRef.GraphId].get<Graph>();
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

Expect<ErrNo> unload(WasiNNEnvironment &Env, uint32_t GraphId) noexcept {
  auto &GraphRef = Env.NNGraph[GraphId].get<Graph>();
  const bool IsDebugLog = GraphRef.EnableDebugLog;
  LOG_DEBUG(IsDebugLog, "unload"sv)

  // TODO: Move the resource deallocation into the destructor.
  if (GraphRef.LlamaModel != nullptr) {
    LOG_DEBUG(IsDebugLog, "unload: free llama model"sv)
    GraphRef.LlamaModel.reset();
    LOG_DEBUG(IsDebugLog, "unload: free llama model...Done"sv)
  }
  if (GraphRef.LlamaContext != nullptr) {
    LOG_DEBUG(IsDebugLog, "unload: free llama context"sv)
    GraphRef.LlamaContext.reset();
    LOG_DEBUG(IsDebugLog, "unload: free llama context...Done"sv)
  }
  if (GraphRef.VisionContext != nullptr) {
    LOG_DEBUG(IsDebugLog, "unload: free mtmd context"sv)
    GraphRef.VisionContext.reset();
    LOG_DEBUG(IsDebugLog, "unload: free mtmd context...Done"sv)
  }
  if (GraphRef.VisionInputChunks != nullptr) {
    LOG_DEBUG(IsDebugLog, "unload: free mtmd chunks"sv)
    GraphRef.VisionInputChunks.reset();
    LOG_DEBUG(IsDebugLog, "unload: free mtmd chunks...Done"sv)
  }
  if (GraphRef.TTSModel != nullptr) {
    LOG_DEBUG(IsDebugLog, "unload: free TTS model"sv)
    GraphRef.TTSModel.reset();
    LOG_DEBUG(IsDebugLog, "unload: free TTS model...Done"sv)
  }
  if (GraphRef.TTSContext != nullptr) {
    LOG_DEBUG(IsDebugLog, "unload: free TTS context"sv)
    GraphRef.TTSContext.reset();
    LOG_DEBUG(IsDebugLog, "unload: free TTS context...Done"sv)
  }
  if (!GraphRef.TensorBuftOverrides.empty()) {
    LOG_DEBUG(IsDebugLog, "unload: free tensor buffer overrides"sv)
    GraphRef.TensorBuftOverrides.clear();
    LOG_DEBUG(IsDebugLog, "unload: free tensor buffer overrides...Done"sv)
  }
  Env.deleteGraph(GraphId);
  Env.mdRemoveById(GraphId);

  LOG_DEBUG(IsDebugLog, "unload...Done"sv)
  return ErrNo::Success;
}

Expect<ErrNo> finalizeExecCtx(WasiNNEnvironment &Env,
                              uint32_t ContextId) noexcept {
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  auto &GraphRef = Env.NNGraph[CxtRef.GraphId].get<Graph>();
  LOG_DEBUG(GraphRef.EnableDebugLog, "finalize_execution_context"sv)

  if (CxtRef.LlamaSampler != nullptr) {
    LOG_DEBUG(GraphRef.EnableDebugLog,
              "finalize_execution_context: free compute_single sampler"sv)
    common_sampler_free(CxtRef.LlamaSampler);
    CxtRef.LlamaSampler = nullptr;
    LOG_DEBUG(
        GraphRef.EnableDebugLog,
        "finalize_execution_context: free compute_single sampler...Done"sv)
  }
  llama_batch_free(CxtRef.LlamaBatch);
  llama_batch_free(CxtRef.OutputBatch);
  Env.deleteContext(ContextId);

  LOG_DEBUG(GraphRef.EnableDebugLog, "finalize_execution_context...Done"sv)
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
Expect<ErrNo> finalizeExecCtx(WasiNNEnvironment &, uint32_t) noexcept {
  return reportBackendNotSupported();
}

#endif
} // namespace WasmEdge::Host::WASINN::GGML
