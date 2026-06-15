// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/executor/engine/simd_ops.h - SIMD op free templates ------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Free-function template implementations of SIMD operations.  Each op takes
/// its operand(s) by value (const-qualified) and returns the computed result.
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
// Lane-wise binary arithmetic.  T is the lane element type
// (e.g. uint32_t, float, double).
// ---------------------------------------------------------------------------

template <typename T>
inline SIMDArray<T, 16> vectorAdd(const SIMDArray<T, 16> V1,
                                  const SIMDArray<T, 16> V2) noexcept {
  return V1 + V2;
}

template <typename T>
inline SIMDArray<T, 16> vectorSub(const SIMDArray<T, 16> V1,
                                  const SIMDArray<T, 16> V2) noexcept {
  return V1 - V2;
}

template <typename T>
inline SIMDArray<T, 16> vectorMul(const SIMDArray<T, 16> V1,
                                  const SIMDArray<T, 16> V2) noexcept {
  return V1 * V2;
}

template <typename T>
inline SIMDArray<T, 16> vectorDiv(const SIMDArray<T, 16> V1,
                                  const SIMDArray<T, 16> V2) noexcept {
  return V1 / V2;
}

// ---------------------------------------------------------------------------
// Lane-wise integer min/max.  Use signed T for _s ops, unsigned T for _u ops.
// ---------------------------------------------------------------------------

template <typename T>
inline SIMDArray<T, 16> vectorMin(const SIMDArray<T, 16> V1,
                                  const SIMDArray<T, 16> V2) noexcept {
  return detail::vectorSelect(V1 > V2, V2, V1);
}

