// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "system/path.h"
#include <gtest/gtest.h>

namespace {
TEST(PathTest, HomeDirectory) {
  std::filesystem::path Home = WasmEdge::Path::home();
  EXPECT_FALSE(Home.empty());
  // The home path should be an absolute path
  EXPECT_TRUE(Home.is_absolute());
}
} // namespace
