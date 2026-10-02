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

#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <utility>

namespace cxx26 {
namespace simd {

using simd_size_type = int;

template <class T, class Abi> class basic_vec;
template <std::size_t Bytes, class Abi> class basic_mask;

namespace detail {

template <simd_size_type N> struct array_abi {};
template <simd_size_type N> struct vector_ext_abi {};
template <simd_size_type N> struct m128i_abi {};

template <class Abi> struct abi_size;
template <template <simd_size_type> class Tag, simd_size_type N>
struct abi_size<Tag<N>> : std::integral_constant<simd_size_type, N> {};
template <class Abi>
inline constexpr simd_size_type abi_size_v = abi_size<Abi>::value;

template <std::size_t Bytes> struct integer_from;
template <> struct integer_from<1> {
  using type = std::int8_t;
};
template <> struct integer_from<2> {
  using type = std::int16_t;
};
template <> struct integer_from<4> {
  using type = std::int32_t;
};
template <> struct integer_from<8> {
  using type = std::int64_t;
};
template <std::size_t Bytes>
using integer_from_t = typename integer_from<Bytes>::type;

template <class T>
using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;

template <class From, class To> constexpr bool value_preserving() noexcept {
  if constexpr (!std::is_arithmetic_v<From> || !std::is_arithmetic_v<To>) {
    return false;
  } else if constexpr (std::is_same_v<From, To>) {
    return true;
  } else if constexpr (std::is_integral_v<From> && std::is_integral_v<To>) {
    return (std::is_unsigned_v<From> || std::is_signed_v<To>) &&
           std::numeric_limits<From>::digits <= std::numeric_limits<To>::digits;
  } else if constexpr (std::is_integral_v<From>) {
    return std::numeric_limits<From>::digits <= std::numeric_limits<To>::digits;
  } else if constexpr (std::is_floating_point_v<To>) {
    return std::numeric_limits<From>::digits <=
               std::numeric_limits<To>::digits &&
           std::numeric_limits<From>::max_exponent <=
               std::numeric_limits<To>::max_exponent;
  } else {
    return false;
  }
}
template <class From, class To>
inline constexpr bool is_value_preserving_v = value_preserving<From, To>();

template <class R, class T>
inline constexpr bool is_generator_result_v =
    std::is_convertible_v<R, T> &&
    (!std::is_arithmetic_v<remove_cvref_t<R>> ||
     is_value_preserving_v<remove_cvref_t<R>, T>);

template <class G, class T, std::size_t... I>
constexpr bool is_generator_for(std::index_sequence<I...>) noexcept {
  return (is_generator_result_v<
              std::invoke_result_t<
                  G &, std::integral_constant<simd_size_type,
                                              static_cast<simd_size_type>(I)>>,
              T> &&
          ...);
}

template <class G, class T, simd_size_type N>
constexpr bool is_generator() noexcept {
  if constexpr (!std::is_invocable_v<
                    G &, std::integral_constant<simd_size_type, 0>>) {
    return false;
  } else {
    return is_generator_for<G, T>(
        std::make_index_sequence<static_cast<std::size_t>(N)>());
  }
}
template <class G, class T, simd_size_type N>
inline constexpr bool is_generator_v = is_generator<G, T, N>();

template <class T, class Abi> struct simd_backend;

constexpr bool is_power_of_two(std::size_t X) noexcept {
  return X > 0 && (X & (X - 1)) == 0;
}

template <class T, simd_size_type N>
using m128i_or_array_abi =
    std::conditional_t<N * sizeof(T) == 16, m128i_abi<N>, array_abi<N>>;

template <class T, simd_size_type N> struct select_abi {
#if defined(CXX26_SIMD_FORCE_GENERIC)
  using type = array_abi<N>;
#elif defined(CXX26_SIMD_FORCE_M128I)
  using type = m128i_or_array_abi<T, N>;
#elif defined(__GNUC__) || defined(__clang__)
  using type = std::conditional_t<is_power_of_two(static_cast<std::size_t>(N)),
                                  vector_ext_abi<N>, array_abi<N>>;
#elif defined(_MSC_VER) && defined(_M_X64)
  using type = m128i_or_array_abi<T, N>;
#else
  using type = array_abi<N>;
#endif
};

struct private_tag {};

struct access {
  template <class V>
  static constexpr const auto &data(const V &Value) noexcept {
    return Value.Data;
  }
  template <class V> static constexpr auto &data(V &Value) noexcept {
    return Value.Data;
  }
  template <class V, class S>
  static constexpr V make(const S &Storage) noexcept {
    return V(private_tag{}, Storage);
  }
};

template <class T, simd_size_type N>
inline constexpr std::size_t storage_alignment =
    is_power_of_two(N * sizeof(T)) ? N * sizeof(T) : alignof(T);

} // namespace detail

template <class T, simd_size_type N>
using vec = basic_vec<T, typename detail::select_abi<T, N>::type>;
template <class T, simd_size_type N>
using mask = basic_mask<sizeof(T), typename detail::select_abi<T, N>::type>;

} // namespace simd
} // namespace cxx26
