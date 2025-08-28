#include "wasinn_bitnet.h"
#include "wasinnenv.h"
#include <cstdint>

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_BITNET
#include "simdjson.h"
#include <algorithm>
#include <common.h>
#include <filesystem>
#include <fmt/ranges.h>
#include <fstream>
#include <llama.h>
#include <sampling.h>
#include <sstream>
#endif

namespace WasmEdge::Host::WASINN::BitNet {
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_BITNET

namespace {

// Macro for logging debug message.
#define LOG_DEBUG(Debug, ...)                                                  \
  if (Debug) {                                                                 \
    spdlog::info("[WASI-NN][Debug] BitNet backend: "sv __VA_ARGS__);           \
  }

// Macro for logging info message.
#define LOG_INFO(Info, ...)                                                    \
  if (Info) {                                                                  \
    spdlog::info("[WASI-NN] BitNet backend: "sv __VA_ARGS__);                  \
  }

// Macro for logging warning message.
#define LOG_WARN(...) spdlog::warn("[WASI-NN] BitNet backend: "sv __VA_ARGS__);

// Macro for logging error message.
#define LOG_ERROR(...)                                                         \
  spdlog::error("[WASI-NN] BitNet backend: "sv __VA_ARGS__);

// Macro for logging error message and return.
#define RET_ERROR(Error, ...)                                                  \
  spdlog::error("[WASI-NN] BitNet backend: "sv __VA_ARGS__);                   \
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
    spdlog::error("[WASI-NN] BitNet.cpp: {}"sv, Text);
  } else if (LogLevel == GGML_LOG_LEVEL_WARN) {
    spdlog::warn("[WASI-NN] BitNet.cpp: {}"sv, Text);
  } else if (LogLevel == GGML_LOG_LEVEL_INFO) {
    spdlog::info("[WASI-NN] BitNet.cpp: {}"sv, Text);
  } else if (LogLevel == GGML_LOG_LEVEL_DEBUG) {
    spdlog::debug("[WASI-NN] BitNet.cpp: {}"sv, Text);
  }
}

// >>>>>>>> Metadata related functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

