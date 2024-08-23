#include "activations.h"
#include <mlx/ops.h>
namespace mlx::core {
mx::array gelu(mx::array X) { return X * (1 + mx::erf(X / std::sqrt(2))) / 2; }
mx::array silu(mx::array X) { return X * mx::sigmoid(X); }
} // namespace mlx::core