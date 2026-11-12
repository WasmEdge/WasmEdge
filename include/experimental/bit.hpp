// SPDX-License-Identifier: CC0-1.0
///
// bit - A C++20 implementation of std::bit_cast
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

#include <cstring>
#include <type_traits>
#if __cplusplus > 201703L && __has_include(<bit>)
#include <bit>
#endif

#if defined(__cpp_lib_bit_cast) && __cpp_lib_bit_cast >= 201806L
namespace cxx20 {
using std::bit_cast;
} // namespace cxx20
#else

#if defined(__has_builtin)
#if __has_builtin(__builtin_bit_cast)
#define CXX20_HAS_BUILTIN_BIT_CAST 1
#endif
#endif
#if !defined(CXX20_HAS_BUILTIN_BIT_CAST) && defined(_MSC_VER) &&               \
    _MSC_VER >= 1927
#define CXX20_HAS_BUILTIN_BIT_CAST 1
#endif

namespace cxx20 {

template <class To, class From>
#if defined(CXX20_HAS_BUILTIN_BIT_CAST)
constexpr
#endif
    std::enable_if_t<sizeof(To) == sizeof(From) &&
                         std::is_trivially_copyable_v<To> &&
                         std::is_trivially_copyable_v<From>,
                     To> bit_cast(const From &Src) noexcept {
#if defined(CXX20_HAS_BUILTIN_BIT_CAST)
  return __builtin_bit_cast(To, Src);
#else
  static_assert(std::is_trivially_default_constructible_v<To>);
  To Dst;
  std::memcpy(static_cast<void *>(&Dst), &Src, sizeof(To));
  return Dst;
#endif
}

} // namespace cxx20

#undef CXX20_HAS_BUILTIN_BIT_CAST
#endif