// Helper function to parse comma-separated string into vector.
void stringToList(const std::string &Raw, std::vector<int> &Out) {
  std::string Copy = Raw;
  std::replace(Copy.begin(), Copy.end(), ',', ' ');
  std::stringstream SS(Copy);
  Out.clear();
  while (SS.good()) {
    int TmpInt;
    SS >> TmpInt;
    Out.push_back(TmpInt);
  }
}

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
  int64_t PrevMainGpu = GraphRef.Params.main_gpu;
  int64_t PrevThreads = GraphRef.Params.cpuparams.n_threads;
  bool PrevFlashAttn = GraphRef.Params.flash_attn;
  int64_t PrevCtxSize = GraphRef.Params.n_ctx;
  bool PrevEmbedding = GraphRef.Params.embedding;
  // Get the current sampler parameters.
  double PrevTemp = GraphRef.Params.sparams.temp;
  double PrevTopP = GraphRef.Params.sparams.top_p;
  double PrevRepeatPenalty = GraphRef.Params.sparams.penalty_repeat;
  double PrevPresencePenalty = GraphRef.Params.sparams.penalty_present;
  double PrevFrequencyPenalty = GraphRef.Params.sparams.penalty_freq;
  std::string PrevGrammar = GraphRef.Params.sparams.grammar;
  uint64_t PrevSeed = GraphRef.Params.sparams.seed;

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
  if (Doc.at_key("tensor-split").error() == simdjson::SUCCESS) {
    // The TensorSplit is a comma-separated list of non-negative values.
    // E.g., "3,2" presents 60% of the data to GPU 0 and 40% to GPU 1.

    // helper function `stringToList` cannot be used here since tensor-split
    // needs a fixed-size array with validation checks.
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
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the n-keep option."sv)
    }
    GraphRef.Params.n_keep = static_cast<int32_t>(NKeep);
  }
  if (Doc.at_key("n-chunks").error() == simdjson::SUCCESS) {
    int64_t NChunks;
    auto Err = Doc["n-chunks"].get<int64_t>().get(NChunks);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the n-chunks option."sv)
    }
    GraphRef.Params.n_chunks = static_cast<int32_t>(NChunks);
  }
  if (Doc.at_key("n-parallel").error() == simdjson::SUCCESS) {
    int64_t NParallel;
    auto Err = Doc["n-parallel"].get<int64_t>().get(NParallel);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the n-parallel option."sv)
    }
    GraphRef.Params.n_parallel = static_cast<int32_t>(NParallel);
  }
  if (Doc.at_key("n-sequences").error() == simdjson::SUCCESS) {
    int64_t NSequences;
    auto Err = Doc["n-sequences"].get<int64_t>().get(NSequences);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the n-sequences option."sv)
    }
    GraphRef.Params.n_sequences = static_cast<int32_t>(NSequences);
  }
  if (Doc.at_key("grp-attn-n").error() == simdjson::SUCCESS) {
    int64_t GrpAttnN;
    auto Err = Doc["grp-attn-n"].get<int64_t>().get(GrpAttnN);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the grp-attn-n option."sv)
    }
    GraphRef.Params.grp_attn_n = static_cast<int32_t>(GrpAttnN);
  }
  if (Doc.at_key("grp-attn-w").error() == simdjson::SUCCESS) {
    int64_t GrpAttnW;
    auto Err = Doc["grp-attn-w"].get<int64_t>().get(GrpAttnW);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the grp-attn-w option."sv)
    }
    GraphRef.Params.grp_attn_w = static_cast<int32_t>(GrpAttnW);
  }
  if (Doc.at_key("n-print").error() == simdjson::SUCCESS) {
    int64_t NPrint;
    auto Err = Doc["n-print"].get<int64_t>().get(NPrint);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the n-print option."sv)
    }
    GraphRef.Params.n_print = static_cast<int32_t>(NPrint);
  }
  if (Doc.at_key("rope-freq-base").error() == simdjson::SUCCESS) {
    double RopeFreqBase;
    auto Err = Doc["rope-freq-base"].get<double>().get(RopeFreqBase);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the rope-freq-base option."sv)
    }
    GraphRef.Params.rope_freq_base = static_cast<float>(RopeFreqBase);
  }
  if (Doc.at_key("rope-freq-scale").error() == simdjson::SUCCESS) {
    double RopeFreqScale;
    auto Err = Doc["rope-freq-scale"].get<double>().get(RopeFreqScale);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the rope-freq-scale option."sv)
    }
    GraphRef.Params.rope_freq_scale = static_cast<float>(RopeFreqScale);
  }
  if (Doc.at_key("yarn-ext-factor").error() == simdjson::SUCCESS) {
    double YarnExtFactor;
    auto Err = Doc["yarn-ext-factor"].get<double>().get(YarnExtFactor);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the yarn-ext-factor option."sv)
    }
    GraphRef.Params.yarn_ext_factor = static_cast<float>(YarnExtFactor);
  }
  if (Doc.at_key("yarn-attn-factor").error() == simdjson::SUCCESS) {
    double YarnAttnFactor;
    auto Err = Doc["yarn-attn-factor"].get<double>().get(YarnAttnFactor);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the yarn-attn-factor option."sv)
    }
    GraphRef.Params.yarn_attn_factor = static_cast<float>(YarnAttnFactor);
  }
  if (Doc.at_key("yarn-beta-fast").error() == simdjson::SUCCESS) {
    double YarnBetaFast;
    auto Err = Doc["yarn-beta-fast"].get<double>().get(YarnBetaFast);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the yarn-beta-fast option."sv)
    }
    GraphRef.Params.yarn_beta_fast = static_cast<float>(YarnBetaFast);
  }
  if (Doc.at_key("yarn-beta-slow").error() == simdjson::SUCCESS) {
    double YarnBetaSlow;
    auto Err = Doc["yarn-beta-slow"].get<double>().get(YarnBetaSlow);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the yarn-beta-slow option."sv)
    }
    GraphRef.Params.yarn_beta_slow = static_cast<float>(YarnBetaSlow);
  }
  if (Doc.at_key("yarn-orig-ctx").error() == simdjson::SUCCESS) {
    int64_t YarnOrigCtx;
    auto Err = Doc["yarn-orig-ctx"].get<int64_t>().get(YarnOrigCtx);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the yarn-orig-ctx option."sv)
    }
    GraphRef.Params.yarn_orig_ctx = static_cast<int32_t>(YarnOrigCtx);
  }
  if (Doc.at_key("defrag-thold").error() == simdjson::SUCCESS) {
    double DefragThold;
    auto Err = Doc["defrag-thold"].get<double>().get(DefragThold);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the defrag-thold option."sv)
    }
    GraphRef.Params.defrag_thold = static_cast<float>(DefragThold);
  }
  if (Doc.at_key("mask-valid").error() == simdjson::SUCCESS) {
    auto Err =
        Doc["mask-valid"].get<bool>().get(GraphRef.Params.cpuparams.mask_valid);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the mask-valid option."sv)
    }
  }
  if (Doc.at_key("priority").error() == simdjson::SUCCESS) {
    int64_t Priority;
    auto Err = Doc["priority"].get<int64_t>().get(Priority);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the priority option."sv)
    }
    GraphRef.Params.cpuparams.priority =
        static_cast<ggml_sched_priority>(Priority);
  }
  if (Doc.at_key("strict-cpu").error() == simdjson::SUCCESS) {
    auto Err =
        Doc["strict-cpu"].get<bool>().get(GraphRef.Params.cpuparams.strict_cpu);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the strict-cpu option."sv)
    }
  }
  if (Doc.at_key("poll").error() == simdjson::SUCCESS) {
    int64_t Poll;
    auto Err = Doc["poll"].get<int64_t>().get(Poll);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument, "Unable to retrieve the poll option."sv)
    }
    GraphRef.Params.cpuparams.poll = static_cast<int32_t>(Poll);
  }
  if (Doc.at_key("mask-valid-batch").error() == simdjson::SUCCESS) {
    auto Err = Doc["mask-valid-batch"].get<bool>().get(
        GraphRef.Params.cpuparams_batch.mask_valid);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the mask-valid-batch option."sv)
    }
  }
  if (Doc.at_key("priority-batch").error() == simdjson::SUCCESS) {
    int64_t Priority;
    auto Err = Doc["priority-batch"].get<int64_t>().get(Priority);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the priority-batch option."sv)
    }
    GraphRef.Params.cpuparams_batch.priority =
        static_cast<ggml_sched_priority>(Priority);
  }
  if (Doc.at_key("strict-cpu-batch").error() == simdjson::SUCCESS) {
    auto Err = Doc["strict-cpu-batch"].get<bool>().get(
        GraphRef.Params.cpuparams_batch.strict_cpu);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the strict-cpu-batch option."sv)
    }
  }
  if (Doc.at_key("poll-batch").error() == simdjson::SUCCESS) {
    int64_t Poll;
    auto Err = Doc["poll-batch"].get<int64_t>().get(Poll);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the poll-batch option."sv)
    }
    GraphRef.Params.cpuparams_batch.poll = static_cast<int32_t>(Poll);
  }
  if (Doc.at_key("numa").error() == simdjson::SUCCESS) {
    int64_t Numa;
    auto Err = Doc["numa"].get<int64_t>().get(Numa);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument, "Unable to retrieve the numa option."sv)
    }
    GraphRef.Params.numa = static_cast<ggml_numa_strategy>(Numa);
  }
  if (Doc.at_key("rope-scaling-type").error() == simdjson::SUCCESS) {
    int64_t RopeScalingType;
    auto Err = Doc["rope-scaling-type"].get<int64_t>().get(RopeScalingType);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the rope-scaling-type option."sv)
    }
    GraphRef.Params.rope_scaling_type =
        static_cast<llama_rope_scaling_type>(RopeScalingType);
  }
  if (Doc.at_key("pooling-type").error() == simdjson::SUCCESS) {
    int64_t PoolingType;
    auto Err = Doc["pooling-type"].get<int64_t>().get(PoolingType);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the pooling-type option."sv)
    }
    GraphRef.Params.pooling_type =
        static_cast<enum llama_pooling_type>(PoolingType);
  }
  if (Doc.at_key("attention-type").error() == simdjson::SUCCESS) {
    int64_t AttentionType;
    auto Err = Doc["attention-type"].get<int64_t>().get(AttentionType);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the attention-type option."sv)
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
    GraphRef.Params.sparams.n_prev = static_cast<int32_t>(NPrev);
  }
  if (Doc.at_key("top-k").error() == simdjson::SUCCESS) {
    int64_t TopK;
    auto Err = Doc["top-k"].get<int64_t>().get(TopK);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the top-k option."sv)
    }
    GraphRef.Params.sparams.top_k = static_cast<int32_t>(TopK);
  }
  if (Doc.at_key("min-p").error() == simdjson::SUCCESS) {
    double MinP;
    auto Err = Doc["min-p"].get<double>().get(MinP);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the min-p option."sv)
    }
    GraphRef.Params.sparams.min_p = static_cast<float>(MinP);
  }
  if (Doc.at_key("typ-p").error() == simdjson::SUCCESS) {
    double TypP;
    auto Err = Doc["typ-p"].get<double>().get(TypP);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the typ-p option."sv)
    }
    GraphRef.Params.sparams.typ_p = static_cast<float>(TypP);
  }
  if (Doc.at_key("penalize-nl").error() == simdjson::SUCCESS) {
    auto Err =
        Doc["penalize-nl"].get<bool>().get(GraphRef.Params.sparams.penalize_nl);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the penalize-nl option."sv)
    }
  }
  if (Doc.at_key("tfs").error() == simdjson::SUCCESS) {
    double TfsZ;
    auto Err = Doc["tfs"].get<double>().get(TfsZ);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument, "Unable to retrieve the tfs option."sv)
    }
    GraphRef.Params.sparams.tfs_z = static_cast<float>(TfsZ);
  }
  if (Doc.at_key("dynatemp-range").error() == simdjson::SUCCESS) {
    double DynaTempRange;
    auto Err = Doc["dynatemp-range"].get<double>().get(DynaTempRange);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the dynatemp-range option."sv)
    }
    GraphRef.Params.sparams.dynatemp_range = static_cast<float>(DynaTempRange);
  }
  if (Doc.at_key("dynatemp-exponent").error() == simdjson::SUCCESS) {
    double DynaTempExponent;
    auto Err = Doc["dynatemp-exponent"].get<double>().get(DynaTempExponent);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the dynatemp-exponent option."sv)
    }
    GraphRef.Params.sparams.dynatemp_exponent =
        static_cast<float>(DynaTempExponent);
  }
  if (Doc.at_key("last-n-penalty").error() == simdjson::SUCCESS) {
    int64_t LastNPenalty;
    auto Err = Doc["last-n-penalty"].get<int64_t>().get(LastNPenalty);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the last-n-penalty option."sv)
    }
    GraphRef.Params.sparams.penalty_last_n = static_cast<int32_t>(LastNPenalty);
  }
  if (Doc.at_key("temp").error() == simdjson::SUCCESS) {
    double Temp;
    auto Err = Doc["temp"].get<double>().get(Temp);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument, "Unable to retrieve the temp option."sv)
    }
    GraphRef.Params.sparams.temp = static_cast<float>(std::max(0.0, Temp));
  }
  if (Doc.at_key("top-p").error() == simdjson::SUCCESS) {
    double TopP;
    auto Err = Doc["top-p"].get<double>().get(TopP);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the top-p option."sv)
    }
    GraphRef.Params.sparams.top_p = static_cast<float>(std::max(0.0, TopP));
  }
  if (Doc.at_key("repeat-penalty").error() == simdjson::SUCCESS) {
    double RepeatPenalty;
    auto Err = Doc["repeat-penalty"].get<double>().get(RepeatPenalty);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the repeat-penalty option."sv)
    }
    GraphRef.Params.sparams.penalty_repeat =
        static_cast<float>(std::max(0.0, RepeatPenalty));
  }
  if (Doc.at_key("presence-penalty").error() == simdjson::SUCCESS) {
    double PresencePenalty;
    auto Err = Doc["presence-penalty"].get<double>().get(PresencePenalty);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the presence-penalty option."sv)
    }
    GraphRef.Params.sparams.penalty_present =
        static_cast<float>(std::max(0.0, PresencePenalty));
  }
  if (Doc.at_key("frequency-penalty").error() == simdjson::SUCCESS) {
    double FrequencyPenalty;
    auto Err = Doc["frequency-penalty"].get<double>().get(FrequencyPenalty);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the frequency-penalty option."sv)
    }
    GraphRef.Params.sparams.penalty_freq =
        static_cast<float>(std::max(0.0, FrequencyPenalty));
  }
  if (Doc.at_key("mirostat").error() == simdjson::SUCCESS) {
    int64_t Mirostat;
    auto Err = Doc["mirostat"].get<int64_t>().get(Mirostat);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the mirostat option."sv)
    }
    GraphRef.Params.sparams.mirostat = static_cast<int32_t>(Mirostat);
  }
  if (Doc.at_key("mirostat-eta").error() == simdjson::SUCCESS) {
    double MirostatEta;
    auto Err = Doc["mirostat-eta"].get<double>().get(MirostatEta);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the mirostat-eta option."sv)
    }
    GraphRef.Params.sparams.mirostat_eta = static_cast<float>(MirostatEta);
  }
  if (Doc.at_key("mirostat-ent").error() == simdjson::SUCCESS) {
    double MirostatEnt;
    auto Err = Doc["mirostat-ent"].get<double>().get(MirostatEnt);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the mirostat-ent option."sv)
    }
    GraphRef.Params.sparams.mirostat_tau = static_cast<float>(MirostatEnt);
  }
  if (Doc.at_key("ignore-eos").error() == simdjson::SUCCESS) {
    auto Err =
        Doc["ignore-eos"].get<bool>().get(GraphRef.Params.sparams.ignore_eos);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the ignore-eos option."sv)
    }
  }
  if (Doc.at_key("no-perf-sampling").error() == simdjson::SUCCESS) {
    auto Err = Doc["no-perf-sampling"].get<bool>().get(
        GraphRef.Params.sparams.no_perf);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the no-perf-sampling option."sv)
    }
  }
  if (Doc.at_key("grammar").error() == simdjson::SUCCESS) {
    std::string_view Grammar;
    auto Err = Doc["grammar"].get<std::string_view>().get(Grammar);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the grammar option."sv)
    }
    GraphRef.Params.sparams.grammar = Grammar;
  }
  if (Doc.at_key("seed").error() == simdjson::SUCCESS) {
    uint64_t Seed;
    auto Err = Doc["seed"].get<uint64_t>().get(Seed);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument, "Unable to retrieve the seed option."sv)
    }
    GraphRef.Params.sparams.seed = static_cast<int32_t>(Seed);
  }
  // The speculative parameters.
  if (Doc.at_key("n-gpu-layers-draft").error() == simdjson::SUCCESS) {
    int64_t NGPULayersDraft;
    auto Err = Doc["n-gpu-layers-draft"].get<int64_t>().get(NGPULayersDraft);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the n-gpu-layers-draft option."sv)
    }
    GraphRef.Params.n_gpu_layers_draft = static_cast<int32_t>(NGPULayersDraft);
  }
  if (Doc.at_key("p-split").error() == simdjson::SUCCESS) {
    double PSplit;
    auto Err = Doc["p-split"].get<double>().get(PSplit);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the p-split option."sv)
    }
    GraphRef.Params.p_split = static_cast<float>(PSplit);
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
    GraphRef.Params.model_url = ModelUrl;
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
    GraphRef.Params.hf_repo = HfRepo;
  }
  if (Doc.at_key("hf-file").error() == simdjson::SUCCESS) {
    std::string_view HfFile;
    auto Err = Doc["hf-file"].get<std::string_view>().get(HfFile);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the hf-file option."sv)
    }
    GraphRef.Params.hf_file = HfFile;
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
    auto Err = Doc["flash-attn"].get<bool>().get(GraphRef.Params.flash_attn);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the flash-attn option."sv)
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
    std::string_view CacheTypeK;
    auto Err = Doc["cache-type-k"].get<std::string_view>().get(CacheTypeK);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the cache-type-k option."sv)
    }
    GraphRef.Params.cache_type_k = CacheTypeK;
  }
  if (Doc.at_key("cache-type-v").error() == simdjson::SUCCESS) {
    std::string_view CacheTypeV;
    auto Err = Doc["cache-type-v"].get<std::string_view>().get(CacheTypeV);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the cache-type-v option."sv)
    }
    GraphRef.Params.cache_type_v = CacheTypeV;
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
    auto Err = Doc["reranking"].get<bool>().get(GraphRef.Params.reranking);
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
    std::string_view NPP;
    auto Err = Doc["n-pp"].get<std::string_view>().get(NPP);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument, "Unable to retrieve the n-pp option."sv)
    }
    stringToList(std::string(NPP), GraphRef.Params.n_pp);
  }
  if (Doc.at_key("n-tg").error() == simdjson::SUCCESS) {
    std::string_view NTG;
    auto Err = Doc["n-tg"].get<std::string_view>().get(NTG);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument, "Unable to retrieve the n-tg option."sv)
    }
    stringToList(std::string(NTG), GraphRef.Params.n_tg);
  }
  if (Doc.at_key("n-pl").error() == simdjson::SUCCESS) {
    std::string_view NPL;
    auto Err = Doc["n-pl"].get<std::string_view>().get(NPL);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument, "Unable to retrieve the n-pl option."sv)
    }
    stringToList(std::string(NPL), GraphRef.Params.n_pl);
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
    GraphRef.Params.chunk_size = static_cast<int32_t>(ChunkSize);
  }
  if (Doc.at_key("chunk-separator").error() == simdjson::SUCCESS) {
    std::string_view ChunkSeparator;
    auto Err =
        Doc["chunk-separator"].get<std::string_view>().get(ChunkSeparator);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the chunk-separator option."sv)
    }
    GraphRef.Params.chunk_separator = ChunkSeparator;
  }
  if (Doc.at_key("n-junk").error() == simdjson::SUCCESS) {
    int64_t NJunk;
    auto Err = Doc["n-junk"].get<int64_t>().get(NJunk);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the n-junk option."sv)
    }
    GraphRef.Params.n_junk = static_cast<int32_t>(NJunk);
  }
  if (Doc.at_key("i-pos").error() == simdjson::SUCCESS) {
    int64_t IPos;
    auto Err = Doc["i-pos"].get<int64_t>().get(IPos);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the i-pos option."sv)
    }
    GraphRef.Params.i_pos = static_cast<int32_t>(IPos);
  }
  if (Doc.at_key("n-out-freq").error() == simdjson::SUCCESS) {
    int64_t NOutFreq;
    auto Err = Doc["n-out-freq"].get<int64_t>().get(NOutFreq);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the n-out-freq option."sv)
    }
    GraphRef.Params.n_out_freq = static_cast<int32_t>(NOutFreq);
  }
  if (Doc.at_key("n-save-freq").error() == simdjson::SUCCESS) {
    int64_t NSaveFreq;
    auto Err = Doc["n-save-freq"].get<int64_t>().get(NSaveFreq);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the n-save-freq option."sv)
    }
    GraphRef.Params.n_save_freq = static_cast<int32_t>(NSaveFreq);
  }
  if (Doc.at_key("i-chunk").error() == simdjson::SUCCESS) {
    int64_t IChunk;
    auto Err = Doc["i-chunk"].get<int64_t>().get(IChunk);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the i-chunk option."sv)
    }
    GraphRef.Params.i_chunk = static_cast<int32_t>(IChunk);
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
    GraphRef.Params.n_pca_batch = static_cast<int32_t>(NPCABatch);
  }
  if (Doc.at_key("n-pca-iterations").error() == simdjson::SUCCESS) {
    int64_t NPCAIterations;
    auto Err = Doc["n-pca-iterations"].get<int64_t>().get(NPCAIterations);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the n-pca-iterations option."sv)
    }
    GraphRef.Params.n_pca_iterations = static_cast<int32_t>(NPCAIterations);
  }
  if (Doc.at_key("cvector-dimre-method").error() == simdjson::SUCCESS) {
    std::string_view CVectorDimreMethod;
    auto Err = Doc["cvector-dimre-method"].get<std::string_view>().get(
        CVectorDimreMethod);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the cvector-dimre-method option."sv)
    }
    if (CVectorDimreMethod == "pca") {
      GraphRef.Params.cvector_dimre_method = DIMRE_METHOD_PCA;
    } else if (CVectorDimreMethod == "mean") {
      GraphRef.Params.cvector_dimre_method = DIMRE_METHOD_MEAN;
    } else {
      RET_ERROR(
          ErrNo::InvalidArgument,
          "Invalid value for cvector-dimre-method: must be 'pca' or 'mean'."sv)
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
    GraphRef.Params.cvector_outfile = CVectorOutfile;
  }
  if (Doc.at_key("cvector-positive-file").error() == simdjson::SUCCESS) {
    std::string_view CVectorPositiveFile;
    auto Err = Doc["cvector-positive-file"].get<std::string_view>().get(
        CVectorPositiveFile);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the cvector-positive-file option."sv)
    }
    GraphRef.Params.cvector_positive_file = CVectorPositiveFile;
  }
  if (Doc.at_key("cvector-negative-file").error() == simdjson::SUCCESS) {
    std::string_view CVectorNegativeFile;
    auto Err = Doc["cvector-negative-file"].get<std::string_view>().get(
        CVectorNegativeFile);
    if (Err) {
      RET_ERROR(ErrNo::InvalidArgument,
                "Unable to retrieve the cvector-negative-file option."sv)
    }
    GraphRef.Params.cvector_negative_file = CVectorNegativeFile;
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
                "Unable to retrieve the out-file option."sv)
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

  // Check if the model parameters are updated.
  if (IsModelUpdated && (PrevNGPULayers != GraphRef.Params.n_gpu_layers ||
                         PrevMainGpu != GraphRef.Params.main_gpu)) {
    *IsModelUpdated = true;
  }

  // Check if the context parameters are updated.
  if (IsContextUpdated && (PrevCtxSize != GraphRef.Params.n_ctx ||
                           PrevThreads != GraphRef.Params.cpuparams.n_threads ||
                           PrevFlashAttn != GraphRef.Params.flash_attn ||
                           PrevEmbedding != GraphRef.Params.embedding)) {
    *IsContextUpdated = true;
  }

  // Check if the sampler parameters are updated.
  if (IsSamplerUpdated &&
      (PrevTemp != GraphRef.Params.sparams.temp ||
       PrevTopP != GraphRef.Params.sparams.top_p ||
       PrevRepeatPenalty != GraphRef.Params.sparams.penalty_repeat ||
       PrevPresencePenalty != GraphRef.Params.sparams.penalty_present ||
       PrevFrequencyPenalty != GraphRef.Params.sparams.penalty_freq ||
       PrevGrammar != GraphRef.Params.sparams.grammar ||
       PrevSeed != GraphRef.Params.sparams.seed)) {
    *IsSamplerUpdated = true;
  }

  return ErrNo::Success;
}

