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

template <class T, class Abi> class basic_vec {
  static constexpr simd_size_type N = detail::abi_size_v<Abi>;
  using backend = detail::simd_backend<T, Abi>;
  using storage_type = typename backend::storage_type;

public:
  using value_type = T;
  using mask_type = basic_mask<sizeof(T), Abi>;
  using abi_type = Abi;

  static constexpr std::integral_constant<simd_size_type, N> size{};

  constexpr basic_vec() noexcept = default;

  template <class U, std::enable_if_t<
                         std::is_convertible_v<U, value_type> &&
                             (std::is_arithmetic_v<detail::remove_cvref_t<U>>
                                  ? detail::is_value_preserving_v<
                                        detail::remove_cvref_t<U>, value_type>
                                  : !detail::is_generator_v<U, value_type, N>),
                         int> = 0>
  constexpr basic_vec(U &&Value) noexcept
      : Data(backend::broadcast(static_cast<value_type>(Value))) {}

  template <class U, class UAbi,
            std::enable_if_t<!std::is_same_v<basic_vec<U, UAbi>, basic_vec> &&
                                 detail::abi_size_v<UAbi> == N &&
                                 detail::is_value_preserving_v<U, value_type>,
                             int> = 0>
  constexpr basic_vec(const basic_vec<U, UAbi> &Other) noexcept
      : Data(convert_from(Other)) {}

  template <class U, class UAbi,
            std::enable_if_t<!std::is_same_v<basic_vec<U, UAbi>, basic_vec> &&
                                 detail::abi_size_v<UAbi> == N &&
                                 !detail::is_value_preserving_v<U, value_type>,
                             int> = 0>
  constexpr explicit basic_vec(const basic_vec<U, UAbi> &Other) noexcept
      : Data(convert_from(Other)) {}

  template <class G,
            std::enable_if_t<!std::is_convertible_v<G, value_type> &&
                                 detail::is_generator_v<G, value_type, N>,
                             int> = 0>
  constexpr explicit basic_vec(G &&Gen) noexcept
      : Data(backend::generate(std::forward<G>(Gen))) {}

  constexpr value_type operator[](simd_size_type I) const noexcept {
    return backend::get(Data, I);
  }

  constexpr basic_vec operator+() const noexcept { return *this; }
  constexpr basic_vec operator-() const noexcept {
    return basic_vec(detail::private_tag{}, backend::neg(Data));
  }
  constexpr basic_vec operator~() const noexcept {
    return basic_vec(detail::private_tag{}, backend::bit_not(Data));
  }
  constexpr mask_type operator!() const noexcept {
    return *this == basic_vec();
  }

#define CXX26_SIMD_VEC_BINARY(OP, NAME)                                        \
  friend constexpr basic_vec operator OP(const basic_vec &A,                   \
                                         const basic_vec &B) noexcept {        \
    return basic_vec(detail::private_tag{}, backend::NAME(A.Data, B.Data));    \
  }                                                                            \
  friend constexpr basic_vec &operator OP## =                                  \
      (basic_vec & A, const basic_vec &B) noexcept {                           \
    return A = A OP B;                                                         \
  }
  CXX26_SIMD_VEC_BINARY(+, add)
  CXX26_SIMD_VEC_BINARY(-, sub)
  CXX26_SIMD_VEC_BINARY(*, mul)
  CXX26_SIMD_VEC_BINARY(/, div)
  CXX26_SIMD_VEC_BINARY(&, bit_and)
  CXX26_SIMD_VEC_BINARY(|, bit_or)
  CXX26_SIMD_VEC_BINARY(^, bit_xor)
  CXX26_SIMD_VEC_BINARY(<<, shl)
  CXX26_SIMD_VEC_BINARY(>>, shr)
#undef CXX26_SIMD_VEC_BINARY

#define CXX26_SIMD_VEC_SHIFT(OP, NAME)                                         \
  friend constexpr basic_vec operator OP(const basic_vec &A,                   \
                                         simd_size_type S) noexcept {          \
    return basic_vec(detail::private_tag{}, backend::NAME(A.Data, S));         \
  }                                                                            \
  friend constexpr basic_vec &operator OP## =                                  \
      (basic_vec & A, simd_size_type S) noexcept {                             \
    return A = A OP S;                                                         \
  }
  CXX26_SIMD_VEC_SHIFT(<<, shl)
  CXX26_SIMD_VEC_SHIFT(>>, shr)
#undef CXX26_SIMD_VEC_SHIFT

#define CXX26_SIMD_VEC_COMPARE(OP, NAME)                                       \
  friend constexpr mask_type operator OP(const basic_vec &A,                   \
                                         const basic_vec &B) noexcept {        \
    return detail::access::make<mask_type>(backend::NAME(A.Data, B.Data));     \
  }
  CXX26_SIMD_VEC_COMPARE(==, eq)
  CXX26_SIMD_VEC_COMPARE(!=, ne)
  CXX26_SIMD_VEC_COMPARE(<, lt)
  CXX26_SIMD_VEC_COMPARE(<=, le)
  CXX26_SIMD_VEC_COMPARE(>, gt)
  CXX26_SIMD_VEC_COMPARE(>=, ge)
#undef CXX26_SIMD_VEC_COMPARE

private:
  friend struct detail::access;
  template <class, class> friend class basic_vec;

  constexpr basic_vec(detail::private_tag, const storage_type &S) noexcept
      : Data(S) {}

  template <class U, class UAbi>
  static constexpr storage_type
  convert_from(const basic_vec<U, UAbi> &Other) noexcept {
    if constexpr (std::is_same_v<UAbi, Abi>) {
      return backend::template convert<U>(Other.Data);
    } else {
      return backend::generate(
          [&](auto I) { return static_cast<value_type>(Other[I]); });
    }
  }

  // The s390x ABI aligns vector types to 8 bytes. alignas gives them the
  // alignment that the other targets have.
  alignas(detail::storage_alignment<T, N>) storage_type Data;
};

