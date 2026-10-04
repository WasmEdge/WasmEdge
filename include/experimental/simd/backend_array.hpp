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

#include "experimental/math.hpp"
#include "experimental/simd/detail.hpp"

#include <array>
#include <cmath>
#include <type_traits>

namespace cxx26 {
namespace simd {
namespace detail {

template <class T, simd_size_type N>
struct alignas(storage_alignment<T, N>) array_storage {
  std::array<T, N> Lanes;
};

template <class T, simd_size_type N> struct simd_backend<T, array_abi<N>> {
  using value_type = T;
  using storage_type = array_storage<T, N>;
  using mask_storage = typename simd_backend<integer_from_t<sizeof(T)>,
                                             array_abi<N>>::storage_type;

  static_assert(sizeof(storage_type) == N * sizeof(T));

  template <class G, std::size_t... I>
  static constexpr storage_type generate(G &&Gen,
                                         std::index_sequence<I...>) noexcept {
    return storage_type{{{static_cast<T>(
        Gen(std::integral_constant<simd_size_type,
                                   static_cast<simd_size_type>(I)>()))...}}};
  }
  template <class G> static constexpr storage_type generate(G &&Gen) noexcept {
    return generate(std::forward<G>(Gen),
                    std::make_index_sequence<static_cast<std::size_t>(N)>());
  }
  static constexpr storage_type broadcast(T Value) noexcept {
    return generate([Value](auto) { return Value; });
  }
  static constexpr T get(const storage_type &S, simd_size_type I) noexcept {
    return S.Lanes[static_cast<std::size_t>(I)];
  }
  static constexpr void set_lane(storage_type &S, simd_size_type I,
                                 T Value) noexcept {
    S.Lanes[static_cast<std::size_t>(I)] = Value;
  }

  template <class U>
  static constexpr storage_type convert(
      const typename simd_backend<U, array_abi<N>>::storage_type &S) noexcept {
    return generate([&](auto I) { return S.Lanes[I]; });
  }

#define CXX26_SIMD_ARRAY_BINARY(NAME, OP)                                      \
  static constexpr storage_type NAME(const storage_type &A,                    \
                                     const storage_type &B) noexcept {         \
    return generate([&](auto I) { return A.Lanes[I] OP B.Lanes[I]; });         \
  }
  CXX26_SIMD_ARRAY_BINARY(add, +)
  CXX26_SIMD_ARRAY_BINARY(sub, -)
  CXX26_SIMD_ARRAY_BINARY(mul, *)
  CXX26_SIMD_ARRAY_BINARY(div, /)
  CXX26_SIMD_ARRAY_BINARY(bit_and, &)
  CXX26_SIMD_ARRAY_BINARY(bit_or, |)
  CXX26_SIMD_ARRAY_BINARY(bit_xor, ^)
  CXX26_SIMD_ARRAY_BINARY(shl, <<)
  CXX26_SIMD_ARRAY_BINARY(shr, >>)
#undef CXX26_SIMD_ARRAY_BINARY

  static constexpr storage_type shl(const storage_type &A,
                                    simd_size_type S) noexcept {
    return generate([&](auto I) { return A.Lanes[I] << S; });
  }
  static constexpr storage_type shr(const storage_type &A,
                                    simd_size_type S) noexcept {
    return generate([&](auto I) { return A.Lanes[I] >> S; });
  }
  static constexpr storage_type neg(const storage_type &A) noexcept {
    if constexpr (std::is_unsigned_v<T>) {
      return generate([&](auto I) { return T{0} - A.Lanes[I]; });
    } else {
      return generate([&](auto I) { return -A.Lanes[I]; });
    }
  }
  static constexpr storage_type bit_not(const storage_type &A) noexcept {
    return generate([&](auto I) { return ~A.Lanes[I]; });
  }

#define CXX26_SIMD_ARRAY_MATH(NAME)                                            \
  static storage_type NAME(const storage_type &A) noexcept {                   \
    return generate([&](auto I) { return quiet_nan(std::NAME(A.Lanes[I])); }); \
  }
  CXX26_SIMD_ARRAY_MATH(sqrt)
  CXX26_SIMD_ARRAY_MATH(ceil)
  CXX26_SIMD_ARRAY_MATH(floor)
  CXX26_SIMD_ARRAY_MATH(trunc)
#undef CXX26_SIMD_ARRAY_MATH

  static storage_type roundeven(const storage_type &A) noexcept {
    return generate(
        [&](auto I) { return quiet_nan(cxx26::roundeven(A.Lanes[I])); });
  }

#define CXX26_SIMD_ARRAY_COMPARE(NAME, OP)                                     \
  static constexpr mask_storage NAME(const storage_type &A,                    \
                                     const storage_type &B) noexcept {         \
    return simd_backend<integer_from_t<sizeof(T)>, array_abi<N>>::generate(    \
        [&](auto I) { return A.Lanes[I] OP B.Lanes[I] ? -1 : 0; });            \
  }
  CXX26_SIMD_ARRAY_COMPARE(eq, ==)
  CXX26_SIMD_ARRAY_COMPARE(ne, !=)
  CXX26_SIMD_ARRAY_COMPARE(lt, <)
  CXX26_SIMD_ARRAY_COMPARE(le, <=)
  CXX26_SIMD_ARRAY_COMPARE(gt, >)
  CXX26_SIMD_ARRAY_COMPARE(ge, >=)
#undef CXX26_SIMD_ARRAY_COMPARE

  static constexpr storage_type select(const mask_storage &M,
                                       const storage_type &A,
                                       const storage_type &B) noexcept {
    return generate(
        [&](auto I) { return M.Lanes[I] ? A.Lanes[I] : B.Lanes[I]; });
  }

  static constexpr storage_type permute(const storage_type &V,
                                        const storage_type &Idx) noexcept {
    return generate([&](auto I) {
      return V.Lanes[static_cast<std::size_t>(Idx.Lanes[I]) %
                     static_cast<std::size_t>(N)];
    });
  }

  static constexpr unsigned long long
  to_ullong(const storage_type &S) noexcept {
    unsigned long long R = 0;
    for (simd_size_type I = 0; I < N; ++I) {
      R |= static_cast<unsigned long long>(
               S.Lanes[static_cast<std::size_t>(I)] != 0)
           << I;
    }
    return R;
  }
};

} // namespace detail
} // namespace simd
} // namespace cxx26
