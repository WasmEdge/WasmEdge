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

#if defined(__s390x__) && defined(__VX__)

#include "experimental/bit.hpp"

#include <limits>
#include <utility>

namespace cxx26 {
namespace simd {
namespace detail {

template <class T, simd_size_type N>
struct vector_ext_intrinsics<T, N, std::enable_if_t<N * sizeof(T) == 16>> {
  // z13 has vector arithmetic for double only. z14 (arch12) adds float and
  // VBPERM.
  static constexpr bool has_math = std::is_same_v<T, double> || __ARCH__ >= 12;
  static constexpr bool has_to_ullong = __ARCH__ >= 12;
  static constexpr bool has_permute = sizeof(T) == 1;

  using storage_type =
      typename simd_backend<T, vector_ext_abi<N>>::storage_type;
  using f32x4 [[gnu::vector_size(16)]] = float;
  using f64x2 [[gnu::vector_size(16)]] = double;
  using u8x16 [[gnu::vector_size(16)]] = unsigned char;
  using u64x2 [[gnu::vector_size(16)]] = unsigned long long;

  static storage_type sqrt(const storage_type &S) noexcept {
    if constexpr (std::is_same_v<T, double>) {
      return cxx20::bit_cast<storage_type>(
          __builtin_s390_vfsqdb(cxx20::bit_cast<f64x2>(S)));
    }
#if __ARCH__ >= 12
    else {
      return cxx20::bit_cast<storage_type>(
          __builtin_s390_vfsqsb(cxx20::bit_cast<f32x4>(S)));
    }
#endif
  }

  // VFI takes the rounding method in M5: 4 nearest even, 5 toward zero, 6
  // toward +inf, 7 toward -inf. M4 = 4 suppresses the inexact exception.
  template <std::float_round_style R>
  static storage_type round(const storage_type &S) noexcept {
    constexpr int Method = R == std::round_to_nearest        ? 4
                           : R == std::round_toward_zero     ? 5
                           : R == std::round_toward_infinity ? 6
                                                             : 7;
    if constexpr (std::is_same_v<T, double>) {
      return cxx20::bit_cast<storage_type>(
          __builtin_s390_vfidb(cxx20::bit_cast<f64x2>(S), 4, Method));
    }
#if __ARCH__ >= 12
    else {
      return cxx20::bit_cast<storage_type>(
          __builtin_s390_vfisb(cxx20::bit_cast<f32x4>(S), 4, Method));
    }
#endif
  }
  static storage_type ceil(const storage_type &S) noexcept {
    return round<std::round_toward_infinity>(S);
  }
  static storage_type floor(const storage_type &S) noexcept {
    return round<std::round_toward_neg_infinity>(S);
  }
  static storage_type trunc(const storage_type &S) noexcept {
    return round<std::round_toward_zero>(S);
  }
  static storage_type roundeven(const storage_type &S) noexcept {
    return round<std::round_to_nearest>(S);
  }

#if __ARCH__ >= 12
  // VBPERM gathers the bits that the index bytes name, bit 0 being the
  // leftmost, into the low 16 bits of doubleword 0 with byte 15's bit lowest.
  // Index byte 15 - I names the first bit of lane I, and index 128 gives zero.
  template <std::size_t... I>
  static constexpr u8x16 bit_index(std::index_sequence<I...>) noexcept {
    return u8x16{static_cast<unsigned char>(
        static_cast<simd_size_type>(15 - I) < N ? (15 - I) * sizeof(T) * 8
                                                : 128)...};
  }
  static unsigned long long to_ullong(const storage_type &S) noexcept {
    const u64x2 Bits = __builtin_s390_vbperm(
        cxx20::bit_cast<u8x16>(S), bit_index(std::make_index_sequence<16>()));
    return Bits[0];
  }
#endif

  // VPERM selects bytes from two vectors by the low five bits of each index,
  // so passing V twice takes the index modulo 16.
  static storage_type permute(const storage_type &V,
                              const storage_type &Idx) noexcept {
    const u8x16 Bytes = cxx20::bit_cast<u8x16>(V);
    return cxx20::bit_cast<storage_type>(
        __builtin_s390_vperm(Bytes, Bytes, cxx20::bit_cast<u8x16>(Idx)));
  }
};

} // namespace detail
} // namespace simd
} // namespace cxx26

#endif
