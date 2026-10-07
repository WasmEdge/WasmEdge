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

} // namespace detail
} // namespace Executor
} // namespace WasmEdge
