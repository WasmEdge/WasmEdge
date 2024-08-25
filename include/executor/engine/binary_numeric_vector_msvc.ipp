// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "executor/executor.h"

namespace WasmEdge {
namespace Executor {

template <typename TIn, typename TOut>
Expect<void> Executor::runReplaceLaneOp(ValVariant &Val1,
                                        const ValVariant &Val2,
                                        const uint8_t Index) const {
  using VTOut = SIMDArray<TOut, 16>;
  VTOut &Result = Val1.get<VTOut>();
  Result[Index] = static_cast<TOut>(Val2.get<TIn>());
  return {};
}

template <typename T>
Expect<void> Executor::runVectorEqOp(ValVariant &Val1,
                                     const ValVariant &Val2) const {
  using VT = SIMDArray<T, 16>;

  VT &V1 = Val1.get<VT>();
  const VT &V2 = Val2.get<VT>();

  int64_t IAllOnes = INT64_C(-1);
  const T AllOnes = reinterpret_cast<T &>(IAllOnes);

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
  Val1.emplace<VT>(VOut);

  return {};
}

template <typename T>
Expect<void> Executor::runVectorNeOp(ValVariant &Val1,
                                     const ValVariant &Val2) const {
  using VT = SIMDArray<T, 16>;

  VT &V1 = Val1.get<VT>();
  const VT &V2 = Val2.get<VT>();

  int64_t IAllOnes = INT64_C(-1);
  const T AllOnes = reinterpret_cast<T &>(IAllOnes);

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
  Val1.emplace<VT>(VOut);

  return {};
}

template <typename T>
Expect<void> Executor::runVectorLtOp(ValVariant &Val1,
                                     const ValVariant &Val2) const {
  using VT = SIMDArray<T, 16>;

  VT &V1 = Val1.get<VT>();
  const VT &V2 = Val2.get<VT>();

  int64_t IAllOnes = INT64_C(-1);
  const T AllOnes = reinterpret_cast<T &>(IAllOnes);

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
  Val1.emplace<VT>(VOut);

  return {};
}

template <typename T>
Expect<void> Executor::runVectorGtOp(ValVariant &Val1,
                                     const ValVariant &Val2) const {
  using VT = SIMDArray<T, 16>;

  VT &V1 = Val1.get<VT>();
  const VT &V2 = Val2.get<VT>();

  int64_t IAllOnes = INT64_C(-1);
  const T AllOnes = reinterpret_cast<T &>(IAllOnes);

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
  Val1.emplace<VT>(VOut);

  return {};
}

template <typename T>
Expect<void> Executor::runVectorLeOp(ValVariant &Val1,
                                     const ValVariant &Val2) const {
  using VT = SIMDArray<T, 16>;

  VT &V1 = Val1.get<VT>();
  const VT &V2 = Val2.get<VT>();

  int64_t IAllOnes = INT64_C(-1);
  const T AllOnes = reinterpret_cast<T &>(IAllOnes);

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
  Val1.emplace<VT>(VOut);

  return {};
}

template <typename T>
Expect<void> Executor::runVectorGeOp(ValVariant &Val1,
                                     const ValVariant &Val2) const {
  using VT = SIMDArray<T, 16>;

  VT &V1 = Val1.get<VT>();
  const VT &V2 = Val2.get<VT>();

  int64_t IAllOnes = INT64_C(-1);
  const T AllOnes = reinterpret_cast<T &>(IAllOnes);

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
  Val1.emplace<VT>(VOut);

  return {};
}

template <typename TIn, typename TOut>
Expect<void> Executor::runVectorNarrowOp(ValVariant &Val1,
                                         const ValVariant &Val2) const {
  static_assert(sizeof(TOut) * 2 == sizeof(TIn));
  static_assert(sizeof(TOut) == 1 || sizeof(TOut) == 2);
  using VTIn = SIMDArray<TIn, 16>;
  using HVTOut = SIMDArray<TOut, 8>;
  using VTOut = SIMDArray<TOut, 16>;

  VTOut Result;
  VTIn V1 = Val1.get<VTIn>();
  constexpr size_t HSize = V1.size();
  for (size_t I = 0; I < HSize; ++I) {
    if (V1[I] > std::numeric_limits<TOut>::max()) {
      Result[I] = std::numeric_limits<TOut>::max();
    } else if (V1[I] < std::numeric_limits<TOut>::min()) {
      Result[I] = std::numeric_limits<TOut>::min();
    } else {
      Result[I] = static_cast<TOut>(V1[I]);
    }
  }
  const VTIn &V2 = Val2.get<VTIn>();
  for (size_t I = 0; I < HSize; ++I) {
    if (V2[I] > std::numeric_limits<TOut>::max()) {
      Result[HSize + I] = std::numeric_limits<TOut>::max();
    } else if (V2[I] < std::numeric_limits<TOut>::min()) {
      Result[HSize + I] = std::numeric_limits<TOut>::min();
    } else {
      Result[HSize + I] = static_cast<TOut>(V2[I]);
    }
  }
  Val1.emplace<VTOut>(Result);
  return {};
}

template <typename T>
Expect<void> Executor::runVectorShlOp(ValVariant &Val1,
                                      const ValVariant &Val2) const {
  using VT = SIMDArray<T, 16>;
  const uint32_t Mask = static_cast<uint32_t>(sizeof(T) * 8 - 1);
  const uint32_t Count = Val2.get<uint32_t>() & Mask;
  VT &V1 = Val1.get<VT>();
  for (size_t I = 0; I < V1.size(); ++I) {
    V1[I] <<= Count;
  }

  return {};
}

template <typename T>
Expect<void> Executor::runVectorShrOp(ValVariant &Val1,
                                      const ValVariant &Val2) const {
  using VT = SIMDArray<T, 16>;
  const uint32_t Mask = static_cast<uint32_t>(sizeof(T) * 8 - 1);
  const uint32_t Count = Val2.get<uint32_t>() & Mask;
  VT &V1 = Val1.get<VT>();
  for (size_t I = 0; I < V1.size(); ++I) {
    V1[I] >>= Count;
  }

  return {};
}

template <typename T>
Expect<void> Executor::runVectorAddOp(ValVariant &Val1,
                                      const ValVariant &Val2) const {
  using VT = SIMDArray<T, 16>;
  VT &V1 = Val1.get<VT>();
  const VT &V2 = Val2.get<VT>();
  for (size_t I = 0; I < V1.size(); ++I) {
    V1[I] += V2[I];
  }

  return {};
}

template <typename T>
Expect<void> Executor::runVectorAddSatOp(ValVariant &Val1,
                                         const ValVariant &Val2) const {
  static_assert(sizeof(T) < 4);
  using VT = SIMDArray<T, 16>;
  VT &V1 = Val1.get<VT>();
  const VT &V2 = Val2.get<VT>();

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
  return {};
}

template <typename T>
Expect<void> Executor::runVectorSubOp(ValVariant &Val1,
                                      const ValVariant &Val2) const {
  using VT = SIMDArray<T, 16>;
  VT &V1 = Val1.get<VT>();
  const VT &V2 = Val2.get<VT>();
  for (size_t I = 0; I < V1.size(); ++I) {
    V1[I] -= V2[I];
  }

  return {};
}

template <typename T>
Expect<void> Executor::runVectorSubSatOp(ValVariant &Val1,
                                         const ValVariant &Val2) const {
  static_assert(sizeof(T) < 4);
  using VT = SIMDArray<T, 16>;
  VT &V1 = Val1.get<VT>();
  const VT &V2 = Val2.get<VT>();

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
  return {};
}

template <typename T>
Expect<void> Executor::runVectorMulOp(ValVariant &Val1,
                                      const ValVariant &Val2) const {
  using VT = SIMDArray<T, 16>;
  VT &V1 = Val1.get<VT>();
  const VT &V2 = Val2.get<VT>();
  for (size_t I = 0; I < V1.size(); ++I) {
    V1[I] *= V2[I];
  }

  return {};
}

template <typename T>
Expect<void> Executor::runVectorDivOp(ValVariant &Val1,
                                      const ValVariant &Val2) const {
  using VT = SIMDArray<T, 16>;
  VT &V1 = Val1.get<VT>();
  const VT &V2 = Val2.get<VT>();
  for (size_t I = 0; I < V1.size(); ++I) {
    V1[I] /= V2[I];
  }

  return {};
}

template <typename T>
Expect<void> Executor::runVectorMinOp(ValVariant &Val1,
                                      const ValVariant &Val2) const {
  using VT = SIMDArray<T, 16>;
  VT &V1 = Val1.get<VT>();
  const VT &V2 = Val2.get<VT>();
  for (size_t I = 0; I < V1.size(); ++I) {
    V1[I] = V1[I] > V2[I] ? V2[I] : V1[I];
  }

  return {};
}

template <typename T>
Expect<void> Executor::runVectorMaxOp(ValVariant &Val1,
                                      const ValVariant &Val2) const {
  using VT = SIMDArray<T, 16>;
  VT &V1 = Val1.get<VT>();
  const VT &V2 = Val2.get<VT>();
  for (size_t I = 0; I < V1.size(); ++I) {
    V1[I] = V1[I] < V2[I] ? V2[I] : V1[I];
  }

  return {};
}

template <typename T>
Expect<void> Executor::runVectorFMinOp(ValVariant &Val1,
                                       const ValVariant &Val2) const {
  static_assert(std::is_floating_point_v<T>);
  using VT = SIMDArray<T, 16>;
  VT &V1 = Val1.get<VT>();
  const VT &V2 = Val2.get<VT>();
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

  return {};
}

template <typename T>
Expect<void> Executor::runVectorFMaxOp(ValVariant &Val1,
                                       const ValVariant &Val2) const {
  using VT = SIMDArray<T, 16>;
  VT &V1 = Val1.get<VT>();
  const VT &V2 = Val2.get<VT>();
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

  return {};
}

template <typename T, typename ET>
Expect<void> Executor::runVectorAvgrOp(ValVariant &Val1,
                                       const ValVariant &Val2) const {
  static_assert(sizeof(T) * 2 == sizeof(ET));
  using VT = SIMDArray<T, 16>;
  VT &V1 = Val1.get<VT>();
  const VT &V2 = Val2.get<VT>();
  for (size_t I = 0; I < V1.size(); ++I) {
    // Add 1 for rounding up .5
    V1[I] = (static_cast<ET>(V1[I]) + static_cast<ET>(V2[I]) + 1) / 2;
  }

  return {};
}

template <typename TIn, typename TOut>
Expect<void> Executor::runVectorExtMulLowOp(ValVariant &Val1,
                                            const ValVariant &Val2) const {
  static_assert(sizeof(TIn) * 2 == sizeof(TOut));
  static_assert(sizeof(TIn) == 1 || sizeof(TIn) == 2 || sizeof(TIn) == 4);
  using VTIn = SIMDArray<TIn, 16>;
  using VTOut = SIMDArray<TOut, 16>;
  const VTIn &V1 = Val1.get<VTIn>();
  const VTIn &V2 = Val2.get<VTIn>();
  VTOut Result;
  for (size_t I = 0; I < Result.size(); ++I) {
    Result[I] = static_cast<TOut>(V1[I]) * static_cast<TOut>(V2[I]);
  }
  Val1.emplace<VTOut>(Result);
  return {};
}

template <typename TIn, typename TOut>
Expect<void> Executor::runVectorExtMulHighOp(ValVariant &Val1,
                                             const ValVariant &Val2) const {
  static_assert(sizeof(TIn) * 2 == sizeof(TOut));
  static_assert(sizeof(TIn) == 1 || sizeof(TIn) == 2 || sizeof(TIn) == 4);
  using VTIn = SIMDArray<TIn, 16>;
  using VTOut = SIMDArray<TOut, 16>;
  VTOut Result;
  const VTIn &V1 = Val1.get<VTIn>();
  const VTIn &V2 = Val2.get<VTIn>();
  constexpr size_t HSize = Result.size();
  for (size_t I = 0; I < HSize; ++I) {
    Result[I] =
        static_cast<TOut>(V1[HSize + I]) * static_cast<TOut>(V2[HSize + I]);
  }
  Val1.emplace<VTOut>(Result);
  return {};
}

inline Expect<void>
Executor::runVectorQ15MulSatOp(ValVariant &Val1, const ValVariant &Val2) const {
  using int32x8_t = SIMDArray<int32_t, 32>;
  const auto &V1 = Val1.get<int16x8_t>();
  const auto &V2 = Val2.get<int16x8_t>();
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
  Val1.emplace<int16x8_t>(VOut);
  return {};
}

template <typename T>
Expect<void>
Executor::runVectorRelaxedLaneselectOp(ValVariant &Val1, const ValVariant &Val2,
                                       const ValVariant &Mask) const {
  using VT = SIMDArray<T, 16>;

  VT &V1 = Val1.get<VT>();
  const VT &V2 = Val2.get<VT>();
  const VT &C = Mask.get<VT>();

  for (size_t I = 0; I < V1.size(); ++I) {
    V1[I] = (V1[I] & C[I]) | (V2[I] & ~C[I]);
  }

  return {};
}

inline Expect<void>
Executor::runVectorRelaxedIntegerDotProductOp(ValVariant &Val1,
                                              const ValVariant &Val2) const {
  using int8x16_t = SIMDArray<int8_t, 16>;
  using int16x8_t = SIMDArray<int16_t, 16>;

  const int8x16_t &V1 = Val1.get<int8x16_t>();
  const int8x16_t &V2 = Val2.get<int8x16_t>();

  int16x8_t Result;
  for (size_t I = 0; I < Result.size(); ++I) {
    Result[I] =
        static_cast<int16_t>(V1[I * 2]) * static_cast<int16_t>(V2[I * 2]) +
        static_cast<int16_t>(V1[I * 2 + 1]) *
            static_cast<int16_t>(V2[I * 2 + 1]);
  }

  Val1.emplace<int16x8_t>(Result);
  return {};
}

inline Expect<void> Executor::runVectorRelaxedIntegerDotProductOpAdd(
    ValVariant &Val1, const ValVariant &Val2, const ValVariant &C) const {
  using int8x16_t = SIMDArray<int8_t, 16>;
  using int16x8_t = SIMDArray<int16_t, 16>;
  using int32x4_t = SIMDArray<int32_t, 16>;

  const int8x16_t &V1 = Val1.get<int8x16_t>();
  const int8x16_t &V2 = Val2.get<int8x16_t>();
  const int32x4_t &VC = C.get<int32x4_t>();

  int32x4_t Result{0, 0, 0, 0};

  for (size_t I = 0; I < V1.size(); ++I) {
    Result[I / 4] += static_cast<int16_t>(V1[I]) * static_cast<int16_t>(V2[I]);
  }

  for (size_t I = 0; I < VC.size(); ++I) {
    Result[I] = Result[I] + VC[I];
  }

  Val1.emplace<int32x4_t>(Result);
  return {};
}

} // namespace Executor
} // namespace WasmEdge
