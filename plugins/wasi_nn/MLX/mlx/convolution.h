// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "base.h"

namespace WasmEdge::Host::WASINN::MLX {
namespace mlx::core::nn {

class Conv2d : public nn::Module {
  std::pair<int, int> Padding;
  std::pair<int, int> Stride;
  std::pair<int, int> Dilation;
  int Groups;

public:
  Conv2d(int InChannels, int OutChannels, int KernelSize,
         std::pair<int, int> Stride = {1, 1},
         std::pair<int, int> Padding = {0, 0},
         std::pair<int, int> Dilation = {1, 1}, int Groups = 1,
         bool Bias = true)
      : Padding(Padding), Stride(Stride), Dilation(Dilation), Groups(Groups) {

    if (InChannels % Groups != 0) {
      // InChannels must be divisible by Groups
      assumingUnreachable();
    }
    double Scale = std::sqrt(1.0 / (InChannels * KernelSize * KernelSize));
    registerParameter("weight", mx::random::uniform(-Scale, Scale,
                                                    {OutChannels, InChannels,
                                                     KernelSize, KernelSize}));
    if (Bias) {
      registerParameter("bias", mx::zeros({OutChannels}));
    }
  }
  mx::array forward(mx::array Input);
};

} // namespace mlx::core::nn

} // namespace WasmEdge::Host::WASINN::MLX
