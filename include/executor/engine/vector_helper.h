// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "common/endian.h"
#include "experimental/bit.hpp"
#include "experimental/simd.hpp"

namespace WasmEdge {
namespace Executor {
namespace detail {

namespace simd = cxx26::simd;

template <typename T, simd::simd_size_type N =
                          static_cast<simd::simd_size_type>(16 / sizeof(T))>
using Vec = simd::vec<T, N>;

/// Memory lane index of logical Wasm lane I.
template <typename T>
constexpr simd::simd_size_type lane(simd::simd_size_type I) noexcept {
  return Endian::native == Endian::little ? I : Vec<T>::size - 1 - I;
}

/// All-one-bits lanes where Mask is true and zero lanes elsewhere, as a V.
template <typename V, typename M> V maskToVec(const M &Mask) noexcept {
  return cxx20::bit_cast<V>(-Mask);
}

/// X with each lane limited to the range from Lo to Hi.
template <typename V> V clamp(const V &X, const V &Lo, const V &Hi) noexcept {
  return select(X > Hi, Hi, select(X < Lo, Lo, X));
}

/// The half of X that holds Wasm lanes 0 to size / 2 - 1, as an H.
template <typename H, typename V> H lowHalf(const V &X) noexcept {
  return simd::chunk<H>(X)[Endian::native == Endian::little ? 0 : 1];
}

/// The half of X that holds Wasm lanes size / 2 to size - 1, as an H.
template <typename H, typename V> H highHalf(const V &X) noexcept {
  return simd::chunk<H>(X)[Endian::native == Endian::little ? 1 : 0];
}

/// Lo in the low Wasm lanes and Hi in the high Wasm lanes.
template <typename H> auto catLowHigh(const H &Lo, const H &Hi) noexcept {
  return Endian::native == Endian::little ? cat(Lo, Hi) : cat(Hi, Lo);
}

} // namespace detail
} // namespace Executor
} // namespace WasmEdge
