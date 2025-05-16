// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "mlx/base.h"

#include <mlx/array.h>
#include <mlx/random.h>

#include <cmath>
#include <memory>

namespace WasmEdge::Host::WASINN::MLX {
namespace mlx::core::nn {

class Embedding : public Module {
public:
  Embedding() = default;

  Embedding(int NumEmbeddings, int Dims) {
    const double Scale = std::sqrt(1.0 / Dims);
    registerParameter("weight",
                      mx::random::normal({NumEmbeddings, Dims}, 0.0, Scale));
  }

  virtual mx::array forward(mx::array Input);

  mx::array asLinear(mx::array Input);

  std::shared_ptr<nn::Module>
  toQuantized(int GroupSize = 64, int Bits = 4, const std::string &Prefix = "",
              const std::unordered_map<std::string, mx::array> &Parameters = {})
      override;

  virtual bool hasQuantize() override { return true; }
};

} // namespace mlx::core::nn
} // namespace WasmEdge::Host::WASINN::MLX