// <<<<<<<< Metadata related functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

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
  llama_kv_cache_clear(GraphRef.LlamaContext.get());
  common_sampler_reset(CxtRef.LlamaSampler.get());
  CxtRef.NPos = 0;
  CxtRef.LlamaOutputTokens.clear();
  CxtRef.LlamaOutputs.clear();
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
      CxtRef.LlamaSampler.get(), GraphRef.LlamaContext.get(), /* idx */ -1);
  common_sampler_accept(CxtRef.LlamaSampler.get(), Id,
                        /* accept_grammar */ true);

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
  // Only stop on EOS if GraphRef.Params.sparams.ignore_eos is false.
  if (!GraphRef.Params.sparams.ignore_eos) {
    if (llama_token_is_eog(GraphRef.LlamaModel.get(), Id)) {
      LOG_INFO(GraphRef.EnableLog, "sampleOutput: EOS token found."sv)
      return ErrNo::EndOfSequence;
    }
  }
  // Evaluate the output token.
  return evaluateTokens(Span<const llama_token>(&Id, 1), GraphRef,
                        CxtRef.OutputBatch, CxtRef.NPos, true);
}

// TODO: Merge into compute.
Expect<ErrNo> getEmbedding(Graph &GraphRef, Context &CxtRef) noexcept {
  LOG_DEBUG(GraphRef.EnableDebugLog, "getEmbedding"sv)

  const llama_token SepTokenId = llama_token_sep(GraphRef.LlamaModel.get());
  if (SepTokenId > -1) {
    if (CxtRef.LlamaInputs.size() > 0 &&
        CxtRef.LlamaInputs.back() != SepTokenId) {
      LOG_WARN(
          "getEmbedding: last token in the prompt is not SEP, "sv
          "'tokenizer.ggml.add_eos_token' should be set to 'true' in the GGUF "sv
          "header."sv)
    }
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
  const struct llama_model *LlamaModel =
      llama_get_model(GraphRef.LlamaContext.get());
  const int32_t NEmbd = llama_n_embd(LlamaModel);
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

// <<<<<<<< Compute related functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

} // namespace

Expect<ErrNo> load(WasiNNEnvironment &Env, Span<const Span<uint8_t>> Builders,
                   [[maybe_unused]] Device Device, uint32_t &GraphId) noexcept {
  if (Builders.empty()) {
    RET_ERROR(ErrNo::InvalidArgument,
              "Invalid builders size, builders size must be > 0.");
  }

  // Add a graph
  const uint32_t GId = Env.newGraph(Backend::BitNet);
  auto &GraphRef = Env.NNGraph[GId].get<Graph>();

  // Initialize the plugin parameters.
  GraphRef.EnableLog = false;
  GraphRef.EnableDebugLog = false;
  const common_params CommonParamsDefault;
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
  GraphRef.Params.mmproj = ""sv;
  GraphRef.Params.warmup = false;
  // Initialize the context parameters.
  llama_context_params ContextParamsDefault = llama_context_default_params();
  GraphRef.Params.n_ctx = ContextParamsDefault.n_ctx;
  GraphRef.Params.n_batch = ContextParamsDefault.n_batch;
  GraphRef.Params.n_ubatch = ContextParamsDefault.n_ubatch;
  GraphRef.Params.cpuparams.n_threads = ContextParamsDefault.n_threads_batch;
  GraphRef.Params.cpuparams_batch.n_threads =
      ContextParamsDefault.n_threads_batch;
  GraphRef.Params.rope_scaling_type = ContextParamsDefault.rope_scaling_type;
  GraphRef.Params.pooling_type = ContextParamsDefault.pooling_type;
  GraphRef.Params.attention_type = ContextParamsDefault.attention_type;
  GraphRef.Params.rope_freq_base = ContextParamsDefault.rope_freq_base;
  GraphRef.Params.rope_freq_scale = ContextParamsDefault.rope_freq_scale;
  GraphRef.Params.yarn_ext_factor = ContextParamsDefault.yarn_ext_factor;
  GraphRef.Params.yarn_attn_factor = ContextParamsDefault.yarn_attn_factor;
  GraphRef.Params.yarn_beta_fast = ContextParamsDefault.yarn_beta_fast;
  GraphRef.Params.yarn_beta_slow = ContextParamsDefault.yarn_beta_slow;
  GraphRef.Params.yarn_orig_ctx = ContextParamsDefault.yarn_orig_ctx;
  GraphRef.Params.defrag_thold = ContextParamsDefault.defrag_thold;
  GraphRef.Params.cb_eval = ContextParamsDefault.cb_eval;
  GraphRef.Params.cb_eval_user_data = ContextParamsDefault.cb_eval_user_data;
  GraphRef.Params.embedding = ContextParamsDefault.embeddings;
  GraphRef.Params.no_kv_offload = !ContextParamsDefault.offload_kqv;
  GraphRef.Params.flash_attn = ContextParamsDefault.flash_attn;
  GraphRef.Params.no_perf = ContextParamsDefault.no_perf;

  // Initialize the sampling parameters.
  const common_sampler_params SamplerParamsDefault;
  GraphRef.Params.sparams = SamplerParamsDefault;

  // Initialize the config parameters.
  GraphRef.Conf.StreamStdout = false;
  GraphRef.Conf.EmbdNormalize =
      static_cast<EmbdNormalizeType>(CommonParamsDefault.embd_normalize);
  GraphRef.Conf.NPredict = ContextParamsDefault.n_ctx;
  GraphRef.Conf.ReversePrompt = ""sv;

  // Set llama log callback.
  llama_log_set(llamaLogCallback, &GraphRef);
  LOG_DEBUG(GraphRef.EnableDebugLog, "load start."sv)

  // If the graph builder length > 1, the data of builder[1] is the metadata.
  if (Builders.size() > 1) {
    const std::string Metadata(
        reinterpret_cast<const char *>(Builders[1].data()), Builders[1].size());
    // Ignore context or model updates when initializing the graph.
    auto Res = parseMetadata(GraphRef, GraphRef.Conf, Metadata);
    if (Res != ErrNo::Success) {
      Env.deleteGraph(GId);
      RET_ERROR(Res, "load: Failed to parse metadata."sv);
    }
  }

  LOG_INFO(GraphRef.EnableLog, "LLAMA_COMMIT {}"sv, LLAMA_COMMIT)
  LOG_INFO(GraphRef.EnableLog, "LLAMA_BUILD_NUMBER {}"sv, LLAMA_BUILD_NUMBER)

  LOG_DEBUG(GraphRef.EnableDebugLog, "load: handling model path."sv)
  const auto &Weight = Builders[0];
  const std::string_view BinModel(reinterpret_cast<const char *>(Weight.data()),
                                  Weight.size());

  if (BinModel.substr(0, 8) == "preload:"sv) {
    GraphRef.Params.model = std::string(BinModel.substr(8));
  } else {
    LOG_DEBUG(GraphRef.EnableDebugLog,
              "load: Model path not found in nn-preload, write model into "sv
              "a tmpfile."sv)
    GraphRef.Params.model = "bitnet-model.bin"sv;
    std::ofstream TempFile(GraphRef.Params.model,
                           std::ios::out | std::ios::binary | std::ios::trunc);
    if (!TempFile) {
      Env.deleteGraph(GId);
      RET_ERROR(ErrNo::InvalidArgument, "Failed to create temp model file."sv)
    }
    TempFile.write(BinModel.data(), BinModel.size());
    TempFile.close();
    LOG_DEBUG(GraphRef.EnableDebugLog,
              "load: Write model into a tmpfile...Done"sv)
  }
  LOG_DEBUG(GraphRef.EnableDebugLog, "load: handling model path...Done"sv)

  // Check if the model exists.
  if (!std::filesystem::exists(
          std::filesystem::u8path(GraphRef.Params.model))) {
    Env.deleteGraph(GId);
    RET_ERROR(ErrNo::ModelNotFound,
              "load: Model file not found at path: '{}'."sv,
              GraphRef.Params.model)
  }

  LOG_INFO(GraphRef.EnableLog, "load: Loading model from '{}'."sv,
           GraphRef.Params.model)

  // Initialize model parameters.
  LOG_DEBUG(GraphRef.EnableDebugLog,
            "load: initialize model with given parameters."sv)

  llama_backend_init();
  llama_numa_init(GraphRef.Params.numa);

  // Initialize the llama model and context.
  common_init_result LlamaInit = common_init_from_params(GraphRef.Params);
  GraphRef.LlamaModel.reset(LlamaInit.model);
  GraphRef.LlamaContext.reset(LlamaInit.context);

  if (GraphRef.LlamaModel == nullptr) {
    Env.deleteGraph(GId);
    RET_ERROR(ErrNo::InvalidArgument, "load: Unable to init model."sv)
  }
  if (GraphRef.LlamaContext == nullptr) {
    Env.deleteGraph(GId);
    RET_ERROR(ErrNo::InvalidArgument, "load: Unable to init context."sv)
  }

  LOG_DEBUG(GraphRef.EnableDebugLog,
            "load: initialize model with given parameters...Done"sv)

  // Store the loaded graph.
  GraphId = GId;
  Env.NNGraph[GId].setReady();

  LOG_DEBUG(GraphRef.EnableDebugLog, "load...Done"sv)
  return ErrNo::Success;
}

Expect<ErrNo> initExecCtx(WasiNNEnvironment &Env, uint32_t GraphId,
                          uint32_t &ContextId) noexcept {
  auto &GraphRef = Env.NNGraph[GraphId].get<Graph>();
  LOG_DEBUG(GraphRef.EnableDebugLog, "initExecCtx"sv)
  ContextId = Env.newContext(GraphId, Env.NNGraph[GraphId]);
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();

  LOG_INFO(GraphRef.EnableLog, "llama_system_info: {}"sv,
           llama_print_system_info())

  // Allocate the batch for input string prompt tokens.
  CxtRef.LlamaBatch = allocBatch(GraphRef.Params.n_batch);
  CxtRef.CurrentBatchSize = GraphRef.Params.n_batch;

  // Allocate the batch for single-token output sampling.
  CxtRef.OutputBatch = allocBatch(1);

  // Allocate the sampler
  CxtRef.LlamaSampler.reset(
      common_sampler_init(GraphRef.LlamaModel.get(), GraphRef.Params.sparams));

  Env.NNContext[ContextId].setReady();
  LOG_DEBUG(GraphRef.EnableDebugLog, "initExecCtx...Done"sv)
  return ErrNo::Success;
}

Expect<ErrNo> setInput(WasiNNEnvironment &Env, uint32_t ContextId,
                       uint32_t Index, const TensorData &Tensor) noexcept {
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  auto &GraphRef = Env.NNGraph[CxtRef.GraphId].get<Graph>();
  LOG_DEBUG(GraphRef.EnableDebugLog, "setInput"sv)

  // Handle Metadata at Index 1
  if (Index == 1) {
    LOG_DEBUG(GraphRef.EnableDebugLog, "setInput: found Metadata, processing"sv)
    bool IsModelUpdated = false;
    bool IsContextUpdated = false;
    bool IsSamplerUpdated = false;
    const std::string Metadata(
        reinterpret_cast<const char *>(Tensor.Tensor.data()),
        Tensor.Tensor.size());

    auto Res = parseMetadata(GraphRef, CxtRef.Conf, Metadata, &IsModelUpdated,
                             &IsContextUpdated, &IsSamplerUpdated);
    if (Res != ErrNo::Success) {
      RET_ERROR(Res, "setInput: failed to parse metadata."sv)
    }

    if (IsModelUpdated || GraphRef.LlamaModel == nullptr) {
      // The llama model may be nullptr if set_input with updated model params
      // last time. Therefore besides the model params updated, we should
      // reload the llama model if the model is nullptr.
      LOG_INFO(GraphRef.EnableLog,
               "setInput: Reloading model due to parameter change"sv)

      // Prepare model parameters for the reload.
      llama_model_params ModelParams = llama_model_default_params();
      ModelParams.n_gpu_layers =
          static_cast<int32_t>(GraphRef.Params.n_gpu_layers);
      ModelParams.main_gpu = static_cast<int32_t>(GraphRef.Params.main_gpu);

      // Free all resources that depend on the old model.
      GraphRef.LlamaModel.reset();

      // Due to the model change, the context and sampler should also be
      // reloaded. The new context and sampler will be created in the next
      // block.
      GraphRef.LlamaContext.reset();
      if (CxtRef.LlamaSampler) {
        CxtRef.LlamaSampler.reset();
        CxtRef.LlamaSampler = nullptr;
      }

      // Attempt to load the model from file with new parameters.
      GraphRef.LlamaModel.reset(llama_load_model_from_file(
          GraphRef.Params.model.c_str(), ModelParams));
      if (GraphRef.LlamaModel == nullptr) {
        Env.NNGraph[CxtRef.GraphId].setInvalid();
        RET_ERROR(ErrNo::InvalidArgument, "setInput: unable to init model."sv)
      }
    }

    // Reload context if its parameters changed OR if it was cleared by a model
    // reload.
    if (IsContextUpdated || GraphRef.LlamaContext == nullptr) {
      LOG_INFO(GraphRef.EnableLog,
               "setInput: Reloading llama context due to parameter change."sv)
      GraphRef.LlamaContext.reset();
      llama_context_params CtxParams =
          common_context_params_to_llama(GraphRef.Params);
      GraphRef.LlamaContext.reset(
          llama_new_context_with_model(GraphRef.LlamaModel.get(), CtxParams));
      if (GraphRef.LlamaContext == nullptr) {
        Env.NNGraph[CxtRef.GraphId].setInvalid();
        RET_ERROR(ErrNo::InvalidArgument, "setInput: unable to init context."sv)
      }
    }

    // Re-initialize sampler if its parameters changed OR if it was cleared.
    if (IsSamplerUpdated || CxtRef.LlamaSampler == nullptr) {
      LOG_INFO(GraphRef.EnableLog,
               "setInput: Re-initializing sampler due to parameter change."sv);
      CxtRef.LlamaSampler.reset(common_sampler_init(GraphRef.LlamaModel.get(),
                                                    GraphRef.Params.sparams));
      if (CxtRef.LlamaSampler == nullptr) {
        Env.NNGraph[CxtRef.GraphId].setInvalid();
        RET_ERROR(ErrNo::InvalidArgument, "setInput: unable to init sampler."sv)
      }
    }

    // Re-allocate batch if the batch size changed.
    if (CxtRef.CurrentBatchSize != GraphRef.Params.n_batch) {
      LOG_INFO(GraphRef.EnableLog,
               "Re-allocating batch due to n_batch change.");
      llama_batch_free(CxtRef.LlamaBatch);
      CxtRef.LlamaBatch = allocBatch(GraphRef.Params.n_batch);
      if (!CxtRef.LlamaBatch.token) {
        RET_ERROR(ErrNo::InvalidArgument, "Failed to re-allocate llama_batch.");
      }
      CxtRef.CurrentBatchSize = GraphRef.Params.n_batch;
    }

    Env.NNGraph[CxtRef.GraphId].setReady();
    LOG_DEBUG(GraphRef.EnableDebugLog, "setInput: metadata processing...Done");
    return ErrNo::Success;
  }

  if (Index != 0) {
    RET_ERROR(ErrNo::InvalidArgument,
              "Only prompt (index 0) and metadata (index 1) are supported.");
  }

  // Check the graph is valid after reloading during previous set_input.
  if (!Env.NNGraph[CxtRef.GraphId].isReady()) {
    RET_ERROR(
        ErrNo::InvalidArgument,
        "setInput: Graph is invalid. Please reload again by passing metadata "sv
        "in set_input or unload graph."sv)
  }

  LOG_DEBUG(GraphRef.EnableLog, "setInput: Clearing KV cache for new prompt."sv)
  llama_kv_cache_clear(GraphRef.LlamaContext.get());
  LOG_DEBUG(GraphRef.EnableLog,
            "setInput: Clearing KV cache for new prompt...done"sv)

  // Check tensor type.
  if (Tensor.RType != TensorType::U8) {
    RET_ERROR(ErrNo::InvalidArgument,
              "Input tensor must be a UTF-8 string (U8).");
  }

  // Tokenize the new prompt.
  const std::string Prompt(reinterpret_cast<const char *>(Tensor.Tensor.data()),
                           Tensor.Tensor.size());
  LOG_DEBUG(GraphRef.EnableDebugLog, "setInput: tokenize text prompt"sv)
  CxtRef.LlamaInputs =
      common_tokenize(GraphRef.LlamaContext.get(), Prompt,
                      llama_add_bos_token(GraphRef.LlamaModel.get()), true);
  LOG_DEBUG(GraphRef.EnableDebugLog, "setInput: tokenize text prompt...Done"sv)

  // Get the number of input tokens (for the metadata).
  CxtRef.LlamaNInputs = CxtRef.LlamaInputs.size();

  // Reset state for the compute loop.
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

  // Handle Metadata Output at Index 1
  if (Index == 1) {
    const std::string Metadata = buildOutputMetadata(CxtRef);
    const size_t BytesToCopy =
        std::min(static_cast<size_t>(OutBuffer.size()), Metadata.length());
    std::copy_n(Metadata.data(), BytesToCopy, OutBuffer.data());
    BytesWritten = static_cast<uint32_t>(Metadata.length());

    LOG_DEBUG(GraphRef.EnableDebugLog, "getOutput: Metadata (Index 1)...Done"sv)
    return ErrNo::Success;
  }

  const size_t BytesToCopy = std::min(static_cast<size_t>(OutBuffer.size()),
                                      CxtRef.LlamaOutputs.size());
  std::copy_n(CxtRef.LlamaOutputs.data(), BytesToCopy, OutBuffer.data());
  BytesWritten = CxtRef.LlamaOutputs.size();

  LOG_DEBUG(GraphRef.EnableDebugLog, "getOutput: Text (Index 0)...Done"sv)
  return ErrNo::Success;
}

Expect<ErrNo> compute(WasiNNEnvironment &Env, uint32_t ContextId) noexcept {
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  auto &GraphRef = Env.NNGraph[CxtRef.GraphId].get<Graph>();
  LOG_DEBUG(GraphRef.EnableDebugLog, "compute");

  // Clear the context and reset the sampler.
  clearContext(GraphRef, CxtRef);

  if (GraphRef.Params.embedding) {
    return getEmbedding(GraphRef, CxtRef);
  }

  // Evaluate the input tokens.
  auto ReturnCode = evaluateInput(GraphRef, CxtRef, "compute"sv);
  if (ReturnCode != ErrNo::Success) {
    return ReturnCode;
  }

  // Main prediction loop.
  LOG_DEBUG(GraphRef.EnableDebugLog, "compute: enter main prediction loop"sv)
  int64_t NPredict = CxtRef.Conf.NPredict;
  if (NPredict < 0) {
    NPredict = INT32_MAX;
  }

  while (NPredict > 0) {
    ReturnCode = sampleOutput(GraphRef, CxtRef);
    if (ReturnCode != ErrNo::Success) {
      break;
    }
    NPredict--;
  }

  if (ReturnCode == ErrNo::EndOfSequence || ReturnCode == ErrNo::ContextFull) {
    LOG_INFO(GraphRef.EnableLog, "compute finished with status: {}."sv,
             static_cast<uint32_t>(ReturnCode))
    return ErrNo::Success;
  }

  LOG_DEBUG(GraphRef.EnableDebugLog,
            "compute: enter main prediction loop...Done"sv)

  if (GraphRef.EnableLog) {
    common_perf_print(GraphRef.LlamaContext.get(), CxtRef.LlamaSampler.get());
  }

  LOG_DEBUG(GraphRef.EnableDebugLog, "compute...Done")
  return ReturnCode;
}

Expect<ErrNo> getOutputSingle(WasiNNEnvironment &Env, uint32_t ContextId,
                              uint32_t Index, Span<uint8_t> OutBuffer,
                              uint32_t &BytesWritten) noexcept {
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  auto &GraphRef = Env.NNGraph[CxtRef.GraphId].get<Graph>();
  LOG_DEBUG(GraphRef.EnableDebugLog, "getOutputSingle: with Index {}"sv, Index)

  // Metadata Output at Index 1
  if (Index == 1) {
    const std::string Metadata = buildOutputMetadata(CxtRef);
    const size_t BytesToCopy =
        std::min(static_cast<size_t>(OutBuffer.size()), Metadata.length());
    std::copy_n(Metadata.data(), BytesToCopy, OutBuffer.data());
    BytesWritten = static_cast<uint32_t>(Metadata.length());

    LOG_DEBUG(GraphRef.EnableDebugLog,
              "getOutputSingle: Metadata (Index 1)...Done"sv)
    return ErrNo::Success;
  }

  if (CxtRef.LlamaOutputTokens.empty()) {
    BytesWritten = 0;
    return ErrNo::Success;
  }

  const std::string LastTokenStr = common_token_to_piece(
      GraphRef.LlamaContext.get(), CxtRef.LlamaOutputTokens.back());

  const size_t BytesToCopy =
      std::min(static_cast<size_t>(OutBuffer.size()), LastTokenStr.length());
  std::copy_n(LastTokenStr.data(), BytesToCopy, OutBuffer.data());
  BytesWritten = LastTokenStr.length();

  LOG_DEBUG(GraphRef.EnableDebugLog, "getOutputSingle: Text (Index 0)...Done"sv)
  return ErrNo::Success;
}

Expect<ErrNo> computeSingle(WasiNNEnvironment &Env,
                            uint32_t ContextId) noexcept {
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  auto &GraphRef = Env.NNGraph[CxtRef.GraphId].get<Graph>();
  LOG_DEBUG(GraphRef.EnableDebugLog, "computeSingle"sv)

  auto ReturnCode = ErrNo::Success;
  if (!CxtRef.ComputeSingleStarted) {
    // Clear the context and reset the sampler.
    clearContext(GraphRef, CxtRef);
    ReturnCode = evaluateInput(GraphRef, CxtRef, "computeSingle"sv);
    if (ReturnCode != ErrNo::Success) {
      return ReturnCode;
    }

    CxtRef.ComputeSingleStarted = true;
  }

  // Main prediction process.
  LOG_DEBUG(GraphRef.EnableDebugLog,
            "computeSingle: enter main prediction process"sv)
  ReturnCode = sampleOutput(GraphRef, CxtRef, /* IsSingleTokenMode */ true);
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
  LOG_DEBUG(GraphRef.EnableDebugLog, "finiSingle");

  if (GraphRef.EnableLog) {
    common_perf_print(GraphRef.LlamaContext.get(), CxtRef.LlamaSampler.get());
  }

  // Reset the llama sampler.
  common_sampler_reset(CxtRef.LlamaSampler.get());

  // Clear the outputs.
  LOG_DEBUG(GraphRef.EnableDebugLog,
            "finiSingle: clear the previous output and tokens"sv)
  CxtRef.LlamaOutputs.clear();
  CxtRef.LlamaOutputTokens.clear();
  LOG_DEBUG(GraphRef.EnableDebugLog,
            "finiSingle: clear the previous output and tokens...Done"sv)

  CxtRef.NPos = 0;
  CxtRef.ComputeSingleStarted = false;

  LOG_DEBUG(GraphRef.EnableDebugLog, "finiSingle...Done"sv)
  return ErrNo::Success;
}

Expect<ErrNo> unload(WasiNNEnvironment &Env, uint32_t GraphId) noexcept {

  auto &GraphRef = Env.NNGraph[GraphId].get<Graph>();
  if (GraphId >= Env.NNGraph.size() || GraphRef.LlamaModel == nullptr) {
    return ErrNo::Success;
  }

  LOG_DEBUG(GraphRef.EnableDebugLog, "unload"sv)

  Env.deleteGraph(GraphId);

  LOG_DEBUG(GraphRef.EnableDebugLog, "unload...Done"sv)
  return ErrNo::Success;
}

Expect<ErrNo> finalizeExecCtx(WasiNNEnvironment &Env,
                              uint32_t ContextId) noexcept {

  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  auto &GraphRef = Env.NNGraph[CxtRef.GraphId].get<Graph>();
  LOG_DEBUG(GraphRef.EnableDebugLog, "finalizeExecCtx"sv)

  CxtRef.LlamaSampler.reset();
  llama_batch_free(CxtRef.LlamaBatch);
  llama_batch_free(CxtRef.OutputBatch);
  Env.deleteContext(ContextId);

  LOG_DEBUG(GraphRef.EnableDebugLog, "finalizeExecCtx...Done"sv)
  return ErrNo::Success;
}

#else
namespace {
Expect<ErrNo> reportBackendNotSupported() noexcept {
  spdlog::error("[WASI-NN] BitNet backend is not built. Please build with "
                "-DWASMEDGE_PLUGIN_WASI_NN_BACKEND_BITNET=ON."sv);
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
} // namespace WasmEdge::Host::WASINN::BitNet