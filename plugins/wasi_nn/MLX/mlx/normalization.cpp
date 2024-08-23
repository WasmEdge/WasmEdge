#include "normalization.h"
namespace mlx::core::nn {
mx::array RMSNorm::forward(mx::array Input) {
  return mx::fast::rms_norm(Input, Parameters.at("weight"), Eps);
}
} // namespace mlx::core::nn