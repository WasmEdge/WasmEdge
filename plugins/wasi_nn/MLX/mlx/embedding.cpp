#include "embedding.h"
#include "base.h"
#include "quantized.h"
#include <mlx/array.h>
#include <mlx/ops.h>

namespace mlx::core::nn {
mx::array Embedding::forward(mx::array Input) {
  return take(Parameters.at("weight"), Input, 0);
}
mx::array Embedding::asLinear(mx::array Input) {
  return matmul(Input, transpose(Parameters.at("weight")));
}
nn::Module *Embedding::toQuantized(int GroupSize, int Bits) {
  auto *QuantModel = QuantizedEmbedding::fromEmbedding(this, GroupSize, Bits);
  QuantModel->Name = Name;
  return QuantModel;
}
} // namespace mlx::core::nn