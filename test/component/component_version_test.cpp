// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "validator/component_version.h"

#include <gtest/gtest.h>

namespace {

using namespace WasmEdge::Validator::ComponentVersion;
using namespace std::literals;

TEST(ComponentVersion, ParseSemVer) {
  // Valid semantic versions
  {
    auto ver = parseSemVer("1.2.3");
    ASSERT_TRUE(ver.has_value());
    EXPECT_EQ(ver->Major, 1u);
    EXPECT_EQ(ver->Minor, 2u);
    EXPECT_EQ(ver->Patch, 3u);
    EXPECT_EQ(ver->PreRelease, "");
    EXPECT_EQ(ver->BuildMetadata, "");
  }
  {
    auto ver = parseSemVer("0.2.6-rc.1");
    ASSERT_TRUE(ver.has_value());
    EXPECT_EQ(ver->Major, 0u);
    EXPECT_EQ(ver->Minor, 2u);
    EXPECT_EQ(ver->Patch, 6u);
    EXPECT_EQ(ver->PreRelease, "rc.1");
    EXPECT_EQ(ver->BuildMetadata, "");
  }
  {
    auto ver = parseSemVer("0.0.1-alpha+build.123");
    ASSERT_TRUE(ver.has_value());
    EXPECT_EQ(ver->Major, 0u);
    EXPECT_EQ(ver->Minor, 0u);
    EXPECT_EQ(ver->Patch, 1u);
    EXPECT_EQ(ver->PreRelease, "alpha");
    EXPECT_EQ(ver->BuildMetadata, "build.123");
  }
  {
    auto ver = parseSemVer("2.0.0+metadata");
    ASSERT_TRUE(ver.has_value());
    EXPECT_EQ(ver->Major, 2u);
    EXPECT_EQ(ver->Minor, 0u);
    EXPECT_EQ(ver->Patch, 0u);
    EXPECT_EQ(ver->PreRelease, "");
    EXPECT_EQ(ver->BuildMetadata, "metadata");
  }
  
  // Invalid semantic versions
  EXPECT_FALSE(parseSemVer("1.2").has_value());
  EXPECT_FALSE(parseSemVer("1").has_value());
  EXPECT_FALSE(parseSemVer("a.b.c").has_value());
  EXPECT_FALSE(parseSemVer("").has_value());
}

TEST(ComponentVersion, CanonicalizeSemVerStructure) {
  // major > 0: keep only major
  {
    SemVer ver{1, 2, 3, "", ""};
    auto [canon, suffix] = canonicalizeSemVer(ver);
    EXPECT_EQ(canon, "1");
    EXPECT_EQ(suffix, ".2.3");
  }
  {
    SemVer ver{2, 5, 10, "beta", "build"};
    auto [canon, suffix] = canonicalizeSemVer(ver);
    EXPECT_EQ(canon, "2");
    EXPECT_EQ(suffix, ".5.10-beta+build");
  }
  
  // major == 0, minor > 0: keep major.minor
  {
    SemVer ver{0, 2, 6, "rc.1", ""};
    auto [canon, suffix] = canonicalizeSemVer(ver);
    EXPECT_EQ(canon, "0.2");
    EXPECT_EQ(suffix, ".6-rc.1");
  }
  {
    SemVer ver{0, 5, 0, "", ""};
    auto [canon, suffix] = canonicalizeSemVer(ver);
    EXPECT_EQ(canon, "0.5");
    EXPECT_EQ(suffix, ".0");
  }
  
  // major == 0, minor == 0: keep major.minor.patch
  {
    SemVer ver{0, 0, 1, "alpha", ""};
    auto [canon, suffix] = canonicalizeSemVer(ver);
    EXPECT_EQ(canon, "0.0.1");
    EXPECT_EQ(suffix, "-alpha");
  }
  {
    SemVer ver{0, 0, 5, "", "metadata"};
    auto [canon, suffix] = canonicalizeSemVer(ver);
    EXPECT_EQ(canon, "0.0.5");
    EXPECT_EQ(suffix, "+metadata");
  }
}

TEST(ComponentVersion, CanonicalizeSemVerString) {
  // Examples from Component Model spec
  {
    auto [canon, suffix] = canonicalizeSemVer("1.2.3");
    EXPECT_EQ(canon, "1");
    EXPECT_EQ(suffix, ".2.3");
  }
  {
    auto [canon, suffix] = canonicalizeSemVer("0.2.6-rc.1");
    EXPECT_EQ(canon, "0.2");
    EXPECT_EQ(suffix, ".6-rc.1");
  }
  {
    auto [canon, suffix] = canonicalizeSemVer("0.0.1-alpha");
    EXPECT_EQ(canon, "0.0.1");
    EXPECT_EQ(suffix, "-alpha");
  }
  
  // Additional edge cases
  {
    auto [canon, suffix] = canonicalizeSemVer("10.0.0");
    EXPECT_EQ(canon, "10");
    EXPECT_EQ(suffix, ".0.0");
  }
  {
    auto [canon, suffix] = canonicalizeSemVer("0.99.1");
    EXPECT_EQ(canon, "0.99");
    EXPECT_EQ(suffix, ".1");
  }
}

TEST(ComponentVersion, VersionCompatibility) {
  // Compatible: exact match
  EXPECT_TRUE(areVersionsCompatible("1", "1"));
  EXPECT_TRUE(areVersionsCompatible("0.2", "0.2"));
  EXPECT_TRUE(areVersionsCompatible("0.0.1", "0.0.1"));
  
  // Incompatible: different canonical versions
  EXPECT_FALSE(areVersionsCompatible("1", "2"));
  EXPECT_FALSE(areVersionsCompatible("0.2", "0.3"));
  EXPECT_FALSE(areVersionsCompatible("0.0.1", "0.0.2"));
  
  // Incompatible: different granularity indicates different major versions
  EXPECT_FALSE(areVersionsCompatible("1", "0.2"));
  EXPECT_FALSE(areVersionsCompatible("0.2", "0.0.1"));
}

TEST(ComponentVersion, WAsiPreview2Examples) {
  // WASI Preview 2 examples
  {
    // wasi:http/types@0.2.1 and wasi:http/types@0.2.6 should both canonicalize to 0.2
    auto [canon1, suffix1] = canonicalizeSemVer("0.2.1");
    auto [canon2, suffix2] = canonicalizeSemVer("0.2.6");
    EXPECT_EQ(canon1, "0.2");
    EXPECT_EQ(canon2, "0.2");
    EXPECT_TRUE(areVersionsCompatible(canon1, canon2));
  }
  {
    // wasi:filesystem/types@0.2.0
    auto [canon, suffix] = canonicalizeSemVer("0.2.0");
    EXPECT_EQ(canon, "0.2");
    EXPECT_EQ(suffix, ".0");
  }
}

} // namespace
