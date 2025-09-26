// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "wasinn_ggml.h"
#include "common/types.h"
#include "host/wasi/vfs_io.h"
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

// JSON parsing helper template
template <typename T>
T getJsonValue(const simdjson::dom::element &Doc, std::string_view Key) {
  if (Doc.at_key(Key).error() == simdjson::SUCCESS) {
    T Value{};
    auto Err = Doc[Key].get<T>().get(Value);
    if (Err) {
      std::string Msg = fmt::format("Unable to retrieve the {} option.", Key);
      spdlog::error("[WASI-NN] GGML backend: {}", Msg);
      throw ErrNo(ErrNo::InvalidArgument);
    }
    return Value;
  }
  throw ErrNo(ErrNo::NotFound);
}

template <typename T>
void parseJsonAuto(const simdjson::dom::element &Doc, std::string_view Key,
                   T &Var) {
  try {
    auto Result = getJsonValue<T>(Doc, Key);
    Var = Result;
  } catch (ErrNo E) {
    if (E != ErrNo::NotFound) {
      throw E;
    }
  }
}

template <typename FromType, typename ToType>
void parseJsonWithCastAuto(const simdjson::dom::element &Doc,
                           std::string_view Key, ToType &Var) {
  try {
    auto Result = getJsonValue<FromType>(Doc, Key);
    Var = static_cast<ToType>(Result);
  } catch (ErrNo E) {
    if (E != ErrNo::NotFound) {
      throw E;
    }
  }
}

