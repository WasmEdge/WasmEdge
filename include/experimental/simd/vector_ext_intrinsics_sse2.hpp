// SPDX-License-Identifier: CC0-1.0
///
// simd - A C++17 implementation of a subset of the C++26 std::simd library
//
// To the extent possible under law, the author(s) have dedicated all
// copyright and related and neighboring rights to this software to the
// public domain worldwide. This software is distributed without any warranty.
//
// You should have received a copy of the CC0 Public Domain Dedication
// along with this software. If not, see
// <http://creativecommons.org/publicdomain/zero/1.0/>.
///

#pragma once

#include "experimental/simd/vector_ext_intrinsics.hpp"

#if defined(__SSE2__)

#include "experimental/bit.hpp"
#include "experimental/simd/backend_m128i.hpp"

namespace cxx26 {
namespace simd {
namespace detail {

template <class T, simd_size_type N>
struct vector_ext_intrinsics<T, N, std::enable_if_t<N * sizeof(T) == 16>> {
  static constexpr bool has_math = true;
  static constexpr bool has_to_ullong = true;
  static constexpr bool has_permute = false;

  using storage_type =
      typename simd_backend<T, vector_ext_abi<N>>::storage_type;
  using backend = simd_backend<T, m128i_abi<N>>;

  static __m128i to_m128i(const storage_type &S) noexcept {
    return cxx20::bit_cast<__m128i>(S);
  }

#define CXX26_SIMD_VECTOR_EXT_SSE2(NAME)                                       \
  static storage_type NAME(const storage_type &S) noexcept {                   \
    return cxx20::bit_cast<storage_type>(backend::NAME(to_m128i(S)));          \
  }
  CXX26_SIMD_VECTOR_EXT_SSE2(sqrt)
  CXX26_SIMD_VECTOR_EXT_SSE2(ceil)
  CXX26_SIMD_VECTOR_EXT_SSE2(floor)
  CXX26_SIMD_VECTOR_EXT_SSE2(trunc)
  CXX26_SIMD_VECTOR_EXT_SSE2(roundeven)
#undef CXX26_SIMD_VECTOR_EXT_SSE2

  static unsigned long long to_ullong(const storage_type &S) noexcept {
    return backend::to_ullong(to_m128i(S));
  }
};

} // namespace detail
} // namespace simd
} // namespace cxx26

#endif
