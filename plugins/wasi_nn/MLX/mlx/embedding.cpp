#include "embedding.h"
#include <mlx/array.h>
#include <mlx/ops.h>

namespace mlx::core::nn {
mx::array Embedding::forward(mx::array Input) {
  return take(Parameters.at("weight"), Input, 0);
}
mx::array Embedding::asLinear(mx::array Input) {
  return matmul(Input, transpose(Parameters.at("weight")));
}
} // namespace mlx::core::nn