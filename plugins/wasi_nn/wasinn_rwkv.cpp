// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2026 Second State INC

#include "wasinn_rwkv.h"
#include "wasinnenv.h"
#include "wasinntypes.h"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <ios>
#include <string>
#include <string_view>
#include <vector>
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_RWKV
#include "simdjson.h"

#include <cmath>
#include <filesystem>
#include <fstream>
#endif

using namespace std::literals;

namespace WasmEdge::Host::WASINN::RWKV {
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_RWKV

namespace {

constexpr size_t SequenceChunkSize = 16;

#define LOG_DEBUG(Debug, ...)                                                  \
  if (Debug) {                                                                 \
    spdlog::debug("[WASI-NN][Debug] RWKV backend: "sv __VA_ARGS__);            \
  }

#define LOG_INFO(Info, ...)                                                    \
  if (Info) {                                                                  \
    spdlog::info("[WASI-NN] RWKV backend: "sv __VA_ARGS__);                    \
  }

#define LOG_WARN(...) spdlog::warn("[WASI-NN] RWKV backend: "sv __VA_ARGS__);

#define LOG_ERROR(...) spdlog::error("[WASI-NN] RWKV backend: "sv __VA_ARGS__);

// Implementation inspired by llama.cpp sampling logic:
// https://github.com/ggml-org/llama.cpp/blob/b7feacf7f39d79518e6ca48a9fcf697c5244b585/common/sampling.cpp
uint32_t sampleToken(std::vector<float> &Logits, const Config &Cfg,
                     std::unordered_map<uint32_t, uint32_t> &TokenFrequencies,
                     std::mt19937 &Rng,
                     std::vector<Context::Candidate> &Candidates) {
  const size_t NVocab = Logits.size();

  if (NVocab == 0) {
    LOG_ERROR("sampleToken called with empty logits buffer"sv);
    return 0;
  }

  if (Candidates.size() != NVocab) {
    Candidates.resize(NVocab);
  }

  if (Cfg.PresencePenalty != 0.0f || Cfg.FrequencyPenalty != 0.0f) {
    for (const auto &[TokenId, Count] : TokenFrequencies) {
      if (TokenId < NVocab) {
        if (Cfg.PresencePenalty != 0.0f) {
          Logits[TokenId] -= Cfg.PresencePenalty;
        }
        if (Cfg.FrequencyPenalty != 0.0f) {
          Logits[TokenId] -= Cfg.FrequencyPenalty * static_cast<float>(Count);
        }
      }
    }
  }

  if (Cfg.Temperature <= 0.0f) {
    return static_cast<uint32_t>(std::distance(
        Logits.begin(), std::max_element(Logits.begin(), Logits.end())));
  }

  for (float &Logit : Logits) {
    Logit /= Cfg.Temperature;
  }

  float MaxLogit = *std::max_element(Logits.begin(), Logits.end());

  double SumExp = 0.0;
  for (size_t I = 0; I < NVocab; ++I) {
    Candidates[I].Id = static_cast<uint32_t>(I);
    Candidates[I].Logit = Logits[I];
    Candidates[I].P = std::exp(Logits[I] - MaxLogit);
    SumExp += Candidates[I].P;
  }

  for (size_t I = 0; I < NVocab; ++I) {
    Candidates[I].P = static_cast<float>(Candidates[I].P / SumExp);
  }

  auto CompareCandidates = [](const Context::Candidate &A,
                              const Context::Candidate &B) {
    return A.Logit > B.Logit;
  };

  size_t CurrentSize = NVocab;
  if (Cfg.TopK > 0 && Cfg.TopK < CurrentSize) {
    std::partial_sort(Candidates.begin(), Candidates.begin() + Cfg.TopK,
                      Candidates.end(), CompareCandidates);
    CurrentSize = static_cast<size_t>(Cfg.TopK);
  }

  if (Cfg.TopP < 1.0f) {
    size_t SortLimit = std::min(CurrentSize, static_cast<size_t>(256));
    if (CurrentSize == NVocab) {
      std::partial_sort(Candidates.begin(), Candidates.begin() + SortLimit,
                        Candidates.end(), CompareCandidates);
    }

    float CumulativeP = 0.0f;
    size_t CutoffIdx = CurrentSize;
    for (size_t I = 0; I < CurrentSize; ++I) {
      if (I == SortLimit && I < CurrentSize) {
        std::sort(Candidates.begin() + SortLimit,
                  Candidates.begin() + CurrentSize, CompareCandidates);
        SortLimit = CurrentSize;
      }

      CumulativeP += Candidates[I].P;
      if (CumulativeP >= Cfg.TopP && I + 1 >= Cfg.MinKeep) {
        CutoffIdx = I + 1;
        break;
      }
    }
    CurrentSize = CutoffIdx;
  }

  float TotalP = 0.0f;
  for (size_t I = 0; I < CurrentSize; ++I) {
    TotalP += Candidates[I].P;
  }

  std::uniform_real_distribution<float> Dist(0.0f, TotalP);
  float R = Dist(Rng);
  float Cumulative = 0.0f;
  for (size_t I = 0; I < CurrentSize; ++I) {
    Cumulative += Candidates[I].P;
    if (Cumulative >= R) {
      return Candidates[I].Id;
    }
  }

  return Candidates[0].Id;
}

Expect<ErrNo> parseMetadata(Config &ConfigRef, const std::string &Metadata) {
  simdjson::dom::parser Parser;
  simdjson::dom::element Doc;
  auto ParseError = Parser.parse(Metadata).get(Doc);
  if (ParseError) {
    LOG_ERROR("Failed to parse metadata JSON: {}"sv,
              simdjson::error_message(ParseError));
    return ErrNo::InvalidEncoding;
  }

  auto GetUint = [&](const char *Key, uint64_t &Val) -> ErrNo {
    auto Res = Doc.at_key(Key);
    if (Res.error() == simdjson::SUCCESS) {
      if (Res.is_number()) {
        Val = static_cast<uint64_t>(Res.get_double());
        return ErrNo::Success;
      }
      LOG_ERROR("Metadata field '{}' must be a number"sv, Key);
      return ErrNo::InvalidEncoding;
    }
    return ErrNo::Success;
  };

  auto GetFloat = [&](const char *Key, float &Val) -> ErrNo {
    auto Res = Doc.at_key(Key);
    if (Res.error() == simdjson::SUCCESS) {
      if (Res.is_number()) {
        Val = static_cast<float>(Res.get_double());
        return ErrNo::Success;
      }
      LOG_ERROR("Metadata field '{}' must be a number"sv, Key);
      return ErrNo::InvalidEncoding;
    }
    return ErrNo::Success;
  };

  auto GetBool = [&](const char *Key, bool &Val) -> ErrNo {
    auto Res = Doc.at_key(Key);
    if (Res.error() == simdjson::SUCCESS) {
      if (Res.get(Val) == simdjson::SUCCESS) {
        return ErrNo::Success;
      }
      LOG_ERROR("Metadata field '{}' must be a boolean"sv, Key);
      return ErrNo::InvalidEncoding;
    }
    return ErrNo::Success;
  };

  ErrNo Res;
  if ((Res = GetUint("n-predict", ConfigRef.MaxTokens)) != ErrNo::Success)
    return Res;
  if ((Res = GetFloat("temp", ConfigRef.Temperature)) != ErrNo::Success)
    return Res;
  if ((Res = GetUint("top-k", ConfigRef.TopK)) != ErrNo::Success)
    return Res;
  if ((Res = GetFloat("top-p", ConfigRef.TopP)) != ErrNo::Success)
    return Res;
  if ((Res = GetUint("min-keep", ConfigRef.MinKeep)) != ErrNo::Success)
    return Res;
  if ((Res = GetFloat("presence-penalty", ConfigRef.PresencePenalty)) !=
      ErrNo::Success)
    return Res;
  if ((Res = GetFloat("frequency-penalty", ConfigRef.FrequencyPenalty)) !=
      ErrNo::Success)
    return Res;
  if ((Res = GetUint("threads", ConfigRef.ThreadsNum)) != ErrNo::Success)
    return Res;
  if ((Res = GetBool("enable-log", ConfigRef.EnableLog)) != ErrNo::Success)
    return Res;
  if ((Res = GetBool("enable-debug-log", ConfigRef.EnableDebugLog)) !=
      ErrNo::Success)
    return Res;
  if ((Res = GetBool("reset-state", ConfigRef.ResetStateOnPrompt)) !=
      ErrNo::Success)
    return Res;

  uint64_t TempEos;
  if (Doc.at_key("eos-token-id").error() == simdjson::SUCCESS) {
    if ((Res = GetUint("eos-token-id", TempEos)) != ErrNo::Success)
      return Res;
    ConfigRef.EosTokenId = static_cast<uint32_t>(TempEos);
  }

  if ((Res = GetUint("n-gpu-layers", ConfigRef.NGPULayers)) != ErrNo::Success)
    return Res;

  if (Doc.at_key("tokenizer").error() == simdjson::SUCCESS) {
    if (std::string_view Val; Doc["tokenizer"].get(Val) == simdjson::SUCCESS) {
      ConfigRef.TokenizerPath = std::string(Val);
    } else {
      LOG_ERROR("Metadata field 'tokenizer' must be a string"sv);
      return ErrNo::InvalidEncoding;
    }
  }

  uint64_t Seed = 0;
  if ((Res = GetUint("seed", Seed)) != ErrNo::Success) {
    return Res;
  }
  ConfigRef.Seed = Seed;

  return ErrNo::Success;
}

std::vector<uint8_t> readFileBytes(const std::string &Path) {
  std::ifstream File(Path, std::ios::binary | std::ios::ate);
  if (!File.is_open()) {
    return {};
  }
  std::streamsize Size = File.tellg();
  if (Size < 0) {
    return {};
  }
  File.seekg(0, std::ios::beg);

  std::vector<uint8_t> Buffer(static_cast<size_t>(Size));
  if (File.read(reinterpret_cast<char *>(Buffer.data()), Size)) {
    return Buffer;
  }
  return {};
}

} // namespace

Expect<ErrNo> load(WasiNNEnvironment &Env, Span<const Span<uint8_t>> Builders,
                   [[maybe_unused]] Device Device, uint32_t &GraphId) noexcept {
  if (Builders.size() < 1) {
    LOG_ERROR("[RWKV] Requires at least 1 builder (model path)"sv);
    return ErrNo::InvalidArgument;
  }

  GraphId = Env.newGraph(Backend::RWKV);
  auto &Graph = Env.NNGraph[GraphId].get<Backend::RWKV>();

  if (Builders.size() >= 2) {
    std::string Metadata(reinterpret_cast<const char *>(Builders[1].data()),
                         Builders[1].size());
    auto Res = parseMetadata(Graph.RWKVConfig, Metadata);
    if (Res != ErrNo::Success) {
      Env.deleteGraph(GraphId);
      return Res;
    }
  }

  std::string_view BuildData(reinterpret_cast<const char *>(Builders[0].data()),
                             Builders[0].size());
  std::string ModelPath;
  if (BuildData.substr(0, 8) == "preload:"sv) {
    ModelPath = std::string(BuildData.substr(8));
  } else {
    ModelPath = std::string(BuildData);
  }
  Graph.ModelFilePath = ModelPath;

  LOG_INFO(Graph.RWKVConfig.EnableLog, "Loading model from: {}"sv, ModelPath);
  LOG_INFO(Graph.RWKVConfig.EnableLog, "System info: {}"sv,
           rwkv_get_system_info_string());

  rwkv_set_print_errors(nullptr, Graph.RWKVConfig.EnableDebugLog);

  Graph.RWKVCtx.reset(rwkv_init_from_file(
      ModelPath.c_str(), static_cast<uint32_t>(Graph.RWKVConfig.ThreadsNum),
      static_cast<uint32_t>(Graph.RWKVConfig.NGPULayers)));

  if (!Graph.RWKVCtx) {
    LOG_ERROR("Failed to load model from: {}"sv, ModelPath);
    Env.deleteGraph(GraphId);
    return ErrNo::InvalidArgument;
  }

  Graph.NVocab = rwkv_get_logits_len(Graph.RWKVCtx.get());

  if (!Graph.RWKVConfig.TokenizerPath.empty()) {
    auto TokenizerBytes = readFileBytes(Graph.RWKVConfig.TokenizerPath);
    if (TokenizerBytes.empty()) {
      LOG_ERROR("Failed to read tokenizer from: {}"sv,
                Graph.RWKVConfig.TokenizerPath);
      Graph.RWKVCtx.reset();
      Env.deleteGraph(GraphId);
      return ErrNo::InvalidArgument;
    }
    std::string TokenizerJson(TokenizerBytes.begin(), TokenizerBytes.end());
    Graph.Tok = tokenizers::Tokenizer::FromBlobJSON(TokenizerJson);
    if (!Graph.Tok) {
      LOG_ERROR("Failed to parse tokenizer JSON"sv);
      Graph.RWKVCtx.reset();
      Env.deleteGraph(GraphId);
      return ErrNo::InvalidArgument;
    }
    LOG_INFO(Graph.RWKVConfig.EnableLog, "Loaded tokenizer from: {}"sv,
             Graph.RWKVConfig.TokenizerPath);
  } else {
    std::filesystem::path ModelDir =
        std::filesystem::path(ModelPath).parent_path();
    if (ModelDir.empty()) {
      ModelDir = ".";
    }

    LOG_ERROR("No tokenizer found. Please provide tokenizer path "
              "in metadata"sv);
    Graph.RWKVCtx.reset();
    Env.deleteGraph(GraphId);
    return ErrNo::InvalidArgument;
  }

  LOG_INFO(Graph.RWKVConfig.EnableLog,
           "Model loaded successfully. Vocab size: {}, "
           "Tokenizer loaded: {}"sv,
           Graph.NVocab, Graph.Tok != nullptr);

  Env.NNGraph[GraphId].setReady();

  return ErrNo::Success;
}

Expect<ErrNo> initExecCtx(WasiNNEnvironment &Env, uint32_t GraphId,
                          uint32_t &ContextId) noexcept {
  if (GraphId >= Env.NNGraph.size()) {
    LOG_ERROR("[RWKV] Invalid graph ID: {}"sv, GraphId);
    return ErrNo::InvalidArgument;
  }

  auto &Graph = Env.NNGraph[GraphId].get<Backend::RWKV>();
  if (!Graph.RWKVCtx) {
    LOG_ERROR("[RWKV] Graph not loaded"sv);
    return ErrNo::MissingMemory;
  }

  ContextId = Env.newContext(GraphId, Env.NNGraph[GraphId]);
  auto &CxtRef = Env.NNContext[ContextId].get<Backend::RWKV>();

  CxtRef.RWKVCtx.reset(rwkv_clone_context(
      Graph.RWKVCtx.get(), static_cast<uint32_t>(Graph.RWKVConfig.ThreadsNum)));
  if (!CxtRef.RWKVCtx) {
    LOG_ERROR("Failed to clone RWKV context"sv);
    Env.deleteContext(ContextId);
    return ErrNo::RuntimeError;
  }

  CxtRef.StateBufferSize = rwkv_get_state_len(CxtRef.RWKVCtx.get());
  CxtRef.LogitsBufferSize = rwkv_get_logits_len(CxtRef.RWKVCtx.get());
  CxtRef.StateBuffer.resize(CxtRef.StateBufferSize, 0.0f);
  CxtRef.LogitsBuffer.resize(CxtRef.LogitsBufferSize, 0.0f);

  rwkv_init_state(CxtRef.RWKVCtx.get(), CxtRef.StateBuffer.data());

  Env.NNContext[ContextId].setReady();

  return ErrNo::Success;
}

Expect<ErrNo> setInput(WasiNNEnvironment &Env, uint32_t ContextId,
                       uint32_t Index [[maybe_unused]],
                       const TensorData &Tensor) noexcept {
  if (ContextId >= Env.NNContext.size()) {
    LOG_ERROR("[RWKV] Invalid context ID: {}"sv, ContextId);
    return ErrNo::InvalidArgument;
  }

  auto &CxtRef = Env.NNContext[ContextId].get<Backend::RWKV>();
  uint32_t GraphId = CxtRef.GraphId;
  auto &Graph = Env.NNGraph[GraphId].get<Backend::RWKV>();

  if (Index == 1 && Tensor.Tensor.size() > 0) {
    std::string Metadata(reinterpret_cast<const char *>(Tensor.Tensor.data()),
                         Tensor.Tensor.size());
    auto Res = parseMetadata(CxtRef.RWKVConfig, Metadata);
    if (Res != ErrNo::Success) {
      return Res;
    }
    return ErrNo::Success;
  }

  if (Index != 0) {
    LOG_ERROR("Invalid tensor index: {}"sv, Index);
    return ErrNo::InvalidArgument;
  }

  CxtRef.InputPrompt =
      std::string(reinterpret_cast<const char *>(Tensor.Tensor.data()),
                  Tensor.Tensor.size());

  if (Graph.Tok) {
    CxtRef.InputTokens = Graph.Tok->Encode(CxtRef.InputPrompt);

    if (CxtRef.InputTokens.empty()) {
      LOG_ERROR("Tokenization produced no tokens for input"sv);
      return ErrNo::InvalidArgument;
    }

    LOG_INFO(CxtRef.RWKVConfig.EnableLog,
             "Input tokenized: {} chars -> {} tokens"sv,
             CxtRef.InputPrompt.size(), CxtRef.InputTokens.size());
  } else {
    LOG_ERROR(
        "No tokenizer loaded. Please provide tokenizer path "
        "in metadata or place a valid tokenizer.json in model directory"sv);
    return ErrNo::InvalidArgument;
  }

  if (CxtRef.RWKVConfig.ResetStateOnPrompt) {
    CxtRef.TokenFrequencies.clear();
    rwkv_init_state(CxtRef.RWKVCtx.get(), CxtRef.StateBuffer.data());
  }

  CxtRef.PromptProcessed = false;
  CxtRef.ComputeSingleStarted = false;
  CxtRef.TokensGenerated = 0;
  CxtRef.GeneratedTokens.clear();
  CxtRef.LastToken = -1;

  return ErrNo::Success;
}

Expect<ErrNo> getOutput(WasiNNEnvironment &Env, uint32_t ContextId,
                        uint32_t Index [[maybe_unused]],
                        Span<uint8_t> OutBuffer,
                        uint32_t &BytesWritten) noexcept {
  if (ContextId >= Env.NNContext.size()) {
    LOG_ERROR("Invalid context ID: {}"sv, ContextId);
    return ErrNo::InvalidArgument;
  }

  auto &CxtRef = Env.NNContext[ContextId].get<Backend::RWKV>();
  const size_t OutputSize = CxtRef.Outputs.size();
  const size_t BytesToCopy =
      std::min(static_cast<size_t>(OutBuffer.size()), OutputSize);

  std::copy_n(CxtRef.Outputs.data(), BytesToCopy, OutBuffer.data());
  BytesWritten = static_cast<uint32_t>(OutputSize);

  LOG_INFO(CxtRef.RWKVConfig.EnableLog, "Output: {} bytes written"sv,
           BytesWritten);

  return ErrNo::Success;
}

Expect<ErrNo> compute(WasiNNEnvironment &Env, uint32_t ContextId) noexcept {
  if (ContextId >= Env.NNContext.size()) {
    LOG_ERROR("Invalid context ID: {}"sv, ContextId);
    return ErrNo::InvalidArgument;
  }

  auto &CxtRef = Env.NNContext[ContextId].get<Backend::RWKV>();
  uint32_t GraphId = CxtRef.GraphId;

  if (GraphId >= Env.NNGraph.size()) {
    LOG_ERROR("Invalid graph ID in context: {}"sv, GraphId);
    return ErrNo::InvalidArgument;
  }

  auto &Graph = Env.NNGraph[GraphId].get<Backend::RWKV>();

  if (!CxtRef.RWKVCtx) {
    LOG_ERROR("Context not initialized"sv);
    return ErrNo::RuntimeError;
  }

  LOG_INFO(CxtRef.RWKVConfig.EnableLog,
           "Processing {} input tokens, generating up to {} tokens"sv,
           CxtRef.InputTokens.size(), CxtRef.RWKVConfig.MaxTokens);

  if (!rwkv_eval_sequence_in_chunks(
          CxtRef.RWKVCtx.get(),
          reinterpret_cast<const uint32_t *>(CxtRef.InputTokens.data()),
          CxtRef.InputTokens.size(), SequenceChunkSize,
          CxtRef.StateBuffer.data(), CxtRef.StateBuffer.data(),
          CxtRef.LogitsBuffer.data())) {
    LOG_ERROR("Failed to evaluate input sequence in chunks"sv);
    return ErrNo::RuntimeError;
  }

  std::vector<int32_t> GeneratedTokens;
  GeneratedTokens.reserve(CxtRef.RWKVConfig.MaxTokens);

  for (uint64_t I = 0; I < CxtRef.RWKVConfig.MaxTokens; ++I) {
    uint32_t NextToken =
        sampleToken(CxtRef.LogitsBuffer, CxtRef.RWKVConfig,
                    CxtRef.TokenFrequencies, CxtRef.Rng, CxtRef.Candidates);

    if (NextToken == CxtRef.RWKVConfig.EosTokenId) {
      break;
    }

    CxtRef.TokenFrequencies[NextToken]++;

    GeneratedTokens.push_back(static_cast<int32_t>(NextToken));

    bool Success =
        rwkv_eval(CxtRef.RWKVCtx.get(), NextToken, CxtRef.StateBuffer.data(),
                  CxtRef.StateBuffer.data(), CxtRef.LogitsBuffer.data());
    if (!Success) {
      LOG_ERROR("Failed to evaluate generated token {}"sv, I);
      return ErrNo::RuntimeError;
    }
  }

  if (Graph.Tok) {
    CxtRef.Outputs = Graph.Tok->Decode(GeneratedTokens);
  } else {
    CxtRef.Outputs = "";
  }

  LOG_INFO(CxtRef.RWKVConfig.EnableLog,
           "Generated {} tokens, {} chars output"sv, GeneratedTokens.size(),
           CxtRef.Outputs.size());

  return ErrNo::Success;
}

Expect<ErrNo> unload(WasiNNEnvironment &Env, uint32_t GraphId) noexcept {
  if (GraphId >= Env.NNGraph.size()) {
    LOG_ERROR("Invalid graph ID: {}"sv, GraphId);
    return ErrNo::InvalidArgument;
  }

  if (Env.NNGraph[GraphId].getContextCount() > 0) {
    LOG_ERROR("Cannot unload graph with active contexts"sv);
    return ErrNo::Busy;
  }

  Env.deleteGraph(GraphId);

  return ErrNo::Success;
}

Expect<ErrNo> finalizeExecCtx(WasiNNEnvironment &Env,
                              uint32_t ContextId) noexcept {
  if (ContextId >= Env.NNContext.size()) {
    LOG_ERROR("Invalid context ID: {}"sv, ContextId);
    return ErrNo::InvalidArgument;
  }

  Env.deleteContext(ContextId);

  return ErrNo::Success;
}

Expect<ErrNo> computeSingle(WasiNNEnvironment &Env,
                            uint32_t ContextId) noexcept {
  if (ContextId >= Env.NNContext.size()) {
    LOG_ERROR("computeSingle: Invalid context ID: {}"sv, ContextId);
    return ErrNo::InvalidArgument;
  }

  auto &CxtRef = Env.NNContext[ContextId].get<Backend::RWKV>();
  uint32_t GraphId = CxtRef.GraphId;

  if (GraphId >= Env.NNGraph.size()) {
    LOG_ERROR("computeSingle: Invalid graph ID in context: {}"sv, GraphId);
    return ErrNo::InvalidArgument;
  }

  auto &Graph = Env.NNGraph[GraphId].get<Backend::RWKV>();

  if (!Graph.RWKVCtx) {
    LOG_ERROR("computeSingle: Model not loaded"sv);
    return ErrNo::RuntimeError;
  }

  if (!CxtRef.ComputeSingleStarted) {
    CxtRef.ComputeSingleStarted = true;
    CxtRef.PromptProcessed = false;
    CxtRef.TokensGenerated = 0;
    CxtRef.GeneratedTokens.clear();
    CxtRef.LastToken = -1;

    if (CxtRef.RWKVConfig.ResetStateOnPrompt) {
      CxtRef.TokenFrequencies.clear();
      rwkv_init_state(CxtRef.RWKVCtx.get(), CxtRef.StateBuffer.data());
    }
  }

  if (!CxtRef.PromptProcessed) {
    LOG_INFO(CxtRef.RWKVConfig.EnableLog,
             "computeSingle: Processing {} input tokens"sv,
             CxtRef.InputTokens.size());

    if (!rwkv_eval_sequence_in_chunks(
            CxtRef.RWKVCtx.get(),
            reinterpret_cast<const uint32_t *>(CxtRef.InputTokens.data()),
            CxtRef.InputTokens.size(), SequenceChunkSize,
            CxtRef.StateBuffer.data(), CxtRef.StateBuffer.data(),
            CxtRef.LogitsBuffer.data())) {
      LOG_ERROR("computeSingle: Failed to evaluate input sequence in chunks"sv);
      CxtRef.ComputeSingleStarted = false;
      return ErrNo::RuntimeError;
    }

    CxtRef.PromptProcessed = true;
  }

  if (CxtRef.TokensGenerated >= CxtRef.RWKVConfig.MaxTokens) {
    LOG_INFO(CxtRef.RWKVConfig.EnableLog,
             "computeSingle: Reached max tokens ({})"sv,
             CxtRef.RWKVConfig.MaxTokens);
    return ErrNo::EndOfSequence;
  }

  uint32_t NextToken =
      sampleToken(CxtRef.LogitsBuffer, CxtRef.RWKVConfig,
                  CxtRef.TokenFrequencies, CxtRef.Rng, CxtRef.Candidates);

  if (NextToken == CxtRef.RWKVConfig.EosTokenId) {
    LOG_INFO(CxtRef.RWKVConfig.EnableLog,
             "computeSingle: EOS token generated"sv);
    return ErrNo::EndOfSequence;
  }

  CxtRef.TokenFrequencies[NextToken]++;

  CxtRef.GeneratedTokens.push_back(static_cast<int32_t>(NextToken));
  CxtRef.LastToken = static_cast<int32_t>(NextToken);
  CxtRef.TokensGenerated++;

  bool Success =
      rwkv_eval(CxtRef.RWKVCtx.get(), NextToken, CxtRef.StateBuffer.data(),
                CxtRef.StateBuffer.data(), CxtRef.LogitsBuffer.data());
  if (!Success) {
    LOG_ERROR("computeSingle: Failed to evaluate generated token"sv);
    CxtRef.ComputeSingleStarted = false;
    return ErrNo::RuntimeError;
  }

  LOG_DEBUG(CxtRef.RWKVConfig.EnableDebugLog,
            "computeSingle: Generated token {} (total: {})"sv, NextToken,
            CxtRef.TokensGenerated);

  return ErrNo::Success;
}

Expect<ErrNo> getOutputSingle(WasiNNEnvironment &Env, uint32_t ContextId,
                              uint32_t Index, Span<uint8_t> OutBuffer,
                              uint32_t &BytesWritten) noexcept {
  if (ContextId >= Env.NNContext.size()) {
    LOG_ERROR("getOutputSingle: Invalid context ID: {}"sv, ContextId);
    return ErrNo::InvalidArgument;
  }

  auto &CxtRef = Env.NNContext[ContextId].get<Backend::RWKV>();
  uint32_t GraphId = CxtRef.GraphId;

  if (GraphId >= Env.NNGraph.size()) {
    LOG_ERROR("getOutputSingle: Invalid graph ID: {}"sv, GraphId);
    return ErrNo::InvalidArgument;
  }

  auto &Graph = Env.NNGraph[GraphId].get<Backend::RWKV>();

  if (Index == 1) {
    std::string Metadata =
        "{\"input_tokens\":" + std::to_string(CxtRef.InputTokens.size()) +
        ",\"output_tokens\":" + std::to_string(CxtRef.TokensGenerated) + "}";
    if (OutBuffer.size() < Metadata.size()) {
      BytesWritten = static_cast<uint32_t>(Metadata.size());
      return ErrNo::TooLarge;
    }
    std::copy_n(Metadata.data(), Metadata.size(), OutBuffer.data());
    BytesWritten = static_cast<uint32_t>(Metadata.size());
    return ErrNo::Success;
  }

  if (CxtRef.LastToken < 0 || !Graph.Tok) {
    BytesWritten = 0;
    return ErrNo::Success;
  }

  std::string TokenStr = Graph.Tok->Decode({CxtRef.LastToken});
  const size_t BytesToCopy =
      std::min(static_cast<size_t>(OutBuffer.size()), TokenStr.size());

  std::copy_n(TokenStr.data(), BytesToCopy, OutBuffer.data());
  BytesWritten = static_cast<uint32_t>(TokenStr.size());

  LOG_DEBUG(CxtRef.RWKVConfig.EnableDebugLog,
            "getOutputSingle: Token {} decoded to '{}'"sv, CxtRef.LastToken,
            TokenStr);

  return ErrNo::Success;
}

Expect<ErrNo> finiSingle(WasiNNEnvironment &Env, uint32_t ContextId) noexcept {
  if (ContextId >= Env.NNContext.size()) {
    LOG_ERROR("finiSingle: Invalid context ID: {}"sv, ContextId);
    return ErrNo::InvalidArgument;
  }

  auto &CxtRef = Env.NNContext[ContextId].get<Backend::RWKV>();

  LOG_INFO(CxtRef.RWKVConfig.EnableLog,
           "finiSingle: Finalizing streaming session. "
           "Generated {} tokens"sv,
           CxtRef.TokensGenerated);

  CxtRef.ComputeSingleStarted = false;
  CxtRef.PromptProcessed = false;
  CxtRef.TokensGenerated = 0;
  CxtRef.GeneratedTokens.clear();
  CxtRef.LastToken = -1;
  CxtRef.TokenFrequencies.clear();

  CxtRef.Outputs.clear();

  return ErrNo::Success;
}

#else
namespace {
Expect<WASINN::ErrNo> reportBackendNotSupported() noexcept {
  spdlog::error("[WASI-NN] RWKV backend is not supported. Use "
                "-DWASMEDGE_PLUGIN_WASI_NN_BACKEND=RWKV to build it."sv);
  return WASINN::ErrNo::InvalidArgument;
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
Expect<ErrNo> unload(WasiNNEnvironment &, uint32_t) noexcept {
  return reportBackendNotSupported();
}
Expect<ErrNo> finalizeExecCtx(WasiNNEnvironment &, uint32_t) noexcept {
  return reportBackendNotSupported();
}
Expect<ErrNo> computeSingle(WasiNNEnvironment &, uint32_t) noexcept {
  return reportBackendNotSupported();
}
Expect<ErrNo> getOutputSingle(WasiNNEnvironment &, uint32_t, uint32_t,
                              Span<uint8_t>, uint32_t &) noexcept {
  return reportBackendNotSupported();
}
Expect<ErrNo> finiSingle(WasiNNEnvironment &, uint32_t) noexcept {
  return reportBackendNotSupported();
}
#endif
} // namespace WasmEdge::Host::WASINN::RWKV
