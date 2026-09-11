// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/common/roundeven.h - rounding to the nearest integer -----===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the helper function for rounding to the nearest integer.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "errcode.h"

#include <cfenv>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <type_traits>

#ifndef __has_builtin
#define __has_builtin(x) 0
#endif

#if defined(__GLIBC_PREREQ)
#if __GLIBC_PREREQ(2, 26) && __has_builtin(__builtin_roundeven)
#define HAVE_BUILTIN_ROUNDEVEN 1
#elif defined(__clang__) && __has_builtin(__builtin_roundeven)
#define HAVE_BUILTIN_ROUNDEVEN 1
#endif
#endif
#if !__has_builtin(__builtin_is_constant_evaluated)
#define __builtin_is_constant_evaluated() false
#endif

namespace WasmEdge {
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
#elif defined(__aarch64__)
  float Ret;
  __asm__("frintn %s0, %s1" : "=w"(Ret) : "w"(Value));
  return Ret;
#else
  assuming(fegetround() == FE_TONEAREST);
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
#elif defined(__aarch64__)
  double Ret;
  __asm__("frintn %d0, %d1" : "=w"(Ret) : "w"(Value));
  return Ret;
#else
  assuming(fegetround() == FE_TONEAREST);
  return std::nearbyint(Value);
#endif
}

} // namespace detail

using detail::roundeven;
inline float roundeven(float Value) { return detail::roundevenf(Value); }

/// Sets the most significant bit of a NaN payload to 1 and leaves other values
/// as it is. Operators propagating a NaN operand, and the libm ceil, floor,
/// and trunc for 32-bit floats on some platforms, can produce a signaling NaN,
/// while the specification requires an arithmetic NaN.
template <typename T> inline T quietNaN(T Value) noexcept {
  static_assert(std::is_floating_point_v<T>);
  if (!std::isnan(Value)) {
    return Value;
  }
  if constexpr (sizeof(T) == sizeof(uint32_t)) {
    uint32_t I32;
    std::memcpy(&I32, &Value, sizeof(T));
    I32 |= static_cast<uint32_t>(0x01U) << 22;
    std::memcpy(&Value, &I32, sizeof(T));
  } else if constexpr (sizeof(T) == sizeof(uint64_t)) {
    uint64_t I64;
    std::memcpy(&I64, &Value, sizeof(T));
    I64 |= static_cast<uint64_t>(0x01U) << 51;
    std::memcpy(&Value, &I64, sizeof(T));
  }
  return Value;
}

} // namespace WasmEdge