template <std::size_t Bytes, class Abi> class basic_mask {
  static constexpr simd_size_type N = detail::abi_size_v<Abi>;
  using lane_type = detail::integer_from_t<Bytes>;
  using backend = detail::simd_backend<lane_type, Abi>;
  using storage_type = typename backend::storage_type;
  using lanes_type = basic_vec<lane_type, Abi>;

public:
  using value_type = bool;
  using abi_type = Abi;

  static constexpr std::integral_constant<simd_size_type, N> size{};

  constexpr basic_mask() noexcept = default;

  template <class U,
            std::enable_if_t<std::is_same_v<detail::remove_cvref_t<U>, bool>,
                             int> = 0>
  constexpr explicit basic_mask(U Value) noexcept
      : Data(backend::broadcast(static_cast<lane_type>(Value ? -1 : 0))) {}

  template <class G,
            std::enable_if_t<
                !std::is_convertible_v<G, bool> &&
                    std::is_invocable_r_v<
                        bool, G &, std::integral_constant<simd_size_type, 0>>,
                int> = 0>
  constexpr explicit basic_mask(G &&Gen) noexcept
      : Data(backend::generate([&](auto I) -> lane_type {
          return static_cast<bool>(Gen(I)) ? -1 : 0;
        })) {}

  constexpr value_type operator[](simd_size_type I) const noexcept {
    return backend::get(Data, I) != 0;
  }

  constexpr basic_mask operator!() const noexcept {
    return basic_mask(detail::private_tag{}, backend::bit_not(Data));
  }
  constexpr lanes_type operator+() const noexcept {
    return detail::access::make<lanes_type>(backend::neg(Data));
  }
  constexpr lanes_type operator-() const noexcept {
    return detail::access::make<lanes_type>(Data);
  }
  constexpr lanes_type operator~() const noexcept {
    return detail::access::make<lanes_type>(
        backend::bit_not(backend::neg(Data)));
  }

#define CXX26_SIMD_MASK_BINARY(OP, NAME)                                       \
  friend constexpr basic_mask operator OP(const basic_mask &A,                 \
                                          const basic_mask &B) noexcept {      \
    return basic_mask(detail::private_tag{}, backend::NAME(A.Data, B.Data));   \
  }
  CXX26_SIMD_MASK_BINARY(&&, bit_and)
  CXX26_SIMD_MASK_BINARY(||, bit_or)
  CXX26_SIMD_MASK_BINARY(&, bit_and)
  CXX26_SIMD_MASK_BINARY(|, bit_or)
  CXX26_SIMD_MASK_BINARY(^, bit_xor)
#undef CXX26_SIMD_MASK_BINARY

  constexpr unsigned long long to_ullong() const noexcept {
    return backend::to_ullong(Data);
  }

private:
  friend struct detail::access;

  constexpr basic_mask(detail::private_tag, const storage_type &S) noexcept
      : Data(S) {}

  alignas(detail::storage_alignment<lane_type, N>) storage_type Data;
};

} // namespace simd
} // namespace cxx26
