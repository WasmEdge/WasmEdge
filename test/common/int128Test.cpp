// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "common/int128.h"

#include <cstdint>
#include <gtest/gtest.h>
#include <sstream>

namespace {

TEST(Int128Test, Int128OutputTest) {
  using WasmEdge::operator<<;

  WasmEdge::uint128_t Value;
  std::ostringstream OS;

  {
    OS.str("");
    OS.clear();
    Value = 0;
    OS << Value;
    EXPECT_EQ(OS.str(), "0");
  }

  {
    OS.str("");
    OS.clear();
    Value = WasmEdge::uint128_t(1) << 69;
    OS << Value;
    EXPECT_EQ(OS.str(), "590295810358705651712");
  }

  {
    OS.str("");
    OS.clear();
    Value = WasmEdge::uint128_t(1) << 127;
    OS << Value;
    EXPECT_EQ(OS.str(), "170141183460469231731687303715884105728");
  }

  {
    OS.str("");
    OS.clear();
    Value = ~WasmEdge::uint128_t(0);
    OS << Value;
    EXPECT_EQ(OS.str(), "340282366920938463463374607431768211455");
  }

  {
    OS.str("");
    OS.clear();
    const WasmEdge::uint128_t P10(10000000000000);
    Value = 1234567890123 * P10 * P10 + 1234567890123;
    OS << Value;
    EXPECT_EQ(OS.str(), "123456789012300000000000001234567890123");
  }

  {
    OS.str("");
    OS.clear();
    const WasmEdge::uint128_t P10(10000000000000);
    Value = 1234567890123 * P10 * P10;
    OS << Value;
    EXPECT_EQ(OS.str(), "123456789012300000000000000000000000000");
  }

  {
    OS.str("");
    OS.clear();
    const WasmEdge::uint128_t P10(10000000000000);
    Value = 1234567890123 * P10 * P10 + 1234567890123 * P10;
    OS << Value;
    EXPECT_EQ(OS.str(), "123456789012312345678901230000000000000");
  }
}
} // namespace
