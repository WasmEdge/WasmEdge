// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "experimental/bit.hpp"

#include <array>
#include <cstdint>
#include <gtest/gtest.h>
#include <type_traits>
#include <utility>

namespace {

static_assert(cxx20::bit_cast<std::uint32_t>(1.0f) == UINT32_C(0x3f800000));
static_assert(cxx20::bit_cast<std::uint64_t>(-0.0) ==
              UINT64_C(0x8000000000000000));
static_assert(cxx20::bit_cast<float>(cxx20::bit_cast<std::uint32_t>(2.5f)) ==
              2.5f);

template <class To, class From, class = void>
struct CanBitCast : std::false_type {};
template <class To, class From>
struct CanBitCast<
    To, From, std::void_t<decltype(cxx20::bit_cast<To>(std::declval<From>()))>>
    : std::true_type {};

static_assert(CanBitCast<std::uint32_t, float>::value);
static_assert(!CanBitCast<std::uint64_t, std::uint32_t>::value);

struct NotTriviallyCopyable {
  NotTriviallyCopyable(const NotTriviallyCopyable &) {}
  std::uint32_t Value;
};
static_assert(!CanBitCast<std::uint32_t, NotTriviallyCopyable>::value);

TEST(BitCastTest, RoundTripsBytes) {
  const std::array<std::uint8_t, 8> Bytes = {0x00, 0x01, 0x02, 0x03,
                                             0x04, 0x05, 0x06, 0x07};
  const auto Value = cxx20::bit_cast<std::uint64_t>(Bytes);
  EXPECT_EQ((cxx20::bit_cast<std::array<std::uint8_t, 8>>(Value)), Bytes);
}

TEST(BitCastTest, PreservesNaNPayload) {
  const std::uint32_t Bits = UINT32_C(0x7fc00123);
  EXPECT_EQ(cxx20::bit_cast<std::uint32_t>(cxx20::bit_cast<float>(Bits)), Bits);
}

} // namespace
