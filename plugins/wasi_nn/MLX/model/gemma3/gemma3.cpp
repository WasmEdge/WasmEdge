// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "model/gemma3/gemma3.h"
#include "language.h"
#include "mlx/embedding.h"
#include "vision.h"
#include <filesystem>
#include <memory>
#include <mlx/array.h>
#include <mlx/ops.h>
#include <vector>

namespace WasmEdge::Host::WASINN::MLX {
namespace gemma3 {

ModelConfig ModelConfig::fromDict(const simdjson::dom::object &Obj) {
  ModelConfig Config;
  auto ModelTypeResult = Obj["model_type"].get_string();
  if (!ModelTypeResult.error()) {
    Config.ModelType = std::string(ModelTypeResult.value());
  }
  auto VocabResult = Obj["vocab_size"].get_int64();
  if (!VocabResult.error()) {
    Config.VocabSize = static_cast<int>(VocabResult.value());
  }
  auto IgnoreResult = Obj["ignore_index"].get_int64();
  if (!IgnoreResult.error()) {
    Config.IgnoreIndex = static_cast<int>(IgnoreResult.value());
  }
  auto ImageTokenResult = Obj["image_token_index"].get_int64();
  if (!ImageTokenResult.error()) {
    Config.ImageTokenIndex = static_cast<int>(ImageTokenResult.value());
  }
  auto HiddenSizeResult = Obj["hidden_size"].get_int64();
  if (!HiddenSizeResult.error()) {
    Config.HiddenSize = static_cast<int>(HiddenSizeResult.value());
  }
  auto PadTokenResult = Obj["pad_token_id"].get_int64();
  if (!PadTokenResult.error()) {
    Config.PadTokenId = static_cast<int>(PadTokenResult.value());
  }
  return Config;
}

Gemma3MultiModalProjector::Gemma3MultiModalProjector(
    const ModelConfig &Config) {
  registerModule("mm_soft_emb_norm",
                 std::make_shared<RMSNorm>(Config.VisionConfig.HiddenSize,
                                           Config.VisionConfig.LayerNormEps));
  registerParameter(
      "mm_input_projection_weight",
      mx::ones({Config.VisionConfig.HiddenSize, Config.TextConfig.HiddenSize}));
  PatchesPerImage =
      Config.VisionConfig.ImageSize / Config.VisionConfig.PatchSize;
  TokensPerSide = static_cast<int>(
      std::sqrt(static_cast<float>(Config.TextConfig.MmTokensPerImage)));
  KernelSize = PatchesPerImage / TokensPerSide;
  AvgPool =
      nn::AvgPool2d(std::vector<int>{KernelSize}, std::vector<int>{KernelSize});
}

mx::array Gemma3MultiModalProjector::forward(const mx::array &X) {
  int B = X.shape()[0];
  int L = X.shape()[2];
  mx::array ReshapedVisionOutputs = transpose(X, {0, 2, 1});
  ReshapedVisionOutputs =
      reshape(ReshapedVisionOutputs, {B, L, PatchesPerImage, PatchesPerImage});
  ReshapedVisionOutputs = transpose(ReshapedVisionOutputs, {0, 2, 3, 1});
  mx::array PooledVisionOutputs = AvgPool.forward(ReshapedVisionOutputs);
  PooledVisionOutputs = transpose(PooledVisionOutputs, {0, 3, 1, 2});
  PooledVisionOutputs = flatten(PooledVisionOutputs, 2);
  PooledVisionOutputs = transpose(PooledVisionOutputs, {0, 2, 1});
  mx::array NormedVisionOutputs =
      std::dynamic_pointer_cast<RMSNorm>(Submodules["mm_soft_emb_norm"])
          ->forward(PooledVisionOutputs);
  mx::array ProjectedVisionOutputs = mx::einsum(
      "btm,md->btd",
      std::vector<mx::array>(
          {NormedVisionOutputs, Parameters.at("mm_input_projection_weight")}));
  return astype(ProjectedVisionOutputs, X.dtype());
}

Model::Model(const ModelConfig &Config) : Config(Config) {
  registerModule("vision_tower",
                 std::make_shared<VisionModel>(Config.VisionConfig));
  registerModule("language_model",
                 std::make_shared<LanguageModel>(Config.TextConfig));
  registerModule("multi_modal_projector",
                 std::make_shared<Gemma3MultiModalProjector>(Config));
  ModelType = Config.ModelType;
}

std::pair<mx::array, mx::array>
Model::getInputEmbeddings(const mx::array &InputIds,
                          const mx::array &PixelValues, const mx::array &Mask) {
  if (PixelValues.size() == 0) {
    mx::array Embeds = std::dynamic_pointer_cast<nn::Embedding>(
                           Submodules["language_model"]
                               ->Submodules["model"]
                               ->Submodules["embed_tokens"])
                           ->forward(InputIds);
    return {Embeds, mx::array({})};
  }
  mx::array InputsEmbeds =
      std::dynamic_pointer_cast<nn::Embedding>(Submodules["language_model"]
                                                   ->Submodules["model"]
                                                   ->Submodules["embed_tokens"])
          ->forward(InputIds);
  mx::array HiddenState = mx::array({}), Temp1 = mx::array({}),
            Temp2 = mx::array({});
  std::tie(HiddenState, Temp1, Temp2) =
      std::dynamic_pointer_cast<VisionModel>(Submodules["vision_tower"])
          ->forward(astype(transpose(PixelValues, {0, 2, 3, 1}),
                           InputsEmbeds.dtype()),
                    true);
  auto NewShape = HiddenState.shape();
  NewShape.insert(NewShape.begin(), 1);
  mx::array ImageFeatures =
      astype(reshape(HiddenState, NewShape), PixelValues.dtype());
  ImageFeatures = std::dynamic_pointer_cast<Gemma3MultiModalProjector>(
                      Submodules["multi_modal_projector"])
                      ->forward(ImageFeatures);
  return _prepareInputsForMultimodal(ImageFeatures, InputsEmbeds, InputIds,
                                     Mask);
}

std::pair<mx::array, mx::array> Model::_prepareInputsForMultimodal(
    const mx::array &ImageFeatures, const mx::array &InputsEmbeds,
    const mx::array &InputIds, const mx::array &AttentionMask) {
  int EmbedDim = ImageFeatures.shape().back();
  int BatchSize = InputIds.shape()[0];
  int SequenceLength = InputIds.shape()[1];
  mx::array ScaledImageFeatures =
      ImageFeatures / std::pow(Config.HiddenSize, 0.5);
  mx::array FinalEmbedding = mx::zeros({BatchSize, SequenceLength, EmbedDim});
  int PadTokenId = Config.PadTokenId;
  mx::array TextMask =
      (InputIds != Config.ImageTokenIndex) & (InputIds != PadTokenId);
  mx::array ImageMask = (InputIds == Config.ImageTokenIndex);
  mx::array PadMask = (InputIds == PadTokenId);
  mx::array TextMaskExpanded = expand_dims(TextMask, -1);
  TextMaskExpanded = repeat(TextMaskExpanded, EmbedDim, -1);
  mx::array PadMaskExpanded = expand_dims(PadMask, -1);
  PadMaskExpanded = repeat(PadMaskExpanded, EmbedDim, -1);
  FinalEmbedding = mx::where(TextMaskExpanded, InputsEmbeds, FinalEmbedding);
  FinalEmbedding = mx::where(PadMaskExpanded, mx::zeros_like(FinalEmbedding),
                             FinalEmbedding);
  int PadSize = FinalEmbedding.shape()[1] - ScaledImageFeatures.shape()[1];
  ScaledImageFeatures =
      mx::pad(ScaledImageFeatures, {{0, 0}, {0, PadSize}, {0, 0}});
  mx::array ImageMaskExpanded = expand_dims(ImageMask, -1);
  ImageMaskExpanded = repeat(ImageMaskExpanded, EmbedDim, -1);
  FinalEmbedding =
      mx::where(ImageMaskExpanded, ScaledImageFeatures, FinalEmbedding);
  FinalEmbedding = mx::where(PadMaskExpanded, mx::zeros_like(FinalEmbedding),
                             FinalEmbedding);
  mx::array AttentionMaskExpanded1 = expand_dims(AttentionMask, 1);
  mx::array AttentionMaskExpanded2 = expand_dims(AttentionMask, 2);
  mx::array FinalAttentionMask4d =
      AttentionMaskExpanded1 * AttentionMaskExpanded2;
  FinalAttentionMask4d = expand_dims(FinalAttentionMask4d, 1);
  return {FinalEmbedding, FinalAttentionMask4d};
}

std::tuple<mx::array, std::optional<mx::array>> Model::forward(
    const mx::array &InputIds, const mx::array &PixelValues,
    const mx::array &Mask,
    const std::optional<std::vector<std::shared_ptr<vlm::BaseCache>>> &Cache) {
  auto Pair = getInputEmbeddings(InputIds, PixelValues, Mask);
  mx::array InputEmbeds = Pair.first;

  // TODO: Waiting for upstream fix Mask
  auto Logits = std::dynamic_pointer_cast<gemma3::LanguageModel>(
                    Submodules["language_model"])
                    ->forward(InputIds, InputEmbeds, std::nullopt, Cache);
  return Logits;
}

std::shared_ptr<Model> Model::fromPretrained(const std::string &ModelPath) {
  std::filesystem::path Path(ModelPath);
  simdjson::dom::parser Parser;
  simdjson::dom::element Doc;
  auto Error = Parser.load((Path / "config.json").string()).get(Doc);
  if (Error) {
    spdlog::error("Could not open config.json");
    assumingUnreachable();
  }
  auto Obj = Doc.get_object();
  ModelConfig ModelConfigObj = ModelConfig::fromDict(Obj.value());
  ModelConfigObj.VisionConfig =
      VisionConfig::fromDict(Obj["vision_config"].get_object().value());
  ModelConfigObj.TextConfig =
      TextConfig::fromDict(Obj["text_config"].get_object().value());
  auto Model = std::make_shared<gemma3::Model>(gemma3::Model(ModelConfigObj));
  std::vector<std::filesystem::path> WeightFiles;
  for (auto &P : std::filesystem::directory_iterator(Path)) {
    if (P.path().extension() == ".safetensors")
      WeightFiles.push_back(P.path());
  }
  if (WeightFiles.empty()) {
    spdlog::error("No safetensors found in {}.", Path.string());
    assumingUnreachable();
  }
  std::unordered_map<std::string, mx::array> Weights;
  for (auto &Wf : WeightFiles) {
    auto W = mx::load_safetensors(Wf.string());
    Weights.insert(W.first.begin(), W.first.end());
  }
  Weights = Model->sanitize(Weights);
  Weights = gemma3::VisionModel(ModelConfigObj.VisionConfig).sanitize(Weights);
  auto QuantResult = Obj["quantization"].get_object();
  if (!QuantResult.error()) {
    auto GroupSize =
        static_cast<int>(QuantResult.value()["group_size"].get_int64());
    auto Bits = static_cast<int>(QuantResult.value()["bits"].get_int64());
    spdlog::info("Quantizing model to {} bits, {} group size.", Bits,
                 GroupSize);
    Model = std::dynamic_pointer_cast<gemma3::Model>(
        Model->toQuantized(GroupSize, Bits, "", Weights));
  }
  Model->update(Weights);
  return Model;
}

std::unordered_map<std::string, mx::array>
Model::sanitize(const std::unordered_map<std::string, mx::array> &Weights) {
  std::unordered_map<std::string, mx::array> Sanitized;
  for (auto &Pair : Weights) {
    std::string Key = Pair.first;
    if (Key.find("vision_tower") == std::string::npos) {
      size_t Pos = Key.find("vision_model");
      if (Pos != std::string::npos)
        Key.replace(Pos, std::string("vision_model").length(), "vision_tower");
    }
    if (Key.find("model") == 0) {
      Key.replace(0, std::string("model").length(), "");
    }
    Sanitized.insert({Key, Pair.second});
  }
  return Sanitized;
}

} // namespace gemma3
} // namespace WasmEdge::Host::WASINN::MLX
