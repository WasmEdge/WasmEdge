// SPDX-License-Identifier: CC0-1.0
///
// assume - A C++17 implementation of the C++23 [[assume]] attribute
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

#include <cassert>

// Debug builds check the assumption. Release builds give it to the optimizer
// without evaluating it where the compiler allows. Clang's __builtin_assume
// is not used, because Clang ignores assumptions that call functions and
// warns with -Wassume.
#if !defined(NDEBUG)
#define CXX23_ASSUME(R) assert(R)
#endif
#if !defined(CXX23_ASSUME) && defined(__has_cpp_attribute)
#if __has_cpp_attribute(gnu::assume)
#define CXX23_ASSUME(R) [[gnu::assume(R)]]
#endif
#endif
#if !defined(CXX23_ASSUME) && defined(_MSC_VER) && !defined(__clang__)
#define CXX23_ASSUME(R)                                                        \
  (static_cast<bool>(R) ? static_cast<void>(0) : __assume(0))
#endif
#if !defined(CXX23_ASSUME)
#define CXX23_ASSUME(R)                                                        \
  (static_cast<bool>(R) ? static_cast<void>(0) : __builtin_unreachable())
#endif
