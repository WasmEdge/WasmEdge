// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "common/endian.h"
#include "executor/engine/simd_ops.h"
#include "executor/engine/vector_helper.h"
#include "executor/executor.h"

namespace WasmEdge {
namespace Executor {

template <typename TIn, typename TOut>
Expect<void> Executor::runExtractLaneOp(ValVariant &Val,
                                        const uint8_t Index) const {
  const TOut Result = Val.get<detail::Vec<TIn>>()[detail::lane<TIn>(Index)];
  Val.emplace<TOut>(Result);
  return {};
}

template <typename TIn, typename TOut>
Expect<void> Executor::runSplatOp(ValVariant &Val) const {
  const TOut Part = static_cast<TOut>(Val.get<TIn>());
  Val.emplace<detail::Vec<TOut>>(Part);
  return {};
}

template <typename TIn, typename TOut>
Expect<void> Executor::runVectorExtendLowOp(ValVariant &Val) const {
  static_assert(sizeof(TIn) * 2 == sizeof(TOut));
  using VTOut = detail::Vec<TOut>;
  using HVTIn = detail::Vec<TIn, VTOut::size>;
  const auto Halves = detail::simd::chunk<HVTIn>(Val.get<detail::Vec<TIn>>());
  Val.emplace<VTOut>(VTOut(Halves[Endian::native == Endian::little ? 0 : 1]));
  return {};
}

template <typename TIn, typename TOut>
Expect<void> Executor::runVectorExtendHighOp(ValVariant &Val) const {
  static_assert(sizeof(TIn) * 2 == sizeof(TOut));
  using VTOut = detail::Vec<TOut>;
  using HVTIn = detail::Vec<TIn, VTOut::size>;
  const auto Halves = detail::simd::chunk<HVTIn>(Val.get<detail::Vec<TIn>>());
  Val.emplace<VTOut>(VTOut(Halves[Endian::native == Endian::little ? 1 : 0]));
  return {};
}

template <typename TIn, typename TOut>
Expect<void> Executor::runVectorExtAddPairwiseOp(ValVariant &Val) const {
  static_assert(sizeof(TIn) * 2 == sizeof(TOut));
  using VTOut = detail::Vec<TOut>;
  const int Size = static_cast<int>(sizeof(TIn) * 8);
  const VTOut &V = Val.get<VTOut>();
  const auto L = V >> Size;
  const auto R = (V << Size) >> Size;
  Val.emplace<VTOut>(L + R);
  return {};
}

template <typename T>
Expect<void> Executor::runVectorAbsOp(ValVariant &Val) const {
  simdOps::vectorAbs<T>(Val);
  return {};
}

template <typename T>
Expect<void> Executor::runVectorNegOp(ValVariant &Val) const {
  simdOps::vectorNeg<T>(Val);
  return {};
}

inline Expect<void> Executor::runVectorPopcntOp(ValVariant &Val) const {
  simdOps::vectorPopcnt(Val);
  return {};
}

template <typename T>
Expect<void> Executor::runVectorSqrtOp(ValVariant &Val) const {
  simdOps::vectorSqrt<T>(Val);
  return {};
}

template <typename TIn, typename TOut>
Expect<void> Executor::runVectorTruncSatOp(ValVariant &Val) const {
  static_assert((sizeof(TIn) == 4 || sizeof(TIn) == 8) && sizeof(TOut) == 4);
  using VTIn = detail::Vec<TIn>;
  using VTOut = detail::Vec<TOut>;
  const VTIn FMin(static_cast<TIn>(std::numeric_limits<TOut>::min()));
  const VTIn FMax(static_cast<TIn>(std::numeric_limits<TOut>::max()));
  const VTIn X = trunc(Val.get<VTIn>());
  if constexpr (sizeof(TIn) == sizeof(TOut)) {
    const VTOut IMin(std::numeric_limits<TOut>::min());
    const VTOut IMax(std::numeric_limits<TOut>::max());
    VTOut Y(X);
    // NOLINTNEXTLINE(misc-redundant-expression): IEEE NaN check
    Y = select(X == X, Y, VTOut());
    Y = select(X <= FMin, IMin, Y);
    Y = select(X >= FMax, IMax, Y);
    Val.emplace<VTOut>(Y);
  } else {
    using TWide = std::conditional_t<std::is_signed_v<TOut>, int64_t, uint64_t>;
    using VTWide = detail::Vec<TWide>;
    // NOLINTNEXTLINE(misc-redundant-expression): IEEE NaN check
    VTIn C = select(X == X, X, VTIn());
    C = select(C <= FMin, FMin, C);
    C = select(C >= FMax, FMax, C);
    const auto Narrow = detail::Vec<TOut, 2>(VTWide(C));
    const detail::Vec<TOut, 2> Zero{};
    Val.emplace<VTOut>(Endian::native == Endian::little ? cat(Narrow, Zero)
                                                        : cat(Zero, Narrow));
  }
  return {};
}

template <typename TIn, typename TOut>
Expect<void> Executor::runVectorConvertOp(ValVariant &Val) const {
  static_assert((sizeof(TIn) == 4 && (sizeof(TOut) == 4 || sizeof(TOut) == 8)));
  using VTIn = detail::Vec<TIn>;
  using VTOut = detail::Vec<TOut>;
  const VTIn &V = Val.get<VTIn>();
  if constexpr (sizeof(TIn) == sizeof(TOut)) {
    Val.emplace<VTOut>(VTOut(V));
  } else {
    const auto Halves = detail::simd::chunk<detail::Vec<TIn, 2>>(V);
    Val.emplace<VTOut>(VTOut(Halves[Endian::native == Endian::little ? 0 : 1]));
  }
  return {};
}

inline Expect<void> Executor::runVectorDemoteOp(ValVariant &Val) const {
  using HVT = detail::Vec<float, 2>;
  const HVT V(Val.get<doublex2_t>());
  const HVT Zero{};
  if constexpr (Endian::native == Endian::little) {
    Val.emplace<floatx4_t>(cat(V, Zero));
  } else {
    Val.emplace<floatx4_t>(cat(Zero, permute(V, [](auto I) { return 1 - I; })));
  }
  return {};
}

inline Expect<void> Executor::runVectorPromoteOp(ValVariant &Val) const {
  const auto Halves =
      detail::simd::chunk<detail::Vec<float, 2>>(Val.get<floatx4_t>());
  if constexpr (Endian::native == Endian::little) {
    Val.emplace<doublex2_t>(doublex2_t(Halves[0]));
  } else {
    Val.emplace<doublex2_t>(
        doublex2_t(permute(Halves[1], [](auto I) { return 1 - I; })));
  }
  return {};
}

inline Expect<void> Executor::runVectorAnyTrueOp(ValVariant &Val) const {
  auto &Vector = Val.get<uint128_t>();
  const uint128_t Zero = 0U;
  const uint32_t Result = (Vector != Zero);
  Val.emplace<uint32_t>(Result);
  return {};
}

template <typename T>
Expect<void> Executor::runVectorAllTrueOp(ValVariant &Val) const {
  using VT = detail::Vec<T>;
  const uint32_t Result = all_of(Val.get<VT>() != VT());
  Val.emplace<uint32_t>(Result);
  return {};
}

template <typename T>
Expect<void> Executor::runVectorBitMaskOp(ValVariant &Val) const {
  using SVT = detail::Vec<std::make_signed_t<T>>;
  const auto Bits = (Val.get<SVT>() < SVT()).to_ullong();
  uint32_t Result = static_cast<uint32_t>(Bits);
  if constexpr (Endian::native == Endian::big) {
    Result = 0;
    for (int I = 0; I < SVT::size; ++I) {
      Result |= static_cast<uint32_t>((Bits >> I) & 1) << (SVT::size - 1 - I);
    }
  }
  Val.emplace<uint32_t>(Result);
  return {};
}

template <typename T>
Expect<void> Executor::runVectorCeilOp(ValVariant &Val) const {
  simdOps::vectorCeil<T>(Val);
  return {};
}

template <typename T>
Expect<void> Executor::runVectorFloorOp(ValVariant &Val) const {
  simdOps::vectorFloor<T>(Val);
  return {};
}

template <typename T>
Expect<void> Executor::runVectorTruncOp(ValVariant &Val) const {
  simdOps::vectorTrunc<T>(Val);
  return {};
}

template <typename T>
Expect<void> Executor::runVectorNearestOp(ValVariant &Val) const {
  simdOps::vectorNearest<T>(Val);
  return {};
}

} // namespace Executor
} // namespace WasmEdge
