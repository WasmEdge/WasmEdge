// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "mlx/linear.h"
#include "mlx/quantized.h"

#include <memory>

namespace WasmEdge::Host::WASINN::MLX {
namespace mlx::core::nn {

mx::array Linear::forward(mx::array Input) {
  if (EnableBias) {
    return mx::addmm(Parameters.at("bias"), Input,
                     transpose(Parameters.at("weight")));
  }
  return matmul(Input, transpose(Parameters.at("weight")));
}

std::shared_ptr<nn::Module>
Linear::toQuantized(int GroupSize, int Bits, const std::string &,
                    const std::unordered_map<std::string, mx::array> &) {
  auto QuantModel = QuantizedLinear::fromLinear(
      std::dynamic_pointer_cast<Linear>(shared_from_this()), GroupSize, Bits);
  QuantModel->Name = Name;
  return QuantModel;
}

} // namespace mlx::core::nn
} // namespace WasmEdge::Host::WASINN::MLX
