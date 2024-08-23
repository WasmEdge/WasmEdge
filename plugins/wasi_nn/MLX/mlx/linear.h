#pragma once

#include "base.h"
#include "mlx/mlx.h"
#include <cmath>
#include <mlx/ops.h>
#include <mlx/random.h>

namespace mlx::core::nn {

class Linear : public Module {
  bool EnableBias = true;

public:
  Linear(int InputDims, int OutputDims, bool EnableBias = true)
      : EnableBias(EnableBias) {
    const double Scale = std::sqrt(1.0 / InputDims);
    registerParameter(
        "weight", mx::random::uniform(-Scale, Scale, {OutputDims, InputDims}));
    if (EnableBias) {
      registerParameter("bias", mx::random::uniform(-Scale, Scale,
                                                    {
                                                        OutputDims,
                                                    }));
    }
  }
  mx::array forward(mx::array Input);
};
} // namespace mlx::core::nn