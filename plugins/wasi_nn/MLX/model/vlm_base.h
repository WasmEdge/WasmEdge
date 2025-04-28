// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once
#include "mlx/base.h"
#include <map>
#include <mlx/array.h>
#include <optional>
#include <sstream>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace WasmEdge::Host::WASINN::MLX {

namespace vlm {
class BaseCache {
public:
  int Offset = 0;

  virtual ~BaseCache() = default;

  virtual std::tuple<mx::array, mx::array>
  updateAndFetch(const mx::array &NewKeys, const mx::array &NewValues) = 0;

  virtual std::vector<mx::array> getState() const;
  virtual void setState(const std::vector<mx::array> &State);

  virtual std::string getMetaState() const;
  virtual void setMetaState(const std::string &Value);

  virtual bool isTrimmable() const;
  virtual int trim(int N);
  virtual std::string getType() const { return "BaseCache"; }
};

class KVCache : public BaseCache {
public:
  int NKVHeads;
  int KHeadDim;
  int VHeadDim;
  mx::array Keys = mx::array({});
  mx::array Values = mx::array({});
  int Step;

  KVCache(int HeadDim, int NKVHeads, int Step = 256);
  KVCache(std::pair<int, int> HeadDims, int NKVHeads, int Step = 256);
  virtual std::tuple<mx::array, mx::array>
  updateAndFetch(const mx::array &NewKeys, const mx::array &NewValues) override;

  std::tuple<mx::array, mx::array> fetch() const;

  void update(const mx::array &NewKeys, const mx::array &NewValues);

  std::vector<mx::array> getState() const override;
  void setState(const std::vector<mx::array> &State) override;

  bool isTrimmable() const override;
  int trim(int N) override;
  std::string getType() const override { return "KVCache"; }
};

class RotatingKVCache : public KVCache {
public:
  int Keep;
  int MaxSize;
  int Idx;

  RotatingKVCache(int MaxSize = -1, int Keep = 0, int Step = 256);

  std::tuple<mx::array, mx::array>
  updateAndFetch(const mx::array &NewKeys, const mx::array &NewValues) override;

  std::tuple<mx::array, mx::array> updateInPlace(const mx::array &NewKeys,
                                                 const mx::array &NewValues);

  std::tuple<mx::array, mx::array> updateConcat(const mx::array &NewKeys,
                                                const mx::array &NewValues);

  mx::array trim(int TrimSize, const mx::array &V,
                 std::optional<mx::array> Append = std::nullopt);

  mx::array temporalOrder(const mx::array &V);

  std::string getMetaState() const override;
  void setMetaState(const std::string &Value) override;

  bool isTrimmable() const override;
  int trim(int N) override;
  std::string getType() const override { return "RotatingKVCache"; }
};

class Module : public mlx::core::nn::Module {
public:
  virtual std::tuple<mx::array, std::optional<mx::array>> forward(
      const mx::array &InputIds, const mx::array &PixelValues,
      const mx::array &Mask,
      const std::optional<std::vector<std::shared_ptr<vlm::BaseCache>>> &Cache =
          std::nullopt) = 0;
  struct GenerationResult {
    std::string Text;
    int Token;
    std::vector<float> LogProbs;
    int PromptTokens;
    int GenerationTokens;
    float PromptTps;
    float GenerationTps;
    float PeakMemory;
  };

  // Add this struct to hold generation state
  struct StreamGenerationState {
    void *Model;
    void *Processor;
    mx::array PromptTokens;
    mx::array InputIds;
    mx::array PixelValues;
    mx::array Mask;
    mx::array CurrentToken;
    std::vector<float> CurrentLogProbs;
    int TokenCount;
    double StartTime;
    double PromptTime;
    float PromptTps;
    bool IsComplete;
    std::map<std::string, mx::array> Kwargs;

    // Generation parameters
    int MaxTokens = 256;
    float Temperature = 0.0;
    std::optional<float> RepetitionPenalty = std::nullopt;
    std::optional<int> RepetitionContextSize = 20;
    float TopP = 1.0;
    std::map<int, float> LogitBias;

    // State for generate_step
    std::vector<int> RepetitionContext;
    std::vector<void *> Cache; // Will hold appropriate cache objects
    mx::array CrossAttentionStates;
    mx::array EncoderOutputs;
  };

  std::vector<int> generate(
      const std::string &Prompt = {},
      std::optional<std::string> Image = std::nullopt, bool Verbose = false,
      std::map<std::string, std::variant<mx::array, int, float, std::string>>
          Kwargs = {});
};
class LanguageModel : public mlx::core::nn::Module {
public:
  virtual int headDim() const = 0;
  virtual int nKvHeads() const = 0;
  virtual int layers() const = 0;
  virtual std::vector<std::shared_ptr<BaseCache>> makeCache() {
    std::vector<std::shared_ptr<BaseCache>> Cache;
    int HeadDim = headDim();
    auto KVHeads = nKvHeads();
    for (int I = 0; I < layers(); ++I) {
      Cache.emplace_back(std::make_shared<KVCache>(HeadDim, KVHeads));
    }
    return Cache;
  }
  virtual std::tuple<mx::array, std::optional<mx::array>> forward(
      const mx::array &Inputs,
      const std::optional<std::vector<std::shared_ptr<vlm::BaseCache>>> &Cache =
          std::nullopt) = 0;
};

std::optional<mx::array> createAttentionMask(
    mx::array H,
    std::optional<std::vector<std::shared_ptr<vlm::BaseCache>>> = std::nullopt);

mx::array createAdditiveCausalMask(int N, int Offset = 0);

} // namespace vlm
} // namespace WasmEdge::Host::WASINN::MLX
