// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "executor/engine/simd_ops.h"
#include "executor/engine/vector_helper.h"
#include "executor/executor.h"
#include "experimental/bit.hpp"
#include "experimental/simd/ext.hpp"

namespace WasmEdge {
namespace Executor {

template <typename TIn, typename TOut>
Expect<void> Executor::runReplaceLaneOp(ValVariant &Val1,
                                        const ValVariant &Val2,
                                        const uint8_t Index) const {
  cxx26::simd_ext::replace_lane(Val1.get<detail::Vec<TOut>>(),
                                detail::lane<TOut>(Index),
                                static_cast<TOut>(Val2.get<TIn>()));
  return {};
}

template <typename T>
Expect<void> Executor::runVectorEqOp(ValVariant &Val1,
                                     const ValVariant &Val2) const {
  using VT = detail::Vec<T>;
  VT &V1 = Val1.get<VT>();
  V1 = detail::maskToVec<VT>(V1 == Val2.get<VT>());
  return {};
}

template <typename T>
Expect<void> Executor::runVectorNeOp(ValVariant &Val1,
                                     const ValVariant &Val2) const {
  using VT = detail::Vec<T>;
  VT &V1 = Val1.get<VT>();
  V1 = detail::maskToVec<VT>(V1 != Val2.get<VT>());
  return {};
}

template <typename T>
Expect<void> Executor::runVectorLtOp(ValVariant &Val1,
                                     const ValVariant &Val2) const {
  using VT = detail::Vec<T>;
  VT &V1 = Val1.get<VT>();
  V1 = detail::maskToVec<VT>(V1 < Val2.get<VT>());
  return {};
}

template <typename T>
Expect<void> Executor::runVectorGtOp(ValVariant &Val1,
                                     const ValVariant &Val2) const {
  using VT = detail::Vec<T>;
  VT &V1 = Val1.get<VT>();
  V1 = detail::maskToVec<VT>(V1 > Val2.get<VT>());
  return {};
}

template <typename T>
Expect<void> Executor::runVectorLeOp(ValVariant &Val1,
                                     const ValVariant &Val2) const {
  using VT = detail::Vec<T>;
  VT &V1 = Val1.get<VT>();
  V1 = detail::maskToVec<VT>(V1 <= Val2.get<VT>());
  return {};
}

template <typename T>
Expect<void> Executor::runVectorGeOp(ValVariant &Val1,
                                     const ValVariant &Val2) const {
  using VT = detail::Vec<T>;
  VT &V1 = Val1.get<VT>();
  V1 = detail::maskToVec<VT>(V1 >= Val2.get<VT>());
  return {};
}

template <typename TIn, typename TOut>
Expect<void> Executor::runVectorNarrowOp(ValVariant &Val1,
                                         const ValVariant &Val2) const {
  static_assert(sizeof(TOut) * 2 == sizeof(TIn));
  static_assert(sizeof(TOut) == 1 || sizeof(TOut) == 2);
  using VTIn = detail::Vec<TIn>;
  using HVTOut = detail::Vec<TOut, VTIn::size>;

  const VTIn Min(static_cast<TIn>(std::numeric_limits<TOut>::min()));
  const VTIn Max(static_cast<TIn>(std::numeric_limits<TOut>::max()));
  VTIn V1 = Val1.get<VTIn>();
  VTIn V2 = Val2.get<VTIn>();
  V1 = select(V1 < Min, Min, V1);
  V1 = select(V1 > Max, Max, V1);
  V2 = select(V2 < Min, Min, V2);
  V2 = select(V2 > Max, Max, V2);
  const HVTOut HV1(V1);
  const HVTOut HV2(V2);
  if constexpr (Endian::native == Endian::little) {
    Val1.emplace<detail::Vec<TOut>>(cat(HV1, HV2));
  } else {
    Val1.emplace<detail::Vec<TOut>>(cat(HV2, HV1));
  }
  return {};
}

template <typename T>
Expect<void> Executor::runVectorShlOp(ValVariant &Val1,
                                      const ValVariant &Val2) const {
  using VT = detail::Vec<T>;
  const uint32_t Mask = static_cast<uint32_t>(sizeof(T) * 8 - 1);
  VT &V1 = Val1.get<VT>();
  V1 <<= static_cast<int>(Val2.get<uint32_t>() & Mask);
  return {};
}

template <typename T>
Expect<void> Executor::runVectorShrOp(ValVariant &Val1,
                                      const ValVariant &Val2) const {
  using VT = detail::Vec<T>;
  const uint32_t Mask = static_cast<uint32_t>(sizeof(T) * 8 - 1);
  VT &V1 = Val1.get<VT>();
  V1 >>= static_cast<int>(Val2.get<uint32_t>() & Mask);
  return {};
}

template <typename T>
Expect<void> Executor::runVectorAddOp(ValVariant &Val1,
                                      const ValVariant &Val2) const {
  simdOps::vectorAdd<T>(Val1, Val2);
  return {};
}

template <typename T>
Expect<void> Executor::runVectorAddSatOp(ValVariant &Val1,
                                         const ValVariant &Val2) const {
  using U = std::make_unsigned_t<T>;
  using VT = detail::Vec<T>;
  using UVT = detail::Vec<U>;
  UVT &V1 = Val1.get<UVT>();
  const UVT &V2 = Val2.get<UVT>();
  const UVT Result = V1 + V2;

  if constexpr (std::is_signed_v<T>) {
    const UVT Limit = (V1 >> static_cast<int>(sizeof(T) * 8 - 1)) +
                      UVT(static_cast<U>(std::numeric_limits<T>::max()));
    const VT Over = cxx20::bit_cast<VT>((V1 ^ V2) | ~(V2 ^ Result));
    V1 = select(Over >= VT(), Limit, Result);
  } else {
    V1 = Result | detail::maskToVec<UVT>(Result < V1);
  }
  return {};
}

template <typename T>
Expect<void> Executor::runVectorSubOp(ValVariant &Val1,
                                      const ValVariant &Val2) const {
  simdOps::vectorSub<T>(Val1, Val2);
  return {};
}

template <typename T>
Expect<void> Executor::runVectorSubSatOp(ValVariant &Val1,
                                         const ValVariant &Val2) const {
  using U = std::make_unsigned_t<T>;
  using VT = detail::Vec<T>;
  using UVT = detail::Vec<U>;
  UVT &V1 = Val1.get<UVT>();
  const UVT &V2 = Val2.get<UVT>();
  const UVT Result = V1 - V2;

  if constexpr (std::is_signed_v<T>) {
    const UVT Limit = (V1 >> static_cast<int>(sizeof(T) * 8 - 1)) +
                      UVT(static_cast<U>(std::numeric_limits<T>::max()));
    const VT Under = cxx20::bit_cast<VT>((V1 ^ V2) & (V1 ^ Result));
    V1 = select(Under < VT(), Limit, Result);
  } else {
    V1 = Result & detail::maskToVec<UVT>(Result <= V1);
  }
  return {};
}

template <typename T>
Expect<void> Executor::runVectorMulOp(ValVariant &Val1,
                                      const ValVariant &Val2) const {
  simdOps::vectorMul<T>(Val1, Val2);
  return {};
}

template <typename T>
Expect<void> Executor::runVectorDivOp(ValVariant &Val1,
                                      const ValVariant &Val2) const {
  simdOps::vectorDiv<T>(Val1, Val2);
  return {};
}

template <typename T>
Expect<void> Executor::runVectorMinOp(ValVariant &Val1,
                                      const ValVariant &Val2) const {
  simdOps::vectorMin<T>(Val1, Val2);
  return {};
}

template <typename T>
Expect<void> Executor::runVectorMaxOp(ValVariant &Val1,
                                      const ValVariant &Val2) const {
  simdOps::vectorMax<T>(Val1, Val2);
  return {};
}

template <typename T>
Expect<void> Executor::runVectorFMinOp(ValVariant &Val1,
                                       const ValVariant &Val2) const {
  simdOps::vectorFMin<T>(Val1, Val2);
  return {};
}

template <typename T>
Expect<void> Executor::runVectorFMaxOp(ValVariant &Val1,
                                       const ValVariant &Val2) const {
  simdOps::vectorFMax<T>(Val1, Val2);
  return {};
}

template <typename T, typename ET>
Expect<void> Executor::runVectorAvgrOp(ValVariant &Val1,
                                       const ValVariant &Val2) const {
  static_assert(sizeof(T) * 2 == sizeof(ET));
  using VT = detail::Vec<T>;
  using EVT = detail::Vec<ET, VT::size>;
  VT &V1 = Val1.get<VT>();
  const EVT EV1(V1);
  const EVT EV2(Val2.get<VT>());
  // Add 1 for rounding up .5
  V1 = VT((EV1 + EV2 + EVT(ET{1})) / EVT(ET{2}));
  return {};
}

template <typename TIn, typename TOut>
Expect<void> Executor::runVectorExtMulLowOp(ValVariant &Val1,
                                            const ValVariant &Val2) const {
  static_assert(sizeof(TIn) * 2 == sizeof(TOut));
  using HVTIn = detail::Vec<TIn, detail::Vec<TOut>::size>;
  using VTOut = detail::Vec<TOut>;
  const VTOut E1(detail::simd::chunk<HVTIn>(Val1.get<detail::Vec<TIn>>())[0]);
  const VTOut E2(detail::simd::chunk<HVTIn>(Val2.get<detail::Vec<TIn>>())[0]);
  Val1.emplace<VTOut>(E1 * E2);
  return {};
}

template <typename TIn, typename TOut>
Expect<void> Executor::runVectorExtMulHighOp(ValVariant &Val1,
                                             const ValVariant &Val2) const {
  static_assert(sizeof(TIn) * 2 == sizeof(TOut));
  using HVTIn = detail::Vec<TIn, detail::Vec<TOut>::size>;
  using VTOut = detail::Vec<TOut>;
  const VTOut E1(detail::simd::chunk<HVTIn>(Val1.get<detail::Vec<TIn>>())[1]);
  const VTOut E2(detail::simd::chunk<HVTIn>(Val2.get<detail::Vec<TIn>>())[1]);
  Val1.emplace<VTOut>(E1 * E2);
  return {};
}

inline Expect<void>
Executor::runVectorQ15MulSatOp(ValVariant &Val1, const ValVariant &Val2) const {
  using int32x8_t = detail::Vec<int32_t, 8>;
  const int32x8_t EV1(Val1.get<int16x8_t>());
  const int32x8_t EV2(Val2.get<int16x8_t>());
  const auto ER = (EV1 * EV2 + int32x8_t(INT32_C(0x4000))) >> 15;
  const int32x8_t Cap(INT32_C(0x7fff));
  Val1.emplace<int16x8_t>(int16x8_t(select(ER > Cap, Cap, ER)));
  return {};
}

template <typename T>
Expect<void>
Executor::runVectorRelaxedLaneselectOp(ValVariant &Val1, const ValVariant &Val2,
                                       const ValVariant &Mask) const {
  using VT = detail::Vec<T>;
  VT &V1 = Val1.get<VT>();
  const VT &V2 = Val2.get<VT>();
  const VT &C = Mask.get<VT>();
  V1 = (V1 & C) | (V2 & ~C);
  return {};
}

inline Expect<void>
Executor::runVectorRelaxedIntegerDotProductOp(ValVariant &Val1,
                                              const ValVariant &Val2) const {
  const int16x8_t &V1 = Val1.get<int16x8_t>();
  const int16x8_t &V2 = Val2.get<int16x8_t>();
  const int Size = 8;

  const auto V1L = V1 >> Size;
  const auto V1R = (V1 << Size) >> Size;
  const auto V2L = V2 >> Size;
  const auto V2R = (V2 << Size) >> Size;

  Val1.emplace<int16x8_t>(V1L * V2L + V1R * V2R);
  return {};
}

inline Expect<void> Executor::runVectorRelaxedIntegerDotProductOpAdd(
    ValVariant &Val1, const ValVariant &Val2, const ValVariant &C) const {
  const int16x8_t &V1 = Val1.get<int16x8_t>();
  const int16x8_t &V2 = Val2.get<int16x8_t>();
  const int Size = 8;
  const int32x4_t &VC = C.get<int32x4_t>();

  const auto V1L = V1 >> Size;
  const auto V1R = (V1 << Size) >> Size;
  const auto V2L = V2 >> Size;
  const auto V2R = (V2 << Size) >> Size;

  const auto IM = cxx20::bit_cast<int32x4_t>(V1L * V2L + V1R * V2R);
  const int IMSize = 16;
  const auto IML = IM >> IMSize;
  const auto IMR = (IM << IMSize) >> IMSize;

  Val1.emplace<int32x4_t>(IML + IMR + VC);
  return {};
}

} // namespace Executor
} // namespace WasmEdge
