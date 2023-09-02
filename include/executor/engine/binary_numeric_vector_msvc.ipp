// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

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
  const T AllOnes = reinterpret_cast<T&>(IAllOnes);

  // unrolling V1 = (V1 == V2);
  VT VOut;
  for(size_t I = 0; I < V1.size(); I++) {
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
  const T AllOnes = reinterpret_cast<T&>(IAllOnes);

  // unrolling V1 = (V1 != V2);
  VT VOut;
  for(size_t I = 0; I < V1.size(); I++) {
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
  const T AllOnes = reinterpret_cast<T&>(IAllOnes);

  // unrolling V1 = (V1 < V2);
  VT VOut;
  for(size_t I = 0; I < V1.size(); I++) {
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
  const T AllOnes = reinterpret_cast<T&>(IAllOnes);

  // unrolling V1 = (V1 > V2);
  VT VOut;
  for(size_t I = 0; I < V1.size(); I++) {
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
  const T AllOnes = reinterpret_cast<T&>(IAllOnes);

  // unrolling V1 = (V1 <= V2);
  VT VOut;
  for(size_t I = 0; I < V1.size(); I++) {
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
  const T AllOnes = reinterpret_cast<T&>(IAllOnes);

  // unrolling V1 = (V1 >= V2);
  VT VOut;
  for(size_t I = 0; I < V1.size(); I++) {
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
  for(size_t I = 0; I < HSize; ++I) {
    if (V1[I] > std::numeric_limits<TOut>::max()) {
      Result[I] = std::numeric_limits<TOut>::max();
    } else if (V1[I] < std::numeric_limits<TOut>::min()) {
      Result[I] = std::numeric_limits<TOut>::min();
    } else {
      Result[I] = static_cast<TOut>(V1[I]);
    }
  }
  const VTIn &V2 = Val2.get<VTIn>();
  for(size_t I = 0; I < HSize; ++I) {
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
  for(size_t I = 0; I < V1.size(); ++I) {
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
  for(size_t I = 0; I < V1.size(); ++I) {
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
      if (std::numeric_limits<T>::min() - V1[I] > V2[I] ) {
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
    Result[I] = static_cast<TOut>(V1[HSize + I]) * static_cast<TOut>(V2[HSize + I]);
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
  for(size_t I = 0; I < 8; I++) {
    int32_t ER = (static_cast<int32_t>(V1[I]) * static_cast<int32_t>(V2[I]) + INT32_C(0x4000)) >> INT32_C(15);
    if (ER > 0x7fff) {
      ER = 0x7fff;
    }
    VOut[I] = static_cast<int16_t>(ER);
  }
  Val1.emplace<int16x8_t>(VOut);
  return {};
}

} // namespace Executor
} // namespace WasmEdge
