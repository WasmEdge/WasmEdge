// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "common/endian.h"
#include "common/roundeven.h"
#include "executor/executor.h"

namespace WasmEdge {
namespace Executor {

template <typename TIn, typename TOut>
Expect<void> Executor::runExtractLaneOp(Runtime::StackManager &StackMgr,
                                        const uint8_t Index) const noexcept {
  using VTIn [[gnu::vector_size(16)]] = TIn;
  const uint8_t Lane =
      Endian::native == Endian::little ? Index : 16 / sizeof(TIn) - 1 - Index;
  TOut Result = StackMgr.peekTop<VTIn>()[Lane];
  StackMgr.emplaceTop(std::move(Result));
  return {};
}

template <typename TIn, typename TOut>
Expect<void>
Executor::runSplatOp(Runtime::StackManager &StackMgr) const noexcept {
  const TOut Part = static_cast<TOut>(StackMgr.peekTop<TIn>());
  using VTOut [[gnu::vector_size(16)]] = TOut;
  if constexpr (!std::is_floating_point_v<TOut>) {
    StackMgr.emplaceTop(VTOut{} + Part);
  } else if constexpr (sizeof(TOut) == 4) {
    StackMgr.emplaceTop(VTOut{Part, Part, Part, Part});
  } else if constexpr (sizeof(TOut) == 8) {
    StackMgr.emplaceTop(VTOut{Part, Part});
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
  const auto V = StackMgr.peekTop<VTIn>();
  if constexpr (Endian::native == Endian::little) {
    if constexpr (sizeof(TIn) == 1) {
      StackMgr.emplaceTop(__builtin_convertvector(
          HVTIn{V[0], V[1], V[2], V[3], V[4], V[5], V[6], V[7]}, VTOut));
    } else if constexpr (sizeof(TIn) == 2) {
      StackMgr.emplaceTop(
          __builtin_convertvector(HVTIn{V[0], V[1], V[2], V[3]}, VTOut));
    } else if constexpr (sizeof(TIn) == 4) {
      StackMgr.emplaceTop(__builtin_convertvector(HVTIn{V[0], V[1]}, VTOut));
    }
  } else {
    if constexpr (sizeof(TIn) == 1) {
      StackMgr.emplaceTop(__builtin_convertvector(
          HVTIn{V[8], V[9], V[10], V[11], V[12], V[13], V[14], V[15]}, VTOut));
    } else if constexpr (sizeof(TIn) == 2) {
      StackMgr.emplaceTop(
          __builtin_convertvector(HVTIn{V[4], V[5], V[6], V[7]}, VTOut));
    } else if constexpr (sizeof(TIn) == 4) {
      StackMgr.emplaceTop(__builtin_convertvector(HVTIn{V[2], V[3]}, VTOut));
    }
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
  const auto V = StackMgr.peekTop<VTIn>();
  if constexpr (Endian::native == Endian::little) {
    if constexpr (sizeof(TIn) == 1) {
      StackMgr.emplaceTop(__builtin_convertvector(
          HVTIn{V[8], V[9], V[10], V[11], V[12], V[13], V[14], V[15]}, VTOut));
    } else if constexpr (sizeof(TIn) == 2) {
      StackMgr.emplaceTop(
          __builtin_convertvector(HVTIn{V[4], V[5], V[6], V[7]}, VTOut));
    } else if constexpr (sizeof(TIn) == 4) {
      StackMgr.emplaceTop(__builtin_convertvector(HVTIn{V[2], V[3]}, VTOut));
    }
  } else {
    if constexpr (sizeof(TIn) == 1) {
      StackMgr.emplaceTop(__builtin_convertvector(
          HVTIn{V[0], V[1], V[2], V[3], V[4], V[5], V[6], V[7]}, VTOut));
    } else if constexpr (sizeof(TIn) == 2) {
      StackMgr.emplaceTop(
          __builtin_convertvector(HVTIn{V[0], V[1], V[2], V[3]}, VTOut));
    } else if constexpr (sizeof(TIn) == 4) {
      StackMgr.emplaceTop(__builtin_convertvector(HVTIn{V[0], V[1]}, VTOut));
    }
  }
  return {};
}

template <typename TIn, typename TOut>
Expect<void> Executor::runVectorExtAddPairwiseOp(
    Runtime::StackManager &StackMgr) const noexcept {
  static_assert(sizeof(TIn) * 2 == sizeof(TOut));
  using VTOut [[gnu::vector_size(16)]] = TOut;
  const auto Size = sizeof(TIn) * 8;
  const auto V = StackMgr.peekTop<VTOut>();
  const auto L = V >> Size;
  const auto R = (V << Size) >> Size;
  StackMgr.emplaceTop(L + R);

  return {};
}

template <typename T>
Expect<void>
Executor::runVectorAbsOp(Runtime::StackManager &StackMgr) const noexcept {
  using VT [[gnu::vector_size(16)]] = T;
  VT Result = StackMgr.peekTop<VT>();
  if constexpr (std::is_floating_point_v<T>) {
    if constexpr (sizeof(T) == 4) {
      using IVT [[gnu::vector_size(16)]] = uint32_t;
      IVT Mask = IVT{} + UINT32_C(0x7fffffff);
      Result = reinterpret_cast<VT>(reinterpret_cast<IVT>(Result) & Mask);
    } else {
      using IVT [[gnu::vector_size(16)]] = uint64_t;
      IVT Mask = IVT{} + UINT64_C(0x7fffffffffffffff);
      Result = reinterpret_cast<VT>(reinterpret_cast<IVT>(Result) & Mask);
    }
  } else {
    Result = detail::vectorSelect(Result > 0, Result, -Result);
  }
  StackMgr.emplaceTop(std::move(Result));
  return {};
}

template <typename T>
Expect<void>
Executor::runVectorNegOp(Runtime::StackManager &StackMgr) const noexcept {
  using VT [[gnu::vector_size(16)]] = T;
  VT Result = StackMgr.peekTop<VT>();
  Result = -Result;
  StackMgr.emplaceTop(std::move(Result));
  return {};
}

inline Expect<void>
Executor::runVectorPopcntOp(Runtime::StackManager &StackMgr) const noexcept {
  auto Result = StackMgr.peekTop<uint8x16_t>();
  Result -= ((Result >> UINT8_C(1)) & UINT8_C(0x55));
  Result = (Result & UINT8_C(0x33)) + ((Result >> UINT8_C(2)) & UINT8_C(0x33));
  Result += Result >> UINT8_C(4);
  Result &= UINT8_C(0x0f);
  StackMgr.emplaceTop(std::move(Result));
  return {};
}

template <typename T>
Expect<void>
Executor::runVectorSqrtOp(Runtime::StackManager &StackMgr) const noexcept {
  using VT [[gnu::vector_size(16)]] = T;
  auto Result = StackMgr.peekTop<VT>();
  if constexpr (sizeof(T) == 4) {
    Result = VT{std::sqrt(Result[0]), std::sqrt(Result[1]),
                std::sqrt(Result[2]), std::sqrt(Result[3])};
  } else if constexpr (sizeof(T) == 8) {
    Result = VT{std::sqrt(Result[0]), std::sqrt(Result[1])};
  }
  StackMgr.emplaceTop(std::move(Result));
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
  auto V = StackMgr.peekTop<VTIn>();
  if constexpr (sizeof(TIn) == sizeof(TOut)) {
    const VTOut IMin = VTOut{} + std::numeric_limits<TOut>::min();
    const VTOut IMax = VTOut{} + std::numeric_limits<TOut>::max();
    VTIn X = {std::trunc(V[0]), std::trunc(V[1]), std::trunc(V[2]),
              std::trunc(V[3])};
    VTOut Y = __builtin_convertvector(X, VTOut);
    Y = detail::vectorSelect(X == X, Y, VTOut{});
    Y = detail::vectorSelect(X <= FMin, IMin, Y);
    Y = detail::vectorSelect(X >= FMax, IMax, Y);
    StackMgr.emplaceTop(std::move(Y));
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
    auto T = Endian::native == Endian::little ? VTOut22{Y[0], Y[1], 0, 0}
                                              : VTOut22{0, 0, Y[0], Y[1]};
    StackMgr.emplaceTop(__builtin_convertvector(T, VTOut));
  }
  return {};
}

template <typename TIn, typename TOut>
Expect<void>
Executor::runVectorConvertOp(Runtime::StackManager &StackMgr) const noexcept {
  static_assert((sizeof(TIn) == 4 && (sizeof(TOut) == 4 || sizeof(TOut) == 8)));
  using VTIn [[gnu::vector_size(16)]] = TIn;
  using VTOut [[gnu::vector_size(16)]] = TOut;
  auto V = StackMgr.peekTop<VTIn>();
  if constexpr (sizeof(TIn) == sizeof(TOut)) {
    StackMgr.emplaceTop(__builtin_convertvector(V, VTOut));
  } else {
    using VTIn2 [[gnu::vector_size(8)]] = TIn;
    auto X = Endian::native == Endian::little ? VTIn2{V[0], V[1]}
                                              : VTIn2{V[2], V[3]};
    StackMgr.emplaceTop(__builtin_convertvector(X, VTOut));
  }
  return {};
}

inline Expect<void>
Executor::runVectorDemoteOp(Runtime::StackManager &StackMgr) const noexcept {
  const auto V = StackMgr.peekTop<doublex2_t>();
  const auto V2 =
      Endian::native == Endian::little
          ? floatx4_t{static_cast<float>(V[0]), static_cast<float>(V[1]), 0, 0}
          : floatx4_t{0, 0, static_cast<float>(V[1]), static_cast<float>(V[0])};
  StackMgr.emplaceTop(V2);
  return {};
}

inline Expect<void>
Executor::runVectorPromoteOp(Runtime::StackManager &StackMgr) const noexcept {
  const auto V = StackMgr.peekTop<floatx4_t>();
  const auto V2 =
      Endian::native == Endian::little
          ? doublex2_t{static_cast<double>(V[0]), static_cast<double>(V[1])}
          : doublex2_t{static_cast<double>(V[3]), static_cast<double>(V[2])};
  StackMgr.emplaceTop(V2);
  return {};
}

inline Expect<void>
Executor::runVectorAnyTrueOp(Runtime::StackManager &StackMgr) const noexcept {
  const auto Vector = StackMgr.peekTop<uint128_t>();
  const uint128_t Zero = 0;
  const uint32_t Result = (Vector != Zero);
  StackMgr.emplaceTop(std::move(Result));

  return {};
}

template <typename T>
Expect<void>
Executor::runVectorAllTrueOp(Runtime::StackManager &StackMgr) const noexcept {
  using VT [[gnu::vector_size(16)]] = T;
  auto Vector = StackMgr.peekTop<VT>();
  VT Z = Vector != 0;
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
  StackMgr.emplaceTop(std::move(Result));

  return {};
}

template <typename T>
Expect<void>
Executor::runVectorBitMaskOp(Runtime::StackManager &StackMgr) const noexcept {
  using SVT [[gnu::vector_size(16)]] = std::make_signed_t<T>;
  using UVT [[gnu::vector_size(16)]] = std::make_unsigned_t<T>;
  const auto Vector = StackMgr.peekTop<SVT>();
  const SVT MSB = Vector < 0;
  const UVT Z [[maybe_unused]] = reinterpret_cast<UVT>(MSB);
  if constexpr (sizeof(T) == 1) {
    using int16x16_t [[gnu::vector_size(32)]] = int16_t;
    using uint16x16_t [[gnu::vector_size(32)]] = uint16_t;
    const uint16x16_t Mask =
        Endian::native == Endian::little
            ? uint16x16_t{0x1,    0x2,    0x4,    0x8,   0x10,  0x20,
                          0x40,   0x80,   0x100,  0x200, 0x400, 0x800,
                          0x1000, 0x2000, 0x4000, 0x8000}
            : uint16x16_t{0x8000, 0x4000, 0x2000, 0x1000, 0x800, 0x400,
                          0x200,  0x100,  0x80,   0x40,   0x20,  0x10,
                          0x8,    0x4,    0x2,    0x1};
    uint16x16_t V =
        reinterpret_cast<uint16x16_t>(__builtin_convertvector(MSB, int16x16_t));
    V &= Mask;
    const uint16_t Result = V[0] | V[1] | V[2] | V[3] | V[4] | V[5] | V[6] |
                            V[7] | V[8] | V[9] | V[10] | V[11] | V[12] | V[13] |
                            V[14] | V[15];
    StackMgr.emplaceTop(static_cast<uint32_t>(Result));
  } else if constexpr (sizeof(T) == 2) {
    const uint16x8_t Mask =
        Endian::native == Endian::little
            ? uint16x8_t{0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80}
            : uint16x8_t{0x80, 0x40, 0x20, 0x10, 0x8, 0x4, 0x2, 0x1};
    using uint8x8_t [[gnu::vector_size(8)]] = uint8_t;
    const uint8x8_t V = __builtin_convertvector(Z & Mask, uint8x8_t);
    const uint8_t Result =
        V[0] | V[1] | V[2] | V[3] | V[4] | V[5] | V[6] | V[7];
    StackMgr.emplaceTop(static_cast<uint32_t>(Result));
  } else if constexpr (sizeof(T) == 4) {
    const uint32x4_t Mask = Endian::native == Endian::little
                                ? uint32x4_t{0x1, 0x2, 0x4, 0x8}
                                : uint32x4_t{0x8, 0x4, 0x2, 0x1};
    using uint8x4_t [[gnu::vector_size(4)]] = uint8_t;
    const uint8x4_t V = __builtin_convertvector(Z & Mask, uint8x4_t);
    const uint8_t Result = V[0] | V[1] | V[2] | V[3];
    StackMgr.emplaceTop(static_cast<uint32_t>(Result));
  } else if constexpr (sizeof(T) == 8) {
    const uint64x2_t Mask = Endian::native == Endian::little
                                ? uint64x2_t{0x1, 0x2}
                                : uint64x2_t{0x2, 0x1};
    using uint8x2_t [[gnu::vector_size(2)]] = uint8_t;
    const uint8x2_t V = __builtin_convertvector(Z & Mask, uint8x2_t);
    const uint8_t Result = V[0] | V[1];
    StackMgr.emplaceTop(static_cast<uint32_t>(Result));
  }

  return {};
}

template <typename T>
Expect<void>
Executor::runVectorCeilOp(Runtime::StackManager &StackMgr) const noexcept {
  using VT [[gnu::vector_size(16)]] = T;
  auto Result = StackMgr.peekTop<VT>();
  if constexpr (sizeof(T) == 4) {
    Result = VT{std::ceil(Result[0]), std::ceil(Result[1]),
                std::ceil(Result[2]), std::ceil(Result[3])};
  } else if constexpr (sizeof(T) == 8) {
    Result = VT{std::ceil(Result[0]), std::ceil(Result[1])};
  }
  StackMgr.emplaceTop(std::move(Result));
  return {};
}

template <typename T>
Expect<void>
Executor::runVectorFloorOp(Runtime::StackManager &StackMgr) const noexcept {
  using VT [[gnu::vector_size(16)]] = T;
  auto Result = StackMgr.peekTop<VT>();
  if constexpr (sizeof(T) == 4) {
    Result = VT{std::floor(Result[0]), std::floor(Result[1]),
                std::floor(Result[2]), std::floor(Result[3])};
  } else if constexpr (sizeof(T) == 8) {
    Result = VT{std::floor(Result[0]), std::floor(Result[1])};
  }
  StackMgr.emplaceTop(std::move(Result));
  return {};
}

template <typename T>
Expect<void>
Executor::runVectorTruncOp(Runtime::StackManager &StackMgr) const noexcept {
  using VT [[gnu::vector_size(16)]] = T;
  auto Result = StackMgr.peekTop<VT>();
  if constexpr (sizeof(T) == 4) {
    Result = VT{std::trunc(Result[0]), std::trunc(Result[1]),
                std::trunc(Result[2]), std::trunc(Result[3])};
  } else if constexpr (sizeof(T) == 8) {
    Result = VT{std::trunc(Result[0]), std::trunc(Result[1])};
  }
  StackMgr.emplaceTop(std::move(Result));
  return {};
}

template <typename T>
Expect<void>
Executor::runVectorNearestOp(Runtime::StackManager &StackMgr) const noexcept {
  using VT [[gnu::vector_size(16)]] = T;
  auto Result = StackMgr.peekTop<VT>();
  if constexpr (sizeof(T) == 4) {
    Result = VT{WasmEdge::roundeven(Result[0]), WasmEdge::roundeven(Result[1]),
                WasmEdge::roundeven(Result[2]), WasmEdge::roundeven(Result[3])};
  } else if constexpr (sizeof(T) == 8) {
    Result = VT{WasmEdge::roundeven(Result[0]), WasmEdge::roundeven(Result[1])};
  }
  StackMgr.emplaceTop(std::move(Result));
  return {};
}

} // namespace Executor
} // namespace WasmEdge
