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

#if defined(__riscv_v) && defined(__riscv_v_intrinsic) &&                      \
    __riscv_v_intrinsic >= 12000

#include <limits>
#include <riscv_vector.h>
#include <type_traits>

namespace cxx26 {
namespace simd {
namespace detail {

template <class T, simd_size_type N>
struct vector_ext_intrinsics<T, N, std::enable_if_t<N * sizeof(T) == 16>> {
  static constexpr bool has_math = true;
  static constexpr bool has_to_ullong = true;
  // GCC already lowers a runtime __builtin_shuffle to vrgather, while Clang
  // expands __builtin_shufflevector lane by lane.
#if defined(__clang__)
  static constexpr bool has_permute = std::is_integral_v<T>;
#else
  static constexpr bool has_permute = false;
#endif

  using storage_type =
      typename simd_backend<T, vector_ext_abi<N>>::storage_type;
  using bits_type = std::make_unsigned_t<integer_from_t<sizeof(T)>>;
  using lane_type =
      std::conditional_t<std::is_floating_point_v<T>, T, bits_type>;

  // The V extension has registers of at least 128 bits, so one LMUL=1
  // register with VL = N holds the whole vector. The vector extension types do
  // not convert to the scalable types, so values move through memory.
  template <class L = lane_type>
  static auto load(const storage_type &S) noexcept {
    const L *P = reinterpret_cast<const L *>(&S);
    if constexpr (std::is_same_v<L, float>) {
      return __riscv_vle32_v_f32m1(P, N);
    } else if constexpr (std::is_same_v<L, double>) {
      return __riscv_vle64_v_f64m1(P, N);
    } else if constexpr (sizeof(L) == 1) {
      return __riscv_vle8_v_u8m1(P, N);
    } else if constexpr (sizeof(L) == 2) {
      return __riscv_vle16_v_u16m1(P, N);
    } else if constexpr (sizeof(L) == 4) {
      return __riscv_vle32_v_u32m1(P, N);
    } else {
      return __riscv_vle64_v_u64m1(P, N);
    }
  }
  template <class V> static storage_type store(V Vec) noexcept {
    storage_type R;
    lane_type *P = reinterpret_cast<lane_type *>(&R);
    if constexpr (sizeof(T) == 1) {
      __riscv_vse8(P, Vec, N);
    } else if constexpr (sizeof(T) == 2) {
      __riscv_vse16(P, Vec, N);
    } else if constexpr (sizeof(T) == 4) {
      __riscv_vse32(P, Vec, N);
    } else {
      __riscv_vse64(P, Vec, N);
    }
    return R;
  }

  static storage_type sqrt(const storage_type &S) noexcept {
    return store(__riscv_vfsqrt(load(S), N));
  }

  // The vector unit has no round-to-integral instruction. Converting to an
  // integer under a static rounding mode and back rounds lanes below
  // 2^(digits-1), and the sign injection keeps negative zero. Larger lanes are
  // already integral, infinite or NaN, and A + 0 returns them with any
  // signaling NaN quieted.
  template <std::float_round_style R>
  static storage_type round(const storage_type &S) noexcept {
    constexpr unsigned Mode = R == std::round_to_nearest    ? __RISCV_FRM_RNE
                              : R == std::round_toward_zero ? __RISCV_FRM_RTZ
                              : R == std::round_toward_infinity
                                  ? __RISCV_FRM_RUP
                                  : __RISCV_FRM_RDN;
    const auto V = load(S);
    const auto Rounded =
        __riscv_vfsgnj(__riscv_vfcvt_f(__riscv_vfcvt_x(V, Mode, N), N), V, N);
    const auto Small = __riscv_vmflt(
        __riscv_vfabs(V, N),
        static_cast<T>(1ULL << (std::numeric_limits<T>::digits - 1)), N);
    return store(__riscv_vmerge(__riscv_vfadd(V, static_cast<T>(0), N), Rounded,
                                Small, N));
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

  // The compare writes one mask bit per lane in lane order. Bits past VL are
  // unspecified.
  static unsigned long long to_ullong(const storage_type &S) noexcept {
    const auto Mask = __riscv_vmsne(load<bits_type>(S), 0, N);
    return __riscv_vmv_x_s_u64m1_u64(__riscv_vreinterpret_u64m1(Mask)) &
           ((1ULL << N) - 1);
  }

  // vrgather gives zero for an index past the register and reads a tail lane
  // for an index from N up to the register length. The other backends take
  // the index modulo N.
  static storage_type permute(const storage_type &V,
                              const storage_type &Idx) noexcept {
    return store(__riscv_vrgather(
        load(V), __riscv_vand(load(Idx), static_cast<lane_type>(N - 1), N), N));
  }
};

} // namespace detail
} // namespace simd
} // namespace cxx26

#endif