template <typename T, typename Processor>
void parseJsonWithProcessorAuto(const simdjson::dom::element &Doc,
                                std::string_view Key, Processor Proc) {
  try {
    auto Result = getJsonValue<T>(Doc, Key);
    if (!Proc(Result)) {
      throw ErrNo{ErrNo::InvalidArgument};
    }
  } catch (ErrNo E) {
    if (E != ErrNo::NotFound) {
      throw E;
    }
  }
}

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

  try {
    parseJsonAuto(Doc, "enable-log", GraphRef.EnableLog);
    parseJsonAuto(Doc, "enable-debug-log", GraphRef.EnableDebugLog);

    parseJsonWithCastAuto<int64_t>(Doc, "main-gpu", GraphRef.Params.main_gpu);
    parseJsonWithCastAuto<int64_t>(Doc, "n-gpu-layers",
                                   GraphRef.Params.n_gpu_layers);

    parseJsonWithProcessorAuto<bool>(Doc, "cpu-moe",
                                     [&GraphRef](const bool &CpuMoe) -> bool {
                                       if (CpuMoe) {
                                         GraphRef.TensorBuftOverrides.push_back(
                                             "\\.ffn_(up|down|gate)_exps");
                                       }
                                       return true;
                                     });

    parseJsonWithProcessorAuto<int64_t>(
        Doc, "n-cpu-moe", [&GraphRef](const int64_t &NCpuMoe) -> bool {
          if (NCpuMoe < 0) {
            spdlog::error("[WASI-NN] GGML backend: Invalid n-cpu-moe value.");
            return false;
          }
          for (int I = 0; I < NCpuMoe; I++) {
            GraphRef.TensorBuftOverrides.push_back(
                string_format("blk\\.%d\\.ffn_(up|down|gate)_exps", I));
          }
          return true;
        });
    parseJsonWithProcessorAuto<std::string_view>(
        Doc, "tensor-split", [&GraphRef](const std::string_view &TSV) -> bool {
          // The TensorSplit is a comma-separated list of non-negative values.
          // E.g., "3,2" presents 60% of the data to GPU 0 and 40% to GPU 1.
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
            spdlog::error(
                "[WASI-NN] GGML backend: Number of Tensor-Split is larger than "
                "MaxDevices, please reduce the size of tensor-split.");
            return false;
          }
          for (size_t Idx = TensorSplitSize; Idx < NDevices; Idx++) {
            GraphRef.Params.tensor_split[TensorSplitSize++] = 0.0f;
          }
          return true;
        });
    parseJsonAuto<bool>(Doc, "embedding", GraphRef.Params.embedding);
    parseJsonWithProcessorAuto<std::string_view>(
        Doc, "split-mode",
        [&GraphRef](const std::string_view &SplitMode) -> bool {
          if (SplitMode == "none"sv) {
            GraphRef.Params.split_mode = LLAMA_SPLIT_MODE_NONE;
          } else if (SplitMode == "layer"sv) {
            GraphRef.Params.split_mode = LLAMA_SPLIT_MODE_LAYER;
          } else if (SplitMode == "row"sv) {
            GraphRef.Params.split_mode = LLAMA_SPLIT_MODE_ROW;
          } else {
            spdlog::error("[WASI-NN] GGML backend: Unknown split-mode: "
                          "{}. Valid: none, layer, row.",
                          SplitMode);
            return false;
          }
          return true;
        });
    parseJsonWithCastAuto<std::string_view>(Doc, "mmproj",
                                            GraphRef.Params.mmproj.path);
    // The TTS parameters using macros
    parseJsonAuto<bool>(Doc, "tts", GraphRef.TextToSpeech);
    parseJsonWithCastAuto<std::string_view>(Doc, "model-vocoder",
                                            GraphRef.Params.vocoder.model.path);
    parseJsonWithCastAuto<std::string_view>(Doc, "tts-output-file",
                                            GraphRef.TTSOutputFilePath);

    parseJsonWithCastAuto<std::string_view>(Doc, "tts-speaker-file",
                                            GraphRef.TTSSpeakerFilePath);

    // The context parameters
    parseJsonWithCastAuto<int64_t>(Doc, "ctx-size", GraphRef.Params.n_ctx);
    parseJsonWithCastAuto<int64_t>(Doc, "batch-size", GraphRef.Params.n_batch);
    parseJsonWithCastAuto<int64_t>(Doc, "ubatch-size",
                                   GraphRef.Params.n_ubatch);
    parseJsonWithCastAuto<int64_t>(Doc, "n-keep", GraphRef.Params.n_keep);
    parseJsonWithCastAuto<int64_t>(Doc, "n-chunks", GraphRef.Params.n_chunks);
    parseJsonWithCastAuto<int64_t>(Doc, "n-parallel",
                                   GraphRef.Params.n_parallel);
    parseJsonWithCastAuto<int64_t>(Doc, "n-sequences",
                                   GraphRef.Params.n_sequences);
    parseJsonWithCastAuto<int64_t>(Doc, "grp-attn-n",
                                   GraphRef.Params.grp_attn_n);
    parseJsonWithCastAuto<int64_t>(Doc, "grp-attn-w",
                                   GraphRef.Params.grp_attn_w);
    parseJsonWithCastAuto<int64_t>(Doc, "n-print", GraphRef.Params.n_print);
    parseJsonWithCastAuto<double>(Doc, "rope-freq-base",
                                  GraphRef.Params.rope_freq_base);
    parseJsonWithCastAuto<double>(Doc, "rope-freq-scale",
                                  GraphRef.Params.rope_freq_scale);
    parseJsonWithCastAuto<double>(Doc, "yarn-ext-factor",
                                  GraphRef.Params.yarn_ext_factor);
    parseJsonWithCastAuto<double>(Doc, "yarn-attn-factor",
                                  GraphRef.Params.yarn_attn_factor);
    parseJsonWithCastAuto<double>(Doc, "yarn-beta-fast",
                                  GraphRef.Params.yarn_beta_fast);
    parseJsonWithCastAuto<double>(Doc, "yarn-beta-slow",
                                  GraphRef.Params.yarn_beta_slow);
    parseJsonWithCastAuto<int64_t>(Doc, "yarn-orig-ctx",
                                   GraphRef.Params.yarn_orig_ctx);
    parseJsonAuto<bool>(Doc, "mask-valid",
                        GraphRef.Params.cpuparams.mask_valid);
    parseJsonWithCastAuto<int64_t>(Doc, "priority",
                                   GraphRef.Params.cpuparams.priority);
    parseJsonAuto<bool>(Doc, "strict-cpu",
                        GraphRef.Params.cpuparams.strict_cpu);
    parseJsonWithCastAuto<int64_t>(Doc, "poll", GraphRef.Params.cpuparams.poll);
    parseJsonAuto<bool>(Doc, "mask-valid-batch",
                        GraphRef.Params.cpuparams_batch.mask_valid);
    parseJsonWithProcessorAuto<int64_t>(
        Doc, "priority-batch", [&GraphRef](const int64_t &Priority) -> bool {
          GraphRef.Params.cpuparams_batch.priority =
              static_cast<ggml_sched_priority>(Priority);
          return true;
        });
    parseJsonAuto<bool>(Doc, "strict-cpu-batch",
                        GraphRef.Params.cpuparams_batch.strict_cpu);
    parseJsonWithCastAuto<int64_t>(Doc, "poll-batch",
                                   GraphRef.Params.cpuparams_batch.poll);

    parseJsonWithCastAuto<int64_t>(Doc, "numa", GraphRef.Params.numa);

    parseJsonWithCastAuto<int64_t>(Doc, "rope-scaling-type",
                                   GraphRef.Params.rope_scaling_type);
    parseJsonWithCastAuto<int64_t>(Doc, "pooling-type",
                                   GraphRef.Params.pooling_type);
    parseJsonWithCastAuto<int64_t>(Doc, "attention-type",
                                   GraphRef.Params.attention_type);
    parseJsonWithProcessorAuto<int64_t>(
        Doc, "threads", [&GraphRef](const int64_t &NThreads) -> bool {
          GraphRef.Params.cpuparams.n_threads = static_cast<int32_t>(NThreads);
          return true;
        });
    parseJsonWithCastAuto<int64_t>(Doc, "threads-batch",
                                   GraphRef.Params.cpuparams_batch.n_threads);
    parseJsonWithCastAuto<int64_t>(Doc, "n-prev",
                                   GraphRef.Params.sampling.n_prev);
    parseJsonWithCastAuto<int64_t>(Doc, "n-probs",
                                   GraphRef.Params.sampling.n_probs);
    parseJsonWithCastAuto<int64_t>(Doc, "min-keep",
                                   GraphRef.Params.sampling.min_keep);
    parseJsonWithCastAuto<int64_t>(Doc, "top-k",
                                   GraphRef.Params.sampling.top_k);
    parseJsonWithCastAuto<double>(Doc, "min-p", GraphRef.Params.sampling.min_p);
    parseJsonWithCastAuto<double>(Doc, "xtc-probability",
                                  GraphRef.Params.sampling.xtc_probability);
    parseJsonWithCastAuto<double>(Doc, "xtc-threshold",
                                  GraphRef.Params.sampling.xtc_threshold);
    parseJsonWithCastAuto<double>(Doc, "typ-p", GraphRef.Params.sampling.typ_p);
    parseJsonWithCastAuto<double>(Doc, "dynatemp-range",
                                  GraphRef.Params.sampling.dynatemp_range);
    parseJsonWithCastAuto<double>(Doc, "dynatemp-exponent",
                                  GraphRef.Params.sampling.dynatemp_exponent);
    parseJsonWithCastAuto<int64_t>(Doc, "last-n-penalty",
                                   GraphRef.Params.sampling.penalty_last_n);
    parseJsonWithProcessorAuto<double>(
        Doc, "temp", [&GraphRef](const double &Temp) -> bool {
          GraphRef.Params.sampling.temp =
              static_cast<float>(std::max(0.0, Temp));
          return true;
        });

    parseJsonWithProcessorAuto<double>(
        Doc, "top-p", [&GraphRef](const double &TopP) -> bool {
          GraphRef.Params.sampling.top_p =
              static_cast<float>(std::max(0.0, TopP));
          return true;
        });
    parseJsonWithProcessorAuto<double>(
        Doc, "repeat-penalty",
        [&GraphRef](const double &RepeatPenalty) -> bool {
          GraphRef.Params.sampling.penalty_repeat =
              static_cast<float>(std::max(0.0, RepeatPenalty));
          return true;
        });
    parseJsonWithProcessorAuto<double>(
        Doc, "presence-penalty",
        [&GraphRef](const double &PresencePenalty) -> bool {
          GraphRef.Params.sampling.penalty_present =
              static_cast<float>(std::max(0.0, PresencePenalty));
          return true;
        });
    parseJsonWithProcessorAuto<double>(
        Doc, "frequency-penalty",
        [&GraphRef](const double &FrequencyPenalty) -> bool {
          GraphRef.Params.sampling.penalty_freq =
              static_cast<float>(std::max(0.0, FrequencyPenalty));
          return true;
        });
    parseJsonWithCastAuto<double>(Doc, "dry-multipier",
                                  GraphRef.Params.sampling.dry_multiplier);
    parseJsonWithCastAuto<double>(Doc, "dry-base",
                                  GraphRef.Params.sampling.dry_base);
    parseJsonWithCastAuto<int64_t>(Doc, "dry-allowed-length",
                                   GraphRef.Params.sampling.dry_allowed_length);
    parseJsonWithCastAuto<int64_t>(Doc, "dry-last-n-penalty",
                                   GraphRef.Params.sampling.penalty_last_n);
    parseJsonWithCastAuto<int64_t>(Doc, "mirostat",
                                   GraphRef.Params.sampling.mirostat);
    parseJsonWithCastAuto<double>(Doc, "mirostat-eta",
                                  GraphRef.Params.sampling.mirostat_eta);
    parseJsonAuto<bool>(Doc, "ignore-eos", GraphRef.Params.sampling.ignore_eos);
    parseJsonAuto<bool>(Doc, "no-perf-sampling",
                        GraphRef.Params.sampling.no_perf);
    parseJsonAuto<bool>(Doc, "timing-per-token",
                        GraphRef.Params.sampling.timing_per_token);

    parseJsonWithCastAuto<std::string_view>(Doc, "grammar",
                                            GraphRef.Params.sampling.grammar);
    parseJsonWithProcessorAuto<std::string_view>(
        Doc, "json-schema",
        [&GraphRef](const std::string_view &JsonSchema) -> bool {
          GraphRef.Params.sampling.grammar =
              json_schema_to_grammar(nlohmann::ordered_json::parse(JsonSchema));
          return true;
        });
    parseJsonWithCastAuto<uint64_t>(Doc, "seed", GraphRef.Params.sampling.seed);

    // The speculative parameters.
    parseJsonWithCastAuto<int64_t>(Doc, "n-ctx-speculative",
                                   GraphRef.Params.speculative.n_ctx);
    parseJsonWithCastAuto<int64_t>(Doc, "n-max-speculative",
                                   GraphRef.Params.speculative.n_max);
    parseJsonWithCastAuto<int64_t>(Doc, "n-min-speculative",
                                   GraphRef.Params.speculative.n_min);
    parseJsonWithCastAuto<int64_t>(Doc, "n-gpu-layers-speculative",
                                   GraphRef.Params.speculative.n_gpu_layers);
    parseJsonWithCastAuto<double>(Doc, "p-split-speculative",
                                  GraphRef.Params.speculative.p_split);
    parseJsonWithCastAuto<double>(Doc, "p-min-speculative",
                                  GraphRef.Params.speculative.p_min);
    // The vocoder parameters.
    parseJsonWithCastAuto<std::string_view>(
        Doc, "hf-repo-vocoder", GraphRef.Params.vocoder.model.hf_repo);
    parseJsonWithCastAuto<std::string_view>(
        Doc, "hf-file-vocoder", GraphRef.Params.vocoder.model.hf_file);
    parseJsonWithCastAuto<std::string_view>(Doc, "model-url-vocoder",
                                            GraphRef.Params.vocoder.model.url);

    // The config parameters.
    parseJsonAuto<bool>(Doc, "stream-stdout", ConfRef.StreamStdout);
    parseJsonAuto<int64_t>(Doc, "n-predict", ConfRef.NPredict);
    parseJsonWithCastAuto<std::string_view>(Doc, "reverse-prompt",
                                            ConfRef.ReversePrompt);
    parseJsonWithCastAuto<std::string_view>(Doc, "image", ConfRef.ImagePath);
    parseJsonAuto<bool>(Doc, "always-regenerate-image-embd",
                        ConfRef.AlwaysRegenerateImageEmbd);
    parseJsonWithCastAuto<std::string_view>(Doc, "model-alias",
                                            GraphRef.Params.model_alias);
    parseJsonWithCastAuto<std::string_view>(Doc, "model-url",
                                            GraphRef.Params.model.url);
    parseJsonWithCastAuto<std::string_view>(Doc, "hf-token",
                                            GraphRef.Params.hf_token);
    parseJsonWithCastAuto<std::string_view>(Doc, "hf-repo",
                                            GraphRef.Params.model.hf_repo);
    parseJsonWithCastAuto<std::string_view>(Doc, "hf-file",
                                            GraphRef.Params.model.hf_file);
    parseJsonWithCastAuto<std::string_view>(Doc, "prompt-file",
                                            GraphRef.Params.prompt_file);
    parseJsonWithCastAuto<std::string_view>(Doc, "path-prompt-cache",
                                            GraphRef.Params.path_prompt_cache);
    parseJsonWithCastAuto<std::string_view>(Doc, "input-prefix",
                                            GraphRef.Params.input_prefix);
    parseJsonWithCastAuto<std::string_view>(Doc, "input-suffix",
                                            GraphRef.Params.input_suffix);
    parseJsonWithCastAuto<std::string_view>(
        Doc, "lookup-cache-static", GraphRef.Params.lookup_cache_static);
    parseJsonWithCastAuto<std::string_view>(
        Doc, "lookup-cache-dynamic", GraphRef.Params.lookup_cache_dynamic);
    parseJsonWithCastAuto<std::string_view>(Doc, "logits-file",
                                            GraphRef.Params.logits_file);
    parseJsonAuto<bool>(Doc, "lora-init-without-apply",
                        GraphRef.Params.lora_init_without_apply);
    parseJsonWithCastAuto<int64_t>(Doc, "verbosity", GraphRef.Params.verbosity);
    parseJsonWithCastAuto<int64_t>(Doc, "control-vector-layer-start",
                                   GraphRef.Params.control_vector_layer_start);
    parseJsonWithCastAuto<int64_t>(Doc, "control-vector-layer-end",
                                   GraphRef.Params.control_vector_layer_end);
    parseJsonWithCastAuto<int64_t>(Doc, "ppl-stride",
                                   GraphRef.Params.ppl_stride);
    parseJsonWithCastAuto<int64_t>(Doc, "ppl-output-type",
                                   GraphRef.Params.ppl_output_type);
    parseJsonAuto<bool>(Doc, "hellaswag", GraphRef.Params.hellaswag);
    parseJsonWithCastAuto<uint64_t, uint64_t>(Doc, "hellaswag-tasks",
                                              GraphRef.Params.hellaswag_tasks);
    parseJsonAuto<bool>(Doc, "winogrande", GraphRef.Params.winogrande);
    parseJsonWithCastAuto<uint64_t, uint64_t>(Doc, "winogrande-tasks",
                                              GraphRef.Params.winogrande_tasks);
    parseJsonAuto<bool>(Doc, "multiple-choice",
                        GraphRef.Params.multiple_choice);
    parseJsonWithCastAuto<uint64_t, uint64_t>(
        Doc, "multiple-choice-tasks", GraphRef.Params.multiple_choice_tasks);
    parseJsonAuto<bool>(Doc, "kl-divergence", GraphRef.Params.kl_divergence);
    parseJsonAuto<bool>(Doc, "usage", GraphRef.Params.usage);
    parseJsonAuto<bool>(Doc, "use-color", GraphRef.Params.use_color);
    parseJsonAuto<bool>(Doc, "special", GraphRef.Params.special);
    parseJsonAuto<bool>(Doc, "interactive", GraphRef.Params.interactive);
    parseJsonAuto<bool>(Doc, "interactive-first",
                        GraphRef.Params.interactive_first);
    parseJsonAuto<bool>(Doc, "prompt-cache-all",
                        GraphRef.Params.prompt_cache_all);
    parseJsonAuto<bool>(Doc, "prompt-cache-ro",
                        GraphRef.Params.prompt_cache_ro);
    parseJsonAuto<bool>(Doc, "escape", GraphRef.Params.escape);
    parseJsonAuto<bool>(Doc, "multiline-input",
                        GraphRef.Params.multiline_input);
    parseJsonAuto<bool>(Doc, "simple-io", GraphRef.Params.simple_io);
    parseJsonAuto<bool>(Doc, "cont-batching", GraphRef.Params.cont_batching);
    parseJsonWithProcessorAuto<std::string_view>(
        Doc, "flash-attn",
        [&GraphRef](const std::string_view &FlashAttn) -> bool {
          if (FlashAttn == "on"sv || FlashAttn == "enabled"sv) {
            GraphRef.Params.flash_attn_type = LLAMA_FLASH_ATTN_TYPE_ENABLED;
          } else if (FlashAttn == "off"sv || FlashAttn == "disabled"sv) {
            GraphRef.Params.flash_attn_type = LLAMA_FLASH_ATTN_TYPE_DISABLED;
          } else if (FlashAttn == "auto"sv) {
            GraphRef.Params.flash_attn_type = LLAMA_FLASH_ATTN_TYPE_AUTO;
          } else {
            spdlog::error(
                "[WASI-NN] GGML backend: The flash-attn option must be "
                "one of: on, off, auto.");
            return false;
          }
          return true;
        });
    parseJsonAuto<bool>(Doc, "no-perf", GraphRef.Params.no_perf);
    parseJsonAuto<bool>(Doc, "ctx-shift", GraphRef.Params.ctx_shift);
    parseJsonAuto<bool>(Doc, "input-prefix-bos",
                        GraphRef.Params.input_prefix_bos);
    parseJsonAuto<bool>(Doc, "use-mlock", GraphRef.Params.use_mlock);
    parseJsonAuto<bool>(Doc, "use-mmap", GraphRef.Params.use_mmap);
    parseJsonAuto<bool>(Doc, "verbose-prompt", GraphRef.Params.verbose_prompt);
    parseJsonAuto<bool>(Doc, "display-prompt", GraphRef.Params.display_prompt);
    parseJsonAuto<bool>(Doc, "no-kv-offload", GraphRef.Params.no_kv_offload);
    parseJsonAuto<bool>(Doc, "warmup", GraphRef.Params.warmup);
    parseJsonAuto<bool>(Doc, "check-tensors", GraphRef.Params.check_tensors);
    parseJsonWithCastAuto<int64_t>(Doc, "cache-type-k",
                                   GraphRef.Params.cache_type_k);
    parseJsonWithCastAuto<int64_t>(Doc, "cache-type-v",
                                   GraphRef.Params.cache_type_v);

    parseJsonWithCastAuto<int64_t>(Doc, "embd-normalize",
                                   GraphRef.Params.embd_normalize);
    parseJsonWithCastAuto<std::string_view>(Doc, "embd-out",
                                            GraphRef.Params.embd_out);

    parseJsonWithCastAuto<std::string_view>(Doc, "embd-sep",
                                            GraphRef.Params.embd_sep);
    parseJsonWithProcessorAuto<bool>(
        Doc, "reranking", [&GraphRef](const bool &) -> bool {
          GraphRef.Params.embedding = true;
          GraphRef.Params.pooling_type = LLAMA_POOLING_TYPE_RANK;
          return true;
        });
    parseJsonWithCastAuto<int64_t>(Doc, "port", GraphRef.Params.port);
    parseJsonWithCastAuto<int64_t>(Doc, "timeout-read",
                                   GraphRef.Params.timeout_read);
    parseJsonWithCastAuto<int64_t>(Doc, "timeout-write",
                                   GraphRef.Params.timeout_write);
    parseJsonWithCastAuto<int64_t>(Doc, "n-threads-http",
                                   GraphRef.Params.n_threads_http);
    parseJsonWithCastAuto<int64_t>(Doc, "n-cache-reuse",
                                   GraphRef.Params.n_cache_reuse);
    parseJsonWithCastAuto<std::string_view>(Doc, "hostname",
                                            GraphRef.Params.hostname);
    parseJsonWithCastAuto<std::string_view>(Doc, "public-path",
                                            GraphRef.Params.public_path);
    parseJsonWithCastAuto<std::string_view>(Doc, "chat-template",
                                            GraphRef.Params.chat_template);
    parseJsonAuto<bool>(Doc, "enable-chat-template",
                        GraphRef.Params.enable_chat_template);
    parseJsonWithCastAuto<std::string_view>(Doc, "ssl-file-key",
                                            GraphRef.Params.ssl_file_key);
    parseJsonWithCastAuto<std::string_view>(Doc, "ssl-file-cert",
                                            GraphRef.Params.ssl_file_cert);
    parseJsonAuto<bool>(Doc, "webui", GraphRef.Params.webui);
    parseJsonAuto<bool>(Doc, "endpoint-slots", GraphRef.Params.endpoint_slots);
    parseJsonAuto<bool>(Doc, "endpoint-props", GraphRef.Params.endpoint_props);
    parseJsonAuto<bool>(Doc, "endpoint-metrics",
                        GraphRef.Params.endpoint_metrics);
    parseJsonAuto<bool>(Doc, "log-json", GraphRef.Params.log_json);

    // Slot parameters
    parseJsonWithCastAuto<std::string_view>(Doc, "slot-save-path",
                                            GraphRef.Params.slot_save_path);

    parseJsonWithCastAuto<double>(Doc, "slot-prompt-similarity",
                                  GraphRef.Params.slot_prompt_similarity);
    parseJsonAuto<bool>(Doc, "is-pp-shared", GraphRef.Params.is_pp_shared);
    parseJsonWithProcessorAuto<int64_t>(
        Doc, "n-pp", [&GraphRef](const int64_t &NPP) -> bool {
          GraphRef.Params.n_pp.emplace_back(NPP);
          return true;
        });
    parseJsonWithProcessorAuto<int64_t>(
        Doc, "n-tg", [&GraphRef](const int64_t &NTG) -> bool {
          GraphRef.Params.n_tg.emplace_back(NTG);
          return true;
        });
    parseJsonWithProcessorAuto<int64_t>(
        Doc, "n-pl", [&GraphRef](const int64_t &NPL) -> bool {
          GraphRef.Params.n_pl.emplace_back(NPL);
          return true;
        });
    parseJsonWithProcessorAuto<std::string_view>(
        Doc, "context-files",
        [&GraphRef](const std::string_view &ContextFile) -> bool {
          GraphRef.Params.context_files.emplace_back(ContextFile);
          return true;
        });
    parseJsonWithCastAuto<int64_t>(Doc, "chunk-size",
                                   GraphRef.Params.chunk_size);
    parseJsonWithCastAuto<std::string_view>(Doc, "chunk-separator",
                                            GraphRef.Params.chunk_separator);
    parseJsonWithCastAuto<int64_t>(Doc, "n-junk", GraphRef.Params.n_junk);
    parseJsonWithCastAuto<int64_t>(Doc, "i-pos", GraphRef.Params.i_pos);
    parseJsonWithCastAuto<std::string_view>(Doc, "out-file",
                                            GraphRef.Params.out_file);
    parseJsonWithCastAuto<int64_t>(Doc, "n-out-freq",
                                   GraphRef.Params.n_out_freq);
    parseJsonWithCastAuto<int64_t>(Doc, "n-save-freq",
                                   GraphRef.Params.n_save_freq);
    parseJsonWithCastAuto<int64_t>(Doc, "i-chunk", GraphRef.Params.i_chunk);
    parseJsonAuto<bool>(Doc, "process-output", GraphRef.Params.process_output);
    parseJsonAuto<bool>(Doc, "compute-ppl", GraphRef.Params.compute_ppl);
    parseJsonWithCastAuto<int64_t>(Doc, "n-pca-batch",
                                   GraphRef.Params.n_pca_batch);
    parseJsonWithCastAuto<int64_t>(Doc, "n-pca-iterations",
                                   GraphRef.Params.n_pca_iterations);
    parseJsonWithProcessorAuto<std::string_view>(
        Doc, "cvector-dimre-method",
        [&GraphRef](const std::string_view &Method) -> bool {
          if (Method == "DIMRE_METHOD_PCA") {
            GraphRef.Params.cvector_dimre_method =
                dimre_method::DIMRE_METHOD_PCA;
            return true;
          }
          if (Method == "DIMRE_METHOD_MEAN") {
            GraphRef.Params.cvector_dimre_method =
                dimre_method::DIMRE_METHOD_MEAN;
            return true;
          }
          return false;
        });
    parseJsonWithCastAuto<std::string_view>(
        Doc, "cvector-positive-file", GraphRef.Params.cvector_positive_file);
    parseJsonWithCastAuto<std::string_view>(
        Doc, "cvector-negative-file", GraphRef.Params.cvector_negative_file);
    parseJsonAuto<bool>(Doc, "spm-infill", GraphRef.Params.spm_infill);
    parseJsonWithCastAuto<std::string_view>(Doc, "out-file",
                                            GraphRef.Params.out_file);
    parseJsonAuto<bool>(Doc, "batched-bench-output-jsonl",
                        GraphRef.Params.batched_bench_output_jsonl);
  } catch (const ErrNo &Error) {
    return Error;
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
getSpeakerProfileFromFile(const std::string &FilePath, WasiNNEnvironment &Env) {
  WasmEdge::FStream::IFStream JsonFile(FilePath, Env.getEnv());
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

std::vector<llama_token> processTTSPrompt(WasiNNEnvironment &Env,
                                          Graph &GraphRef,
                                          std::string &Prompt) noexcept {
  // Use the custom speaker profile if available.
  TTSSpeakerProfile SpeakerProfile = TTSDefaultSpeakerProfile;
  if (!GraphRef.TTSSpeakerFilePath.empty()) {
    std::optional<TTSSpeakerProfile> SpeakerProfileOpt =
        getSpeakerProfileFromFile(GraphRef.TTSSpeakerFilePath, Env);
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
ErrNo codesToSpeech(WasiNNEnvironment &Env, Graph &GraphRef,
                    Context &CxtRef) noexcept {
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
    WasmEdge::FStream::OFStream File(GraphRef.TTSOutputFilePath, Env.getEnv());
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
    WasmEdge::FStream::OFStream TempFile(GraphRef.Params.model.path,
                                         Env.getEnv());
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
    ReturnCode = codesToSpeech(Env, GraphRef, CxtRef);
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
