// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

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

#include "common/types.h"
#include "executor/engine/vector_helper.h"
#include "experimental/bit.hpp"
#include "experimental/simd/ext.hpp"

#include <cstdint>
#include <limits>
#include <type_traits>

namespace WasmEdge {
namespace Executor {
namespace simdOps {

using cxx20::bit_cast;
using detail::Vec;

// ---------------------------------------------------------------------------
// Lane-wise binary arithmetic.  T is the lane element type
// (e.g. uint32_t, float, double).
// ---------------------------------------------------------------------------

template <typename T>
inline void vectorAdd(ValVariant &V1, const ValVariant &V2) noexcept {
  V1.get<Vec<T>>() += V2.get<Vec<T>>();
}

template <typename T>
inline void vectorSub(ValVariant &V1, const ValVariant &V2) noexcept {
  V1.get<Vec<T>>() -= V2.get<Vec<T>>();
}

template <typename T>
inline void vectorMul(ValVariant &V1, const ValVariant &V2) noexcept {
  V1.get<Vec<T>>() *= V2.get<Vec<T>>();
}

template <typename T>
inline void vectorDiv(ValVariant &V1, const ValVariant &V2) noexcept {
  V1.get<Vec<T>>() /= V2.get<Vec<T>>();
}

// ---------------------------------------------------------------------------
// Lane-wise integer min/max.  Use signed T for _s ops, unsigned T for _u ops.
// ---------------------------------------------------------------------------

template <typename T>
inline void vectorMin(ValVariant &V1, const ValVariant &V2) noexcept {
  auto &A = V1.get<Vec<T>>();
  const auto &B = V2.get<Vec<T>>();
  A = select(A > B, B, A);
}

template <typename T>
inline void vectorMax(ValVariant &V1, const ValVariant &V2) noexcept {
  auto &A = V1.get<Vec<T>>();
  const auto &B = V2.get<Vec<T>>();
  A = select(B > A, B, A);
}

// ---------------------------------------------------------------------------
// Float min/max with NaN propagation.
//
//   fmin:  R = bits(A) | bits(B)    // merge NaN payloads
//          if A < B: R = A
//          if A > B: R = B
//          if A is NaN: R = A with the quiet bit set
//          if B is NaN: R = B with the quiet bit set
//   fmax:  same but & instead of |, and reversed comparisons.
// ---------------------------------------------------------------------------

/// V with the most significant payload bit set, so a NaN lane becomes an
/// arithmetic NaN.
template <typename T> inline Vec<T> setQuietBit(const Vec<T> &V) noexcept {
  using U = std::conditional_t<sizeof(T) == 4, uint32_t, uint64_t>;
  const Vec<U> Bit(
      static_cast<U>(U{1} << (std::numeric_limits<T>::digits - 2)));
  return bit_cast<Vec<T>>(bit_cast<Vec<U>>(V) | Bit);
}

template <typename T>
inline void vectorFMin(ValVariant &V1, const ValVariant &V2) noexcept {
  static_assert(std::is_floating_point_v<T>);
  auto &A = V1.get<Vec<T>>();
  const auto &B = V2.get<Vec<T>>();
  auto R = bit_cast<Vec<T>>(bit_cast<uint64x2_t>(A) | bit_cast<uint64x2_t>(B));
  R = select(A < B, A, R);
  R = select(A > B, B, R);
  // NOLINTBEGIN(misc-redundant-expression): IEEE NaN checks on vector lanes
  R = select(A == A, R, setQuietBit<T>(A));
  R = select(B == B, R, setQuietBit<T>(B));
  // NOLINTEND(misc-redundant-expression)
  A = R;
}

template <typename T>
inline void vectorFMax(ValVariant &V1, const ValVariant &V2) noexcept {
  static_assert(std::is_floating_point_v<T>);
  auto &A = V1.get<Vec<T>>();
  const auto &B = V2.get<Vec<T>>();
  auto R = bit_cast<Vec<T>>(bit_cast<uint64x2_t>(A) & bit_cast<uint64x2_t>(B));
  R = select(A < B, B, R);
  R = select(A > B, A, R);
  // NOLINTBEGIN(misc-redundant-expression): IEEE NaN checks on vector lanes
  R = select(A == A, R, setQuietBit<T>(A));
  R = select(B == B, R, setQuietBit<T>(B));
  // NOLINTEND(misc-redundant-expression)
  A = R;
}

// ---------------------------------------------------------------------------
// Lane-wise unary operations.
// ---------------------------------------------------------------------------

template <typename T> inline void vectorNeg(ValVariant &Val) noexcept {
  Val.get<Vec<T>>() = -Val.get<Vec<T>>();
}

/// Integer and float abs.
/// Float: clears sign bit via integer mask.
/// Integer: select(x > 0, x, -x) — wraps at INT_MIN per Wasm spec.
template <typename T> inline void vectorAbs(ValVariant &Val) noexcept {
  auto &Result = Val.get<Vec<T>>();
  if constexpr (std::is_floating_point_v<T>) {
    using U = std::conditional_t<sizeof(T) == 4, uint32_t, uint64_t>;
    const Vec<U> Mask(static_cast<U>(~(U(1) << (sizeof(U) * 8 - 1))));
    Result = bit_cast<Vec<T>>(bit_cast<Vec<U>>(Result) & Mask);
  } else {
    Result = select(Result > Vec<T>(), Result, -Result);
  }
}

template <typename T> inline void vectorSqrt(ValVariant &Val) noexcept {
  static_assert(std::is_floating_point_v<T>);
  auto &Result = Val.get<Vec<T>>();
  Result = sqrt(Result);
}

template <typename T> inline void vectorCeil(ValVariant &Val) noexcept {
  static_assert(std::is_floating_point_v<T>);
  auto &Result = Val.get<Vec<T>>();
  Result = ceil(Result);
}

template <typename T> inline void vectorFloor(ValVariant &Val) noexcept {
  static_assert(std::is_floating_point_v<T>);
  auto &Result = Val.get<Vec<T>>();
  Result = floor(Result);
}

template <typename T> inline void vectorTrunc(ValVariant &Val) noexcept {
  static_assert(std::is_floating_point_v<T>);
  auto &Result = Val.get<Vec<T>>();
  Result = trunc(Result);
}

/// Per-lane roundeven (ties-to-even rounding).
template <typename T> inline void vectorNearest(ValVariant &Val) noexcept {
  static_assert(std::is_floating_point_v<T>);
  auto &Result = Val.get<Vec<T>>();
  Result = cxx26::simd_ext::roundeven(Result);
}

/// i8x16.swizzle — an index of 16 or more selects zero.
inline void vectorSwizzle(ValVariant &V1, const ValVariant &V2) noexcept {
  using V = Vec<uint8_t>;
  V Index = V2.get<V>();
  if constexpr (Endian::native == Endian::big) {
    Index = V(uint8_t{15}) - Index;
  }
  auto &Vector = V1.get<V>();
  Vector = select(Index >= V(uint8_t{16}), V(),
                  permute(Vector, Index & V(uint8_t{15})));
}

/// i8x16.popcnt — per-byte Hamming weight via SWAR.
inline void vectorPopcnt(ValVariant &Val) noexcept {
  using V = Vec<uint8_t>;
  auto &Result = Val.get<V>();
  Result -= ((Result >> 1) & V(uint8_t{0x55}));
  Result = (Result & V(uint8_t{0x33})) + ((Result >> 2) & V(uint8_t{0x33}));
  Result += Result >> 4;
  Result &= V(uint8_t{0x0f});
}

} // namespace simdOps
} // namespace Executor
} // namespace WasmEdge
