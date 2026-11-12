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

#if defined(__aarch64__) && defined(__ARM_NEON) &&                             \
    __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__

#include "experimental/bit.hpp"

#include <arm_neon.h>

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

#define CXX26_SIMD_VECTOR_EXT_NEON_MATH(NAME, OP)                              \
  static storage_type NAME(const storage_type &S) noexcept {                   \
    if constexpr (std::is_same_v<T, float>) {                                  \
      return cxx20::bit_cast<storage_type>(                                    \
          OP##_f32(cxx20::bit_cast<float32x4_t>(S)));                          \
    } else {                                                                   \
      return cxx20::bit_cast<storage_type>(                                    \
          OP##_f64(cxx20::bit_cast<float64x2_t>(S)));                          \
    }                                                                          \
  }
  CXX26_SIMD_VECTOR_EXT_NEON_MATH(sqrt, vsqrtq)
  CXX26_SIMD_VECTOR_EXT_NEON_MATH(ceil, vrndpq)
  CXX26_SIMD_VECTOR_EXT_NEON_MATH(floor, vrndmq)
  CXX26_SIMD_VECTOR_EXT_NEON_MATH(trunc, vrndq)
  CXX26_SIMD_VECTOR_EXT_NEON_MATH(roundeven, vrndnq)
#undef CXX26_SIMD_VECTOR_EXT_NEON_MATH

  // Mask lanes are all ones or zero. Keeping one weighted bit per lane and
  // adding across the vector packs the lanes into an integer.
  static unsigned long long to_ullong(const storage_type &S) noexcept {
    if constexpr (sizeof(T) == 1) {
      const uint8x16_t Weights = {1, 2, 4, 8, 16, 32, 64, 128,
                                  1, 2, 4, 8, 16, 32, 64, 128};
      const uint8x16_t Bits = vandq_u8(cxx20::bit_cast<uint8x16_t>(S), Weights);
      return vaddv_u8(vget_low_u8(Bits)) |
             static_cast<unsigned long long>(vaddv_u8(vget_high_u8(Bits))) << 8;
    } else if constexpr (sizeof(T) == 2) {
      const uint16x8_t Weights = {1, 2, 4, 8, 16, 32, 64, 128};
      return vaddvq_u16(vandq_u16(cxx20::bit_cast<uint16x8_t>(S), Weights));
    } else if constexpr (sizeof(T) == 4) {
      const uint32x4_t Weights = {1, 2, 4, 8};
      return vaddvq_u32(vandq_u32(cxx20::bit_cast<uint32x4_t>(S), Weights));
    } else {
      const uint64x2_t Weights = {1, 2};
      return vaddvq_u64(vandq_u64(cxx20::bit_cast<uint64x2_t>(S), Weights));
    }
  }
};

} // namespace detail
} // namespace simd
} // namespace cxx26

#endif
