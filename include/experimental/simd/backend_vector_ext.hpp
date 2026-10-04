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

#if defined(__GNUC__) || defined(__clang__)

#include "experimental/bit.hpp"
#include "experimental/simd/backend_array.hpp"
#include "experimental/simd/vector_ext_intrinsics.hpp"

#include <cmath>

namespace cxx26 {
namespace simd {
namespace detail {

template <class T, simd_size_type N> struct simd_backend<T, vector_ext_abi<N>> {
  static_assert(is_power_of_two(static_cast<std::size_t>(N)));

  using value_type = T;
  using storage_type [[gnu::vector_size(N * sizeof(T))]] = T;
  using mask_storage = typename simd_backend<integer_from_t<sizeof(T)>,
                                             vector_ext_abi<N>>::storage_type;

  static_assert(sizeof(storage_type) == N * sizeof(T));

  template <class G, std::size_t... I>
  static constexpr storage_type generate(G &&Gen,
                                         std::index_sequence<I...>) noexcept {
    return storage_type{static_cast<T>(
        Gen(std::integral_constant<simd_size_type,
                                   static_cast<simd_size_type>(I)>()))...};
  }
  template <class G> static constexpr storage_type generate(G &&Gen) noexcept {
    return generate(std::forward<G>(Gen),
                    std::make_index_sequence<static_cast<std::size_t>(N)>());
  }
  static constexpr storage_type broadcast(T Value) noexcept {
    return generate([Value](auto) { return Value; });
  }
  static constexpr T get(const storage_type &S, simd_size_type I) noexcept {
    return S[I];
  }
  static constexpr void set_lane(storage_type &S, simd_size_type I,
                                 T Value) noexcept {
    S[I] = Value;
  }

  template <class U>
  static constexpr storage_type
  convert(const typename simd_backend<U, vector_ext_abi<N>>::storage_type
              &S) noexcept {
    return __builtin_convertvector(S, storage_type);
  }

  static constexpr storage_type add(const storage_type &A,
                                    const storage_type &B) noexcept {
    return A + B;
  }
  static constexpr storage_type sub(const storage_type &A,
                                    const storage_type &B) noexcept {
    return A - B;
  }
  static constexpr storage_type mul(const storage_type &A,
                                    const storage_type &B) noexcept {
    return A * B;
  }
  static constexpr storage_type div(const storage_type &A,
                                    const storage_type &B) noexcept {
    return A / B;
  }
  static constexpr storage_type bit_and(const storage_type &A,
                                        const storage_type &B) noexcept {
    return A & B;
  }
  static constexpr storage_type bit_or(const storage_type &A,
                                       const storage_type &B) noexcept {
    return A | B;
  }
  static constexpr storage_type bit_xor(const storage_type &A,
                                        const storage_type &B) noexcept {
    return A ^ B;
  }
  static constexpr storage_type shl(const storage_type &A,
                                    const storage_type &B) noexcept {
    return A << B;
  }
  static constexpr storage_type shr(const storage_type &A,
                                    const storage_type &B) noexcept {
    return A >> B;
  }
  static constexpr storage_type shl(const storage_type &A,
                                    simd_size_type S) noexcept {
    return A << broadcast(static_cast<T>(S));
  }
  static constexpr storage_type shr(const storage_type &A,
                                    simd_size_type S) noexcept {
    return A >> broadcast(static_cast<T>(S));
  }
  static constexpr storage_type neg(const storage_type &A) noexcept {
    return -A;
  }
  static constexpr storage_type bit_not(const storage_type &A) noexcept {
    return ~A;
  }

  using intrinsics = vector_ext_intrinsics<T, N>;

#define CXX26_SIMD_VECTOR_EXT_MATH(NAME, SCALAR)                               \
  static storage_type NAME(const storage_type &A) noexcept {                   \
    if constexpr (intrinsics::has_math) {                                      \
      return intrinsics::NAME(A);                                              \
    } else {                                                                   \
      return generate([&](auto I) { return quiet_nan(SCALAR(get(A, I))); });   \
    }                                                                          \
  }
  CXX26_SIMD_VECTOR_EXT_MATH(sqrt, std::sqrt)
  CXX26_SIMD_VECTOR_EXT_MATH(ceil, std::ceil)
  CXX26_SIMD_VECTOR_EXT_MATH(floor, std::floor)
  CXX26_SIMD_VECTOR_EXT_MATH(trunc, std::trunc)
  CXX26_SIMD_VECTOR_EXT_MATH(roundeven, cxx26::roundeven)
#undef CXX26_SIMD_VECTOR_EXT_MATH

  static constexpr mask_storage eq(const storage_type &A,
                                   const storage_type &B) noexcept {
    return reinterpret_cast<mask_storage>(A == B);
  }
  static constexpr mask_storage ne(const storage_type &A,
                                   const storage_type &B) noexcept {
    return reinterpret_cast<mask_storage>(A != B);
  }
  static constexpr mask_storage lt(const storage_type &A,
                                   const storage_type &B) noexcept {
    return reinterpret_cast<mask_storage>(A < B);
  }
  static constexpr mask_storage le(const storage_type &A,
                                   const storage_type &B) noexcept {
    return reinterpret_cast<mask_storage>(A <= B);
  }
  static constexpr mask_storage gt(const storage_type &A,
                                   const storage_type &B) noexcept {
    return reinterpret_cast<mask_storage>(A > B);
  }
  static constexpr mask_storage ge(const storage_type &A,
                                   const storage_type &B) noexcept {
    return reinterpret_cast<mask_storage>(A >= B);
  }

  static constexpr storage_type select(const mask_storage &M,
                                       const storage_type &A,
                                       const storage_type &B) noexcept {
#if defined(__clang__)
    return reinterpret_cast<storage_type>(
        (M & reinterpret_cast<mask_storage>(A)) |
        (~M & reinterpret_cast<mask_storage>(B)));
#else
    return M ? A : B;
#endif
  }

  static storage_type permute(const storage_type &V,
                              const storage_type &Idx) noexcept {
    if constexpr (intrinsics::has_permute) {
      return intrinsics::permute(V, Idx);
    } else {
#if defined(__clang__)
      return __builtin_shufflevector(V, Idx);
#else
      return __builtin_shuffle(V, Idx);
#endif
    }
  }

  static constexpr unsigned long long
  to_ullong(const storage_type &S) noexcept {
    if constexpr (intrinsics::has_to_ullong) {
      if (!__builtin_is_constant_evaluated()) {
        return intrinsics::to_ullong(S);
      }
    }
#if defined(__clang__) && __clang_major__ >= 15 &&                             \
    __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    if constexpr (N == 8 || N == 16 || N == 32 || N == 64) {
      typedef bool bool_vec __attribute__((ext_vector_type(N)));
      using bits_type = std::make_unsigned_t<integer_from_t<N / 8>>;
      return __builtin_bit_cast(bits_type,
                                __builtin_convertvector(S, bool_vec));
    }
#endif
    unsigned long long R = 0;
    for (simd_size_type I = 0; I < N; ++I) {
      R |= static_cast<unsigned long long>(S[I] != 0) << I;
    }
    return R;
  }
};

} // namespace detail
} // namespace simd
} // namespace cxx26

#endif
