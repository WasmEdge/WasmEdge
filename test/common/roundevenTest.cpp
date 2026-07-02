// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "common/roundeven.h"

#include <cmath>
#include <gtest/gtest.h>
#include <limits>

namespace {
using namespace std::literals;

// WasmEdge::roundeven backs the f64.nearest / f32.nearest instructions
// (include/executor/engine/unary_numeric.ipp) and the SIMD lane rounding in
// include/executor/engine/simd_ops.h. The Wasm spec mandates round-to-nearest,
// ties-to-even (banker's rounding), and the helper has six platform-specific
// implementations (builtin, AVX512, AVX, SSE4.1, aarch64, and an fegetround
// fallback). A wrong path silently corrupts floating-point results, so these
// cases pin the contract on whichever path the current build selects.

TEST(RoundevenTest, DoubleTiesToEven) {
  EXPECT_EQ(WasmEdge::roundeven(0.5), 0.0);
  EXPECT_EQ(WasmEdge::roundeven(1.5), 2.0);
  EXPECT_EQ(WasmEdge::roundeven(2.5), 2.0);
  EXPECT_EQ(WasmEdge::roundeven(3.5), 4.0);
  EXPECT_EQ(WasmEdge::roundeven(4.5), 4.0);
  EXPECT_EQ(WasmEdge::roundeven(-1.5), -2.0);
  EXPECT_EQ(WasmEdge::roundeven(-2.5), -2.0);
}

TEST(RoundevenTest, DoubleNonTies) {
  EXPECT_EQ(WasmEdge::roundeven(0.4), 0.0);
  EXPECT_EQ(WasmEdge::roundeven(0.6), 1.0);
  EXPECT_EQ(WasmEdge::roundeven(-0.4), 0.0);
  EXPECT_EQ(WasmEdge::roundeven(-0.6), -1.0);
  EXPECT_EQ(WasmEdge::roundeven(2.49), 2.0);
  EXPECT_EQ(WasmEdge::roundeven(2.51), 3.0);
}

TEST(RoundevenTest, DoubleSignedZeroPreserved) {
  // -0.5 ties to even 0, but the IEEE round-to-integral result must keep the
  // negative sign (-0.0), as must rounding -0.0 itself.
  EXPECT_TRUE(std::signbit(WasmEdge::roundeven(-0.5)));
  EXPECT_TRUE(std::signbit(WasmEdge::roundeven(-0.0)));
  EXPECT_FALSE(std::signbit(WasmEdge::roundeven(0.5)));
  EXPECT_FALSE(std::signbit(WasmEdge::roundeven(0.0)));
}

TEST(RoundevenTest, DoubleSpecialValues) {
  const double Inf = std::numeric_limits<double>::infinity();
  EXPECT_TRUE(std::isnan(
      WasmEdge::roundeven(std::numeric_limits<double>::quiet_NaN())));
  EXPECT_EQ(WasmEdge::roundeven(Inf), Inf);
  EXPECT_EQ(WasmEdge::roundeven(-Inf), -Inf);
  // Magnitudes large enough to already be integral pass through unchanged.
  EXPECT_EQ(WasmEdge::roundeven(1e16), 1e16);
  EXPECT_EQ(WasmEdge::roundeven(-1e16), -1e16);
}

TEST(RoundevenTest, FloatTiesToEven) {
  EXPECT_EQ(WasmEdge::roundeven(0.5f), 0.0f);
  EXPECT_EQ(WasmEdge::roundeven(1.5f), 2.0f);
  EXPECT_EQ(WasmEdge::roundeven(2.5f), 2.0f);
  EXPECT_EQ(WasmEdge::roundeven(3.5f), 4.0f);
  EXPECT_EQ(WasmEdge::roundeven(-1.5f), -2.0f);
  EXPECT_EQ(WasmEdge::roundeven(-2.5f), -2.0f);
}

TEST(RoundevenTest, FloatNonTiesAndSpecials) {
  EXPECT_EQ(WasmEdge::roundeven(0.4f), 0.0f);
  EXPECT_EQ(WasmEdge::roundeven(0.6f), 1.0f);
  EXPECT_EQ(WasmEdge::roundeven(-0.6f), -1.0f);
  EXPECT_TRUE(std::signbit(WasmEdge::roundeven(-0.5f)));
  const float Inf = std::numeric_limits<float>::infinity();
  EXPECT_TRUE(
      std::isnan(WasmEdge::roundeven(std::numeric_limits<float>::quiet_NaN())));
  EXPECT_EQ(WasmEdge::roundeven(Inf), Inf);
  EXPECT_EQ(WasmEdge::roundeven(-Inf), -Inf);
}

} // namespace
