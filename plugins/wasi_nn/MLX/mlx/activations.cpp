#include "activations.h"
#include <mlx/ops.h>

namespace WasmEdge::Host::WASINN::MLX {
namespace mlx::core {
mx::array gelu(mx::array X) {
  return X * (1 + mx::erf(X / std::sqrt(2.0))) / 2.0;
}
mx::array silu(mx::array X) { return X * mx::sigmoid(X); }
} // namespace mlx::core
} // namespace WasmEdge::Host::WASINN::MLX
