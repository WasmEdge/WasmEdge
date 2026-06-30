// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "mlx/positional_encoding.h"

namespace WasmEdge::Host::WASINN::MLX {
namespace mlx::core::nn {

mx::array RoPE::forward(mx::array Input, int Offset) {
  return mx::fast::rope(Input, Dims, Tranditional, Base, Scale, Offset);
}

} // namespace mlx::core::nn
} // namespace WasmEdge::Host::WASINN::MLX
