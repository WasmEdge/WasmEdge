// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/common/roundeven.h - rounding to nearest integer ------------===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contents the helper function for rounding to nearest integer.
///
//===----------------------------------------------------------------------===//
#pragma once

#include <cassert>
#include <cfenv>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <type_traits>

#ifndef __has_builtin
#define __has_builtin(x) 0
#endif

#if __GLIBC_PREREQ(2, 26) && __has_builtin(__builtin_roundeven)
#define HAVE_BUILTIN_ROUNDEVEN 1
#endif
#if !__has_builtin(__builtin_is_constant_evaluated)
#define __builtin_is_constant_evaluated() false
#endif

namespace SSVM {
namespace detail {

inline float roundevenf(float Value) {
#if defined(HAVE_BUILTIN_ROUNDEVEN)
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
#elif defined(__ARM_NEON__) || defined(__ARM_NEON) || defined(__ARM_NEON_FP)
  float Ret;
  __asm__("frintn %s0, %s0" : "=w"(Ret) : "w"(Value));
  return Ret;
#else
  assert(fegetround() == FE_TONEAREST);
  return std::nearbyint(Value);
#endif
}

inline double roundeven(double Value) noexcept {
#if defined(HAVE_BUILTIN_ROUNDEVEN)
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
#elif defined(__ARM_NEON__) || defined(__ARM_NEON) || defined(__ARM_NEON_FP)
  double Ret;
  __asm__("frintn %d0, %d0" : "=w"(Ret) : "w"(Value));
  return Ret;
#else
  assert(fegetround() == FE_TONEAREST);
  return std::nearbyint(Value);
#endif
}

} // namespace detail

using detail::roundeven;
inline float roundeven(float Value) { return detail::roundevenf(Value); }

} // namespace SSVM
