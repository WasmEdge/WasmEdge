// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once
#include "mlx/activations.h"
#include "mlx/base.h"
#include "mlx/embedding.h"
#include "mlx/linear.h"
#include "mlx/normalization.h"
#include "mlx/positional_encoding.h"
#include "prompt/prompt.h"
#include <mlx/array.h>
#include <mlx/fast.h>
#include <mlx/ops.h>
#include <optional>
#include <tokenizers_cpp.h>
#include <tuple>
#include <unordered_map>
#include <vector>
namespace WasmEdge::Host::WASINN::MLX {
namespace nn = mlx::core::nn;
namespace llm {

class RMSNorm : public nn::Module {
  float Eps;

public:
  RMSNorm(int Dims, float Eps = 1e-5) : Eps(Eps) {
    registerParameter("weight", mx::ones({Dims}));
  }
  mx::array forward(mx::array Input);
};
class Attention : public nn::Module {

  int NHeads;
  int NKVHeads;
  bool NormQKProj;
  double Scale;

public:
  Attention(int Dim, int NHeads, int NKVHeads,
            std::optional<int> HeadDimPar = 0, bool RopeTraditional = false,
            float RopeTheta = 1000,
            std::optional<std::unordered_map<std::string, std::string>>
                RopeScaling = {},
            bool NormQKProj = false, float AttentionNormEps = 1e-6)
      : NHeads(NHeads), NKVHeads(NKVHeads), NormQKProj(NormQKProj) {
    int HeadDim;
    if (HeadDimPar) {
      HeadDim = *HeadDimPar;
    } else {
      HeadDim = Dim / NHeads;
    }
    Scale = pow(HeadDim, -0.5);
    registerModule("q_proj", std::make_shared<nn::Linear>(
                                 nn::Linear(Dim, NHeads * HeadDim, false)));
    registerModule("k_proj", std::make_shared<nn::Linear>(
                                 nn::Linear(Dim, NKVHeads * HeadDim, false)));
    registerModule("v_proj", std::make_shared<nn::Linear>(
                                 nn::Linear(Dim, NKVHeads * HeadDim, false)));
    registerModule("o_proj", std::make_shared<nn::Linear>(
                                 nn::Linear(NHeads * HeadDim, Dim, false)));

    if (NormQKProj) {
      registerModule("q_norm", std::make_shared<nn::RMSNorm>(
                                   nn::RMSNorm(HeadDim, AttentionNormEps)));
      registerModule("k_norm", std::make_shared<nn::RMSNorm>(
                                   nn::RMSNorm(HeadDim, AttentionNormEps)));
    }
    float RopeScale;
    if (RopeScaling && (*RopeScaling)["type"] == "linear") {
      RopeScale = 1 / stof((*RopeScaling)["factor"]);
    } else {
      RopeScale = 1;
    }

    registerModule("rope",
                   std::make_shared<nn::RoPE>(nn::RoPE(HeadDim, RopeTraditional,
                                                       RopeTheta, RopeScale)));
  }
  std::tuple<mx::array, std::tuple<mx::array, mx::array>>
  forward(mx::array Input, std::optional<mx::array> Mask = {},
          std::optional<std::tuple<mx::array, mx::array>> KVCache = {});
};
class MLP : public nn::Module {
  bool Gemma;

public:
  MLP(int Dim, int HiddenDim, bool Gemma = false) : Gemma(Gemma) {
    registerModule("gate_proj", std::make_shared<nn::Linear>(
                                    nn::Linear(Dim, HiddenDim, false)));
    registerModule("down_proj", std::make_shared<nn::Linear>(
                                    nn::Linear(HiddenDim, Dim, false)));
    registerModule("up_proj", std::make_shared<nn::Linear>(
                                  nn::Linear(Dim, HiddenDim, false)));
  }
  mx::array forward(mx::array Input);
};
class TransformerBlock : public nn::Module {
  bool Gemma;

public:
  TransformerBlock(int Dim, int NHeads, int NKVHeads, int HiddenDim,
                   float NormEps, std::optional<int> HeadDim = {},
                   bool RopeTraditional = false, float RopeTheta = 1000,
                   std::optional<std::unordered_map<std::string, std::string>>
                       RopeScaling = {},
                   bool NormQKProj = false, float AttentionNormEps = 1e-6,
                   bool Gemma = false)
      : Gemma(Gemma) {
    registerModule("attention",
                   std::make_shared<Attention>(Attention(
                       Dim, NHeads, NKVHeads, HeadDim, RopeTraditional,
                       RopeTheta, RopeScaling, NormQKProj, AttentionNormEps)));
    registerModule("mlp", std::make_shared<MLP>(MLP(Dim, HiddenDim, Gemma)));
    if (!Gemma) {
      registerModule("attention_norm",
                     std::make_shared<nn::RMSNorm>(nn::RMSNorm(Dim, NormEps)));
      registerModule("mlp_norm",
                     std::make_shared<nn::RMSNorm>(nn::RMSNorm(Dim, NormEps)));
    } else {
      registerModule("attention_norm",
                     std::make_shared<RMSNorm>(RMSNorm(Dim, NormEps)));
      registerModule("mlp_norm",
                     std::make_shared<RMSNorm>(RMSNorm(Dim, NormEps)));
    }
  }
  std::tuple<mx::array, std::tuple<mx::array, mx::array>>
  forward(mx::array Input, std::optional<mx::array> Mask = {},
          std::optional<std::tuple<mx::array, mx::array>> KVCachePar = {});
};
class Transformer : public nn::Module {
  int Dim;
  std::optional<std::vector<int>> HiddenDim;
  bool Gemma;
  bool EmbedAsHead;
  std::vector<std::shared_ptr<TransformerBlock>> Layers{};

public:
  Transformer(
      int Dim, std::optional<std::vector<int>> HiddenDim, int VocabSize,
      int NLayers, std::optional<std::vector<int>> NHeads,
      std::optional<std::vector<int>> NKVHeads = {}, float NormEps = 1e-5,
      std::optional<int> HeadDim = {}, bool RopeTraditional = false,
      float RopeTheta = 1000,
      std::optional<std::vector<std::unordered_map<std::string, std::string>>>
          RopeScaling = {},
      bool NormQKProj = false, float AttentionNormEps = 1e-6,
      bool Gemma = false, bool EmbedAsHeadPar = false)
      : Dim(Dim), HiddenDim(HiddenDim), Gemma(Gemma),
        EmbedAsHead(EmbedAsHeadPar) {
    if (VocabSize <= 0) {
      spdlog::error("VocabSize must be greater than 0.");
      assumingUnreachable();
    }
    EmbedAsHead = Gemma ? true : EmbedAsHead;
    if (!NKVHeads) {
      NKVHeads = NHeads;
    }
    registerModule("token_embed", std::make_shared<nn::Embedding>(
                                      nn::Embedding(VocabSize, Dim)));
    if (HiddenDim->size() == 1) {
      while (static_cast<int>(HiddenDim->size()) < NLayers) {
        HiddenDim->emplace_back((*HiddenDim)[0]);
      }
    }
    if (NHeads->size() == 1) {
      while (static_cast<int>(NHeads->size()) < NLayers) {
        NHeads->emplace_back((*NHeads)[0]);
      }
    }
    if (NKVHeads->size() == 1) {
      while (static_cast<int>(NKVHeads->size()) < NLayers) {
        NKVHeads->emplace_back((*NKVHeads)[0]);
      }
    }
    Layers.reserve(NLayers);
    for (int Idx = 0; Idx < NLayers; Idx++) {
      if (RopeScaling) {
        Layers.push_back(std::make_shared<TransformerBlock>(TransformerBlock(
            Dim, (*NHeads)[Idx], (*NKVHeads)[Idx], (*HiddenDim)[Idx], NormEps,
            HeadDim, RopeTraditional, RopeTheta, (*RopeScaling)[Idx],
            NormQKProj, AttentionNormEps, Gemma)));
      } else {
        Layers.push_back(std::make_shared<TransformerBlock>(TransformerBlock(
            Dim, (*NHeads)[Idx], (*NKVHeads)[Idx], (*HiddenDim)[Idx], NormEps,
            HeadDim, RopeTraditional, RopeTheta, {}, NormQKProj,
            AttentionNormEps, Gemma)));
      }
    }
    registerLayer("layers", Layers);
    if (!Gemma) {
      registerModule("norm",
                     std::make_shared<nn::RMSNorm>(nn::RMSNorm(Dim, NormEps)));
    } else {
      registerModule("norm", std::make_shared<RMSNorm>(RMSNorm(Dim, NormEps)));
    }
    if (!EmbedAsHead) {
      registerModule("head", std::make_shared<nn::Linear>(
                                 nn::Linear(Dim, VocabSize, false)));
    }
  }
  struct LLMOutput {
    std::string Answer;
    std::vector<int32_t> TokenList;
  };
  std::tuple<mx::array,
             std::optional<std::vector<std::tuple<mx::array, mx::array>>>>
  embed(mx::array Input,
        std::optional<std::vector<std::tuple<mx::array, mx::array>>>
            KVCachePar = {},
        bool Norm = false);
  std::tuple<mx::array,
             std::optional<std::vector<std::tuple<mx::array, mx::array>>>>
  forward(mx::array Input,
          std::optional<std::vector<std::tuple<mx::array, mx::array>>>
              KVCachePar = {});
  std::tuple<mx::array,
             std::optional<std::vector<std::tuple<mx::array, mx::array>>>>
  stepGenerate(mx::array Input, std::optional<float> Temp = 0.0);
  std::tuple<mx::array,
             std::optional<std::vector<std::tuple<mx::array, mx::array>>>>
  nextStepGenerate(mx::array Y, std::optional<float> Temp = 0.0,
                   std::optional<std::vector<std::tuple<mx::array, mx::array>>>
                       KVCachePar = {});
  LLMOutput generate(const std::string &Prompt, const BasePrompt &ModelPrompt,
                     const int MaxToken, const bool Verbose,
                     const std::unique_ptr<tokenizers::Tokenizer> &Tok);
};
} // namespace llm
} // namespace WasmEdge::Host::WASINN::MLX
