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

#include "experimental/simd/detail.hpp"

namespace cxx26 {
namespace simd {
namespace detail {

template <class T, simd_size_type N> struct simd_backend<T, vector_ext_abi<N>>;

// Target instructions for 16-byte vectors that the vector extensions do not
// reach. has_math covers square root and rounding, has_to_ullong packs a mask
// into bits, and has_permute shuffles lanes by a runtime index vector.
template <class T, simd_size_type N, class = void>
struct vector_ext_intrinsics {
  static constexpr bool has_math = false;
  static constexpr bool has_to_ullong = false;
  static constexpr bool has_permute = false;
};

} // namespace detail
} // namespace simd
} // namespace cxx26

#include "experimental/simd/vector_ext_intrinsics_neon.hpp"
#include "experimental/simd/vector_ext_intrinsics_rvv.hpp"
#include "experimental/simd/vector_ext_intrinsics_s390x.hpp"
#include "experimental/simd/vector_ext_intrinsics_sse2.hpp"
