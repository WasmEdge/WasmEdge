// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/common/endian.h - endian detect helper -------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the detection helper for endian.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "defines.h"
#include "int128.h"
#include <cstdint>
#include <type_traits>

#define WASMEDGE_ENDIAN_LITTLE_BYTE 0

// Windows is always little-endian
#if WASMEDGE_OS_WINDOWS
#undef WASMEDGE_ENDIAN_LITTLE_BYTE
#define WASMEDGE_ENDIAN_LITTLE_BYTE 1
#endif

// macOS including Apple silicon and Intel-based are always little-endian
#if WASMEDGE_OS_MACOS
#undef WASMEDGE_ENDIAN_LITTLE_BYTE
#define WASMEDGE_ENDIAN_LITTLE_BYTE 1
#endif

// We only handled little-endian on linux
#if WASMEDGE_OS_LINUX
#if defined(__LITTLE_ENDIAN__) || defined(__ARMEL__) ||                        \
    defined(__AARCH64EL__) ||                                                  \
    defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#undef WASMEDGE_ENDIAN_LITTLE_BYTE
#define WASMEDGE_ENDIAN_LITTLE_BYTE 1
#endif
#endif

namespace WasmEdge {
enum class Endian {
  little = 1,
  big = 0,
  native = WASMEDGE_ENDIAN_LITTLE_BYTE,
};

template <typename T> T byteswap(T V) {
  static_assert(std::is_integral_v<T>);
#if defined(__GNUC__) || defined(__INTEL_COMPILER) || defined(__clang__)
  if constexpr (sizeof(T) == 2) {
    return __builtin_bswap16(V);
  } else if constexpr (sizeof(T) == 4) {
    return __builtin_bswap32(V);
  } else if constexpr (sizeof(T) == 8) {
    return __builtin_bswap64(V);
  } else if constexpr (sizeof(T) == 16) {
    return (static_cast<__int128_t>(__builtin_bswap64(static_cast<uint64_t>(V)))
            << 64) |
           static_cast<__int128_t>(
               __builtin_bswap64(static_cast<uint64_t>(V >> 64)));
  }
#elif defined(_MSC_VER)
  if constexpr (sizeof(T) == 2) {
    return _byteswap_ushort(V);
  } else if constexpr (sizeof(T) == 4) {
    return _byteswap_ulong(V);
  } else if constexpr (sizeof(T) == 8) {
    return _byteswap_uint64(V);
  } else if constexpr (sizeof(T) == 16) {
    return (static_cast<uint128_t>(_byteswap_uint64(static_cast<uint64_t>(V)))
            << 64) |
           static_cast<uint128_t>(
               _byteswap_uint64(static_cast<uint64_t>(V >> 64)));
  }
#else
  if constexpr (sizeof(T) == 2) {
    return (((V >> 8) & 0xff) | ((V << 8) & 0xff00));
  } else if constexpr (sizeof(T) == 4) {
    return (((V >> 24) & 0xff) | ((V >> 8) & 0xff00) | ((V << 8) & 0xff0000) |
            ((V << 24) & 0xff000000));
  } else if constexpr (sizeof(T) == 8) {
    return (((V >> 56) & 0xff) | ((V >> 40) & 0xff00) | ((V >> 24) & 0xff0000) |
            ((V >> 8) & 0xff000000) | ((V << 8) & 0xff00000000) |
            ((V << 24) & 0xff0000000000) | ((V << 40) & 0xff000000000000) |
            ((V << 56) & 0xff00000000000000));
  } else if constexpr (sizeof(T) == 16) {
    return (static_cast<uint128_t>(byteswap(static_cast<uint64_t>(V))) << 64) |
           static_cast<uint128_t>(byteswap(static_cast<uint64_t>(V >> 64)));
  }
#endif
  return V;
}
} // namespace WasmEdge
