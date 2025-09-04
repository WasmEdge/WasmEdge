// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "convolution.h"
#include <mlx/array.h>
#include <mlx/ops.h>
namespace WasmEdge::Host::WASINN::MLX {
namespace mlx::core::nn {

mx::array Conv1d::forward(mx::array Input) {
  auto Y = mx::conv1d(Input, Parameters.at("weight"), Stride, Padding, Dilation,
                      Groups);
  if (Parameters.find("bias") != Parameters.end()) {
    Y = Y + Parameters.at("bias");
  }
  return Y;
}

mx::array Conv2d::forward(mx::array Input) {
  auto Y = mx::conv2d(Input, Parameters.at("weight"), Stride, Padding, Dilation,
                      Groups);
  if (Parameters.find("bias") != Parameters.end()) {
    Y = Y + Parameters.at("bias");
  }
  return Y;
}

} // namespace mlx::core::nn
} // namespace WasmEdge::Host::WASINN::MLX
