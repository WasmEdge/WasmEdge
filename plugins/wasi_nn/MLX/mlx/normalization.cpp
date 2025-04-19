// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "mlx/normalization.h"

namespace WasmEdge::Host::WASINN::MLX {
namespace mlx::core::nn {

mx::array RMSNorm::forward(mx::array Input) {
  return mx::fast::rms_norm(Input, Parameters.at("weight"), Eps);
}

mx::array LayerNorm::forward(mx::array Input) {
  std::optional<mx::array> Weight = {};
  std::optional<mx::array> Bias = {};
  if (Parameters.find("weight") != Parameters.end()) {
    Weight = Parameters.at("weight");
  }
  if (Parameters.find("bias") != Parameters.end()) {
    Bias = Parameters.at("bias");
  }

  return mx::fast::layer_norm(Input, Weight, Bias, Eps);
}

} // namespace mlx::core::nn
} // namespace WasmEdge::Host::WASINN::MLX
