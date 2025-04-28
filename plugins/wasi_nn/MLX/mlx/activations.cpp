// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "mlx/activations.h"

#include <mlx/ops.h>

#include <cmath>

namespace WasmEdge::Host::WASINN::MLX {
namespace mlx::core {

mx::array gelu(mx::array X) {
  return X * (1 + mx::erf(X / std::sqrt(2.0))) / 2.0;
}

mx::array silu(mx::array X) { return X * mx::sigmoid(X); }

mx::array geluApprox(mx::array X) {
  return mx::array({0.5}, X.dtype()) * X *
         (mx::array({1}, X.dtype()) +
          mx::tanh(mx::array({std::sqrt(2.0 / M_PI)}, X.dtype()) *
                   (X + mx::array({0.044715}, X.dtype()) *
                            mx::power(X, mx::array({3}, X.dtype())))));
}
} // namespace mlx::core
} // namespace WasmEdge::Host::WASINN::MLX
