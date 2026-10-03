// SPDX-License-Identifier: CC0-1.0
///
// math - A C++17 implementation of roundeven
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

#include "experimental/assume.hpp"

#include <cfenv>
#include <cmath>

#if defined(__has_builtin) && defined(__GLIBC_PREREQ)
#if __has_builtin(__builtin_roundeven) &&                                      \
    (__GLIBC_PREREQ(2, 26) || defined(__clang__))
#define CXX26_HAS_BUILTIN_ROUNDEVEN 1
#endif
#endif

namespace cxx26 {

inline float roundeven(float Value) noexcept {
#if defined(CXX26_HAS_BUILTIN_ROUNDEVEN)
  return __builtin_roundevenf(Value);
#elif defined(__AVX512F__)
  float Ret;
  __asm__("vrndscaless $8, %0, %1, %1" : "=v"(Ret) : "v"(Value));
  return Ret;
#elif defined(__AVX__)
  float Ret;
  __asm__("vroundss $8, %1, %1, %0" : "=v"(Ret) : "v"(Value));
  return Ret;
#elif defined(__SSE4_1__)
  float Ret;
  __asm__("roundss $8, %1, %0" : "=v"(Ret) : "v"(Value));
  return Ret;
#elif defined(__aarch64__)
  float Ret;
  __asm__("frintn %s0, %s1" : "=w"(Ret) : "w"(Value));
  return Ret;
#else
  CXX23_ASSUME(std::fegetround() == FE_TONEAREST);
  return std::nearbyint(Value);
#endif
}

inline double roundeven(double Value) noexcept {
#if defined(CXX26_HAS_BUILTIN_ROUNDEVEN)
  return __builtin_roundeven(Value);
#elif defined(__AVX512F__)
  double Ret;
  __asm__("vrndscalesd $8, %0, %1, %1" : "=v"(Ret) : "v"(Value));
  return Ret;
#elif defined(__AVX__)
  double Ret;
  __asm__("vroundsd $8, %1, %1, %0" : "=v"(Ret) : "v"(Value));
  return Ret;
#elif defined(__SSE4_1__)
  double Ret;
  __asm__("roundsd $8, %1, %0" : "=v"(Ret) : "v"(Value));
  return Ret;
#elif defined(__aarch64__)
  double Ret;
  __asm__("frintn %d0, %d1" : "=w"(Ret) : "w"(Value));
  return Ret;
#else
  CXX23_ASSUME(std::fegetround() == FE_TONEAREST);
  return std::nearbyint(Value);
#endif
}

} // namespace cxx26

#undef CXX26_HAS_BUILTIN_ROUNDEVEN
