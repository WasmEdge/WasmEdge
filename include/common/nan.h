// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/common/nan.h - NaN helpers -------------------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains helpers for the NaN rules of the WebAssembly spec.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "experimental/math.hpp"

#include <cstdint>
#include <limits>
#include <type_traits>

namespace WasmEdge {

/// Unsigned integer with the same size as the floating-point type T.
template <typename T>
using FloatBits = std::conditional_t<sizeof(T) == 4, uint32_t, uint64_t>;

/// The most significant payload bit of T, which is set in a quiet NaN.
template <typename T>
inline constexpr FloatBits<T> QuietBit =
    FloatBits<T>{1} << (std::numeric_limits<T>::digits - 2);

/// Value with the most significant payload bit set when it is a NaN, which
/// makes it an arithmetic NaN. Non-NaN values are returned unchanged.
template <typename T> inline T quietNaN(T Value) noexcept {
  return cxx26::quiet_nan(Value);
}

} // namespace WasmEdge
