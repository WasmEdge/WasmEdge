#pragma once
#include "base.h"
#include <cmath>
#include <mlx/array.h>
#include <mlx/random.h>
namespace mlx::core::nn {
class Embedding : public Module {
public:
  Embedding(int NumEmbeddings, int Dims) {
    const double Scale = std::sqrt(1 / Dims);
    registerParameter("weight",
                      mx::random::normal({NumEmbeddings, Dims}, 0.0, Scale));
  }
  mx::array forward(mx::array Input);
  mx::array asLinear(mx::array Input);
};

} // namespace mlx::core::nn