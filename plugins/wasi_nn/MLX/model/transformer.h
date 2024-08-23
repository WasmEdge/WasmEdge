#pragma once
#include "activations.h"
#include "base.h"
#include "embedding.h"
#include "linear.h"
#include "normalization.h"
#include "positional_encoding.h"
#include <mlx/array.h>
#include <mlx/fast.h>
#include <mlx/ops.h>
#include <optional>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace nn = mlx::core::nn;

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
    registerModule("q_proj", new nn::Linear(Dim, NHeads * HeadDim, false));
    registerModule("k_proj", new nn::Linear(Dim, NKVHeads * HeadDim, false));
    registerModule("v_proj", new nn::Linear(Dim, NKVHeads * HeadDim, false));
    registerModule("o_proj", new nn::Linear(NHeads * HeadDim, Dim, false));

    if (NormQKProj) {
      registerModule("q_norm", new nn::RMSNorm(HeadDim, AttentionNormEps));
      registerModule("k_norm", new nn::RMSNorm(HeadDim, AttentionNormEps));
    }
    float RopeScale;
    if (RopeScaling && (*RopeScaling)["type"] == "linear") {
      RopeScale = 1 / stof((*RopeScaling)["factor"]);
    } else {
      RopeScale = 1;
    }

    registerModule(
        "rope", new nn::RoPE(HeadDim, RopeTraditional, RopeTheta, RopeScale));
  }
  std::tuple<mx::array, std::tuple<mx::array, mx::array>>
  forward(mx::array Input, std::optional<mx::array> Mask = {},
          std::optional<std::tuple<mx::array, mx::array>> KVCache = {});
};
class MLP : public nn::Module {
  bool Gemma;

public:
  MLP(int Dim, int HiddenDim, bool Gemma = false) : Gemma(Gemma) {
    registerModule("gate_proj", new nn::Linear(Dim, HiddenDim, false));
    registerModule("down_proj", new nn::Linear(HiddenDim, Dim, false));
    registerModule("up_proj", new nn::Linear(Dim, HiddenDim, false));
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
                   new Attention(Dim, NHeads, NKVHeads, HeadDim,
                                 RopeTraditional, RopeTheta, RopeScaling,
                                 NormQKProj, AttentionNormEps));
    registerModule("mlp", new MLP(Dim, HiddenDim, Gemma));
    if (!Gemma) {
      registerModule("attention_norm", new nn::RMSNorm(Dim, NormEps));
      registerModule("mlp_norm", new nn::RMSNorm(Dim, NormEps));
    } else {
      registerModule("attention_norm", new RMSNorm(Dim, NormEps));
      registerModule("mlp_norm", new RMSNorm(Dim, NormEps));
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
  std::vector<TransformerBlock *> Layers{};

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
      throw std::invalid_argument("VocabSize must be greater than 0.");
    }
    EmbedAsHead = Gemma ? true : EmbedAsHead;
    if (!NKVHeads) {
      NKVHeads = NHeads;
    }
    registerModule("token_embed", new nn::Embedding(VocabSize, Dim));
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
        Layers.push_back(new TransformerBlock(
            Dim, (*NHeads)[Idx], (*NKVHeads)[Idx], (*HiddenDim)[Idx], NormEps,
            HeadDim, RopeTraditional, RopeTheta, (*RopeScaling)[Idx],
            NormQKProj, AttentionNormEps, Gemma));
      } else {
        Layers.push_back(new TransformerBlock(
            Dim, (*NHeads)[Idx], (*NKVHeads)[Idx], (*HiddenDim)[Idx], NormEps,
            HeadDim, RopeTraditional, RopeTheta, {}, NormQKProj,
            AttentionNormEps, Gemma));
      }
    }
    registerLayer("layers", Layers);
    if (!Gemma) {
      registerModule("norm", new nn::RMSNorm(Dim, NormEps));
    } else {
      registerModule("norm", new RMSNorm(Dim, NormEps));
    }
    if (!EmbedAsHead) {
      registerModule("head", new nn::Linear(Dim, VocabSize, false));
    }
  }
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
  generate(mx::array Input, std::optional<float> Temp = 0.0);
  std::tuple<mx::array,
             std::optional<std::vector<std::tuple<mx::array, mx::array>>>>
  nextGenerate(mx::array Y, std::optional<float> Temp = 0.0,
               std::optional<std::vector<std::tuple<mx::array, mx::array>>>
                   KVCachePar = {});
};