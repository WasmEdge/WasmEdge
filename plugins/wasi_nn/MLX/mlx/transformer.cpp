// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "mlx/transformer.h"

#include <mlx/ops.h>

namespace WasmEdge::Host::WASINN::MLX {
namespace mlx::core::nn {

mx::array MultiHeadAttention::createAdditiveCausalMask(int N, mx::Dtype DType) {
  auto Indices = mx::arange(N);
  // mask = indices[:, None] < indices[None]
  auto Mask = reshape(Indices, {N, 1}) < reshape(Indices, {1, N});
  // using 1e9 instead of INF, and softmax(full(1e9)) != nan
  Mask = astype(Mask, DType) * -1e9;
  return Mask;
}

} // namespace mlx::core::nn
} // namespace WasmEdge::Host::WASINN::MLX
