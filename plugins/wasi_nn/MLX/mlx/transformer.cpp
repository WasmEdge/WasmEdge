#include "transformer.h"
#include <mlx/ops.h>

namespace mlx::core::nn {
mx::array MultiHeadAttention::createAdditiveCausalMask(int N, mx::Dtype DType) {
  auto Indices = mx::arange(N);
  // mask = indices[:, None] < indices[None]
  auto Mask = reshape(Indices, {N, 1}) < reshape(Indices, {1, N});
  Mask = astype(Mask, DType) * -1e9;
  return Mask;
}
} // namespace mlx::core::nn