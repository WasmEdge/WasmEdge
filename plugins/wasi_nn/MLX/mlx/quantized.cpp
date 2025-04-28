// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "mlx/quantized.h"

#include <mlx/array.h>
#include <mlx/ops.h>

#include <iostream>
#include <utility>

namespace WasmEdge::Host::WASINN::MLX {
namespace mlx::core::nn {

mx::array QuantizedEmbedding::forward(mx::array Input) {
  auto S = Input.shape();
  auto X = mx::flatten(Input);
  auto Out =
      mx::dequantize(take(Parameters.at("weight"), Input, 0),
                     take(Parameters.at("scales"), Input, 0),
                     take(Parameters.at("biases"), Input, 0), GroupSize, Bits);
  return Out;
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

std::shared_ptr<QuantizedEmbedding>
QuantizedEmbedding::fromEmbedding(std::shared_ptr<Embedding> EmbeddingModule,
                                  int GroupSize, int Bits) {
  auto EmbeddingShape = EmbeddingModule->Parameters.at("weight").shape();
  auto QuantizedModel = std::make_shared<QuantizedEmbedding>(QuantizedEmbedding(
      EmbeddingShape[0], EmbeddingShape[1], GroupSize, Bits));
  auto Quantized =
      mx::quantize(EmbeddingModule->Parameters.at("weight"), GroupSize, Bits);
  QuantizedModel->Parameters.insert_or_assign("weight", std::get<0>(Quantized));
  QuantizedModel->Parameters.insert_or_assign(
      "scales", std::move(std::get<1>(Quantized)));
  QuantizedModel->Parameters.insert_or_assign(
      "biases", std::move(std::get<2>(Quantized)));
  return QuantizedModel;
}

std::shared_ptr<QuantizedLinear>
QuantizedLinear::fromLinear(std::shared_ptr<Linear> LinearModule, int GroupSize,
                            int Bits) {
  auto LinearShape = LinearModule->Parameters.at("weight").shape();
  auto OutputDims = LinearShape[0];
  auto InputDims = LinearShape[1];
  const bool EnableBias =
      LinearModule->Parameters.find("bias") != LinearModule->Parameters.end();
  auto QuantizedModel = std::make_shared<QuantizedLinear>(
      QuantizedLinear(InputDims, OutputDims, EnableBias, GroupSize, Bits));
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
} // namespace WasmEdge::Host::WASINN::MLX
