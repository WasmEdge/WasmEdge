// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "base.h"
namespace WasmEdge::Host::WASINN::MLX {

namespace mlx::core::nn {
class Pool : public nn::Module {
public:
  Pool(const std::function<mx::array(
           const mx::array &, const std::vector<int> &)> &PoolingFunction,
       const std::vector<int> &KernelSize, const std::vector<int> &Stride,
       const std::vector<std::pair<int, int>> &Padding, int PaddingValue);
  mx::array forward(const mx::array &X);

protected:
  std::function<mx::array(const mx::array &, const std::vector<int> &)>
      PoolingFunction;
  std::vector<int> KernelSize;
  std::vector<int> Stride;
  std::vector<std::pair<int, int>> Padding;
  int PaddingValue;
  std::vector<int> Axes;
};

class Pool2d : public Pool {
public:
  Pool2d(const std::function<mx::array(const mx::array &,
                                       const std::vector<int> &)> &PoolingFn,
         int PadValue, const std::vector<int> &KernelSizes,
         const std::optional<std::vector<int>> &StrideOpt,
         const std::optional<std::vector<int>> &PaddingOpt);
};

class AvgPool2d : public Pool2d {
public:
  AvgPool2d(const std::vector<int> &KernelSizes,
            const std::optional<std::vector<int>> &StrideOpt = std::nullopt,
            const std::optional<std::vector<int>> &PaddingOpt = std::nullopt);
};

} // namespace mlx::core::nn
} // namespace WasmEdge::Host::WASINN::MLX
