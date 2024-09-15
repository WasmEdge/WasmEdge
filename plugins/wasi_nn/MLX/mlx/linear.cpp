#include "linear.h"
#include "base.h"
#include "quantized.h"
namespace mlx::core::nn {
mx::array Linear::forward(mx::array Input) {
  if (EnableBias) {
    return mx::addmm(Parameters.at("bias"), Input,
                     transpose(Parameters.at("weight")));
  }
  return matmul(Input, transpose(Parameters.at("weight")));
}

nn::Module *Linear::toQuantized(int GroupSize, int Bits) {
  auto *QuantModel = QuantizedLinear::fromLinear(this, GroupSize, Bits);
  QuantModel->Name = Name;
  return QuantModel;
}

} // namespace mlx::core::nn