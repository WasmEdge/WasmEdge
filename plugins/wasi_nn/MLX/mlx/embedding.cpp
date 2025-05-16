// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "mlx/embedding.h"
#include "mlx/quantized.h"

#include <mlx/array.h>
#include <mlx/ops.h>

namespace WasmEdge::Host::WASINN::MLX {
namespace mlx::core::nn {

mx::array Embedding::forward(mx::array Input) {
  return take(Parameters.at("weight"), Input, 0);
}

mx::array Embedding::asLinear(mx::array Input) {
  return matmul(Input, transpose(Parameters.at("weight")));
}

std::shared_ptr<nn::Module>
Embedding::toQuantized(int GroupSize, int Bits, const std::string &,
                       const std::unordered_map<std::string, mx::array> &) {
  auto QuantModel = QuantizedEmbedding::fromEmbedding(
      std::dynamic_pointer_cast<Embedding>(shared_from_this()), GroupSize,
      Bits);
  QuantModel->Name = Name;
  return QuantModel;
}

} // namespace mlx::core::nn
} // namespace WasmEdge::Host::WASINN::MLX
