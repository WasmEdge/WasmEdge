
// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "executor/engine/vector_helper.h"
#include "executor/executor.h"

namespace WasmEdge {
namespace Executor {

template <typename TIn, typename TOut>
Expect<void> Executor::runReplaceLaneOp(Runtime::StackManager &StackMgr,
                                        const uint8_t Index) const noexcept {
  using VTOut [[gnu::vector_size(16)]] = TOut;

  const auto [Val2, Val1] = StackMgr.popsPeekTop<TIn, VTOut>();
  VTOut Result = Val1;
  if constexpr (Endian::native == Endian::little) {
    Result[Index] = static_cast<TOut>(Val2);
  } else {
    Result[(16 / sizeof(TOut)) - 1 - Index] = static_cast<TOut>(Val2);
  }
  StackMgr.emplaceTop(std::move(Result));
  return {};
}

template <typename T>
Expect<void>
Executor::runVectorEqOp(Runtime::StackManager &StackMgr) const noexcept {
  using VT [[gnu::vector_size(16)]] = T;

  const auto [Val2, Val1] = StackMgr.popsPeekTop<VT, VT>();
  StackMgr.emplaceTop<VT>(Val1 == Val2);
  return {};
}

template <typename T>
Expect<void>
Executor::runVectorNeOp(Runtime::StackManager &StackMgr) const noexcept {
  using VT [[gnu::vector_size(16)]] = T;

  const auto [Val2, Val1] = StackMgr.popsPeekTop<VT, VT>();
  StackMgr.emplaceTop<VT>(Val1 != Val2);
  return {};
}

template <typename T>
Expect<void>
Executor::runVectorLtOp(Runtime::StackManager &StackMgr) const noexcept {
  using VT [[gnu::vector_size(16)]] = T;

  const auto [Val2, Val1] = StackMgr.popsPeekTop<VT, VT>();
  StackMgr.emplaceTop<VT>(Val1 < Val2);
  return {};
}

template <typename T>
Expect<void>
Executor::runVectorGtOp(Runtime::StackManager &StackMgr) const noexcept {
  using VT [[gnu::vector_size(16)]] = T;

  const auto [Val2, Val1] = StackMgr.popsPeekTop<VT, VT>();
  StackMgr.emplaceTop<VT>(Val1 > Val2);
  return {};
}

template <typename T>
Expect<void>
Executor::runVectorLeOp(Runtime::StackManager &StackMgr) const noexcept {
  using VT [[gnu::vector_size(16)]] = T;

  const auto [Val2, Val1] = StackMgr.popsPeekTop<VT, VT>();
  StackMgr.emplaceTop<VT>(Val1 <= Val2);
  return {};
}

template <typename T>
Expect<void>
Executor::runVectorGeOp(Runtime::StackManager &StackMgr) const noexcept {
  using VT [[gnu::vector_size(16)]] = T;

  const auto [Val2, Val1] = StackMgr.popsPeekTop<VT, VT>();
  StackMgr.emplaceTop<VT>(Val1 >= Val2);
  return {};
}

template <typename TIn, typename TOut>
Expect<void>
Executor::runVectorNarrowOp(Runtime::StackManager &StackMgr) const noexcept {
  static_assert(sizeof(TOut) * 2 == sizeof(TIn));
  static_assert(sizeof(TOut) == 1 || sizeof(TOut) == 2);
  using VTIn [[gnu::vector_size(16)]] = TIn;
  using HVTOut [[gnu::vector_size(8)]] = TOut;
  using VTOut [[gnu::vector_size(16)]] = TOut;

  const VTIn Min = VTIn{} + static_cast<TIn>(std::numeric_limits<TOut>::min());
  const VTIn Max = VTIn{} + static_cast<TIn>(std::numeric_limits<TOut>::max());
  auto [V2, V1] = StackMgr.popsPeekTop<VTIn, VTIn>();
  V1 = detail::vectorSelect(V1 < Min, Min, V1);
  V1 = detail::vectorSelect(V1 > Max, Max, V1);
  V2 = detail::vectorSelect(V2 < Min, Min, V2);
  V2 = detail::vectorSelect(V2 > Max, Max, V2);
  const HVTOut HV1 = __builtin_convertvector(V1, HVTOut);
  const HVTOut HV2 = __builtin_convertvector(V2, HVTOut);

  if constexpr (Endian::native == Endian::little) {
    if constexpr (sizeof(TOut) == 1) {
      StackMgr.emplaceTop<VTOut>(VTOut{
          HV1[0], HV1[1], HV1[2], HV1[3], HV1[4], HV1[5], HV1[6], HV1[7],
          HV2[0], HV2[1], HV2[2], HV2[3], HV2[4], HV2[5], HV2[6], HV2[7]});
    } else if constexpr (sizeof(TOut) == 2) {
      StackMgr.emplaceTop<VTOut>(VTOut{HV1[0], HV1[1], HV1[2], HV1[3], HV2[0],
                                       HV2[1], HV2[2], HV2[3]});
    }
  } else {
    if constexpr (sizeof(TOut) == 1) {
      StackMgr.emplaceTop<VTOut>(VTOut{
          HV2[0], HV2[1], HV2[2], HV2[3], HV2[4], HV2[5], HV2[6], HV2[7],
          HV1[0], HV1[1], HV1[2], HV1[3], HV1[4], HV1[5], HV1[6], HV1[7]});
    } else if constexpr (sizeof(TOut) == 2) {
      StackMgr.emplaceTop<VTOut>(VTOut{HV2[0], HV2[1], HV2[2], HV2[3], HV1[0],
                                       HV1[1], HV1[2], HV1[3]});
    }
  }

  return {};
}

template <typename T>
Expect<void>
Executor::runVectorShlOp(Runtime::StackManager &StackMgr) const noexcept {
  using VT [[gnu::vector_size(16)]] = T;

  const uint32_t Mask = static_cast<uint32_t>(sizeof(T) * 8 - 1);
  const auto [Val2, Val1] = StackMgr.popsPeekTop<uint32_t, VT>();
  StackMgr.emplaceTop(Val1 << (Val2 & Mask));

  return {};
}

template <typename T>
Expect<void>
Executor::runVectorShrOp(Runtime::StackManager &StackMgr) const noexcept {
  using VT [[gnu::vector_size(16)]] = T;

  const uint32_t Mask = static_cast<uint32_t>(sizeof(T) * 8 - 1);
  const auto [Val2, Val1] = StackMgr.popsPeekTop<uint32_t, VT>();
  StackMgr.emplaceTop(Val1 >> (Val2 & Mask));

  return {};
}

template <typename T>
Expect<void>
Executor::runVectorAddOp(Runtime::StackManager &StackMgr) const noexcept {
  using VT [[gnu::vector_size(16)]] = T;

  const auto [Val2, Val1] = StackMgr.popsPeekTop<VT, VT>();
  StackMgr.emplaceTop(Val1 + Val2);

  return {};
}

template <typename T>
Expect<void>
Executor::runVectorAddSatOp(Runtime::StackManager &StackMgr) const noexcept {
  using VT [[gnu::vector_size(16)]] = T;
  using UVT [[gnu::vector_size(16)]] = std::make_unsigned_t<T>;

  const auto [Val2, Val1] = StackMgr.popsPeekTop<UVT, UVT>();
  const UVT Result = Val1 + Val2;

  if constexpr (std::is_signed_v<T>) {
    const UVT Limit =
        (Val1 >> (sizeof(T) * 8 - 1)) + std::numeric_limits<T>::max();
    const VT Over = reinterpret_cast<VT>((Val1 ^ Val2) | ~(Val2 ^ Result));
    StackMgr.emplaceTop(detail::vectorSelect(Over >= 0, Limit, Result));
  } else {
    StackMgr.emplaceTop(Result | (Result < Val1));
  }

  return {};
}

template <typename T>
Expect<void>
Executor::runVectorSubOp(Runtime::StackManager &StackMgr) const noexcept {
  using VT [[gnu::vector_size(16)]] = T;

  const auto [Val2, Val1] = StackMgr.popsPeekTop<VT, VT>();
  StackMgr.emplaceTop(Val1 - Val2);

  return {};
}

template <typename T>
Expect<void>
Executor::runVectorSubSatOp(Runtime::StackManager &StackMgr) const noexcept {
  using VT [[gnu::vector_size(16)]] = T;
  using UVT [[gnu::vector_size(16)]] = std::make_unsigned_t<T>;

  const auto [Val2, Val1] = StackMgr.popsPeekTop<UVT, UVT>();
  const UVT Result = Val1 - Val2;

  if constexpr (std::is_signed_v<T>) {
    const UVT Limit =
        (Val1 >> (sizeof(T) * 8 - 1)) + std::numeric_limits<T>::max();
    const VT Under = reinterpret_cast<VT>((Val1 ^ Val2) & (Val1 ^ Result));
    StackMgr.emplaceTop(detail::vectorSelect(Under < 0, Limit, Result));
  } else {
    StackMgr.emplaceTop(Result & (Result <= Val1));
  }

  return {};
}

template <typename T>
Expect<void>
Executor::runVectorMulOp(Runtime::StackManager &StackMgr) const noexcept {
  using VT [[gnu::vector_size(16)]] = T;

  const auto [Val2, Val1] = StackMgr.popsPeekTop<VT, VT>();
  StackMgr.emplaceTop(Val1 * Val2);

  return {};
}

template <typename T>
Expect<void>
Executor::runVectorDivOp(Runtime::StackManager &StackMgr) const noexcept {
  using VT [[gnu::vector_size(16)]] = T;

  const auto [Val2, Val1] = StackMgr.popsPeekTop<VT, VT>();
  StackMgr.emplaceTop(Val1 / Val2);

  return {};
}

template <typename T>
Expect<void>
Executor::runVectorMAddOp(Runtime::StackManager &StackMgr) const noexcept {
  using VT [[gnu::vector_size(16)]] = T;

  const auto [Val3, Val2, Val1] = StackMgr.popsPeekTop<VT, VT, VT>();
  StackMgr.emplaceTop((Val1 * Val2) + Val3);

  return {};
}

template <typename T>
Expect<void>
Executor::runVectorNMAddOp(Runtime::StackManager &StackMgr) const noexcept {
  using VT [[gnu::vector_size(16)]] = T;

  const auto [Val3, Val2, Val1] = StackMgr.popsPeekTop<VT, VT, VT>();
  StackMgr.emplaceTop((-Val1 * Val2) + Val3);

  return {};
}

template <typename T>
Expect<void>
Executor::runVectorMinOp(Runtime::StackManager &StackMgr) const noexcept {
  using VT [[gnu::vector_size(16)]] = T;

  const auto [V2, V1] = StackMgr.popsPeekTop<VT, VT>();
  StackMgr.emplaceTop(detail::vectorSelect(V1 > V2, V2, V1));

  return {};
}

template <typename T>
Expect<void>
Executor::runVectorMaxOp(Runtime::StackManager &StackMgr) const noexcept {
  using VT [[gnu::vector_size(16)]] = T;

  const auto [V2, V1] = StackMgr.popsPeekTop<VT, VT>();
  StackMgr.emplaceTop(detail::vectorSelect(V2 > V1, V2, V1));

  return {};
}

template <typename T>
Expect<void>
Executor::runVectorFMinOp(Runtime::StackManager &StackMgr) const noexcept {
  static_assert(std::is_floating_point_v<T>);
  using VT [[gnu::vector_size(16)]] = T;

  const auto [V2, V1] = StackMgr.popsPeekTop<VT, VT>();
  VT R = reinterpret_cast<VT>(reinterpret_cast<uint64x2_t>(V1) |
                              reinterpret_cast<uint64x2_t>(V2));
  R = detail::vectorSelect(V1 < V2, V1, R);
  R = detail::vectorSelect(V1 > V2, V2, R);
  R = detail::vectorSelect(V1 == V1, R, V1);
  R = detail::vectorSelect(V2 == V2, R, V2);
  StackMgr.emplaceTop(R);

  return {};
}

template <typename T>
Expect<void>
Executor::runVectorFMaxOp(Runtime::StackManager &StackMgr) const noexcept {
  using VT [[gnu::vector_size(16)]] = T;

  const auto [V2, V1] = StackMgr.popsPeekTop<VT, VT>();
  VT R = reinterpret_cast<VT>(reinterpret_cast<uint64x2_t>(V1) &
                              reinterpret_cast<uint64x2_t>(V2));
  R = detail::vectorSelect(V1 < V2, V2, R);
  R = detail::vectorSelect(V1 > V2, V1, R);
  R = detail::vectorSelect(V1 == V1, R, V1);
  R = detail::vectorSelect(V2 == V2, R, V2);
  StackMgr.emplaceTop(R);

  return {};
}

template <typename T, typename ET>
Expect<void>
Executor::runVectorAvgrOp(Runtime::StackManager &StackMgr) const noexcept {
  static_assert(sizeof(T) * 2 == sizeof(ET));
  using VT [[gnu::vector_size(16)]] = T;
  using EVT [[gnu::vector_size(32)]] = ET;

  const auto [V2, V1] = StackMgr.popsPeekTop<VT, VT>();
  const EVT EV1 = __builtin_convertvector(V1, EVT);
  const EVT EV2 = __builtin_convertvector(V2, EVT);
  // Add 1 for rounding up .5
  StackMgr.emplaceTop(__builtin_convertvector((EV1 + EV2 + 1) / 2, VT));

  return {};
}

template <typename TIn, typename TOut>
Expect<void>
Executor::runVectorExtMulLowOp(Runtime::StackManager &StackMgr) const noexcept {
  static_assert(sizeof(TIn) * 2 == sizeof(TOut));
  static_assert(sizeof(TIn) == 1 || sizeof(TIn) == 2 || sizeof(TIn) == 4);
  using VTIn [[gnu::vector_size(16)]] = TIn;
  using HVTIn [[gnu::vector_size(8)]] = TIn;
  using VTOut [[gnu::vector_size(16)]] = TOut;

  const auto [V2, V1] = StackMgr.popsPeekTop<VTIn, VTIn>();
  if constexpr (sizeof(TIn) == 1) {
    const VTOut E1 = __builtin_convertvector(
        HVTIn{V1[0], V1[1], V1[2], V1[3], V1[4], V1[5], V1[6], V1[7]}, VTOut);
    const VTOut E2 = __builtin_convertvector(
        HVTIn{V2[0], V2[1], V2[2], V2[3], V2[4], V2[5], V2[6], V2[7]}, VTOut);
    StackMgr.emplaceTop<VTOut>(E1 * E2);
  } else if constexpr (sizeof(TIn) == 2) {
    const VTOut E1 =
        __builtin_convertvector(HVTIn{V1[0], V1[1], V1[2], V1[3]}, VTOut);
    const VTOut E2 =
        __builtin_convertvector(HVTIn{V2[0], V2[1], V2[2], V2[3]}, VTOut);
    StackMgr.emplaceTop<VTOut>(E1 * E2);
  } else if constexpr (sizeof(TIn) == 4) {
    const VTOut E1 = __builtin_convertvector(HVTIn{V1[0], V1[1]}, VTOut);
    const VTOut E2 = __builtin_convertvector(HVTIn{V2[0], V2[1]}, VTOut);
    StackMgr.emplaceTop<VTOut>(E1 * E2);
  }
  return {};
}

template <typename TIn, typename TOut>
Expect<void> Executor::runVectorExtMulHighOp(
    Runtime::StackManager &StackMgr) const noexcept {
  static_assert(sizeof(TIn) * 2 == sizeof(TOut));
  static_assert(sizeof(TIn) == 1 || sizeof(TIn) == 2 || sizeof(TIn) == 4);
  using VTIn [[gnu::vector_size(16)]] = TIn;
  using HVTIn [[gnu::vector_size(8)]] = TIn;
  using VTOut [[gnu::vector_size(16)]] = TOut;

  const auto [V2, V1] = StackMgr.popsPeekTop<VTIn, VTIn>();
  if constexpr (sizeof(TIn) == 1) {
    const VTOut E1 = __builtin_convertvector(
        HVTIn{V1[8], V1[9], V1[10], V1[11], V1[12], V1[13], V1[14], V1[15]},
        VTOut);
    const VTOut E2 = __builtin_convertvector(
        HVTIn{V2[8], V2[9], V2[10], V2[11], V2[12], V2[13], V2[14], V2[15]},
        VTOut);
    StackMgr.emplaceTop<VTOut>(E1 * E2);
  } else if constexpr (sizeof(TIn) == 2) {
    const VTOut E1 =
        __builtin_convertvector(HVTIn{V1[4], V1[5], V1[6], V1[7]}, VTOut);
    const VTOut E2 =
        __builtin_convertvector(HVTIn{V2[4], V2[5], V2[6], V2[7]}, VTOut);
    StackMgr.emplaceTop<VTOut>(E1 * E2);
  } else if constexpr (sizeof(TIn) == 4) {
    const VTOut E1 = __builtin_convertvector(HVTIn{V1[2], V1[3]}, VTOut);
    const VTOut E2 = __builtin_convertvector(HVTIn{V2[2], V2[3]}, VTOut);
    StackMgr.emplaceTop<VTOut>(E1 * E2);
  }
  return {};
}

inline Expect<void>
Executor::runVectorQ15MulSatOp(Runtime::StackManager &StackMgr) const noexcept {
  using int32x8_t [[gnu::vector_size(32)]] = int32_t;
  const auto [V2, V1] = StackMgr.popsPeekTop<int16x8_t, int16x8_t>();
  const auto EV1 = __builtin_convertvector(V1, int32x8_t);
  const auto EV2 = __builtin_convertvector(V2, int32x8_t);
  const auto ER = (EV1 * EV2 + INT32_C(0x4000)) >> INT32_C(15);
  const int32x8_t Cap = int32x8_t{} + INT32_C(0x7fff);
  const auto ERSat = detail::vectorSelect(ER > Cap, Cap, ER);
  StackMgr.emplaceTop<int16x8_t>(__builtin_convertvector(ERSat, int16x8_t));
  return {};
}

template <typename T>
Expect<void> Executor::runVectorRelaxedLaneselectOp(
    Runtime::StackManager &StackMgr) const noexcept {
  using VT [[gnu::vector_size(16)]] = T;

  auto [C, V2, V1] = StackMgr.popsPeekTop<VT, VT, VT>();
  StackMgr.emplaceTop((V1 & C) | (V2 & ~C));
  return {};
}

inline Expect<void> Executor::runVectorRelaxedIntegerDotProductOp(
    Runtime::StackManager &StackMgr) const noexcept {
  using int16x8_t [[gnu::vector_size(16)]] = int16_t;

  const auto [V2, V1] = StackMgr.popsPeekTop<int16x8_t, int16x8_t>();
  const int Size = 8;

  const auto V1L = V1 >> Size;
  const auto V1R = (V1 << Size) >> Size;
  const auto V2L = V2 >> Size;
  const auto V2R = (V2 << Size) >> Size;

  StackMgr.emplaceTop<int16x8_t>(V1L * V2L + V1R * V2R);
  return {};
}

inline Expect<void> Executor::runVectorRelaxedIntegerDotProductOpAdd(
    Runtime::StackManager &StackMgr) const noexcept {
  using int16x8_t [[gnu::vector_size(16)]] = int16_t;
  using int32x4_t [[gnu::vector_size(16)]] = int32_t;

  const auto [VC, V2, V1] =
      StackMgr.popsPeekTop<int32x4_t, int16x8_t, int16x8_t>();
  const int Size = 8;

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

  StackMgr.emplaceTop<int32x4_t>(IML + IMR + VC);
  return {};
}

} // namespace Executor
} // namespace WasmEdge
