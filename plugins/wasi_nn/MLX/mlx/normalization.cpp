// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "mlx/normalization.h"

namespace WasmEdge::Host::WASINN::MLX {
namespace mlx::core::nn {

mx::array RMSNorm::forward(mx::array Input) {
  return mx::fast::rms_norm(Input, Parameters.at("weight"), Eps);
}

} // namespace mlx::core::nn
} // namespace WasmEdge::Host::WASINN::MLX
