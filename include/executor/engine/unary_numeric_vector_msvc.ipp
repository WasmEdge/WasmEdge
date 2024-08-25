// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "common/roundeven.h"
#include "executor/executor.h"

namespace WasmEdge {
namespace Executor {

template <typename TIn, typename TOut>
Expect<void> Executor::runExtractLaneOp(ValVariant &Val,
                                        const uint8_t Index) const {
  using VTIn = SIMDArray<TIn, 16>;
  const TOut Result = Val.get<VTIn>()[Index];
  Val.emplace<TOut>(Result);
  return {};
}

template <typename TIn, typename TOut>
Expect<void> Executor::runSplatOp(ValVariant &Val) const {
  const TOut Part = static_cast<TOut>(Val.get<TIn>());
  using VTOut = SIMDArray<TOut, 16>;
  if constexpr (sizeof(TOut) == 1) {
    Val.emplace<VTOut>(VTOut{Part, Part, Part, Part, Part, Part, Part, Part,
                             Part, Part, Part, Part, Part, Part, Part, Part});
  } else if constexpr (sizeof(TOut) == 2) {
    Val.emplace<VTOut>(VTOut{Part, Part, Part, Part, Part, Part, Part, Part});
  } else if constexpr (sizeof(TOut) == 4) {
    Val.emplace<VTOut>(VTOut{Part, Part, Part, Part});
  } else if constexpr (sizeof(TOut) == 8) {
    Val.emplace<VTOut>(VTOut{Part, Part});
  }
  return {};
}

template <typename TIn, typename TOut>
Expect<void> Executor::runVectorExtendLowOp(ValVariant &Val) const {
  static_assert(sizeof(TIn) * 2 == sizeof(TOut));
  static_assert(sizeof(TIn) == 1 || sizeof(TIn) == 2 || sizeof(TIn) == 4);
  using VTIn = SIMDArray<TIn, 16>;
  using HVTIn = SIMDArray<TIn, 8>;
  using VTOut = SIMDArray<TOut, 16>;
  const VTIn &V = Val.get<VTIn>();
  VTOut Result;
  for (size_t I = 0; I < Result.size(); ++I) {
    Result[I] = V[I];
  }
  Val.emplace<VTOut>(Result);
  return {};
}

template <typename TIn, typename TOut>
Expect<void> Executor::runVectorExtendHighOp(ValVariant &Val) const {
  static_assert(sizeof(TIn) * 2 == sizeof(TOut));
  static_assert(sizeof(TIn) == 1 || sizeof(TIn) == 2 || sizeof(TIn) == 4);
  using VTIn = SIMDArray<TIn, 16>;
  using VTOut = SIMDArray<TOut, 16>;
  VTOut Result;
  const VTIn &V = Val.get<VTIn>();
  constexpr size_t HSize = Result.size();
  for (size_t I = 0; I < HSize; ++I) {
    Result[I] = V[HSize + I];
  }
  Val.emplace<VTOut>(Result);
  return {};
}

template <typename TIn, typename TOut>
Expect<void> Executor::runVectorExtAddPairwiseOp(ValVariant &Val) const {
  static_assert(sizeof(TIn) * 2 == sizeof(TOut));
  using VTIn = SIMDArray<TIn, 16>;
  using VTOut = SIMDArray<TOut, 16>;

  VTOut Result;
  const VTIn &V = Val.get<VTIn>();
  for (size_t I = 0; I < Result.size(); ++I) {
    Result[I] = static_cast<TOut>(V[I * 2]) + static_cast<TOut>(V[I * 2 + 1]);
  }
  Val.emplace<VTOut>(Result);

  return {};
}

template <typename T>
Expect<void> Executor::runVectorAbsOp(ValVariant &Val) const {
  using VT = SIMDArray<T, 16>;
  VT &Result = Val.get<VT>();
  for (size_t I = 0; I < Result.size(); ++I) {
    if constexpr (std::is_floating_point_v<T>) {
      if constexpr (sizeof(T) == 4) {
        uint32_t Tmp =
            reinterpret_cast<uint32_t &>(Result[I]) & UINT32_C(0x7fffffff);
        Result[I] = reinterpret_cast<T &>(Tmp);
      } else {
        uint64_t Tmp = reinterpret_cast<uint64_t &>(Result[I]) &
                       UINT64_C(0x7fffffffffffffff);
        Result[I] = reinterpret_cast<T &>(Tmp);
      }
    } else {
      Result[I] = Result[I] > 0 ? Result[I] : -Result[I];
    }
  }
  return {};
}

template <typename T>
Expect<void> Executor::runVectorNegOp(ValVariant &Val) const {
  using VT = SIMDArray<T, 16>;
  VT &Result = Val.get<VT>();
  for (size_t I = 0; I < Result.size(); ++I) {
    Result[I] = -Result[I];
  }
  return {};
}

inline Expect<void> Executor::runVectorPopcntOp(ValVariant &Val) const {
  auto &Result = Val.get<uint8x16_t>();
  for (size_t I = 0; I < 16; ++I) {
    Result[I] -= ((Result[I] >> UINT8_C(1)) & UINT8_C(0x55));
    Result[I] = (Result[I] & UINT8_C(0x33)) +
                ((Result[I] >> UINT8_C(2)) & UINT8_C(0x33));
    Result[I] += Result[I] >> UINT8_C(4);
    Result[I] &= UINT8_C(0x0f);
  }
  return {};
}

template <typename T>
Expect<void> Executor::runVectorSqrtOp(ValVariant &Val) const {
  using VT = SIMDArray<T, 16>;
  VT &Result = Val.get<VT>();
  if constexpr (sizeof(T) == 4) {
    Result = VT{std::sqrt(Result[0]), std::sqrt(Result[1]),
                std::sqrt(Result[2]), std::sqrt(Result[3])};
  } else if constexpr (sizeof(T) == 8) {
    Result = VT{std::sqrt(Result[0]), std::sqrt(Result[1])};
  }
  return {};
}

template <typename TIn, typename TOut>
Expect<void> Executor::runVectorTruncSatOp(ValVariant &Val) const {
  static_assert((sizeof(TIn) == 4 || sizeof(TIn) == 8) && sizeof(TOut) == 4);
  using VTIn = SIMDArray<TIn, 16>;
  using VTOut = SIMDArray<TOut, 16>;
  auto &V = Val.get<VTIn>();
  VTOut Result = {}; // all zero initialization for i32x4.trunc_sat_f64x2
  for (size_t I = 0; I < V.size(); ++I) {
    if (std::isnan(V[I])) {
      // For NaN, return 0.
      Result[I] = static_cast<TOut>(0);
    } else {
      TIn Tr = std::trunc(V[I]);
      if (Tr <= static_cast<TIn>(std::numeric_limits<TOut>::min())) {
        Result[I] = std::numeric_limits<TOut>::min();
      } else if (Tr >= static_cast<TIn>(std::numeric_limits<TOut>::max())) {
        Result[I] = std::numeric_limits<TOut>::max();
      } else {
        Result[I] = static_cast<TOut>(Tr);
      }
    }
  }
  Val.emplace<VTOut>(Result);
  return {};
}

template <typename TIn, typename TOut>
Expect<void> Executor::runVectorConvertOp(ValVariant &Val) const {
  static_assert((sizeof(TIn) == 4 && (sizeof(TOut) == 4 || sizeof(TOut) == 8)));
  using VTIn = SIMDArray<TIn, 16>;
  using VTOut = SIMDArray<TOut, 16>;
  auto &V = Val.get<VTIn>();
  // int32/uint32 to float
  if constexpr (sizeof(TIn) == sizeof(TOut)) {
    Val.emplace<VTOut>(VTOut{static_cast<TOut>(V[0]), static_cast<TOut>(V[1]),
                             static_cast<TOut>(V[2]), static_cast<TOut>(V[3])});
  } else { // int32/uint32 to double
    Val.emplace<VTOut>(VTOut{static_cast<TOut>(V[0]), static_cast<TOut>(V[1])});
  }
  return {};
}

inline Expect<void> Executor::runVectorDemoteOp(ValVariant &Val) const {
  const auto V = Val.get<doublex2_t>();
  Val.emplace<floatx4_t>(
      floatx4_t{static_cast<float>(V[0]), static_cast<float>(V[1]), 0, 0});
  return {};
}

inline Expect<void> Executor::runVectorPromoteOp(ValVariant &Val) const {
  const auto V = Val.get<floatx4_t>();
  Val.emplace<doublex2_t>(
      doublex2_t{static_cast<double>(V[0]), static_cast<double>(V[1])});
  return {};
}

inline Expect<void> Executor::runVectorAnyTrueOp(ValVariant &Val) const {
  auto &Vector = Val.get<uint128_t>();
  const uint128_t Zero = 0;
  const uint32_t Result = (Vector != Zero);
  Val.emplace<uint32_t>(Result);

  return {};
}

template <typename T>
Expect<void> Executor::runVectorAllTrueOp(ValVariant &Val) const {
  using VT = SIMDArray<T, 16>;
  VT &V = Val.get<VT>();
  uint32_t Result;
  if constexpr (sizeof(T) == 1) {
    Result = V[0] != 0 && V[1] != 0 && V[2] != 0 && V[3] != 0 && V[4] != 0 &&
             V[5] != 0 && V[6] != 0 && V[7] != 0 && V[8] != 0 && V[9] != 0 &&
             V[10] != 0 && V[11] != 0 && V[12] != 0 && V[13] != 0 &&
             V[14] != 0 && V[15] != 0;
  } else if constexpr (sizeof(T) == 2) {
    Result = V[0] != 0 && V[1] != 0 && V[2] != 0 && V[3] != 0 && V[4] != 0 &&
             V[5] != 0 && V[6] != 0 && V[7] != 0;
  } else if constexpr (sizeof(T) == 4) {
    Result = V[0] != 0 && V[1] != 0 && V[2] != 0 && V[3] != 0;
  } else if constexpr (sizeof(T) == 8) {
    Result = V[0] != 0 && V[1] != 0;
  }
  Val.emplace<uint32_t>(Result);

  return {};
}

template <typename T>
Expect<void> Executor::runVectorBitMaskOp(ValVariant &Val) const {
  using SVT = SIMDArray<std::make_signed_t<T>, 16>;
  using UVT = SIMDArray<std::make_unsigned_t<T>, 16>;
  SVT &Vector = Val.get<SVT>();
  if constexpr (sizeof(T) == 1) {
    using int16x16_t = SIMDArray<int16_t, 32>;
    using uint16x16_t = SIMDArray<uint16_t, 32>;
    const uint16x16_t Mask = {0x1,    0x2,    0x4,    0x8,   0x10,  0x20,
                              0x40,   0x80,   0x100,  0x200, 0x400, 0x800,
                              0x1000, 0x2000, 0x4000, 0x8000};
    uint16_t Result = 0;
    for (size_t I = 0; I < 16; ++I) {
      Result |= Vector[I] < 0 ? Mask[I] : 0;
    }
    Val.emplace<uint32_t>(Result);
  } else if constexpr (sizeof(T) == 2) {
    const uint16x8_t Mask = {0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80};
    using uint8x8_t = SIMDArray<uint8_t, 8>;
    uint8_t Result = 0;
    for (size_t I = 0; I < 8; ++I) {
      Result |= Vector[I] < 0 ? Mask[I] : 0;
    }
    Val.emplace<uint32_t>(Result);
  } else if constexpr (sizeof(T) == 4) {
    const uint32x4_t Mask = {0x1, 0x2, 0x4, 0x8};
    using uint8x4_t = SIMDArray<uint8_t, 4>;
    uint8_t Result = 0;
    for (size_t I = 0; I < 4; ++I) {
      Result |= Vector[I] < 0 ? Mask[I] : 0;
    }
    Val.emplace<uint32_t>(Result);
  } else if constexpr (sizeof(T) == 8) {
    const uint64x2_t Mask = {0x1, 0x2};
    using uint8x2_t = SIMDArray<uint8_t, 2>;
    uint8_t Result = 0;
    for (size_t I = 0; I < 2; ++I) {
      Result |= Vector[I] < 0 ? Mask[I] : 0;
    }
    Val.emplace<uint32_t>(Result);
  }

  return {};
}

template <typename T>
Expect<void> Executor::runVectorCeilOp(ValVariant &Val) const {
  using VT = SIMDArray<T, 16>;
  VT &Result = Val.get<VT>();
  if constexpr (sizeof(T) == 4) {
    Result = VT{std::ceil(Result[0]), std::ceil(Result[1]),
                std::ceil(Result[2]), std::ceil(Result[3])};
  } else if constexpr (sizeof(T) == 8) {
    Result = VT{std::ceil(Result[0]), std::ceil(Result[1])};
  }
  return {};
}

template <typename T>
Expect<void> Executor::runVectorFloorOp(ValVariant &Val) const {
  using VT = SIMDArray<T, 16>;
  VT &Result = Val.get<VT>();
  if constexpr (sizeof(T) == 4) {
    Result = VT{std::floor(Result[0]), std::floor(Result[1]),
                std::floor(Result[2]), std::floor(Result[3])};
  } else if constexpr (sizeof(T) == 8) {
    Result = VT{std::floor(Result[0]), std::floor(Result[1])};
  }
  return {};
}

template <typename T>
Expect<void> Executor::runVectorTruncOp(ValVariant &Val) const {
  using VT = SIMDArray<T, 16>;
  VT &Result = Val.get<VT>();
  if constexpr (sizeof(T) == 4) {
    Result = VT{std::trunc(Result[0]), std::trunc(Result[1]),
                std::trunc(Result[2]), std::trunc(Result[3])};
  } else if constexpr (sizeof(T) == 8) {
    Result = VT{std::trunc(Result[0]), std::trunc(Result[1])};
  }
  return {};
}

template <typename T>
Expect<void> Executor::runVectorNearestOp(ValVariant &Val) const {
  using VT = SIMDArray<T, 16>;
  VT &Result = Val.get<VT>();
  if constexpr (sizeof(T) == 4) {
    Result = VT{WasmEdge::roundeven(Result[0]), WasmEdge::roundeven(Result[1]),
                WasmEdge::roundeven(Result[2]), WasmEdge::roundeven(Result[3])};
  } else if constexpr (sizeof(T) == 8) {
    Result = VT{WasmEdge::roundeven(Result[0]), WasmEdge::roundeven(Result[1])};
  }
  return {};
}

} // namespace Executor
} // namespace WasmEdge
