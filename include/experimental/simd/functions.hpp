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

#include "experimental/bit.hpp"
#include "experimental/simd/detail.hpp"
#include "experimental/simd/vec.hpp"

#include <array>

namespace cxx26 {
namespace simd {

template <class V, class U = typename V::value_type> struct alignment;
template <class T, class Abi>
struct alignment<basic_vec<T, Abi>, T>
    : std::integral_constant<std::size_t, alignof(basic_vec<T, Abi>)> {};
template <class V, class U = typename V::value_type>
inline constexpr std::size_t alignment_v = alignment<V, U>::value;

template <class T, class Abi>
constexpr basic_vec<T, Abi> select(const basic_mask<sizeof(T), Abi> &M,
                                   const basic_vec<T, Abi> &A,
                                   const basic_vec<T, Abi> &B) noexcept {
  using backend = detail::simd_backend<T, Abi>;
  return detail::access::make<basic_vec<T, Abi>>(
      backend::select(detail::access::data(M), detail::access::data(A),
                      detail::access::data(B)));
}

template <std::size_t Bytes, class Abi>
constexpr bool all_of(const basic_mask<Bytes, Abi> &M) noexcept {
  constexpr simd_size_type N = basic_mask<Bytes, Abi>::size;
  if constexpr (N <= 64) {
    constexpr unsigned long long All = N == 64 ? ~0ULL : (1ULL << N) - 1ULL;
    return M.to_ullong() == All;
  } else {
    bool R = true;
    for (simd_size_type I = 0; I < N; ++I) {
      R = R && M[I];
    }
    return R;
  }
}

template <std::size_t Bytes, class Abi>
constexpr bool any_of(const basic_mask<Bytes, Abi> &M) noexcept {
  constexpr simd_size_type N = basic_mask<Bytes, Abi>::size;
  if constexpr (N <= 64) {
    return M.to_ullong() != 0;
  } else {
    bool R = false;
    for (simd_size_type I = 0; I < N; ++I) {
      R = R || M[I];
    }
    return R;
  }
}

template <std::size_t Bytes, class Abi>
constexpr bool none_of(const basic_mask<Bytes, Abi> &M) noexcept {
  return !any_of(M);
}

template <class V, class T, class Abi>
constexpr auto chunk(const basic_vec<T, Abi> &X) noexcept {
  static_assert(std::is_same_v<typename V::value_type, T>);
  constexpr simd_size_type N = basic_vec<T, Abi>::size;
  constexpr simd_size_type M = V::size;
  static_assert(N % M == 0);
  static_assert(sizeof(V) == M * sizeof(T));
  return cxx20::bit_cast<std::array<V, static_cast<std::size_t>(N / M)>>(X);
}

template <
    class T, class Abi, class... Rest,
    std::enable_if_t<(std::is_same_v<Rest, basic_vec<T, Abi>> && ...), int> = 0>
constexpr auto cat(const basic_vec<T, Abi> &X, const Rest &...Xs) noexcept {
  constexpr simd_size_type N = basic_vec<T, Abi>::size;
  constexpr std::size_t Count = 1 + sizeof...(Rest);
  using R = vec<T, N *static_cast<simd_size_type>(Count)>;
  static_assert(sizeof(R) == sizeof(basic_vec<T, Abi>) * Count);
  return cxx20::bit_cast<R>(std::array<basic_vec<T, Abi>, Count>{X, Xs...});
}

template <class T, class Abi, class F,
          std::enable_if_t<std::is_invocable_v<
                               F &, std::integral_constant<simd_size_type, 0>>,
                           int> = 0>
constexpr basic_vec<T, Abi> permute(const basic_vec<T, Abi> &X,
                                    F &&IdxMap) noexcept {
  using backend = detail::simd_backend<T, Abi>;
  return detail::access::make<basic_vec<T, Abi>>(backend::generate(
      [&](auto I) { return X[static_cast<simd_size_type>(IdxMap(I))]; }));
}

template <
    class T, class Abi, class I, class IAbi,
    std::enable_if_t<std::is_integral_v<I> && basic_vec<I, IAbi>::size() ==
                                                  basic_vec<T, Abi>::size(),
                     int> = 0>
constexpr basic_vec<T, Abi>
permute(const basic_vec<T, Abi> &X,
        const basic_vec<I, IAbi> &Indices) noexcept {
  using backend = detail::simd_backend<T, Abi>;
  if constexpr (std::is_same_v<I, T> && std::is_same_v<IAbi, Abi>) {
    return detail::access::make<basic_vec<T, Abi>>(backend::permute(
        detail::access::data(X), detail::access::data(Indices)));
  } else {
    return detail::access::make<basic_vec<T, Abi>>(backend::generate(
        [&](auto J) { return X[static_cast<simd_size_type>(Indices[J])]; }));
  }
}

#define CXX26_SIMD_MATH(NAME)                                                  \
  template <class T, class Abi,                                                \
            std::enable_if_t<std::is_floating_point_v<T>, int> = 0>            \
  basic_vec<T, Abi> NAME(const basic_vec<T, Abi> &X) noexcept {                \
    using backend = detail::simd_backend<T, Abi>;                              \
    return detail::access::make<basic_vec<T, Abi>>(                            \
        backend::NAME(detail::access::data(X)));                               \
  }
CXX26_SIMD_MATH(sqrt)
CXX26_SIMD_MATH(ceil)
CXX26_SIMD_MATH(floor)
CXX26_SIMD_MATH(trunc)
#undef CXX26_SIMD_MATH

} // namespace simd
} // namespace cxx26
