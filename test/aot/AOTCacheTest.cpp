// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/test/aot/AOTCacheTest.cpp - aot cache unit tests -------------===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contents unit tests of caching compiled WASM.
///
//===----------------------------------------------------------------------===//

#include "aot/cache.h"
#include "common/config.h"
#include "common/span.h"
#include "gtest/gtest.h"
#include <vector>

namespace {

using namespace std::literals::string_literals;
using namespace std::literals::string_view_literals;

TEST(CacheTest, GlobalEmpty) {
  const auto Path =
      SSVM::AOT::Cache::getPath({}, SSVM::AOT::Cache::StorageScope::Global);
  EXPECT_TRUE(Path);
  auto Root = *Path;
  while (Root.filename().u8string() != "ssvm"sv) {
    ASSERT_TRUE(Root.has_parent_path());
    Root = Root.parent_path();
  }
  std::error_code ErrCode;
  const auto Part = std::filesystem::proximate(*Path, Root, ErrCode);
  EXPECT_FALSE(ErrCode);
  EXPECT_EQ(
      Part.u8string(),
      "af1349b9f5f9a1a6a0404dea36dcc9499bcb25c9adc112b7cc9a93cae41f3262"s);
}

TEST(CacheTest, LocalEmpty) {
  const auto Path =
      SSVM::AOT::Cache::getPath({}, SSVM::AOT::Cache::StorageScope::Local);
  EXPECT_TRUE(Path);
  auto Root = *Path;
  while (Root.filename().u8string() != ".ssvm"sv) {
    ASSERT_TRUE(Root.has_parent_path());
    Root = Root.parent_path();
  }
  Root /= "cache"sv;
  std::error_code ErrCode;
  const auto Part = std::filesystem::proximate(*Path, Root, ErrCode);
  EXPECT_FALSE(ErrCode);
  EXPECT_EQ(
      Part.u8string(),
      "af1349b9f5f9a1a6a0404dea36dcc9499bcb25c9adc112b7cc9a93cae41f3262"s);
}

TEST(CacheTest, GlobalKey) {
  const auto Path = SSVM::AOT::Cache::getPath(
      {}, SSVM::AOT::Cache::StorageScope::Global, "key"s);
  EXPECT_TRUE(Path);
  auto Root = *Path;
  while (Root.filename().u8string() != "ssvm"sv) {
    ASSERT_TRUE(Root.has_parent_path());
    Root = Root.parent_path();
  }
  std::error_code ErrCode;
  const auto Part = std::filesystem::proximate(*Path, Root, ErrCode);
  EXPECT_FALSE(ErrCode);
  EXPECT_EQ(
      Part.u8string(),
      "key/af1349b9f5f9a1a6a0404dea36dcc9499bcb25c9adc112b7cc9a93cae41f3262"s);
}

TEST(CacheTest, LocalKey) {
  const auto Path = SSVM::AOT::Cache::getPath(
      {}, SSVM::AOT::Cache::StorageScope::Local, "key"s);
  EXPECT_TRUE(Path);
  auto Root = *Path;
  while (Root.filename().u8string() != ".ssvm"sv) {
    ASSERT_TRUE(Root.has_parent_path());
    Root = Root.parent_path();
  }
  Root /= "cache"sv;
  std::error_code ErrCode;
  const auto Part = std::filesystem::proximate(*Path, Root, ErrCode);
  EXPECT_FALSE(ErrCode);
  EXPECT_EQ(
      Part.u8string(),
      "key/af1349b9f5f9a1a6a0404dea36dcc9499bcb25c9adc112b7cc9a93cae41f3262"s);
}

} // namespace

GTEST_API_ int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
