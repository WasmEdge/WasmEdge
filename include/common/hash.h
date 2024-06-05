// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//===-- wasmedge/common/hash.h - Fast hash function -----------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the functions about hashing data.
/// Currently using rapidhash.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/defines.h"
#include "common/endian.h"
#include "common/errcode.h"
#include "common/int128.h"
#include "common/span.h"
#include "common/variant.h"

#include <array>
#include <cstdint>
#include <random>
#include <string_view>
#include <type_traits>

namespace WasmEdge::Hash {
inline constexpr void rapidMum(uint64_t &A, uint64_t &B) noexcept {
  uint128_t R = A;
  R *= B;
  A = static_cast<uint64_t>(R);
  B = static_cast<uint64_t>(R >> 64);
}
inline constexpr uint64_t rapidMix(uint64_t A, uint64_t B) noexcept {
  rapidMum(A, B);
  return A ^ B;
}

struct RandomEngine {
  using result_type = uint64_t;
  RandomEngine() noexcept {
    std::random_device RD;
    std::uniform_int_distribution<uint64_t> Dist(0, UINT64_MAX);
    Seed = Dist(RD);
  }
  RandomEngine(result_type S) noexcept : Seed(S) {}
  static inline constexpr result_type min() noexcept { return 0; }
  static inline constexpr result_type max() noexcept { return UINT64_MAX; }
  result_type operator()() noexcept {
    Seed += 0x2d358dccaa6c78a5ull;
    return rapidMix(Seed, Seed ^ 0x8bb84b93962eacc9ull);
  }
  result_type Seed;
};

static inline thread_local RandomEngine RandEngine;

struct Hash {
  WASMEDGE_EXPORT static uint64_t
  rapidHash(Span<const std::byte> Data) noexcept;

  template <typename CharT>
  inline uint64_t operator()(const CharT *Str) const noexcept {
    std::basic_string_view<CharT> View(Str);
    Span<const CharT> S(View.data(), View.size());
    return rapidHash(cxx20::as_bytes(S));
  }
  template <typename CharT>
  inline uint64_t
  operator()(const std::basic_string<CharT> &Str) const noexcept {
    Span<const CharT> S(Str.data(), Str.size());
    return rapidHash(cxx20::as_bytes(S));
  }
  template <typename CharT>
  inline uint64_t
  operator()(const std::basic_string_view<CharT> &Str) const noexcept {
    Span<const CharT> S(Str.data(), Str.size());
    return rapidHash(cxx20::as_bytes(S));
  }
  template <typename T, std::enable_if_t<std::is_integral_v<std::remove_cv_t<
                            std::remove_reference_t<T>>>> * = nullptr>
  inline uint64_t operator()(const T &Value) const noexcept {
    RandomEngine E(Value);
    return E();
  }
};

} // namespace WasmEdge::Hash
