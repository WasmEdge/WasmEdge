
// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "executor/engine/vector_helper.h"
#include "executor/executor.h"

namespace WasmEdge {
namespace Executor {

template <typename TIn, typename TOut>
Expect<void> Executor::runReplaceLaneOp(ValVariant &Val1,
                                        const ValVariant &Val2,
                                        const uint8_t Index) const {
  using VTOut [[gnu::vector_size(16)]] = TOut;
  VTOut &Result = Val1.get<VTOut>();
  Result[Index] = static_cast<TOut>(Val2.get<TIn>());
  return {};
}

template <typename T>
Expect<void> Executor::runVectorEqOp(ValVariant &Val1,
                                     const ValVariant &Val2) const {
  using VT [[gnu::vector_size(16)]] = T;

  VT &V1 = Val1.get<VT>();
  const VT &V2 = Val2.get<VT>();

  V1 = (V1 == V2);
  return {};
}

template <typename T>
Expect<void> Executor::runVectorNeOp(ValVariant &Val1,
                                     const ValVariant &Val2) const {
  using VT [[gnu::vector_size(16)]] = T;

  VT &V1 = Val1.get<VT>();
  const VT &V2 = Val2.get<VT>();

  V1 = (V1 != V2);
  return {};
}

template <typename T>
Expect<void> Executor::runVectorLtOp(ValVariant &Val1,
                                     const ValVariant &Val2) const {
  using VT [[gnu::vector_size(16)]] = T;

  VT &V1 = Val1.get<VT>();
  const VT &V2 = Val2.get<VT>();

  V1 = (V1 < V2);
  return {};
}

template <typename T>
Expect<void> Executor::runVectorGtOp(ValVariant &Val1,
                                     const ValVariant &Val2) const {
  using VT [[gnu::vector_size(16)]] = T;

  VT &V1 = Val1.get<VT>();
  const VT &V2 = Val2.get<VT>();

  V1 = (V1 > V2);
  return {};
}

template <typename T>
Expect<void> Executor::runVectorLeOp(ValVariant &Val1,
                                     const ValVariant &Val2) const {
  using VT [[gnu::vector_size(16)]] = T;

  VT &V1 = Val1.get<VT>();
  const VT &V2 = Val2.get<VT>();

  V1 = (V1 <= V2);
  return {};
}

template <typename T>
Expect<void> Executor::runVectorGeOp(ValVariant &Val1,
                                     const ValVariant &Val2) const {
  using VT [[gnu::vector_size(16)]] = T;

  VT &V1 = Val1.get<VT>();
  const VT &V2 = Val2.get<VT>();

  V1 = (V1 >= V2);
  return {};
}

template <typename TIn, typename TOut>
Expect<void> Executor::runVectorNarrowOp(ValVariant &Val1,
                                         const ValVariant &Val2) const {
  static_assert(sizeof(TOut) * 2 == sizeof(TIn));
  static_assert(sizeof(TOut) == 1 || sizeof(TOut) == 2);
  using VTIn [[gnu::vector_size(16)]] = TIn;
  using HVTOut [[gnu::vector_size(8)]] = TOut;
  using VTOut [[gnu::vector_size(16)]] = TOut;

  const VTIn Min = VTIn{} + static_cast<TIn>(std::numeric_limits<TOut>::min());
  const VTIn Max = VTIn{} + static_cast<TIn>(std::numeric_limits<TOut>::max());
  VTIn V1 = Val1.get<VTIn>();
  VTIn V2 = Val2.get<VTIn>();
  V1 = detail::vectorSelect(V1 < Min, Min, V1);
  V1 = detail::vectorSelect(V1 > Max, Max, V1);
  V2 = detail::vectorSelect(V2 < Min, Min, V2);
  V2 = detail::vectorSelect(V2 > Max, Max, V2);
  const HVTOut HV1 = __builtin_convertvector(V1, HVTOut);
  const HVTOut HV2 = __builtin_convertvector(V2, HVTOut);
  if constexpr (sizeof(TOut) == 1) {
    Val1.emplace<VTOut>(VTOut{HV1[0], HV1[1], HV1[2], HV1[3], HV1[4], HV1[5],
                              HV1[6], HV1[7], HV2[0], HV2[1], HV2[2], HV2[3],
                              HV2[4], HV2[5], HV2[6], HV2[7]});
  } else if constexpr (sizeof(TOut) == 2) {
    Val1.emplace<VTOut>(
        VTOut{HV1[0], HV1[1], HV1[2], HV1[3], HV2[0], HV2[1], HV2[2], HV2[3]});
  }

  return {};
}

template <typename T>
Expect<void> Executor::runVectorShlOp(ValVariant &Val1,
                                      const ValVariant &Val2) const {
  using VT [[gnu::vector_size(16)]] = T;
  const uint32_t Mask = static_cast<uint32_t>(sizeof(T) * 8 - 1);
  VT &V1 = Val1.get<VT>();
  V1 <<= Val2.get<uint32_t>() & Mask;

  return {};
}

template <typename T>
Expect<void> Executor::runVectorShrOp(ValVariant &Val1,
                                      const ValVariant &Val2) const {
  using VT [[gnu::vector_size(16)]] = T;
  const uint32_t Mask = static_cast<uint32_t>(sizeof(T) * 8 - 1);
  VT &V1 = Val1.get<VT>();
  V1 >>= Val2.get<uint32_t>() & Mask;

  return {};
}

template <typename T>
Expect<void> Executor::runVectorAddOp(ValVariant &Val1,
                                      const ValVariant &Val2) const {
  using VT [[gnu::vector_size(16)]] = T;
  VT &V1 = Val1.get<VT>();
  V1 += Val2.get<VT>();

  return {};
}

template <typename T>
Expect<void> Executor::runVectorAddSatOp(ValVariant &Val1,
                                         const ValVariant &Val2) const {
  using VT [[gnu::vector_size(16)]] = T;
  using UVT [[gnu::vector_size(16)]] = std::make_unsigned_t<T>;
  UVT &V1 = Val1.get<UVT>();
  const UVT &V2 = Val2.get<UVT>();
  const UVT Result = V1 + V2;

  if constexpr (std::is_signed_v<T>) {
    const UVT Limit =
        (V1 >> (sizeof(T) * 8 - 1)) + std::numeric_limits<T>::max();
    const VT Over = reinterpret_cast<VT>((V1 ^ V2) | ~(V2 ^ Result));
    V1 = detail::vectorSelect(Over >= 0, Limit, Result);
  } else {
    V1 = Result | (Result < V1);
  }

  return {};
}

template <typename T>
Expect<void> Executor::runVectorSubOp(ValVariant &Val1,
                                      const ValVariant &Val2) const {
  using VT [[gnu::vector_size(16)]] = T;
  VT &V1 = Val1.get<VT>();
  V1 -= Val2.get<VT>();

  return {};
}

template <typename T>
Expect<void> Executor::runVectorSubSatOp(ValVariant &Val1,
                                         const ValVariant &Val2) const {
  using VT [[gnu::vector_size(16)]] = T;
  using UVT [[gnu::vector_size(16)]] = std::make_unsigned_t<T>;
  UVT &V1 = Val1.get<UVT>();
  const UVT &V2 = Val2.get<UVT>();
  const UVT Result = V1 - V2;

  if constexpr (std::is_signed_v<T>) {
    const UVT Limit =
        (V1 >> (sizeof(T) * 8 - 1)) + std::numeric_limits<T>::max();
    const VT Under = reinterpret_cast<VT>((V1 ^ V2) & (V1 ^ Result));
    V1 = detail::vectorSelect(Under < 0, Limit, Result);
  } else {
    V1 = Result & (Result <= V1);
  }

  return {};
}

template <typename T>
Expect<void> Executor::runVectorMulOp(ValVariant &Val1,
                                      const ValVariant &Val2) const {
  using VT [[gnu::vector_size(16)]] = T;
  VT &V1 = Val1.get<VT>();
  V1 *= Val2.get<VT>();

  return {};
}

template <typename T>
Expect<void> Executor::runVectorDivOp(ValVariant &Val1,
                                      const ValVariant &Val2) const {
  using VT [[gnu::vector_size(16)]] = T;
  VT &V1 = Val1.get<VT>();
  V1 /= Val2.get<VT>();

  return {};
}

template <typename T>
Expect<void> Executor::runVectorMinOp(ValVariant &Val1,
                                      const ValVariant &Val2) const {
  using VT [[gnu::vector_size(16)]] = T;
  VT &V1 = Val1.get<VT>();
  const VT &V2 = Val2.get<VT>();
  V1 = detail::vectorSelect(V1 > V2, V2, V1);

  return {};
}

template <typename T>
Expect<void> Executor::runVectorMaxOp(ValVariant &Val1,
                                      const ValVariant &Val2) const {
  using VT [[gnu::vector_size(16)]] = T;
  VT &V1 = Val1.get<VT>();
  const VT &V2 = Val2.get<VT>();
  V1 = detail::vectorSelect(V2 > V1, V2, V1);

  return {};
}

template <typename T>
Expect<void> Executor::runVectorFMinOp(ValVariant &Val1,
                                       const ValVariant &Val2) const {
  static_assert(std::is_floating_point_v<T>);
  using VT [[gnu::vector_size(16)]] = T;
  VT &V1 = Val1.get<VT>();
  const VT &V2 = Val2.get<VT>();
  VT R = reinterpret_cast<VT>(reinterpret_cast<uint64x2_t>(V1) |
                              reinterpret_cast<uint64x2_t>(V2));
  R = detail::vectorSelect(V1 < V2, V1, R);
  R = detail::vectorSelect(V1 > V2, V2, R);
  R = detail::vectorSelect(V1 == V1, R, V1);
  R = detail::vectorSelect(V2 == V2, R, V2);
  V1 = R;

  return {};
}

template <typename T>
Expect<void> Executor::runVectorFMaxOp(ValVariant &Val1,
                                       const ValVariant &Val2) const {
  using VT [[gnu::vector_size(16)]] = T;
  VT &V1 = Val1.get<VT>();
  const VT &V2 = Val2.get<VT>();
  VT R = reinterpret_cast<VT>(reinterpret_cast<uint64x2_t>(V1) &
                              reinterpret_cast<uint64x2_t>(V2));
  R = detail::vectorSelect(V1 < V2, V2, R);
  R = detail::vectorSelect(V1 > V2, V1, R);
  R = detail::vectorSelect(V1 == V1, R, V1);
  R = detail::vectorSelect(V2 == V2, R, V2);
  V1 = R;

  return {};
}

template <typename T, typename ET>
Expect<void> Executor::runVectorAvgrOp(ValVariant &Val1,
                                       const ValVariant &Val2) const {
  static_assert(sizeof(T) * 2 == sizeof(ET));
  using VT [[gnu::vector_size(16)]] = T;
  using EVT [[gnu::vector_size(32)]] = ET;
  VT &V1 = Val1.get<VT>();
  const VT &V2 = Val2.get<VT>();
  const EVT EV1 = __builtin_convertvector(V1, EVT);
  const EVT EV2 = __builtin_convertvector(V2, EVT);
  // Add 1 for rounding up .5
  V1 = __builtin_convertvector((EV1 + EV2 + 1) / 2, VT);

  return {};
}

template <typename TIn, typename TOut>
Expect<void> Executor::runVectorExtMulLowOp(ValVariant &Val1,
                                            const ValVariant &Val2) const {
  static_assert(sizeof(TIn) * 2 == sizeof(TOut));
  static_assert(sizeof(TIn) == 1 || sizeof(TIn) == 2 || sizeof(TIn) == 4);
  using VTIn [[gnu::vector_size(16)]] = TIn;
  using HVTIn [[gnu::vector_size(8)]] = TIn;
  using VTOut [[gnu::vector_size(16)]] = TOut;
  const VTIn &V1 = Val1.get<VTIn>();
  const VTIn &V2 = Val2.get<VTIn>();
  if constexpr (sizeof(TIn) == 1) {
    const VTOut E1 = __builtin_convertvector(
        HVTIn{V1[0], V1[1], V1[2], V1[3], V1[4], V1[5], V1[6], V1[7]}, VTOut);
    const VTOut E2 = __builtin_convertvector(
        HVTIn{V2[0], V2[1], V2[2], V2[3], V2[4], V2[5], V2[6], V2[7]}, VTOut);
    Val1.emplace<VTOut>(E1 * E2);
  } else if constexpr (sizeof(TIn) == 2) {
    const VTOut E1 =
        __builtin_convertvector(HVTIn{V1[0], V1[1], V1[2], V1[3]}, VTOut);
    const VTOut E2 =
        __builtin_convertvector(HVTIn{V2[0], V2[1], V2[2], V2[3]}, VTOut);
    Val1.emplace<VTOut>(E1 * E2);
  } else if constexpr (sizeof(TIn) == 4) {
    const VTOut E1 = __builtin_convertvector(HVTIn{V1[0], V1[1]}, VTOut);
    const VTOut E2 = __builtin_convertvector(HVTIn{V2[0], V2[1]}, VTOut);
    Val1.emplace<VTOut>(E1 * E2);
  }
  return {};
}

template <typename TIn, typename TOut>
Expect<void> Executor::runVectorExtMulHighOp(ValVariant &Val1,
                                             const ValVariant &Val2) const {
  static_assert(sizeof(TIn) * 2 == sizeof(TOut));
  static_assert(sizeof(TIn) == 1 || sizeof(TIn) == 2 || sizeof(TIn) == 4);
  using VTIn [[gnu::vector_size(16)]] = TIn;
  using HVTIn [[gnu::vector_size(8)]] = TIn;
  using VTOut [[gnu::vector_size(16)]] = TOut;
  const VTIn &V1 = Val1.get<VTIn>();
  const VTIn &V2 = Val2.get<VTIn>();
  if constexpr (sizeof(TIn) == 1) {
    const VTOut E1 = __builtin_convertvector(
        HVTIn{V1[8], V1[9], V1[10], V1[11], V1[12], V1[13], V1[14], V1[15]},
        VTOut);
    const VTOut E2 = __builtin_convertvector(
        HVTIn{V2[8], V2[9], V2[10], V2[11], V2[12], V2[13], V2[14], V2[15]},
        VTOut);
    Val1.emplace<VTOut>(E1 * E2);
  } else if constexpr (sizeof(TIn) == 2) {
    const VTOut E1 =
        __builtin_convertvector(HVTIn{V1[4], V1[5], V1[6], V1[7]}, VTOut);
    const VTOut E2 =
        __builtin_convertvector(HVTIn{V2[4], V2[5], V2[6], V2[7]}, VTOut);
    Val1.emplace<VTOut>(E1 * E2);
  } else if constexpr (sizeof(TIn) == 4) {
    const VTOut E1 = __builtin_convertvector(HVTIn{V1[2], V1[3]}, VTOut);
    const VTOut E2 = __builtin_convertvector(HVTIn{V2[2], V2[3]}, VTOut);
    Val1.emplace<VTOut>(E1 * E2);
  }
  return {};
}

inline Expect<void>
Executor::runVectorQ15MulSatOp(ValVariant &Val1, const ValVariant &Val2) const {
  using int32x8_t [[gnu::vector_size(32)]] = int32_t;
  const auto &V1 = Val1.get<int16x8_t>();
  const auto &V2 = Val2.get<int16x8_t>();
  const auto EV1 = __builtin_convertvector(V1, int32x8_t);
  const auto EV2 = __builtin_convertvector(V2, int32x8_t);
  const auto ER = (EV1 * EV2 + INT32_C(0x4000)) >> INT32_C(15);
  const int32x8_t Cap = int32x8_t{} + INT32_C(0x7fff);
  const auto ERSat = detail::vectorSelect(ER > Cap, Cap, ER);
  Val1.emplace<int16x8_t>(__builtin_convertvector(ERSat, int16x8_t));
  return {};
}

template <typename T>
Expect<void>
Executor::runVectorRelaxedLaneselectOp(ValVariant &Val1, const ValVariant &Val2,
                                       const ValVariant &Mask) const {
  using VT [[gnu::vector_size(16)]] = T;

  VT &V1 = Val1.get<VT>();
  const VT &V2 = Val2.get<VT>();
  const VT &C = Mask.get<VT>();

  V1 = (V1 & C) | (V2 & ~C);
  return {};
}

inline Expect<void>
Executor::runVectorRelaxedIntegerDotProductOp(ValVariant &Val1,
                                              const ValVariant &Val2) const {
  using int16x8_t [[gnu::vector_size(16)]] = int16_t;

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
  using int16x8_t [[gnu::vector_size(16)]] = int16_t;
  using int32x4_t [[gnu::vector_size(16)]] = int32_t;

  const int16x8_t &V1 = Val1.get<int16x8_t>();
  const int16x8_t &V2 = Val2.get<int16x8_t>();
  const int Size = 8;
  const int32x4_t &VC = C.get<int32x4_t>();

  const auto V1L = V1 >> Size;
  const auto V1R = (V1 << Size) >> Size;
  const auto V2L = V2 >> Size;
  const auto V2R = (V2 << Size) >> Size;

  union VecConverter {
    int16x8_t S16;
    int32x4_t S32;
  } IM;

  IM.S16 = V1L * V2L + V1R * V2R;
  const int IMSize = 16;
  auto IML = IM.S32 >> IMSize;
  auto IMR = (IM.S32 << IMSize) >> IMSize;

  Val1.emplace<int32x4_t>(IML + IMR + VC);
  return {};
}

} // namespace Executor
} // namespace WasmEdge
