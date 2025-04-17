// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "model/gemma3/vision.h"
#include "mlx/activations.h"
#include "mlx/convolution.h"
#include "mlx/embedding.h"
#include "mlx/linear.h"
#include "mlx/normalization.h"
#include <cmath>
#include <stdexcept>

namespace WasmEdge::Host::WASINN::MLX {
namespace gemma3 {

VisionConfig VisionConfig::fromDict(const simdjson::dom::object &Obj) {
  VisionConfig Config;
  auto ModelTypeResult = Obj["model_type"].get_string();
  if (!ModelTypeResult.error()) {
    Config.ModelType = std::string(ModelTypeResult.value());
  }
  auto NumHiddenLayersResult = Obj["num_hidden_layers"].get_int64();
  if (!NumHiddenLayersResult.error()) {
    Config.NumHiddenLayers = static_cast<int>(NumHiddenLayersResult.value());
  }
  auto HiddenSizeResult = Obj["hidden_size"].get_int64();
  if (!HiddenSizeResult.error()) {
    Config.HiddenSize = static_cast<int>(HiddenSizeResult.value());
  }
  auto IntermediateSizeResult = Obj["intermediate_size"].get_int64();
  if (!IntermediateSizeResult.error()) {
    Config.IntermediateSize = static_cast<int>(IntermediateSizeResult.value());
  }
  auto NumAttentionHeadsResult = Obj["num_attention_heads"].get_int64();
  if (!NumAttentionHeadsResult.error()) {
    Config.NumAttentionHeads =
        static_cast<int>(NumAttentionHeadsResult.value());
  }
  auto PatchSizeResult = Obj["patch_size"].get_int64();
  if (!PatchSizeResult.error()) {
    Config.PatchSize = static_cast<int>(PatchSizeResult.value());
  }
  auto ImageSizeResult = Obj["image_size"].get_int64();
  if (!ImageSizeResult.error()) {
    Config.ImageSize = static_cast<int>(ImageSizeResult.value());
  }
  auto NumChannelsResult = Obj["num_channels"].get_int64();
  if (!NumChannelsResult.error()) {
    Config.NumChannels = static_cast<int>(NumChannelsResult.value());
  }
  auto LayerNormEpsResult = Obj["layer_norm_eps"].get_double();
  if (!LayerNormEpsResult.error()) {
    Config.LayerNormEps = static_cast<float>(LayerNormEpsResult.value());
  }
  return Config;
}

bool checkArrayShape(const mx::array &Arr) {
  auto Shape = Arr.shape();
  if (Shape.size() != 4)
    return false;
  int OutChannels = Shape[0];
  int KH = Shape[1];
  int KW = Shape[2];
  return (OutChannels >= KH) && (OutChannels >= KW) && (KH == KW);
}

VisionAttention::VisionAttention(int Dims, int NumHeads,
                                 std::optional<int> QueryInputDims,
                                 std::optional<int> KeyInputDims,
                                 std::optional<int> ValueInputDims,
                                 std::optional<int> ValueDims,
                                 std::optional<int> ValueOutputDims, bool Bias)
    : NumHeads(NumHeads) {
  if (Dims % NumHeads != 0)
    throw std::invalid_argument("Invalid dims");
  int QInput = QueryInputDims.value_or(Dims);
  int KInput = KeyInputDims.value_or(Dims);
  int VInput = ValueInputDims.value_or(KInput);
  int VDim = ValueDims.value_or(Dims);
  int VOutDim = ValueOutputDims.value_or(Dims);
  int HeadDim = Dims / NumHeads;
  Scale = std::pow(HeadDim, -0.5);
  registerModule("q_proj", std::make_shared<nn::Linear>(QInput, Dims, Bias));
  registerModule("k_proj", std::make_shared<nn::Linear>(KInput, Dims, Bias));
  registerModule("v_proj", std::make_shared<nn::Linear>(VInput, VDim, Bias));
  registerModule("out_proj", std::make_shared<nn::Linear>(VDim, VOutDim, Bias));
}

mx::array VisionAttention::forward(const mx::array &X,
                                   const std::optional<mx::array> &Mask) {
  mx::array Queries =
      std::dynamic_pointer_cast<nn::Linear>(Submodules["q_proj"])->forward(X);
  mx::array Keys =
      std::dynamic_pointer_cast<nn::Linear>(Submodules["k_proj"])->forward(X);
  mx::array Values =
      std::dynamic_pointer_cast<nn::Linear>(Submodules["v_proj"])->forward(X);
  int B = Queries.shape()[0];
  int L = Queries.shape()[1];
  Queries = transpose(reshape(Queries, {B, L, NumHeads, -1}), {0, 2, 1, 3});
  int S = Keys.shape()[1];
  Keys = transpose(reshape(Keys, {B, S, NumHeads, -1}), {0, 2, 1, 3});
  Values = transpose(reshape(Values, {B, S, NumHeads, -1}), {0, 2, 1, 3});
  mx::array Output =
      Mask.has_value() ? mx::fast::scaled_dot_product_attention(
                             Queries, Keys, Values, Scale, Mask.value())
                       : mx::fast::scaled_dot_product_attention(Queries, Keys,
                                                                Values, Scale);
  Output = reshape(transpose(Output, {0, 2, 1, 3}), {B, L, -1});
  return std::dynamic_pointer_cast<nn::Linear>(Submodules["out_proj"])
      ->forward(Output);
}

VisionMLP::VisionMLP(const VisionConfig &Config) {
  registerModule("fc1", std::make_shared<nn::Linear>(
                            Config.HiddenSize, Config.IntermediateSize, true));
  registerModule("fc2", std::make_shared<nn::Linear>(Config.IntermediateSize,
                                                     Config.HiddenSize, true));
}

mx::array VisionMLP::forward(const mx::array &X) {
  mx::array Out =
      std::dynamic_pointer_cast<nn::Linear>(Submodules["fc1"])->forward(X);
  Out = mlx::core::geluApprox(Out);
  Out = std::dynamic_pointer_cast<nn::Linear>(Submodules["fc2"])->forward(Out);
  return Out;
}

EncoderLayer::EncoderLayer(const VisionConfig &Config) {
  EmbedDim = Config.HiddenSize;
  registerModule("self_attn", std::make_shared<VisionAttention>(
                                  Config.HiddenSize, Config.NumAttentionHeads,
                                  std::nullopt, std::nullopt, std::nullopt,
                                  std::nullopt, std::nullopt, true));
  registerModule("layer_norm1", std::make_shared<nn::LayerNorm>(
                                    EmbedDim, Config.LayerNormEps));
  registerModule("mlp", std::make_shared<VisionMLP>(Config));
  registerModule("layer_norm2", std::make_shared<nn::LayerNorm>(
                                    EmbedDim, Config.LayerNormEps));
}

mx::array EncoderLayer::forward(const mx::array &X,
                                const std::optional<mx::array> &Mask) {
  mx::array R =
      std::dynamic_pointer_cast<VisionAttention>(Submodules["self_attn"])
          ->forward(std::dynamic_pointer_cast<nn::LayerNorm>(
                        Submodules["layer_norm1"])
                        ->forward(X),
                    Mask);
  mx::array H = X + R;
  R = std::dynamic_pointer_cast<VisionMLP>(Submodules["mlp"])
          ->forward(std::dynamic_pointer_cast<nn::LayerNorm>(
                        Submodules["layer_norm2"])
                        ->forward(H));
  return H + R;
}

Encoder::Encoder(const VisionConfig &Config) {
  for (int I = 0; I < Config.NumHiddenLayers; I++) {
    Layers.push_back(std::make_shared<EncoderLayer>(Config));
  }
  registerLayer("layers", Layers);
}

std::pair<mx::array, std::vector<mx::array>>
Encoder::forward(const mx::array &X,
                 const std::optional<bool> &OutputHiddenStates,
                 const std::optional<mx::array> &Mask) {
  std::vector<mx::array> EncoderStates;
  if (OutputHiddenStates.has_value() && OutputHiddenStates.value())
    EncoderStates.push_back(X);
  mx::array Out = X;
  mx::array H = X;
  for (auto &L : Layers) {
    Out = L->forward(Out, Mask);
    if (OutputHiddenStates.has_value() && OutputHiddenStates.value())
      EncoderStates.push_back(Out);
    H = take(Out, 0, 0);
  }
  return {H, EncoderStates};
}

VisionEmbeddings::VisionEmbeddings(const VisionConfig &Config)
    : Config(Config) {
  EmbedDim = Config.HiddenSize;
  ImageSize = Config.ImageSize;
  PatchSize = Config.PatchSize;
  registerModule("patch_embedding", std::make_shared<nn::Conv2d>(nn::Conv2d(
                                        Config.NumChannels, EmbedDim, PatchSize,
                                        {PatchSize, PatchSize})));
  NumPatches = (ImageSize / PatchSize) * (ImageSize / PatchSize);
  NumPositions = NumPatches;
  registerModule("position_embedding",
                 std::make_shared<nn::Embedding>(NumPositions, EmbedDim));
}

mx::array VisionEmbeddings::forward(const mx::array &X) {
  mx::array PatchEmbeddings =
      std::dynamic_pointer_cast<nn::Conv2d>(Submodules["patch_embedding"])
          ->forward(X);
  PatchEmbeddings = mx::flatten(PatchEmbeddings, 1, 2);
  std::vector<int64_t> PositionIdsShapeVec;
  for (int I = 0; I < NumPositions; I++) {
    PositionIdsShapeVec.emplace_back(I);
  }
  mx::array PositionIds =
      mx::array(PositionIdsShapeVec.data(), {1, NumPositions});
  mx::array Embeddings = PatchEmbeddings;
  Embeddings = Embeddings + std::dynamic_pointer_cast<nn::Embedding>(
                                Submodules["position_embedding"])
                                ->forward(PositionIds);
  return Embeddings;
}

SigLipVisionModel::SigLipVisionModel(const VisionConfig &Config) {
  registerModule("embeddings", std::make_shared<VisionEmbeddings>(Config));
  registerModule("encoder", std::make_shared<Encoder>(Config));
  registerModule("post_layernorm",
                 std::make_shared<nn::LayerNorm>(Config.HiddenSize));
}

std::tuple<mx::array, mx::array, mx::array>
SigLipVisionModel::forward(const mx::array &X,
                           const std::optional<bool> &OutputHiddenStates) {
  mx::array Emb =
      std::dynamic_pointer_cast<VisionEmbeddings>(Submodules["embeddings"])
          ->forward(X);
  auto EncoderOutputs =
      std::dynamic_pointer_cast<Encoder>(Submodules["encoder"])
          ->forward(Emb, OutputHiddenStates, std::nullopt);
  mx::array PoolerOutput =
      std::dynamic_pointer_cast<nn::LayerNorm>(Submodules["post_layernorm"])
          ->forward(std::get<0>(EncoderOutputs));
  return {PoolerOutput, Emb, std::get<1>(EncoderOutputs).back()};
}

VisionModel::VisionModel(const VisionConfig &Config) {
  ModelType = Config.ModelType;
  if (ModelType != "siglip_vision_model" && ModelType != "gemma3" &&
      ModelType != "gemma3_vision")
    throw std::invalid_argument("Unsupported model type: " + ModelType);
  registerModule("vision_model", std::make_shared<SigLipVisionModel>(Config));
}

std::tuple<mx::array, mx::array, mx::array>
VisionModel::forward(const mx::array &X,
                     const std::optional<bool> &OutputHiddenStates) {
  return std::dynamic_pointer_cast<SigLipVisionModel>(
             Submodules["vision_model"])
      ->forward(X, OutputHiddenStates);
}

std::unordered_map<std::string, mx::array> VisionModel::sanitize(
    const std::unordered_map<std::string, mx::array> &Weights) {
  std::unordered_map<std::string, mx::array> SanitizedWeights;
  for (auto &Pair : Weights) {
    if (Pair.first.find("patch_embedding.weight") != std::string::npos) {
      if (checkArrayShape(Pair.second))
        SanitizedWeights.insert({Pair.first, Pair.second});
      else
        SanitizedWeights.insert(
            {Pair.first, transpose(Pair.second, {0, 2, 3, 1})});
    } else {
      SanitizedWeights.insert({Pair.first, Pair.second});
    }
  }
  return SanitizedWeights;
}

} // namespace gemma3
} // namespace WasmEdge::Host::WASINN::MLX
