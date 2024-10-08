// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "mlx/base.h"

#include <mlx/array.h>
#include <mlx/fast.h>

namespace WasmEdge::Host::WASINN::MLX {
namespace mlx::core::nn {

class RoPE : public Module {
  int Dims;
  bool Tranditional;
  float Base;
  float Scale;

public:
  RoPE(int Dims, bool Traditional = false, float Base = 10000,
       float Scale = 1.0)
      : Dims(Dims), Tranditional(Traditional), Base(Base), Scale(Scale) {}

  mx::array forward(mx::array Input, int Offset = 0);
};

} // namespace mlx::core::nn
} // namespace WasmEdge::Host::WASINN::MLX
