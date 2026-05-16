// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/executor/engine/simd_ops.h - SIMD op free templates ------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Free-function template implementations of SIMD operations.  Each op
/// mutates its first argument (Val or V1) in-place.
///
//===----------------------------------------------------------------------===//
#pragma once

#if !defined(_MSC_VER) || defined(__clang__)

#include "common/roundeven.h"
#include "common/types.h"
#include "executor/engine/vector_helper.h"

#include <cmath>
#include <cstdint>
#include <type_traits>

namespace WasmEdge {
namespace Executor {
namespace simdOps {

// ---------------------------------------------------------------------------
// V128 whole-register bitwise operations
// ---------------------------------------------------------------------------

inline void v128Not(ValVariant &Val) noexcept {
  Val.get<uint128_t>() = ~Val.get<uint128_t>();
}

inline void v128And(ValVariant &V1, const ValVariant &V2) noexcept {
  V1.get<uint64x2_t>() &= V2.get<uint64x2_t>();
}

inline void v128Andnot(ValVariant &V1, const ValVariant &V2) noexcept {
  V1.get<uint64x2_t>() &= ~V2.get<uint64x2_t>();
}

inline void v128Or(ValVariant &V1, const ValVariant &V2) noexcept {
  V1.get<uint64x2_t>() |= V2.get<uint64x2_t>();
}

inline void v128Xor(ValVariant &V1, const ValVariant &V2) noexcept {
  V1.get<uint64x2_t>() ^= V2.get<uint64x2_t>();
}

// ---------------------------------------------------------------------------
// Lane-wise binary arithmetic.  T is the lane element type
// (e.g. uint32_t, float, double).
// ---------------------------------------------------------------------------

template <typename T>
inline void vectorAdd(ValVariant &V1, const ValVariant &V2) noexcept {
  using VT [[gnu::vector_size(16)]] = T;
  V1.get<VT>() += V2.get<VT>();
}

template <typename T>
inline void vectorSub(ValVariant &V1, const ValVariant &V2) noexcept {
  using VT [[gnu::vector_size(16)]] = T;
  V1.get<VT>() -= V2.get<VT>();
}

template <typename T>
inline void vectorMul(ValVariant &V1, const ValVariant &V2) noexcept {
  using VT [[gnu::vector_size(16)]] = T;
  V1.get<VT>() *= V2.get<VT>();
}

template <typename T>
inline void vectorDiv(ValVariant &V1, const ValVariant &V2) noexcept {
  using VT [[gnu::vector_size(16)]] = T;
  V1.get<VT>() /= V2.get<VT>();
}

// ---------------------------------------------------------------------------
// Lane-wise integer min/max.  Use signed T for _s ops, unsigned T for _u ops.
// ---------------------------------------------------------------------------

template <typename T>
inline void vectorMin(ValVariant &V1, const ValVariant &V2) noexcept {
  using VT [[gnu::vector_size(16)]] = T;
  VT &A = V1.get<VT>();
  const VT &B = V2.get<VT>();
  A = detail::vectorSelect(A > B, B, A);
}

template <typename T>
inline void vectorMax(ValVariant &V1, const ValVariant &V2) noexcept {
  using VT [[gnu::vector_size(16)]] = T;
  VT &A = V1.get<VT>();
  const VT &B = V2.get<VT>();
  A = detail::vectorSelect(B > A, B, A);
}

// ---------------------------------------------------------------------------
// Float min/max with NaN propagation.
//
//   fmin:  R = bits(A) | bits(B)    // merge NaN payloads
//          if A < B: R = A
//          if A > B: R = B
//          if A is NaN: R = A
//          if B is NaN: R = B
//   fmax:  same but & instead of |, and reversed comparisons.
//
// NaN check uses (V == V) which is false for NaN lanes, so
// vectorSelect(false, R, V) = V.
// ---------------------------------------------------------------------------

template <typename T>
inline void vectorFMin(ValVariant &V1, const ValVariant &V2) noexcept {
  static_assert(std::is_floating_point_v<T>);
  using VT [[gnu::vector_size(16)]] = T;
  VT &A = V1.get<VT>();
  const VT &B = V2.get<VT>();
  VT R = reinterpret_cast<VT>(reinterpret_cast<uint64x2_t>(A) |
                              reinterpret_cast<uint64x2_t>(B));
  R = detail::vectorSelect(A < B, A, R);
  R = detail::vectorSelect(A > B, B, R);
  // NOLINTBEGIN(misc-redundant-expression): A==A and B==B are IEEE NaN checks;
  // for GCC vector types these are lane-wise comparisons, not redundant.
  R = detail::vectorSelect(A == A, R, A);
  R = detail::vectorSelect(B == B, R, B);
  // NOLINTEND(misc-redundant-expression)
  A = R;
}

template <typename T>
inline void vectorFMax(ValVariant &V1, const ValVariant &V2) noexcept {
  static_assert(std::is_floating_point_v<T>);
  using VT [[gnu::vector_size(16)]] = T;
  VT &A = V1.get<VT>();
  const VT &B = V2.get<VT>();
  VT R = reinterpret_cast<VT>(reinterpret_cast<uint64x2_t>(A) &
                              reinterpret_cast<uint64x2_t>(B));
  R = detail::vectorSelect(A < B, B, R);
  R = detail::vectorSelect(A > B, A, R);
  // NOLINTBEGIN(misc-redundant-expression): IEEE NaN checks on vector lanes
  R = detail::vectorSelect(A == A, R, A);
  R = detail::vectorSelect(B == B, R, B);
  // NOLINTEND(misc-redundant-expression)
  A = R;
}

// ---------------------------------------------------------------------------
// Lane-wise unary operations.
// ---------------------------------------------------------------------------

template <typename T> inline void vectorNeg(ValVariant &Val) noexcept {
  using VT [[gnu::vector_size(16)]] = T;
  Val.get<VT>() = -Val.get<VT>();
}

/// Integer and float abs.
/// Float: clears sign bit via integer mask.
/// Integer: vectorSelect(x > 0, x, -x) — wraps at INT_MIN per Wasm spec.
template <typename T> inline void vectorAbs(ValVariant &Val) noexcept {
  using VT [[gnu::vector_size(16)]] = T;
  VT &Result = Val.get<VT>();
  if constexpr (std::is_floating_point_v<T>) {
    if constexpr (sizeof(T) == 4) {
      using IVT [[gnu::vector_size(16)]] = uint32_t;
      IVT Mask = IVT{} + UINT32_C(0x7FFFFFFF);
      Result = reinterpret_cast<VT>(reinterpret_cast<IVT>(Result) & Mask);
    } else {
      using IVT [[gnu::vector_size(16)]] = uint64_t;
      IVT Mask = IVT{} + UINT64_C(0x7FFFFFFFFFFFFFFF);
      Result = reinterpret_cast<VT>(reinterpret_cast<IVT>(Result) & Mask);
    }
  } else {
    Result = detail::vectorSelect(Result > VT{}, Result, -Result);
  }
}

/// Per-lane sqrt.
template <typename T> inline void vectorSqrt(ValVariant &Val) noexcept {
  static_assert(std::is_floating_point_v<T>);
  using VT [[gnu::vector_size(16)]] = T;
  VT &Result = Val.get<VT>();
  if constexpr (sizeof(T) == 4) {
    Result = VT{std::sqrt(Result[0]), std::sqrt(Result[1]),
                std::sqrt(Result[2]), std::sqrt(Result[3])};
  } else {
    Result = VT{std::sqrt(Result[0]), std::sqrt(Result[1])};
  }
}

/// Per-lane ceil.
template <typename T> inline void vectorCeil(ValVariant &Val) noexcept {
  static_assert(std::is_floating_point_v<T>);
  using VT [[gnu::vector_size(16)]] = T;
  VT &Result = Val.get<VT>();
  if constexpr (sizeof(T) == 4) {
    Result = VT{std::ceil(Result[0]), std::ceil(Result[1]),
                std::ceil(Result[2]), std::ceil(Result[3])};
  } else {
    Result = VT{std::ceil(Result[0]), std::ceil(Result[1])};
  }
}

/// Per-lane floor.
template <typename T> inline void vectorFloor(ValVariant &Val) noexcept {
  static_assert(std::is_floating_point_v<T>);
  using VT [[gnu::vector_size(16)]] = T;
  VT &Result = Val.get<VT>();
  if constexpr (sizeof(T) == 4) {
    Result = VT{std::floor(Result[0]), std::floor(Result[1]),
                std::floor(Result[2]), std::floor(Result[3])};
  } else {
    Result = VT{std::floor(Result[0]), std::floor(Result[1])};
  }
}

/// Per-lane trunc.
template <typename T> inline void vectorTrunc(ValVariant &Val) noexcept {
  static_assert(std::is_floating_point_v<T>);
  using VT [[gnu::vector_size(16)]] = T;
  VT &Result = Val.get<VT>();
  if constexpr (sizeof(T) == 4) {
    Result = VT{std::trunc(Result[0]), std::trunc(Result[1]),
                std::trunc(Result[2]), std::trunc(Result[3])};
  } else {
    Result = VT{std::trunc(Result[0]), std::trunc(Result[1])};
  }
}

/// Per-lane roundeven (ties-to-even rounding).
template <typename T> inline void vectorNearest(ValVariant &Val) noexcept {
  static_assert(std::is_floating_point_v<T>);
  using VT [[gnu::vector_size(16)]] = T;
  VT &Result = Val.get<VT>();
  if constexpr (sizeof(T) == 4) {
    Result = VT{WasmEdge::roundeven(Result[0]), WasmEdge::roundeven(Result[1]),
                WasmEdge::roundeven(Result[2]), WasmEdge::roundeven(Result[3])};
  } else {
    Result = VT{WasmEdge::roundeven(Result[0]), WasmEdge::roundeven(Result[1])};
  }
}

/// i8x16.popcnt — per-byte Hamming weight via SWAR.
inline void vectorPopcnt(ValVariant &Val) noexcept {
  auto &Result = Val.get<uint8x16_t>();
  Result -= ((Result >> UINT8_C(1)) & UINT8_C(0x55));
  Result = (Result & UINT8_C(0x33)) + ((Result >> UINT8_C(2)) & UINT8_C(0x33));
  Result += Result >> UINT8_C(4);
  Result &= UINT8_C(0x0f);
}

// ---------------------------------------------------------------------------
// Splat (scalar → vector).  TIn is the scalar input type; TOut is the
// output lane type.
// ---------------------------------------------------------------------------

template <typename TIn, typename TOut>
inline void splatOp(ValVariant &Val) noexcept {
  const TOut Part = static_cast<TOut>(Val.get<TIn>());
  using VTOut [[gnu::vector_size(16)]] = TOut;
  if constexpr (!std::is_floating_point_v<TOut>) {
    Val.emplace<VTOut>(VTOut{} + Part);
  } else if constexpr (sizeof(TOut) == 4) {
    Val.emplace<VTOut>(VTOut{Part, Part, Part, Part});
  } else {
    Val.emplace<VTOut>(VTOut{Part, Part});
  }
}

} // namespace simdOps
} // namespace Executor
} // namespace WasmEdge

#endif // !_MSC_VER || __clang__
