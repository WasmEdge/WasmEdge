// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "mlx/base.h"

namespace WasmEdge::Host::WASINN::MLX {
namespace mlx::core::nn {

class RMSNorm : public nn::Module {
  float Eps;

public:
  RMSNorm(int Dims, float Eps = 1e-5) : Eps(Eps) {
    registerParameter("weight", mx::ones({Dims}));
  }

  mx::array forward(mx::array Input);
};

} // namespace mlx::core::nn
} // namespace WasmEdge::Host::WASINN::MLX
