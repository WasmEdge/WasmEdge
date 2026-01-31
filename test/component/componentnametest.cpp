// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "validator/component_name.h"

#include <gtest/gtest.h>

namespace {

using namespace WasmEdge;
using namespace WasmEdge::Validator;

// Test SemVer validation via interface version parsing
TEST(ComponentNameTest, InterfaceVersionValidSemVer) {
  // Valid SemVer versions
  ComponentName Name1("wasi:http/types@1.0.0");
  EXPECT_EQ(Name1.getKind(), ComponentNameKind::InterfaceType);
  EXPECT_EQ(Name1.getDetails().Interface.Namespace, "wasi");
  EXPECT_EQ(Name1.getDetails().Interface.Package, "http");
  EXPECT_EQ(Name1.getDetails().Interface.Interface, "types");
  EXPECT_EQ(Name1.getDetails().Interface.Version, "1.0.0");

  // Valid with prerelease
  ComponentName Name2("wasi:http/types@1.0.0-alpha");
  EXPECT_EQ(Name2.getKind(), ComponentNameKind::InterfaceType);
  EXPECT_EQ(Name2.getDetails().Interface.Version, "1.0.0-alpha");

  // Valid with prerelease and build metadata
  ComponentName Name3("wasi:http/types@1.0.0-alpha.1+build.123");
  EXPECT_EQ(Name3.getKind(), ComponentNameKind::InterfaceType);
  EXPECT_EQ(Name3.getDetails().Interface.Version, "1.0.0-alpha.1+build.123");

  // Valid with build metadata only
  ComponentName Name4("wasi:http/types@2.3.4+20240101");
  EXPECT_EQ(Name4.getKind(), ComponentNameKind::InterfaceType);
  EXPECT_EQ(Name4.getDetails().Interface.Version, "2.3.4+20240101");

  // Valid with zero versions
  ComponentName Name5("wasi:http/types@0.0.0");
  EXPECT_EQ(Name5.getKind(), ComponentNameKind::InterfaceType);
  EXPECT_EQ(Name5.getDetails().Interface.Version, "0.0.0");

  // Valid with complex prerelease
  ComponentName Name6("wasi:http/types@1.0.0-rc.1.2.3");
  EXPECT_EQ(Name6.getKind(), ComponentNameKind::InterfaceType);
  EXPECT_EQ(Name6.getDetails().Interface.Version, "1.0.0-rc.1.2.3");
}

TEST(ComponentNameTest, InterfaceVersionInvalidSemVer) {
  // Invalid: leading zeros in major
  ComponentName Name1("wasi:http/types@01.0.0");
  EXPECT_EQ(Name1.getKind(), ComponentNameKind::Invalid);

  // Invalid: leading zeros in minor
  ComponentName Name2("wasi:http/types@1.01.0");
  EXPECT_EQ(Name2.getKind(), ComponentNameKind::Invalid);

  // Invalid: leading zeros in patch
  ComponentName Name3("wasi:http/types@1.0.01");
  EXPECT_EQ(Name3.getKind(), ComponentNameKind::Invalid);

  // Invalid: missing minor version
  ComponentName Name4("wasi:http/types@1.0");
  EXPECT_EQ(Name4.getKind(), ComponentNameKind::Invalid);

  // Invalid: missing patch version
  ComponentName Name5("wasi:http/types@1");
  EXPECT_EQ(Name5.getKind(), ComponentNameKind::Invalid);

  // Invalid: non-numeric major
  ComponentName Name6("wasi:http/types@a.0.0");
  EXPECT_EQ(Name6.getKind(), ComponentNameKind::Invalid);

  // Invalid: empty prerelease
  ComponentName Name7("wasi:http/types@1.0.0-");
  EXPECT_EQ(Name7.getKind(), ComponentNameKind::Invalid);

  // Invalid: empty build metadata
  ComponentName Name8("wasi:http/types@1.0.0+");
  EXPECT_EQ(Name8.getKind(), ComponentNameKind::Invalid);

  // Invalid: leading zero in numeric prerelease identifier
  ComponentName Name9("wasi:http/types@1.0.0-01");
  EXPECT_EQ(Name9.getKind(), ComponentNameKind::Invalid);

  // Invalid: double dots in prerelease
  ComponentName Name10("wasi:http/types@1.0.0-alpha..beta");
  EXPECT_EQ(Name10.getKind(), ComponentNameKind::Invalid);
}

// Test Hashname (integrity) parsing
TEST(ComponentNameTest, HashnameValidFormat) {
  // Valid hashname with SHA-256
  ComponentName Name1("integrity=<sha256-abc123def456>");
  EXPECT_EQ(Name1.getKind(), ComponentNameKind::Hash);
  EXPECT_EQ(Name1.getDetails().Hash.HashValue, "sha256-abc123def456");

  // Valid hashname with SHA-384
  ComponentName Name2("integrity=<sha384-xyz789uvw012>");
  EXPECT_EQ(Name2.getKind(), ComponentNameKind::Hash);
  EXPECT_EQ(Name2.getDetails().Hash.HashValue, "sha384-xyz789uvw012");

  // Valid hashname with SHA-512
  ComponentName Name3("integrity=<sha512-ABCDEF123456>");
  EXPECT_EQ(Name3.getKind(), ComponentNameKind::Hash);
  EXPECT_EQ(Name3.getDetails().Hash.HashValue, "sha512-ABCDEF123456");

  // Valid hashname with complex value
  ComponentName Name4("integrity=<sha256-47DEQpj8HBSa+/TImW+5JCeuQeRkm5NMpJWZG3hSuFU=>");
  EXPECT_EQ(Name4.getKind(), ComponentNameKind::Hash);
  EXPECT_EQ(Name4.getDetails().Hash.HashValue, "sha256-47DEQpj8HBSa+/TImW+5JCeuQeRkm5NMpJWZG3hSuFU=");
}

TEST(ComponentNameTest, HashnameInvalidFormat) {
  // Invalid: missing angle brackets
  ComponentName Name1("integrity=sha256-abc123");
  EXPECT_EQ(Name1.getKind(), ComponentNameKind::Invalid);

  // Invalid: missing opening bracket
  ComponentName Name2("integrity=sha256-abc123>");
  EXPECT_EQ(Name2.getKind(), ComponentNameKind::Invalid);

  // Invalid: missing closing bracket
  ComponentName Name3("integrity=<sha256-abc123");
  EXPECT_EQ(Name3.getKind(), ComponentNameKind::Invalid);

  // Invalid: empty integrity value
  ComponentName Name4("integrity=<>");
  EXPECT_EQ(Name4.getKind(), ComponentNameKind::Hash);
  EXPECT_EQ(Name4.getDetails().Hash.HashValue, "");
}

// Test that interface names without version still work
TEST(ComponentNameTest, InterfaceWithoutVersion) {
  ComponentName Name1("wasi:http/types");
  EXPECT_EQ(Name1.getKind(), ComponentNameKind::InterfaceType);
  EXPECT_EQ(Name1.getDetails().Interface.Namespace, "wasi");
  EXPECT_EQ(Name1.getDetails().Interface.Package, "http");
  EXPECT_EQ(Name1.getDetails().Interface.Interface, "types");
  EXPECT_EQ(Name1.getDetails().Interface.Version, "");
}

// Test existing functionality still works
TEST(ComponentNameTest, Constructor) {
  ComponentName Name1("[constructor]my-resource");
  EXPECT_EQ(Name1.getKind(), ComponentNameKind::Constructor);
  EXPECT_EQ(Name1.getDetails().Constructor.Label, "my-resource");
}

TEST(ComponentNameTest, Method) {
  ComponentName Name1("[method]my-resource.do-something");
  EXPECT_EQ(Name1.getKind(), ComponentNameKind::Method);
  EXPECT_EQ(Name1.getDetails().Method.Resource, "my-resource");
  EXPECT_EQ(Name1.getDetails().Method.Method, "do-something");
}

TEST(ComponentNameTest, Static) {
  ComponentName Name1("[static]my-resource.do-something");
  EXPECT_EQ(Name1.getKind(), ComponentNameKind::Static);
  EXPECT_EQ(Name1.getDetails().Static.Resource, "my-resource");
  EXPECT_EQ(Name1.getDetails().Static.Method, "do-something");
}

TEST(ComponentNameTest, Label) {
  ComponentName Name1("my-simple-label");
  EXPECT_EQ(Name1.getKind(), ComponentNameKind::Label);
  
  ComponentName Name2("kebab-case-label");
  EXPECT_EQ(Name2.getKind(), ComponentNameKind::Label);
}

// Edge cases
TEST(ComponentNameTest, EdgeCases) {
  // Empty string
  ComponentName Name1("");
  EXPECT_EQ(Name1.getKind(), ComponentNameKind::Invalid);

  // Just version marker
  ComponentName Name2("@");
  EXPECT_EQ(Name2.getKind(), ComponentNameKind::Invalid);

  // Interface with invalid version format
  ComponentName Name3("wasi:http/types@invalid");
  EXPECT_EQ(Name3.getKind(), ComponentNameKind::Invalid);
}

} // namespace
