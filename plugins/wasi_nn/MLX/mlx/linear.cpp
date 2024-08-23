#include "linear.h"

namespace mlx::core::nn {
mx::array Linear::forward(mx::array Input) {
  if (EnableBias) {
    return mx::addmm(Parameters.at("bias"), Input,
                     transpose(Parameters.at("weight")));
  }
  return matmul(Input, transpose(Parameters.at("weight")));
}

} // namespace mlx::core::nn