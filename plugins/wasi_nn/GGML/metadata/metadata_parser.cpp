// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "metadata_parser.h"

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_GGML
#include <common.h>
#include <fmt/ranges.h>
#include <json-partial.h>
#include <json-schema-to-grammar.h>
#endif

namespace WasmEdge::Host::WASINN::GGML {
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_GGML
// Parse metadata from json.
ErrNo parseMetadata(Graph &GraphRef, LocalConfig &ConfRef,
                    const std::string &Metadata, bool *IsModelUpdated,
                    bool *IsContextUpdated, bool *IsSamplerUpdated) noexcept {
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

#endif
} // namespace WasmEdge::Host::WASINN::GGML
