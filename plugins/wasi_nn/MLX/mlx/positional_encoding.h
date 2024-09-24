#pragma once
#include "base.h"
#include <mlx/array.h>
#include <mlx/fast.h>

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