// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "mlx/base.h"
#include "mlx/pooling.h"
#include "model/gemma3/language.h"
#include "model/gemma3/vision.h"
#include <memory>
#include <mlx/array.h>
#include <optional>
#include <string>
#include <unordered_map>
namespace WasmEdge::Host::WASINN::MLX {

namespace nn = mlx::core::nn;
namespace gemma3 {

struct ModelConfig {
  TextConfig TextConfig;
  VisionConfig VisionConfig;
  std::string ModelType;
  int VocabSize = 257152;
  int IgnoreIndex = -100;
  int ImageTokenIndex = 257152;
  int HiddenSize = 2048;
  int PadTokenId = 0;
  static ModelConfig fromDict(const simdjson::dom::object &Obj);
};

class Gemma3MultiModalProjector : public nn::Module {
public:
  Gemma3MultiModalProjector(const ModelConfig &Config);
  mx::array forward(const mx::array &X);

private:
  nn::AvgPool2d AvgPool = nn::AvgPool2d({0});
  int PatchesPerImage;
  int TokensPerSide;
  int KernelSize;
};

class Model : public vlm::Module {
public:
  Model(const ModelConfig &Config);
  std::pair<mx::array, mx::array>
  getInputEmbeddings(const mx::array &InputIds, const mx::array &PixelValues,
                     const mx::array &Mask);
  std::pair<mx::array, mx::array> _prepareInputsForMultimodal(
      const mx::array &ImageFeatures, const mx::array &InputsEmbeds,
      const mx::array &InputIds, const mx::array &AttentionMask);
  std::tuple<mx::array, std::optional<mx::array>> forward(
      const mx::array &InputIds, const mx::array &PixelValues,
      const mx::array &Mask,
      const std::optional<std::vector<std::shared_ptr<vlm::BaseCache>>> &Cache =
          std::nullopt) override;
  static std::shared_ptr<Model> fromPretrained(const std::string &ModelPath);
  std::unordered_map<std::string, mx::array>
  sanitize(const std::unordered_map<std::string, mx::array> &Weights);
  ModelConfig Config;
  std::string ModelType;
};

} // namespace gemma3
} // namespace WasmEdge::Host::WASINN::MLX
