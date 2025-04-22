// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "mlx/base.h"
#include "simdjson.h"
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace WasmEdge::Host::WASINN::MLX {
namespace nn = mlx::core::nn;
namespace gemma3 {

struct VisionConfig {
  std::string ModelType = "siglip_vision_model";
  int NumHiddenLayers = 27;
  int HiddenSize = 1152;
  int IntermediateSize = 4304;
  int NumAttentionHeads = 16;
  int PatchSize = 14;
  int ImageSize = 896;
  int NumChannels = 3;
  float LayerNormEps = 1e-6f;
  static VisionConfig fromDict(const simdjson::dom::object &Obj);
};

bool checkArrayShape(const mx::array &Arr);

class VisionAttention : public nn::Module {
public:
  VisionAttention(int Dims, int NumHeads,
                  std::optional<int> QueryInputDims = std::nullopt,
                  std::optional<int> KeyInputDims = std::nullopt,
                  std::optional<int> ValueInputDims = std::nullopt,
                  std::optional<int> ValueDims = std::nullopt,
                  std::optional<int> ValueOutputDims = std::nullopt,
                  bool Bias = true);
  mx::array forward(const mx::array &X,
                    const std::optional<mx::array> &Mask = std::nullopt);

private:
  int NumHeads;
  float Scale;
};

class VisionMLP : public nn::Module {
public:
  VisionMLP(const VisionConfig &Config);
  mx::array forward(const mx::array &X);
};

class EncoderLayer : public nn::Module {
public:
  EncoderLayer(const VisionConfig &Config);
  mx::array forward(const mx::array &X,
                    const std::optional<mx::array> &Mask = std::nullopt);

private:
  int EmbedDim;
};

class Encoder : public nn::Module {
public:
  Encoder(const VisionConfig &Config);
  std::pair<mx::array, std::vector<mx::array>>
  forward(const mx::array &X,
          const std::optional<bool> &OutputHiddenStates = std::nullopt,
          const std::optional<mx::array> &Mask = std::nullopt);

private:
  std::vector<std::shared_ptr<EncoderLayer>> Layers;
};

class VisionEmbeddings : public nn::Module {
public:
  VisionEmbeddings(const VisionConfig &Config);
  mx::array forward(const mx::array &X);

private:
  VisionConfig Config;
  int EmbedDim;
  int ImageSize;
  int PatchSize;
  int NumPatches;
  int NumPositions;
};

class SigLipVisionModel : public nn::Module {
public:
  SigLipVisionModel(const VisionConfig &Config);
  std::tuple<mx::array, mx::array, mx::array>
  forward(const mx::array &X,
          const std::optional<bool> &OutputHiddenStates = std::nullopt);
};

class VisionModel : public nn::Module {
public:
  VisionModel(const VisionConfig &Config);
  std::tuple<mx::array, mx::array, mx::array>
  forward(const mx::array &X,
          const std::optional<bool> &OutputHiddenStates = std::nullopt);
  std::unordered_map<std::string, mx::array>
  sanitize(const std::unordered_map<std::string, mx::array> &Weights);

private:
  std::string ModelType;
};

} // namespace gemma3
} // namespace WasmEdge::Host::WASINN::MLX
