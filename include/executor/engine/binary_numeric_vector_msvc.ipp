// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "executor/executor.h"

namespace WasmEdge {
namespace Executor {

namespace {
template <typename T> static inline T allOnes() {
  if constexpr (sizeof(T) == 1) {
    int8_t Val = -1;
    return reinterpret_cast<T &>(Val);
  } else if constexpr (sizeof(T) == 2) {
    int16_t Val = -1;
    return reinterpret_cast<T &>(Val);
  } else if constexpr (sizeof(T) == 4) {
    int32_t Val = -1;
    return reinterpret_cast<T &>(Val);
  } else if constexpr (sizeof(T) == 8) {
    int64_t Val = -1;
    return reinterpret_cast<T &>(Val);
  }
}
} // namespace

template <typename TIn, typename TOut>
Expect<void> Executor::runReplaceLaneOp(Runtime::StackManager &StackMgr,
                                        const uint8_t Index) const noexcept {
  using VTOut = SIMDArray<TOut, 16>;

  const auto [Val2, Val1] = StackMgr.popsPeekTop<TIn, VTOut>();
  VTOut Result = Val1;
  Result[Index] = static_cast<TOut>(Val2);
  StackMgr.emplaceTop(std::move(Result));
  return {};
}

template <typename T>
Expect<void>
Executor::runVectorEqOp(Runtime::StackManager &StackMgr) const noexcept {
  using VT = SIMDArray<T, 16>;

  const auto [V2, V1] = StackMgr.popsPeekTop<VT, VT>();
  const T AllOnes = allOnes<T>();

  // unrolling V1 = (V1 == V2);
  VT VOut;
  for (size_t I = 0; I < V1.size(); I++) {
    if (V1[I] == V2[I]) {
      // all ones
      VOut[I] = AllOnes;
    } else {
      VOut[I] = 0;
    }
  }
  StackMgr.emplaceTop<VT>(std::move(VOut));

  return {};
}

template <typename T>
Expect<void>
Executor::runVectorNeOp(Runtime::StackManager &StackMgr) const noexcept {
  using VT = SIMDArray<T, 16>;

  const auto [V2, V1] = StackMgr.popsPeekTop<VT, VT>();
  const T AllOnes = allOnes<T>();

  // unrolling V1 = (V1 != V2);
  VT VOut;
  for (size_t I = 0; I < V1.size(); I++) {
    if (V1[I] != V2[I]) {
      // all ones
      VOut[I] = AllOnes;
    } else {
      VOut[I] = 0;
    }
  }
  StackMgr.emplaceTop<VT>(std::move(VOut));

  return {};
}

template <typename T>
Expect<void>
Executor::runVectorLtOp(Runtime::StackManager &StackMgr) const noexcept {
  using VT = SIMDArray<T, 16>;

  const auto [V2, V1] = StackMgr.popsPeekTop<VT, VT>();
  const T AllOnes = allOnes<T>();

  // unrolling V1 = (V1 < V2);
  VT VOut;
  for (size_t I = 0; I < V1.size(); I++) {
    if (V1[I] < V2[I]) {
      // all ones
      VOut[I] = AllOnes;
    } else {
      VOut[I] = 0;
    }
  }
  StackMgr.emplaceTop<VT>(std::move(VOut));

  return {};
}

template <typename T>
Expect<void>
Executor::runVectorGtOp(Runtime::StackManager &StackMgr) const noexcept {
  using VT = SIMDArray<T, 16>;

  const auto [V2, V1] = StackMgr.popsPeekTop<VT, VT>();
  const T AllOnes = allOnes<T>();

  // unrolling V1 = (V1 > V2);
  VT VOut;
  for (size_t I = 0; I < V1.size(); I++) {
    if (V1[I] > V2[I]) {
      // all ones
      VOut[I] = AllOnes;
    } else {
      VOut[I] = 0;
    }
  }
  StackMgr.emplaceTop<VT>(std::move(VOut));

  return {};
}

template <typename T>
Expect<void>
Executor::runVectorLeOp(Runtime::StackManager &StackMgr) const noexcept {
  using VT = SIMDArray<T, 16>;

  const auto [V2, V1] = StackMgr.popsPeekTop<VT, VT>();
  const T AllOnes = allOnes<T>();

  // unrolling V1 = (V1 <= V2);
  VT VOut;
  for (size_t I = 0; I < V1.size(); I++) {
    if (V1[I] <= V2[I]) {
      // all ones
      VOut[I] = AllOnes;
    } else {
      VOut[I] = 0;
    }
  }
  StackMgr.emplaceTop<VT>(std::move(VOut));

  return {};
}

template <typename T>
Expect<void>
Executor::runVectorGeOp(Runtime::StackManager &StackMgr) const noexcept {
  using VT = SIMDArray<T, 16>;

  const auto [V2, V1] = StackMgr.popsPeekTop<VT, VT>();
  const T AllOnes = allOnes<T>();

  // unrolling V1 = (V1 >= V2);
  VT VOut;
  for (size_t I = 0; I < V1.size(); I++) {
    if (V1[I] >= V2[I]) {
      // all ones
      VOut[I] = AllOnes;
    } else {
      VOut[I] = 0;
    }
  }
  StackMgr.emplaceTop<VT>(std::move(VOut));

  return {};
}

template <typename TIn, typename TOut>
Expect<void>
Executor::runVectorNarrowOp(Runtime::StackManager &StackMgr) const noexcept {
  static_assert(sizeof(TOut) * 2 == sizeof(TIn));
  static_assert(sizeof(TOut) == 1 || sizeof(TOut) == 2);
  using VTIn = SIMDArray<TIn, 16>;
  using HVTOut = SIMDArray<TOut, 8>;
  using VTOut = SIMDArray<TOut, 16>;

  const auto [V2, V1] = StackMgr.popsPeekTop<VTIn, VTIn>();
  VTOut Result;
  const size_t HSize = V1.size();
  for (size_t I = 0; I < HSize; ++I) {
    if (V1[I] > std::numeric_limits<TOut>::max()) {
      Result[I] = std::numeric_limits<TOut>::max();
    } else if (V1[I] < std::numeric_limits<TOut>::min()) {
      Result[I] = std::numeric_limits<TOut>::min();
    } else {
      Result[I] = static_cast<TOut>(V1[I]);
    }
  }
  for (size_t I = 0; I < HSize; ++I) {
    if (V2[I] > std::numeric_limits<TOut>::max()) {
      Result[HSize + I] = std::numeric_limits<TOut>::max();
    } else if (V2[I] < std::numeric_limits<TOut>::min()) {
      Result[HSize + I] = std::numeric_limits<TOut>::min();
    } else {
      Result[HSize + I] = static_cast<TOut>(V2[I]);
    }
  }
  StackMgr.emplaceTop(std::move(Result));
  return {};
}

template <typename T>
Expect<void>
Executor::runVectorShlOp(Runtime::StackManager &StackMgr) const noexcept {
  using VT = SIMDArray<T, 16>;
  auto [Count, V1] = StackMgr.popsPeekTop<uint32_t, VT>();
  const uint32_t Mask = static_cast<uint32_t>(sizeof(T) * 8 - 1);
  Count &= Mask;
  for (size_t I = 0; I < V1.size(); ++I) {
    V1[I] <<= Count;
  }
  StackMgr.emplaceTop(std::move(V1));

  return {};
}

template <typename T>
Expect<void>
Executor::runVectorShrOp(Runtime::StackManager &StackMgr) const noexcept {
  using VT = SIMDArray<T, 16>;
  auto [Count, V1] = StackMgr.popsPeekTop<uint32_t, VT>();
  const uint32_t Mask = static_cast<uint32_t>(sizeof(T) * 8 - 1);
  Count &= Mask;
  for (size_t I = 0; I < V1.size(); ++I) {
    V1[I] >>= Count;
  }
  StackMgr.emplaceTop(std::move(V1));

  return {};
}

template <typename T>
Expect<void>
Executor::runVectorAddOp(Runtime::StackManager &StackMgr) const noexcept {
  using VT = SIMDArray<T, 16>;
  auto [V2, V1] = StackMgr.popsPeekTop<VT, VT>();
  for (size_t I = 0; I < V1.size(); ++I) {
    V1[I] += V2[I];
  }
  StackMgr.emplaceTop(std::move(V1));

  return {};
}

template <typename T>
Expect<void>
Executor::runVectorAddSatOp(Runtime::StackManager &StackMgr) const noexcept {
  static_assert(sizeof(T) < 4);
  using VT = SIMDArray<T, 16>;
  auto [V2, V1] = StackMgr.popsPeekTop<VT, VT>();

  for (size_t I = 0; I < V1.size(); ++I) {
    // check for overflow
    // see: https://stackoverflow.com/a/3947943
    if (V1[I] >= 0) {
      if (std::numeric_limits<T>::max() - V1[I] < V2[I]) {
        V1[I] = std::numeric_limits<T>::max();
        continue;
      }
    } else {
      if (std::numeric_limits<T>::min() - V1[I] > V2[I]) {
        V1[I] = std::numeric_limits<T>::min();
        continue;
      }
    }
    V1[I] = V1[I] + V2[I];
  }
  StackMgr.emplaceTop(std::move(V1));
  return {};
}

template <typename T>
Expect<void>
Executor::runVectorSubOp(Runtime::StackManager &StackMgr) const noexcept {
  using VT = SIMDArray<T, 16>;
  auto [V2, V1] = StackMgr.popsPeekTop<VT, VT>();
  for (size_t I = 0; I < V1.size(); ++I) {
    V1[I] -= V2[I];
  }
  StackMgr.emplaceTop(std::move(V1));

  return {};
}

template <typename T>
Expect<void>
Executor::runVectorSubSatOp(Runtime::StackManager &StackMgr) const noexcept {
  static_assert(sizeof(T) < 4);
  using VT = SIMDArray<T, 16>;
  auto [V2, V1] = StackMgr.popsPeekTop<VT, VT>();

  for (size_t I = 0; I < V1.size(); ++I) {
    // check for underflow
    if (V2[I] >= 0) {
      if (std::numeric_limits<T>::min() + V2[I] > V1[I]) {
        V1[I] = std::numeric_limits<T>::min();
        continue;
      }
    } else {
      if (std::numeric_limits<T>::max() + V2[I] < V1[I]) {
        V1[I] = std::numeric_limits<T>::max();
        continue;
      }
    }
    V1[I] = V1[I] - V2[I];
  }
  StackMgr.emplaceTop(std::move(V1));
  return {};
}

template <typename T>
Expect<void>
Executor::runVectorMulOp(Runtime::StackManager &StackMgr) const noexcept {
  using VT = SIMDArray<T, 16>;
  auto [V2, V1] = StackMgr.popsPeekTop<VT, VT>();
  for (size_t I = 0; I < V1.size(); ++I) {
    V1[I] *= V2[I];
  }
  StackMgr.emplaceTop(std::move(V1));

  return {};
}

template <typename T>
Expect<void>
Executor::runVectorDivOp(Runtime::StackManager &StackMgr) const noexcept {
  using VT = SIMDArray<T, 16>;
  auto [V2, V1] = StackMgr.popsPeekTop<VT, VT>();
  for (size_t I = 0; I < V1.size(); ++I) {
    V1[I] /= V2[I];
  }
  StackMgr.emplaceTop(std::move(V1));

  return {};
}

template <typename T>
Expect<void>
Executor::runVectorMAddOp(Runtime::StackManager &StackMgr) const noexcept {
  using VT = SIMDArray<T, 16>;
  auto [V3, V2, V1] = StackMgr.popsPeekTop<VT, VT, VT>();
  for (size_t I = 0; I < V1.size(); ++I) {
    V1[I] *= V2[I];
  }
  for (size_t I = 0; I < V1.size(); ++I) {
    V1[I] += V3[I];
  }
  StackMgr.emplaceTop(std::move(V1));

  return {};
}

template <typename T>
Expect<void>
Executor::runVectorNMAddOp(Runtime::StackManager &StackMgr) const noexcept {
  using VT = SIMDArray<T, 16>;
  auto [V3, V2, V1] = StackMgr.popsPeekTop<VT, VT, VT>();
  for (size_t I = 0; I < V1.size(); ++I) {
    V1[I] = -V1[I];
  }
  for (size_t I = 0; I < V1.size(); ++I) {
    V1[I] *= V2[I];
  }
  for (size_t I = 0; I < V1.size(); ++I) {
    V1[I] += V3[I];
  }
  StackMgr.emplaceTop(std::move(V1));

  return {};
}

template <typename T>
Expect<void>
Executor::runVectorMinOp(Runtime::StackManager &StackMgr) const noexcept {
  using VT = SIMDArray<T, 16>;
  auto [V2, V1] = StackMgr.popsPeekTop<VT, VT>();
  for (size_t I = 0; I < V1.size(); ++I) {
    V1[I] = V1[I] > V2[I] ? V2[I] : V1[I];
  }
  StackMgr.emplaceTop(std::move(V1));

  return {};
}

template <typename T>
Expect<void>
Executor::runVectorMaxOp(Runtime::StackManager &StackMgr) const noexcept {
  using VT = SIMDArray<T, 16>;
  auto [V2, V1] = StackMgr.popsPeekTop<VT, VT>();
  for (size_t I = 0; I < V1.size(); ++I) {
    V1[I] = V1[I] < V2[I] ? V2[I] : V1[I];
  }
  StackMgr.emplaceTop(std::move(V1));

  return {};
}

template <typename T>
Expect<void>
Executor::runVectorFMinOp(Runtime::StackManager &StackMgr) const noexcept {
  static_assert(std::is_floating_point_v<T>);
  using VT = SIMDArray<T, 16>;
  auto [V2, V1] = StackMgr.popsPeekTop<VT, VT>();
  for (size_t I = 0; I < V1.size(); ++I) {
    if (V1[I] > V2[I]) {
      V1[I] = V2[I];
    } else if (V1[I] < V2[I]) {
      // do nothing
    } else if (std::isnan(V2[I]) && !std::isnan(V1[I])) {
      V1[I] = V2[I];
    } else if (V1[I] == static_cast<T>(0.0)) {
      // prefer negative zero
      if (std::signbit(V2[I]) && !std::signbit(V1[I])) {
        V1[I] = V2[I];
      }
    }
  }
  StackMgr.emplaceTop(std::move(V1));

  return {};
}

template <typename T>
Expect<void>
Executor::runVectorFMaxOp(Runtime::StackManager &StackMgr) const noexcept {
  using VT = SIMDArray<T, 16>;
  auto [V2, V1] = StackMgr.popsPeekTop<VT, VT>();
  for (size_t I = 0; I < V1.size(); ++I) {
    if (V1[I] < V2[I]) {
      V1[I] = V2[I];
    } else if (V1[I] > V2[I]) {
      // do nothing
    } else if (std::isnan(V2[I]) && !std::isnan(V1[I])) {
      V1[I] = V2[I];
    } else if (V1[I] == static_cast<T>(0.0)) {
      // prefer positive zero
      if (!std::signbit(V2[I]) && std::signbit(V1[I])) {
        V1[I] = V2[I];
      }
    }
  }
  StackMgr.emplaceTop(std::move(V1));

  return {};
}

template <typename T, typename ET>
Expect<void>
Executor::runVectorAvgrOp(Runtime::StackManager &StackMgr) const noexcept {
  static_assert(sizeof(T) * 2 == sizeof(ET));
  using VT = SIMDArray<T, 16>;
  auto [V2, V1] = StackMgr.popsPeekTop<VT, VT>();
  for (size_t I = 0; I < V1.size(); ++I) {
    // Add 1 for rounding up .5
    V1[I] = (static_cast<ET>(V1[I]) + static_cast<ET>(V2[I]) + 1) / 2;
  }
  StackMgr.emplaceTop(std::move(V1));

  return {};
}

template <typename TIn, typename TOut>
Expect<void>
Executor::runVectorExtMulLowOp(Runtime::StackManager &StackMgr) const noexcept {
  static_assert(sizeof(TIn) * 2 == sizeof(TOut));
  static_assert(sizeof(TIn) == 1 || sizeof(TIn) == 2 || sizeof(TIn) == 4);
  using VTIn = SIMDArray<TIn, 16>;
  using VTOut = SIMDArray<TOut, 16>;
  const auto [V2, V1] = StackMgr.popsPeekTop<VTIn, VTIn>();
  VTOut Result;
  for (size_t I = 0; I < Result.size(); ++I) {
    Result[I] = static_cast<TOut>(V1[I]) * static_cast<TOut>(V2[I]);
  }
  StackMgr.emplaceTop(std::move(Result));
  return {};
}

template <typename TIn, typename TOut>
Expect<void> Executor::runVectorExtMulHighOp(
    Runtime::StackManager &StackMgr) const noexcept {
  static_assert(sizeof(TIn) * 2 == sizeof(TOut));
  static_assert(sizeof(TIn) == 1 || sizeof(TIn) == 2 || sizeof(TIn) == 4);
  using VTIn = SIMDArray<TIn, 16>;
  using VTOut = SIMDArray<TOut, 16>;
  const auto [V2, V1] = StackMgr.popsPeekTop<VTIn, VTIn>();
  VTOut Result;
  constexpr const size_t HSize = Result.size();
  for (size_t I = 0; I < HSize; ++I) {
    Result[I] =
        static_cast<TOut>(V1[HSize + I]) * static_cast<TOut>(V2[HSize + I]);
  }
  StackMgr.emplaceTop(std::move(Result));
  return {};
}

inline Expect<void>
Executor::runVectorQ15MulSatOp(Runtime::StackManager &StackMgr) const noexcept {
  using int32x8_t = SIMDArray<int32_t, 32>;
  const auto [V2, V1] = StackMgr.popsPeekTop<int16x8_t, int16x8_t>();
  int16x8_t VOut;
  for (size_t I = 0; I < 8; I++) {
    int32_t ER = (static_cast<int32_t>(V1[I]) * static_cast<int32_t>(V2[I]) +
                  INT32_C(0x4000)) >>
                 INT32_C(15);
    if (ER > 0x7fff) {
      ER = 0x7fff;
    }
    VOut[I] = static_cast<int16_t>(ER);
  }
  StackMgr.emplaceTop(std::move(VOut));
  return {};
}

template <typename T>
Expect<void> Executor::runVectorRelaxedLaneselectOp(
    Runtime::StackManager &StackMgr) const noexcept {
  using VT = SIMDArray<T, 16>;

  auto [C, V2, V1] = StackMgr.popsPeekTop<VT, VT, VT>();

  for (size_t I = 0; I < V1.size(); ++I) {
    V1[I] = (V1[I] & C[I]) | (V2[I] & ~C[I]);
  }
  StackMgr.emplaceTop(std::move(V1));

  return {};
}

inline Expect<void> Executor::runVectorRelaxedIntegerDotProductOp(
    Runtime::StackManager &StackMgr) const noexcept {
  using int8x16_t = SIMDArray<int8_t, 16>;
  using int16x8_t = SIMDArray<int16_t, 16>;

  const auto [V2, V1] = StackMgr.popsPeekTop<int8x16_t, int8x16_t>();

  int16x8_t Result;
  for (size_t I = 0; I < Result.size(); ++I) {
    Result[I] =
        static_cast<int16_t>(V1[I * 2]) * static_cast<int16_t>(V2[I * 2]) +
        static_cast<int16_t>(V1[I * 2 + 1]) *
            static_cast<int16_t>(V2[I * 2 + 1]);
  }
  StackMgr.emplaceTop(std::move(Result));

  return {};
}

inline Expect<void> Executor::runVectorRelaxedIntegerDotProductOpAdd(
    Runtime::StackManager &StackMgr) const noexcept {
  using int8x16_t = SIMDArray<int8_t, 16>;
  using int16x8_t = SIMDArray<int16_t, 16>;
  using int32x4_t = SIMDArray<int32_t, 16>;

  const auto [VC, V2, V1] =
      StackMgr.popsPeekTop<int32x4_t, int8x16_t, int8x16_t>();
  int32x4_t Result{0, 0, 0, 0};

  for (size_t I = 0; I < V1.size(); ++I) {
    Result[I / 4] += static_cast<int16_t>(V1[I]) * static_cast<int16_t>(V2[I]);
  }

  for (size_t I = 0; I < VC.size(); ++I) {
    Result[I] = Result[I] + VC[I];
  }
  StackMgr.emplaceTop(std::move(Result));

  return {};
}

} // namespace Executor
} // namespace WasmEdge
