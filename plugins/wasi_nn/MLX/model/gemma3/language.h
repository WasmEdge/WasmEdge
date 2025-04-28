// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "../vlm_base.h"
#include "simdjson.h"
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
namespace WasmEdge::Host::WASINN::MLX {
namespace nn = mlx::core::nn;
namespace gemma3 {

struct TextConfig {
  std::string ModelType = "gemma3_text";
  int HiddenSize = 2560;
  int NumHiddenLayers = 34;
  int IntermediateSize = 10240;
  int NumAttentionHeads = 8;
  int HeadDim = 256;
  float RmsNormEps = 1.0e-6;
  int VocabSize = 262208;
  int NumKeyValueHeads = 4;
  float RopeGlobalBaseFreq = 1000000.0f;
  float RopeLocalBaseFreq = 10000.0f;
  bool RopeTraditional = false;
  float QueryPreAttnScalar = 256;
  int SlidingWindow = 1024;
  std::optional<
      std::unordered_map<std::string, std::variant<float, std::vector<float>>>>
      RopeScaling;
  int MmTokensPerImage = 256;
  int SlidingWindowPattern = 6;
  static TextConfig fromDict(const simdjson::dom::object &Obj);
};

class RMSNorm : public nn::Module {
public:
  RMSNorm(int Dims, float Eps = 1e-5);
  mx::array forward(const mx::array &X);

private:
  float Eps;
};

class Attention : public nn::Module {
public:
  Attention(const TextConfig &Config, int LayerIdx);
  mx::array forward(const mx::array &X,
                    const std::optional<mx::array> &Mask = std::nullopt,
                    const std::optional<std::shared_ptr<vlm::BaseCache>>
                        &Cache = std::nullopt);

private:
  int NHeads;
  int NKVHeads;
  int Repeats;
  int HeadDim;
  int LayerIdx;
  float Scale;
  RMSNorm QNorm;
  RMSNorm KNorm;
  bool IsSliding;
};

class MLP : public nn::Module {
public:
  MLP(int Dim, int HiddenDim);
  mx::array forward(const mx::array &X);
};

class TransformerBlock : public nn::Module {
public:
  TransformerBlock(const TextConfig &Config, int LayerIdx);
  mx::array forward(const mx::array &X,
                    const std::optional<mx::array> &Mask = std::nullopt,
                    const std::optional<std::shared_ptr<vlm::BaseCache>>
                        &Cache = std::nullopt);

private:
  int NumAttentionHeads;
  int HiddenSize;
};

class Gemma3Model : public nn::Module {
public:
  Gemma3Model(const TextConfig &Config);
  mx::array forward(
      const mx::array &Inputs,
      const std::optional<mx::array> &InputsEmbeds = std::nullopt,
      const std::optional<mx::array> &Mask = std::nullopt,
      const std::optional<std::vector<std::shared_ptr<vlm::BaseCache>>> &Cache =
          std::nullopt);
  std::vector<std::shared_ptr<TransformerBlock>> Layers;
  TextConfig Config;
};

class LanguageModel : public vlm::LanguageModel {
public:
  LanguageModel(const TextConfig &Config);
  std::tuple<mx::array, std::optional<mx::array>> forward(
      const mx::array &Inputs,
      const std::optional<std::vector<std::shared_ptr<vlm::BaseCache>>> &Cache =
          std::nullopt) override {
    return forward(Inputs, std::nullopt, std::nullopt, Cache);
  }
  std::tuple<mx::array, std::optional<mx::array>> forward(
      const mx::array &Inputs,
      const std::optional<mx::array> &InputsEmbeds = std::nullopt,
      const std::optional<mx::array> &Mask = std::nullopt,
      const std::optional<std::vector<std::shared_ptr<vlm::BaseCache>>> &Cache =
          std::nullopt);
  std::unordered_map<std::string, mx::array>
  sanitize(const std::unordered_map<std::string, mx::array> &Weights);
  int headDim() const override;
  int nKvHeads() const override;
  int layers() const override;
  std::vector<std::shared_ptr<vlm::BaseCache>> makeCache() override;
  TextConfig Config;
};

} // namespace gemma3
} // namespace WasmEdge::Host::WASINN::MLX
