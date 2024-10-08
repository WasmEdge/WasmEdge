// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "mlx/base.h"
#include "mlx/embedding.h"
#include "mlx/linear.h"

#include <mlx/array.h>
#include <mlx/ops.h>

#include <memory>

namespace WasmEdge::Host::WASINN::MLX {
namespace mlx::core::nn {

class QuantizedEmbedding : public Embedding {
public:
  int GroupSize;
  int Bits;
  int NumEmbeddings;
  int Dims;

  QuantizedEmbedding(int NumEmbeddings, int Dims, int GroupSize = 64,
                     int Bits = 4)
      : GroupSize(GroupSize), Bits(Bits), NumEmbeddings(NumEmbeddings),
        Dims(Dims) {
    const double Scale = std::sqrt(1.0 / Dims);
    registerParameter("weight",
                      mx::random::normal({NumEmbeddings, Dims}, 0.0, Scale));
    auto Quantized = mx::quantize(Parameters.at("weight"), GroupSize, Bits);
    Parameters.insert_or_assign("weight", std::get<0>(Quantized));
    registerParameter("scales", std::move(std::get<1>(Quantized)));
    registerParameter("biases", std::move(std::get<2>(Quantized)));
  }

  mx::array forward(mx::array Input) override;

  static std::shared_ptr<QuantizedEmbedding>
  fromEmbedding(std::shared_ptr<Embedding> EmbeddingModule, int GroupSize = 64,
                int Bits = 4);
};

class QuantizedLinear : public Linear {
public:
  int GroupSize;
  int Bits;

  QuantizedLinear(int InputDims, int OutputDim, bool Bias = true,
                  int GroupSize = 64, int Bits = 4)
      : GroupSize(GroupSize), Bits(Bits) {
    const double Scale = std::sqrt(1.0 / InputDims);
    registerParameter(
        "weight", mx::random::uniform(-Scale, Scale, {OutputDim, InputDims}));
    auto Quantized = mx::quantize(Parameters.at("weight"), GroupSize, Bits);
    Parameters.insert_or_assign("weight", std::get<0>(Quantized));
    registerParameter("scales", std::move(std::get<1>(Quantized)));
    registerParameter("biases", std::move(std::get<2>(Quantized)));
    if (Bias) {
      registerParameter("bias", mx::zeros({OutputDim}));
    }
  }

  mx::array forward(mx::array Input) override;

  static std::shared_ptr<QuantizedLinear>
  fromLinear(std::shared_ptr<Linear> LinearModule, int GroupSize = 64,
             int Bits = 4);
};

} // namespace mlx::core::nn
} // namespace WasmEdge::Host::WASINN::MLX
