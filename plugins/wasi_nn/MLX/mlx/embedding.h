#pragma once
#include "base.h"
#include <cmath>
#include <mlx/array.h>
#include <mlx/random.h>
namespace mlx::core::nn {
class Embedding : public Module {
public:
  Embedding() = default;
  Embedding(int NumEmbeddings, int Dims) {
    const double Scale = std::sqrt(1.0 / Dims);
    registerParameter("weight",
                      mx::random::normal({NumEmbeddings, Dims}, 0.0, Scale));
  }
  virtual mx::array forward(mx::array Input);
  mx::array asLinear(mx::array Input);
  nn::Module *toQuantized(int GroupSize = 64, int Bits = 4) override;
};

} // namespace mlx::core::nn