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
// Non-standard extensions. They are not part of C++26 std::simd.
///

#pragma once

#include "experimental/simd.hpp"

#include <type_traits>

namespace cxx26 {
namespace simd_ext {

using simd::simd_size_type;

template <class T, class Abi,
          std::enable_if_t<std::is_floating_point_v<T>, int> = 0>
simd::basic_vec<T, Abi> roundeven(const simd::basic_vec<T, Abi> &X) noexcept {
  return simd::detail::access::make<simd::basic_vec<T, Abi>>(
      simd::detail::simd_backend<T, Abi>::roundeven(
          simd::detail::access::data(X)));
}

template <class T, class Abi>
void replace_lane(simd::basic_vec<T, Abi> &Vec, simd_size_type I,
                  T Value) noexcept {
  simd::detail::simd_backend<T, Abi>::set_lane(simd::detail::access::data(Vec),
                                               I, Value);
}

} // namespace simd_ext
} // namespace cxx26
