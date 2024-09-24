#include "quantized.h"
#include <iostream>
#include <mlx/array.h>
#include <mlx/ops.h>
#include <utility>
namespace mlx::core::nn {
mx::array QuantizedEmbedding::forward(mx::array Input) {
  auto S = Input.shape();
  auto X = mx::flatten(Input);
  auto Out =
      mx::dequantize(take(Parameters.at("weight"), Input, 0),
                     take(Parameters.at("scales"), Input, 0),
                     take(Parameters.at("biases"), Input, 0), GroupSize, Bits);
  S.emplace_back(-1);
  return reshape(Out, {S});
}

mx::array QuantizedLinear::forward(mx::array Input) {
  auto Out = mx::quantized_matmul(
      Input, Parameters.at("weight"), Parameters.at("scales"),
      Parameters.at("biases"), true, GroupSize, Bits);
  if (Parameters.find("bias") != Parameters.end()) {
    Out = add(Out, Parameters.at("bias"));
  }
  return Out;
}
QuantizedEmbedding *
QuantizedEmbedding::fromEmbedding(Embedding *EmbeddingModule, int GroupSize,
                                  int Bits) {
  auto EmbeddingShape = EmbeddingModule->Parameters.at("weight").shape();
  auto *QuantizedModel = new QuantizedEmbedding(
      EmbeddingShape[0], EmbeddingShape[1], GroupSize, Bits);
  auto Quantized =
      mx::quantize(EmbeddingModule->Parameters.at("weight"), GroupSize, Bits);
  QuantizedModel->Parameters.insert_or_assign("weight", std::get<0>(Quantized));
  QuantizedModel->Parameters.insert_or_assign(
      "scales", std::move(std::get<1>(Quantized)));
  QuantizedModel->Parameters.insert_or_assign(
      "biases", std::move(std::get<2>(Quantized)));
  return QuantizedModel;
}
QuantizedLinear *QuantizedLinear::fromLinear(Linear *LinearModule,
                                             int GroupSize, int Bits) {
  auto LinearShape = LinearModule->Parameters.at("weight").shape();
  const bool EnableBias =
      LinearModule->Parameters.find("bias") != LinearModule->Parameters.end();
  auto *QuantizedModel = new QuantizedLinear(LinearShape[0], LinearShape[1],
                                             EnableBias, GroupSize, Bits);
  auto Quantized =
      mx::quantize(LinearModule->Parameters.at("weight"), GroupSize, Bits);
  QuantizedModel->Parameters.insert_or_assign("weight", std::get<0>(Quantized));
  QuantizedModel->Parameters.insert_or_assign(
      "scales", std::move(std::get<1>(Quantized)));
  QuantizedModel->Parameters.insert_or_assign(
      "biases", std::move(std::get<2>(Quantized)));
  if (EnableBias) {
    QuantizedModel->Parameters.insert_or_assign(
        "bias", LinearModule->Parameters.at("bias"));
  }
  return QuantizedModel;
}
} // namespace mlx::core::nn