#pragma once
#include "base.h"
#include <cmath>
#include <mlx/array.h>

namespace WasmEdge::Host::WASINN::MLX {
namespace mlx::core {
mx::array gelu(mx::array X);
mx::array silu(mx::array X);
} // namespace mlx::core
} // namespace WasmEdge::Host::WASINN::MLX
