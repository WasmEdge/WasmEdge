// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "common/roundeven.h"
#include "executor/executor.h"

#include <cmath>

namespace WasmEdge {
namespace Executor {

template <typename T>
TypeU<T> Executor::runClzOp(Runtime::StackManager &StackMgr) const noexcept {
  T I = StackMgr.pop<T>();
  // Return the count of leading zero bits in i.
  if (I != 0U) {
    T Cnt = 0;
    T Mask = static_cast<T>(0x1U) << (sizeof(T) * 8 - 1);
    while ((I & Mask) == 0U) {
      Cnt++;
      I <<= 1;
    }
    StackMgr.push<T>(Cnt);
  } else {
    StackMgr.push<T>(static_cast<T>(sizeof(T) * 8));
  }
  return {};
}

template <typename T>
TypeU<T> Executor::runCtzOp(Runtime::StackManager &StackMgr) const noexcept {
  T I = StackMgr.pop<T>();
  // Return the count of trailing zero bits in i.
  if (I != 0U) {
    T Cnt = 0;
    T Mask = static_cast<T>(0x1U);
    while ((I & Mask) == 0U) {
      Cnt++;
      I >>= 1;
    }
    StackMgr.push<T>(Cnt);
  } else {
    StackMgr.push<T>(static_cast<T>(sizeof(T) * 8));
  }
  return {};
}

template <typename T>
TypeU<T> Executor::runPopcntOp(Runtime::StackManager &StackMgr) const noexcept {
  T I = StackMgr.pop<T>();
  // Return the count of non-zero bits in i.
  if (I != 0U) {
    T Cnt = 0;
    T Mask = static_cast<T>(0x1U);
    while (I != 0U) {
      if (I & Mask) {
        Cnt++;
      }
      I >>= 1;
    }
    I = Cnt;
  }
  StackMgr.push<T>(I);
  return {};
}

template <typename T>
TypeF<T> Executor::runAbsOp(Runtime::StackManager &StackMgr) const noexcept {
  StackMgr.push<T>(std::fabs(StackMgr.pop<T>()));
  return {};
}

template <typename T>
TypeF<T> Executor::runNegOp(Runtime::StackManager &StackMgr) const noexcept {
  StackMgr.push<T>(-StackMgr.pop<T>());
  return {};
}

template <typename T>
TypeF<T> Executor::runCeilOp(Runtime::StackManager &StackMgr) const noexcept {
  StackMgr.push<T>(std::ceil(StackMgr.pop<T>()));
  return {};
}

template <typename T>
TypeF<T> Executor::runFloorOp(Runtime::StackManager &StackMgr) const noexcept {
  StackMgr.push<T>(std::floor(StackMgr.pop<T>()));
  return {};
}

template <typename T>
TypeF<T> Executor::runTruncOp(Runtime::StackManager &StackMgr) const noexcept {
  StackMgr.push<T>(std::trunc(StackMgr.pop<T>()));
  return {};
}

template <typename T>
TypeF<T>
Executor::runNearestOp(Runtime::StackManager &StackMgr) const noexcept {
  StackMgr.push<T>(WasmEdge::roundeven(StackMgr.pop<T>()));
  return {};
}

template <typename T>
TypeF<T> Executor::runSqrtOp(Runtime::StackManager &StackMgr) const noexcept {
  StackMgr.push<T>(std::sqrt(StackMgr.pop<T>()));
  return {};
}

template <typename TIn, typename TOut>
Expect<void> Executor::runExtractLaneOp(Runtime::StackManager &StackMgr,
                                        const uint8_t Index) const noexcept {
  using VTIn [[gnu::vector_size(16)]] = TIn;
  StackMgr.push<TOut>(StackMgr.pop<VTIn>()[Index]);
  return {};
}

template <typename TIn, typename TOut>
Expect<void>
Executor::runSplatOp(Runtime::StackManager &StackMgr) const noexcept {
  const TOut Part = static_cast<TOut>(StackMgr.pop<TIn>());
  using VTOut [[gnu::vector_size(16)]] = TOut;
  if constexpr (sizeof(TOut) == 8) {
    StackMgr.push<VTOut>(VTOut{Part, Part});
  } else if constexpr (sizeof(TOut) == 4) {
    StackMgr.push<VTOut>(VTOut{Part, Part, Part, Part});
  } else {
    StackMgr.push<VTOut>(VTOut{} + Part);
  }
  return {};
}

template <typename TIn, typename TOut>
Expect<void>
Executor::runVectorExtendLowOp(Runtime::StackManager &StackMgr) const noexcept {
  static_assert(sizeof(TIn) * 2 == sizeof(TOut));
  static_assert(sizeof(TIn) == 1 || sizeof(TIn) == 2 || sizeof(TIn) == 4);
  using VTIn [[gnu::vector_size(16)]] = TIn;
  using HVTIn [[gnu::vector_size(8)]] = TIn;
  using VTOut [[gnu::vector_size(16)]] = TOut;
  const VTIn V = StackMgr.pop<VTIn>();
  if constexpr (sizeof(TIn) == 1) {
    StackMgr.push<VTOut>(__builtin_convertvector(
        HVTIn{V[0], V[1], V[2], V[3], V[4], V[5], V[6], V[7]}, VTOut));
  } else if constexpr (sizeof(TIn) == 2) {
    StackMgr.push<VTOut>(
        __builtin_convertvector(HVTIn{V[0], V[1], V[2], V[3]}, VTOut));
  } else if constexpr (sizeof(TIn) == 4) {
    StackMgr.push<VTOut>(__builtin_convertvector(HVTIn{V[0], V[1]}, VTOut));
  }
  return {};
}

template <typename TIn, typename TOut>
Expect<void> Executor::runVectorExtendHighOp(
    Runtime::StackManager &StackMgr) const noexcept {
  static_assert(sizeof(TIn) * 2 == sizeof(TOut));
  static_assert(sizeof(TIn) == 1 || sizeof(TIn) == 2 || sizeof(TIn) == 4);
  using VTIn [[gnu::vector_size(16)]] = TIn;
  using HVTIn [[gnu::vector_size(8)]] = TIn;
  using VTOut [[gnu::vector_size(16)]] = TOut;
  const VTIn &V = StackMgr.pop<VTIn>();
  if constexpr (sizeof(TIn) == 1) {
    StackMgr.push<VTOut>(__builtin_convertvector(
        HVTIn{V[8], V[9], V[10], V[11], V[12], V[13], V[14], V[15]}, VTOut));
  } else if constexpr (sizeof(TIn) == 2) {
    StackMgr.push<VTOut>(
        __builtin_convertvector(HVTIn{V[4], V[5], V[6], V[7]}, VTOut));
  } else if constexpr (sizeof(TIn) == 4) {
    StackMgr.push<VTOut>(__builtin_convertvector(HVTIn{V[2], V[3]}, VTOut));
  }
  return {};
}

template <typename TIn, typename TOut>
Expect<void> Executor::runVectorExtAddPairwiseOp(
    Runtime::StackManager &StackMgr) const noexcept {
  static_assert(sizeof(TIn) * 2 == sizeof(TOut));
  using VTOut [[gnu::vector_size(16)]] = TOut;
  const auto Size = sizeof(TIn) * 8;
  const VTOut V = StackMgr.pop<VTOut>();
  const auto L = V >> Size;
  const auto R = (V << Size) >> Size;
  StackMgr.push<VTOut>(L + R);

  return {};
}

template <typename T>
Expect<void>
Executor::runVectorAbsOp(Runtime::StackManager &StackMgr) const noexcept {
  using VT [[gnu::vector_size(16)]] = T;
  if constexpr (std::is_floating_point_v<T>) {
    if constexpr (sizeof(T) == 4) {
      using IVT [[gnu::vector_size(16)]] = uint32_t;
      IVT Mask = IVT{} + UINT32_C(0x7fffffff);
      StackMgr.push<IVT>(StackMgr.pop<IVT>() & Mask);
    } else {
      using IVT [[gnu::vector_size(16)]] = uint64_t;
      IVT Mask = IVT{} + UINT64_C(0x7fffffffffffffff);
      StackMgr.push<IVT>(StackMgr.pop<IVT>() & Mask);
    }
  } else {
    const VT Result = StackMgr.pop<VT>();
    StackMgr.push<VT>(detail::vectorSelect(Result > 0, Result, -Result));
  }
  return {};
}

template <typename T>
Expect<void>
Executor::runVectorNegOp(Runtime::StackManager &StackMgr) const noexcept {
  using VT [[gnu::vector_size(16)]] = T;
  StackMgr.push<VT>(-StackMgr.pop<VT>());
  return {};
}

inline Expect<void>
Executor::runVectorPopcntOp(Runtime::StackManager &StackMgr) const noexcept {
  uint8x16_t Result = StackMgr.pop<uint8x16_t>();
#if 1
  Result = (Result & UINT8_C(0x55)) + ((Result >> UINT8_C(1)) & UINT8_C(0x55));
  Result = (Result & UINT8_C(0x33)) + ((Result >> UINT8_C(2)) & UINT8_C(0x33));
  Result = (Result & UINT8_C(0x0f)) + ((Result >> UINT8_C(4)) & UINT8_C(0x0f));
#else
  Result -= ((Result >> UINT8_C(1)) & UINT8_C(0x55));
  Result = (Result & UINT8_C(0x33)) + ((Result >> UINT8_C(2)) & UINT8_C(0x33));
  Result += Result >> UINT8_C(4);
  Result &= UINT8_C(0x0f);
#endif
  StackMgr.push<uint8x16_t>(Result);
  return {};
}

template <typename T>
Expect<void>
Executor::runVectorSqrtOp(Runtime::StackManager &StackMgr) const noexcept {
  using VT [[gnu::vector_size(16)]] = T;
  VT Result = StackMgr.pop<VT>();
  if constexpr (sizeof(T) == 4) {
    Result = VT{std::sqrt(Result[0]), std::sqrt(Result[1]),
                std::sqrt(Result[2]), std::sqrt(Result[3])};
  } else if constexpr (sizeof(T) == 8) {
    Result = VT{std::sqrt(Result[0]), std::sqrt(Result[1])};
  }
  StackMgr.push<VT>(Result);
  return {};
}

template <typename TIn, typename TOut>
Expect<void>
Executor::runVectorTruncSatOp(Runtime::StackManager &StackMgr) const noexcept {
  static_assert((sizeof(TIn) == 4 || sizeof(TIn) == 8) && sizeof(TOut) == 4);
  using VTIn [[gnu::vector_size(16)]] = TIn;
  using VTOut [[gnu::vector_size(16)]] = TOut;
  const VTIn FMin = VTIn{} + static_cast<TIn>(std::numeric_limits<TOut>::min());
  const VTIn FMax = VTIn{} + static_cast<TIn>(std::numeric_limits<TOut>::max());
  const VTIn V = StackMgr.pop<VTIn>();
  if constexpr (sizeof(TIn) == sizeof(TOut)) {
    const VTOut IMin = VTOut{} + std::numeric_limits<TOut>::min();
    const VTOut IMax = VTOut{} + std::numeric_limits<TOut>::max();
    VTIn X = {std::trunc(V[0]), std::trunc(V[1]), std::trunc(V[2]),
              std::trunc(V[3])};
    VTOut Y = __builtin_convertvector(X, VTOut);
    Y = detail::vectorSelect(X == X, Y, VTOut{});
    Y = detail::vectorSelect(X <= FMin, IMin, Y);
    Y = detail::vectorSelect(X >= FMax, IMax, Y);
    StackMgr.push<VTOut>(Y);
  } else {
    using TOut2 = std::conditional_t<std::is_signed_v<TOut>, int64_t, uint64_t>;
    using VTOut2 [[gnu::vector_size(16)]] = TOut2;
    const VTOut2 IMin = VTOut2{} + std::numeric_limits<TOut>::min();
    const VTOut2 IMax = VTOut2{} + std::numeric_limits<TOut>::max();
    VTIn X = {std::trunc(V[0]), std::trunc(V[1])};
    VTOut2 Y = __builtin_convertvector(X, VTOut2);
    Y = detail::vectorSelect(X <= FMin, IMin, Y);
    Y = detail::vectorSelect(X >= FMax, IMax, Y);
    using VTOut22 [[gnu::vector_size(32)]] = TOut2;
    VTOut22 T = {Y[0], Y[1], 0, 0};
    StackMgr.push<VTOut>(__builtin_convertvector(T, VTOut));
  }
  return {};
}

template <typename TIn, typename TOut>
Expect<void>
Executor::runVectorConvertOp(Runtime::StackManager &StackMgr) const noexcept {
  static_assert((sizeof(TIn) == 4 && (sizeof(TOut) == 4 || sizeof(TOut) == 8)));
  using VTIn [[gnu::vector_size(16)]] = TIn;
  using VTOut [[gnu::vector_size(16)]] = TOut;
  const auto V = StackMgr.pop<VTIn>();
  if constexpr (sizeof(TIn) == sizeof(TOut)) {
    StackMgr.push<VTOut>(__builtin_convertvector(V, VTOut));
  } else {
    using VTIn2 [[gnu::vector_size(8)]] = TIn;
    VTIn2 X = {V[0], V[1]};
    StackMgr.push<VTOut>(__builtin_convertvector(X, VTOut));
  }
  return {};
}

inline Expect<void>
Executor::runVectorDemoteOp(Runtime::StackManager &StackMgr) const noexcept {
  const auto V = StackMgr.pop<doublex2_t>();
  StackMgr.push<floatx4_t>(
      floatx4_t{static_cast<float>(V[0]), static_cast<float>(V[1]), 0, 0});
  return {};
}

inline Expect<void>
Executor::runVectorPromoteOp(Runtime::StackManager &StackMgr) const noexcept {
  const auto V = StackMgr.pop<floatx4_t>();
  StackMgr.push<doublex2_t>(
      doublex2_t{static_cast<double>(V[0]), static_cast<double>(V[1])});
  return {};
}

inline Expect<void>
Executor::runVectorAnyTrueOp(Runtime::StackManager &StackMgr) const noexcept {
  const auto Vector = StackMgr.pop<uint128_t>();
  const uint128_t Zero = 0;
  const uint32_t Result = Vector != Zero ? 1U : 0U;
  StackMgr.push<uint32_t>(Result);

  return {};
}

template <typename T>
Expect<void>
Executor::runVectorAllTrueOp(Runtime::StackManager &StackMgr) const noexcept {
  using VT [[gnu::vector_size(16)]] = T;
  const VT Vector = StackMgr.pop<VT>();
  const VT Z = Vector != 0;
  uint32_t Result;
  if constexpr (sizeof(T) == 1) {
    Result = Z[0] && Z[1] && Z[2] && Z[3] && Z[4] && Z[5] && Z[6] && Z[7] &&
             Z[8] && Z[9] && Z[10] && Z[11] && Z[12] && Z[13] && Z[14] && Z[15];
  } else if constexpr (sizeof(T) == 2) {
    Result = Z[0] && Z[1] && Z[2] && Z[3] && Z[4] && Z[5] && Z[6] && Z[7];
  } else if constexpr (sizeof(T) == 4) {
    Result = Z[0] && Z[1] && Z[2] && Z[3];
  } else if constexpr (sizeof(T) == 8) {
    Result = Z[0] && Z[1];
  }
  StackMgr.push<uint32_t>(Result);

  return {};
}

template <typename T>
Expect<void>
Executor::runVectorBitMaskOp(Runtime::StackManager &StackMgr) const noexcept {
  using SVT [[gnu::vector_size(16)]] = std::make_signed_t<T>;
  using UVT [[gnu::vector_size(16)]] = std::make_unsigned_t<T>;
  const SVT Vector = StackMgr.pop<SVT>();
  const SVT MSB = Vector < 0;
  const UVT Z [[maybe_unused]] = reinterpret_cast<UVT>(MSB);
  if constexpr (sizeof(T) == 1) {
    using int16x16_t [[gnu::vector_size(32)]] = int16_t;
    using uint16x16_t [[gnu::vector_size(32)]] = uint16_t;
    const uint16x16_t Mask = {0x1,    0x2,    0x4,    0x8,   0x10,  0x20,
                              0x40,   0x80,   0x100,  0x200, 0x400, 0x800,
                              0x1000, 0x2000, 0x4000, 0x8000};
    uint16x16_t V =
        reinterpret_cast<uint16x16_t>(__builtin_convertvector(MSB, int16x16_t));
    V &= Mask;
    const uint16_t Result = V[0] | V[1] | V[2] | V[3] | V[4] | V[5] | V[6] |
                            V[7] | V[8] | V[9] | V[10] | V[11] | V[12] | V[13] |
                            V[14] | V[15];
    StackMgr.push<uint32_t>(Result);
  } else if constexpr (sizeof(T) == 2) {
    const uint16x8_t Mask = {0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80};
    using uint8x8_t [[gnu::vector_size(8)]] = uint8_t;
    const uint8x8_t V = __builtin_convertvector(Z & Mask, uint8x8_t);
    const uint8_t Result =
        V[0] | V[1] | V[2] | V[3] | V[4] | V[5] | V[6] | V[7];
    StackMgr.push<uint32_t>(Result);
  } else if constexpr (sizeof(T) == 4) {
    const uint32x4_t Mask = {0x1, 0x2, 0x4, 0x8};
    using uint8x4_t [[gnu::vector_size(4)]] = uint8_t;
    const uint8x4_t V = __builtin_convertvector(Z & Mask, uint8x4_t);
    const uint8_t Result = V[0] | V[1] | V[2] | V[3];
    StackMgr.push<uint32_t>(Result);
  } else if constexpr (sizeof(T) == 8) {
    const uint64x2_t Mask = {0x1, 0x2};
    using uint8x2_t [[gnu::vector_size(2)]] = uint8_t;
    const uint8x2_t V = __builtin_convertvector(Z & Mask, uint8x2_t);
    const uint8_t Result = V[0] | V[1];
    StackMgr.push<uint32_t>(Result);
  }

  return {};
}

template <typename T>
Expect<void>
Executor::runVectorCeilOp(Runtime::StackManager &StackMgr) const noexcept {
  using VT [[gnu::vector_size(16)]] = T;
  using std::ceil;
  const VT V = StackMgr.pop<VT>();
  VT R;
  if constexpr (sizeof(T) == 4) {
    R = VT{ceil(V[0]), ceil(V[1]), ceil(V[2]), ceil(V[3])};
  } else if constexpr (sizeof(T) == 8) {
    R = VT{ceil(V[0]), ceil(V[1])};
  }
  StackMgr.push<VT>(R);
  return {};
}

template <typename T>
Expect<void>
Executor::runVectorFloorOp(Runtime::StackManager &StackMgr) const noexcept {
  using VT [[gnu::vector_size(16)]] = T;
  using std::floor;
  const VT V = StackMgr.pop<VT>();
  VT R;
  if constexpr (sizeof(T) == 4) {
    R = VT{floor(V[0]), floor(V[1]), floor(V[2]), floor(V[3])};
  } else if constexpr (sizeof(T) == 8) {
    R = VT{floor(V[0]), floor(V[1])};
  }
  StackMgr.push<VT>(R);
  return {};
}

template <typename T>
Expect<void>
Executor::runVectorTruncOp(Runtime::StackManager &StackMgr) const noexcept {
  using VT [[gnu::vector_size(16)]] = T;
  using std::trunc;
  const VT V = StackMgr.pop<VT>();
  VT R;
  if constexpr (sizeof(T) == 4) {
    R = VT{trunc(V[0]), trunc(V[1]), trunc(V[2]), trunc(V[3])};
  } else if constexpr (sizeof(T) == 8) {
    R = VT{trunc(V[0]), trunc(V[1])};
  }
  StackMgr.push<VT>(R);
  return {};
}

template <typename T>
Expect<void>
Executor::runVectorNearestOp(Runtime::StackManager &StackMgr) const noexcept {
  using VT [[gnu::vector_size(16)]] = T;
  using WasmEdge::roundeven;
  const VT V = StackMgr.pop<VT>();
  VT R;
  if constexpr (sizeof(T) == 4) {
    R = VT{roundeven(V[0]), roundeven(V[1]), roundeven(V[2]), roundeven(V[3])};
  } else if constexpr (sizeof(T) == 8) {
    R = VT{roundeven(V[0]), roundeven(V[1])};
  }
  StackMgr.push<VT>(R);
  return {};
}

} // namespace Executor
} // namespace WasmEdge