template <typename T>
inline SIMDArray<T, 16> vectorMax(const SIMDArray<T, 16> V1,
                                  const SIMDArray<T, 16> V2) noexcept {
  return detail::vectorSelect(V2 > V1, V2, V1);
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
inline SIMDArray<T, 16> vectorFMin(const SIMDArray<T, 16> V1,
                                   const SIMDArray<T, 16> V2) noexcept {
  static_assert(std::is_floating_point_v<T>);
  using VT = SIMDArray<T, 16>;
  VT R = reinterpret_cast<VT>(reinterpret_cast<uint64x2_t>(V1) |
                              reinterpret_cast<uint64x2_t>(V2));
  R = detail::vectorSelect(V1 < V2, V1, R);
  R = detail::vectorSelect(V1 > V2, V2, R);
  // NOLINTBEGIN(misc-redundant-expression): V1==V1 and V2==V2 are IEEE NaN
  // checks; for GCC vector types these are lane-wise comparisons.
  R = detail::vectorSelect(V1 == V1, R, V1);
  R = detail::vectorSelect(V2 == V2, R, V2);
  // NOLINTEND(misc-redundant-expression)
  return R;
}

template <typename T>
inline SIMDArray<T, 16> vectorFMax(const SIMDArray<T, 16> V1,
                                   const SIMDArray<T, 16> V2) noexcept {
  static_assert(std::is_floating_point_v<T>);
  using VT = SIMDArray<T, 16>;
  VT R = reinterpret_cast<VT>(reinterpret_cast<uint64x2_t>(V1) &
                              reinterpret_cast<uint64x2_t>(V2));
  R = detail::vectorSelect(V1 < V2, V2, R);
  R = detail::vectorSelect(V1 > V2, V1, R);
  // NOLINTBEGIN(misc-redundant-expression): IEEE NaN checks on vector lanes
  R = detail::vectorSelect(V1 == V1, R, V1);
  R = detail::vectorSelect(V2 == V2, R, V2);
  // NOLINTEND(misc-redundant-expression)
  return R;
}

// ---------------------------------------------------------------------------
// Lane-wise unary operations.
// ---------------------------------------------------------------------------

template <typename T>
inline SIMDArray<T, 16> vectorNeg(const SIMDArray<T, 16> V) noexcept {
  return -V;
}

/// Integer and float abs.
/// Float: clears sign bit via integer mask.
/// Integer: vectorSelect(x > 0, x, -x) — wraps at INT_MIN per Wasm spec.
template <typename T>
inline SIMDArray<T, 16> vectorAbs(const SIMDArray<T, 16> V) noexcept {
  using VT = SIMDArray<T, 16>;
  if constexpr (std::is_floating_point_v<T>) {
    if constexpr (sizeof(T) == 4) {
      uint32x4_t Mask = uint32x4_t{} + UINT32_C(0x7FFFFFFF);
      return reinterpret_cast<VT>(reinterpret_cast<uint32x4_t>(V) & Mask);
    } else {
      uint64x2_t Mask = uint64x2_t{} + UINT64_C(0x7FFFFFFFFFFFFFFF);
      return reinterpret_cast<VT>(reinterpret_cast<uint64x2_t>(V) & Mask);
    }
  } else {
    return detail::vectorSelect(V > VT{}, V, -V);
  }
}

/// Per-lane sqrt.
template <typename T>
inline SIMDArray<T, 16> vectorSqrt(const SIMDArray<T, 16> V) noexcept {
  static_assert(std::is_floating_point_v<T>);
  using VT = SIMDArray<T, 16>;
  if constexpr (sizeof(T) == 4) {
    return VT{std::sqrt(V[0]), std::sqrt(V[1]), std::sqrt(V[2]),
              std::sqrt(V[3])};
  } else {
    return VT{std::sqrt(V[0]), std::sqrt(V[1])};
  }
}

/// Per-lane ceil.
template <typename T>
inline SIMDArray<T, 16> vectorCeil(const SIMDArray<T, 16> V) noexcept {
  static_assert(std::is_floating_point_v<T>);
  using VT = SIMDArray<T, 16>;
  if constexpr (sizeof(T) == 4) {
    return VT{std::ceil(V[0]), std::ceil(V[1]), std::ceil(V[2]),
              std::ceil(V[3])};
  } else {
    return VT{std::ceil(V[0]), std::ceil(V[1])};
  }
}

/// Per-lane floor.
template <typename T>
inline SIMDArray<T, 16> vectorFloor(const SIMDArray<T, 16> V) noexcept {
  static_assert(std::is_floating_point_v<T>);
  using VT = SIMDArray<T, 16>;
  if constexpr (sizeof(T) == 4) {
    return VT{std::floor(V[0]), std::floor(V[1]), std::floor(V[2]),
              std::floor(V[3])};
  } else {
    return VT{std::floor(V[0]), std::floor(V[1])};
  }
}

/// Per-lane trunc.
template <typename T>
inline SIMDArray<T, 16> vectorTrunc(const SIMDArray<T, 16> V) noexcept {
  static_assert(std::is_floating_point_v<T>);
  using VT = SIMDArray<T, 16>;
  if constexpr (sizeof(T) == 4) {
    return VT{std::trunc(V[0]), std::trunc(V[1]), std::trunc(V[2]),
              std::trunc(V[3])};
  } else {
    return VT{std::trunc(V[0]), std::trunc(V[1])};
  }
}

/// Per-lane roundeven (ties-to-even rounding).
template <typename T>
inline SIMDArray<T, 16> vectorNearest(const SIMDArray<T, 16> V) noexcept {
  static_assert(std::is_floating_point_v<T>);
  using VT = SIMDArray<T, 16>;
  if constexpr (sizeof(T) == 4) {
    return VT{WasmEdge::roundeven(V[0]), WasmEdge::roundeven(V[1]),
              WasmEdge::roundeven(V[2]), WasmEdge::roundeven(V[3])};
  } else {
    return VT{WasmEdge::roundeven(V[0]), WasmEdge::roundeven(V[1])};
  }
}

/// i8x16.popcnt — per-byte Hamming weight via SWAR.
inline uint8x16_t vectorPopcnt(const uint8x16_t V) noexcept {
  uint8x16_t R = V;
  R -= ((R >> UINT8_C(1)) & UINT8_C(0x55));
  R = (R & UINT8_C(0x33)) + ((R >> UINT8_C(2)) & UINT8_C(0x33));
  R += R >> UINT8_C(4);
  R &= UINT8_C(0x0f);
  return R;
}

} // namespace simdOps
} // namespace Executor
} // namespace WasmEdge

#endif // !_MSC_VER || __clang__
