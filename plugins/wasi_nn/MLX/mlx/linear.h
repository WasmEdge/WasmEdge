// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "mlx/base.h"

#include <mlx/mlx.h>
#include <mlx/ops.h>
#include <mlx/random.h>

#include <cmath>
#include <memory>

namespace WasmEdge::Host::WASINN::MLX {
namespace mlx::core::nn {

class Linear : public Module {
  bool EnableBias = true;

public:
  Linear() = default;
  Linear(int InputDims, int OutputDims, bool EnableBias = true)
      : EnableBias(EnableBias) {
    const double Scale = std::sqrt(1.0 / InputDims);
    registerParameter(
        "weight", mx::random::uniform(-Scale, Scale, {OutputDims, InputDims}));
    if (EnableBias) {
      registerParameter("bias", mx::random::uniform(-Scale, Scale,
                                                    {
                                                        OutputDims,
                                                    }));
    }
  }

  virtual mx::array forward(mx::array Input);
  std::shared_ptr<nn::Module>
  toQuantized(int GroupSize = 64, int Bits = 4, const std::string &Prefix = "",
              const std::unordered_map<std::string, mx::array> &Parameters = {})
      override;

  virtual bool hasQuantize() override { return true; }
};

} // namespace mlx::core::nn
} // namespace WasmEdge::Host::WASINN::MLX
