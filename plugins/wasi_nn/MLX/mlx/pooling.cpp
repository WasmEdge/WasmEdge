// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "pooling.h"
#include "base.h"
#include "spdlog/spdlog.h"
#include <cstdint>
#include <mlx/array.h>
#include <mlx/ops.h>
#include <vector>

namespace WasmEdge::Host::WASINN::MLX {
namespace mlx::core::nn {

namespace {
std::vector<int> valueOrList(const int Value, int Len) {
  return std::vector<int>(Len, Value);
}

std::vector<std::pair<int, int>>
makePaddingPairs(const std::vector<int> &Pads) {
  std::vector<std::pair<int, int>> Pairs;
  for (int P : Pads) {
    Pairs.push_back({P, P});
  }
  return Pairs;
}

std::vector<std::pair<int, int>>
makePadShape(const std::vector<std::pair<int, int>> &Padding) {
  std::vector<std::pair<int, int>> PadShape;
  PadShape.push_back({0, 0});
  for (auto &P : Padding) {
    PadShape.push_back(P);
  }
  PadShape.push_back({0, 0});
  return PadShape;
}

} // namespace

mx::array nonOverlappingSlidingWindows(const mx::array &X,
                                       const std::vector<int> &Shape,
                                       const std::vector<int> &WindowShape) {
  std::vector<int> NewShape;
  NewShape.push_back(Shape[0]);
  for (size_t I = 1; I < std::min(Shape.size(), WindowShape.size() + 1); I++) {
    int S = Shape[I];
    int W = WindowShape[I - 1];
    NewShape.push_back(S / W);
    NewShape.push_back(W);
  }
  NewShape.push_back(Shape.back());
  int LastAxis = NewShape.size() - 1;
  std::vector<int> AxisOrder;
  AxisOrder.push_back(0);
  for (int I = 1; I < LastAxis; I += 2) {
    AxisOrder.push_back(I);
  }
  for (int I = 2; I < LastAxis; I += 2) {
    AxisOrder.push_back(I);
  }
  AxisOrder.push_back(LastAxis);
  return transpose(reshape(X, NewShape), AxisOrder);
}

mx::array slidingWindows(const mx::array &X,
                         const std::vector<int> &WindowShape,
                         const std::vector<int> &WindowStrides) {
  if (X.ndim() < 3) {
    spdlog::error(
        "To extract sliding windows at least 1 spatial dimension (3 total) is "
        "needed but the input only has " +
        std::to_string(X.ndim()) + " dimensions.");
    assumingUnreachable();
  }
  std::vector<int> Shape = X.shape();
  size_t SpatialDimsCount = Shape.size() - 2;
  if (SpatialDimsCount != WindowShape.size() ||
      WindowShape.size() != WindowStrides.size()) {

    spdlog::error(
        "The window shapes and strides must have the same number of spatial "
        "dimensions as the signal.");
    assumingUnreachable();
  }
  bool UseNonOverlap = true;
  for (size_t I = 0; I < SpatialDimsCount; I++) {
    if (!(WindowShape[I] == WindowStrides[I] &&
          (Shape[I + 1] % WindowShape[I] == 0))) {
      UseNonOverlap = false;
      break;
    }
  }
  if (UseNonOverlap)
    return nonOverlappingSlidingWindows(X, Shape, WindowShape);
  size_t N = Shape.size();
  std::vector<int> BaseStrides(N);
  BaseStrides[N - 1] = 1;
  for (int I = N - 2; I >= 0; I--) {
    BaseStrides[I] = Shape[I + 1] * BaseStrides[I + 1];
  }
  std::vector<int> FinalShape;
  FinalShape.push_back(Shape[0]);
  for (size_t I = 0; I < SpatialDimsCount; I++) {
    int OutDim = (Shape[I + 1] - WindowShape[I]) / WindowStrides[I] + 1;
    FinalShape.push_back(OutDim);
  }
  FinalShape.insert(FinalShape.end(), WindowShape.begin(), WindowShape.end());
  FinalShape.push_back(Shape.back());
  std::vector<int64_t> FinalStrides;
  FinalStrides.push_back(BaseStrides[0]);
  for (size_t I = 1; I < BaseStrides.size() - 1; I++) {
    FinalStrides.push_back(BaseStrides[I] * WindowStrides[I - 1]);
  }
  for (size_t I = 1; I < BaseStrides.size(); I++) {
    FinalStrides.push_back(BaseStrides[I]);
  }
  return mx::as_strided(X, FinalShape, FinalStrides, 0);
}

Pool::Pool(
    const std::function<mx::array(const mx::array &, const std::vector<int> &)>
        &PoolingFunction,
    const std::vector<int> &KernelSize, const std::vector<int> &Stride,
    const std::vector<std::pair<int, int>> &Padding, int PaddingValue)
    : PoolingFunction(PoolingFunction), KernelSize(KernelSize), Stride(Stride),
      Padding(Padding), PaddingValue(PaddingValue) {
  int N = KernelSize.size();
  for (int I = -(N + 1); I < -1; I++) {
    Axes.push_back(I);
  }
}

mx::array Pool::forward(const mx::array &X) {
  mx::array Out = X;
  bool NeedPad = false;
  for (auto &P : Padding) {
    if (P.first > 0) {
      NeedPad = true;
      break;
    }
  }
  if (NeedPad) {
    std::vector<std::pair<int, int>> PadShape = makePadShape(Padding);
    Out = mx::pad(Out, PadShape, mx::array(PaddingValue));
  }
  Out = slidingWindows(Out, KernelSize, Stride);
  return PoolingFunction(Out, Axes);
}

Pool2d::Pool2d(
    const std::function<mx::array(const mx::array &, const std::vector<int> &)>
        &PoolingFunction,
    int PaddingValue, const std::vector<int> &KernelSize,
    const std::optional<std::vector<int>> &StrideOpt,
    const std::optional<std::vector<int>> &PaddingOpt)
    : Pool(PoolingFunction,
           KernelSize.size() == 1 ? valueOrList(KernelSize[0], 2) : KernelSize,
           (StrideOpt.has_value()
                ? (StrideOpt.value().size() == 1
                       ? valueOrList(StrideOpt.value()[0], 2)
                       : StrideOpt.value())
                : (KernelSize.size() == 1 ? valueOrList(KernelSize[0], 2)
                                          : KernelSize)),
           makePaddingPairs(PaddingOpt.has_value() ? PaddingOpt.value()
                                                   : valueOrList(0, 2)),
           PaddingValue) {}

AvgPool2d::AvgPool2d(const std::vector<int> &KernelSize,
                     const std::optional<std::vector<int>> &Stride,
                     const std::optional<std::vector<int>> &Padding)
    : Pool2d(
          [](const mx::array &A, const std::vector<int> &Axis) -> mx::array {
            return mx::mean(A, Axis, false);
          },
          0, KernelSize, Stride, Padding) {}

} // namespace mlx::core::nn
} // namespace WasmEdge::Host::WASINN::MLX
