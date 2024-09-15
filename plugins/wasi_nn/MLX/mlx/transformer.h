#pragma once
#include "base.h"
#include "linear.h"
#include <optional>

namespace mlx::core::nn {
class MultiHeadAttention : public Module {
  int NumHeads;

public:
  MultiHeadAttention(int Dims, int NumHeads,
                     std::optional<int> QueryInputDims = {},
                     std::optional<int> KeyInputDims = {},
                     std::optional<int> ValueInputDims = {},
                     std::optional<int> ValueDims = {},
                     std::optional<int> ValueOutputDims = {}, bool Bias = false)
      : NumHeads(NumHeads) {
    if (Dims % NumHeads != 0) {
      spdlog::error("Dims must be divisible by NumHeads");
      assumingUnreachable();
    }
    if (!QueryInputDims) {
      QueryInputDims = Dims;
    }
    if (!KeyInputDims) {
      KeyInputDims = Dims;
    }
    if (!ValueInputDims) {
      ValueInputDims = KeyInputDims;
    }
    if (!ValueDims) {
      ValueDims = Dims;
    }
    if (!ValueOutputDims) {
      ValueOutputDims = Dims;
    }
    registerModule("query_proj", new Linear(*QueryInputDims, Dims, Bias));
    registerModule("key_proj", new Linear(*KeyInputDims, Dims, Bias));
    registerModule("value_proj", new Linear(*ValueInputDims, *ValueDims, Bias));
    registerModule("out_proj", new Linear(*ValueDims, *ValueOutputDims, Bias));
  };
  mx::array forward(mx::array Queries, mx::array Keys, mx::array Values,
                    mx::array Mask);
  static mx::array createAdditiveCausalMask(int N,
                                            mx::Dtype DType = mx::float32);
};
} // namespace mlx::core::nn