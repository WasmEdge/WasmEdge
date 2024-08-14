// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include <gtest/gtest.h>

#include "wasmedge/wasmedge.h"

namespace {

TEST(Val, Padding) {
  auto Size = sizeof(WasmEdge_Value);
  EXPECT_EQ(Size, 32);
}

} // namespace
