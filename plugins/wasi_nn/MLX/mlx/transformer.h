// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "mlx/base.h"
#include "mlx/linear.h"

#include <optional>

namespace WasmEdge::Host::WASINN::MLX {
namespace mlx::core::nn {

class MultiHeadAttention : public Module {
  int NumHeads;

public:
  MultiHeadAttention(int Dims, int NumHeads,
                     std::optional<int> QueryInputDims = {},
                     std::optional<int> KeyInputDims = {},
                     std::optional<int> ValueInputDims = {},
                     std::optional<int> ValueDims = {},
                     std::optional<int> ValueOutputDims = {}, bool Bias = false)
      : NumHeads(NumHeads) {
    if (Dims % NumHeads != 0) {
      spdlog::error(
          "[WASI-NN] MLX backend: Dims must be divisible by NumHeads"sv);
      assumingUnreachable();
    }
    if (!QueryInputDims) {
      QueryInputDims = Dims;
    }
    if (!KeyInputDims) {
      KeyInputDims = Dims;
    }
    if (!ValueInputDims) {
      ValueInputDims = KeyInputDims;
    }
    if (!ValueDims) {
      ValueDims = Dims;
    }
    if (!ValueOutputDims) {
      ValueOutputDims = Dims;
    }
    registerModule("query_proj", std::make_shared<Linear>(
                                     Linear(*QueryInputDims, Dims, Bias)));
    registerModule("key_proj",
                   std::make_shared<Linear>(Linear(*KeyInputDims, Dims, Bias)));
    registerModule("value_proj", std::make_shared<Linear>(Linear(
                                     *ValueInputDims, *ValueDims, Bias)));
    registerModule("out_proj", std::make_shared<Linear>(
                                   Linear(*ValueDims, *ValueOutputDims, Bias)));
  };

  mx::array forward(mx::array Queries, mx::array Keys, mx::array Values,
                    mx::array Mask);

  static mx::array createAdditiveCausalMask(int N,
                                            mx::Dtype DType = mx::float32);
};

} // namespace mlx::core::nn
} // namespace WasmEdge::Host::WASINN::MLX
