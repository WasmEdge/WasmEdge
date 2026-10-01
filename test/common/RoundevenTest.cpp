// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "experimental/math.hpp"

#include <cmath>
#include <gtest/gtest.h>
#include <limits>

namespace {

TEST(RoundevenTest, Double) {
  // Test roundeven on double.
  //
  //   1.  Ties to even.
  //   2.  Non ties.
  //   3.  Signed zero preserved.
  //   4.  Special values.

  // Ties to even.
  EXPECT_EQ(cxx26::roundeven(0.5), 0.0);
  EXPECT_EQ(cxx26::roundeven(1.5), 2.0);
  EXPECT_EQ(cxx26::roundeven(2.5), 2.0);
  EXPECT_EQ(cxx26::roundeven(3.5), 4.0);
  EXPECT_EQ(cxx26::roundeven(4.5), 4.0);
  EXPECT_EQ(cxx26::roundeven(-1.5), -2.0);
  EXPECT_EQ(cxx26::roundeven(-2.5), -2.0);

  // Non ties.
  EXPECT_EQ(cxx26::roundeven(0.4), 0.0);
  EXPECT_EQ(cxx26::roundeven(0.6), 1.0);
  EXPECT_EQ(cxx26::roundeven(-0.4), 0.0);
  EXPECT_EQ(cxx26::roundeven(-0.6), -1.0);
  EXPECT_EQ(cxx26::roundeven(2.49), 2.0);
  EXPECT_EQ(cxx26::roundeven(2.51), 3.0);

  // Signed zero preserved.
  EXPECT_TRUE(std::signbit(cxx26::roundeven(-0.5)));
  EXPECT_TRUE(std::signbit(cxx26::roundeven(-0.0)));
  EXPECT_FALSE(std::signbit(cxx26::roundeven(0.5)));
  EXPECT_FALSE(std::signbit(cxx26::roundeven(0.0)));

  // Special values.
  {
    const double Inf = std::numeric_limits<double>::infinity();
    EXPECT_TRUE(
        std::isnan(cxx26::roundeven(std::numeric_limits<double>::quiet_NaN())));
    EXPECT_EQ(cxx26::roundeven(Inf), Inf);
    EXPECT_EQ(cxx26::roundeven(-Inf), -Inf);
    EXPECT_EQ(cxx26::roundeven(1e16), 1e16);
    EXPECT_EQ(cxx26::roundeven(-1e16), -1e16);
  }
}

TEST(RoundevenTest, Float) {
  // Test roundeven on float.
  //
  //   1.  Ties to even.
  //   2.  Non ties and specials.

  // Ties to even.
  EXPECT_EQ(cxx26::roundeven(0.5f), 0.0f);
  EXPECT_EQ(cxx26::roundeven(1.5f), 2.0f);
  EXPECT_EQ(cxx26::roundeven(2.5f), 2.0f);
  EXPECT_EQ(cxx26::roundeven(3.5f), 4.0f);
  EXPECT_EQ(cxx26::roundeven(-1.5f), -2.0f);
  EXPECT_EQ(cxx26::roundeven(-2.5f), -2.0f);

  // Non ties and specials.
  {
    EXPECT_EQ(cxx26::roundeven(0.4f), 0.0f);
    EXPECT_EQ(cxx26::roundeven(0.6f), 1.0f);
    EXPECT_EQ(cxx26::roundeven(-0.6f), -1.0f);
    EXPECT_TRUE(std::signbit(cxx26::roundeven(-0.5f)));
    const float Inf = std::numeric_limits<float>::infinity();
    EXPECT_TRUE(
        std::isnan(cxx26::roundeven(std::numeric_limits<float>::quiet_NaN())));
    EXPECT_EQ(cxx26::roundeven(Inf), Inf);
    EXPECT_EQ(cxx26::roundeven(-Inf), -Inf);
  }
}

} // namespace
