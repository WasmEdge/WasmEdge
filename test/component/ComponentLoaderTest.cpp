// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "validator/component_context.h"
#include "validator/component_name.h"

#include "ast/component/component.h"
#include "ast/component/section.h"
#include "ast/component/type.h"
#include "common/configure.h"
#include "common/errcode.h"
#include "loader/loader.h"

#include <gtest/gtest.h>

namespace {

using namespace WasmEdge;
using namespace std::literals;

std::vector<uint8_t> leb128U32(uint32_t N) {
  std::vector<uint8_t> Out;
  do {
    uint8_t B = static_cast<uint8_t>(N & 0x7F);
    N >>= 7;
    if (N != 0) {
      B = static_cast<uint8_t>(B | 0x80);
    }
    Out.push_back(B);
  } while (N != 0);
  return Out;
}

const std::vector<uint8_t> ComponentPreamble = {0x00, 0x61, 0x73, 0x6d,
                                                0x0d, 0x00, 0x01, 0x00};

std::vector<uint8_t> makeTypeSection(uint8_t SecId,
                                     const std::vector<uint8_t> &TypeBody) {
  std::vector<uint8_t> Payload = {0x01};
  Payload.insert(Payload.end(), TypeBody.begin(), TypeBody.end());
  std::vector<uint8_t> Vec = ComponentPreamble;
  Vec.push_back(SecId);
  const auto Size = leb128U32(static_cast<uint32_t>(Payload.size()));
  Vec.insert(Vec.end(), Size.begin(), Size.end());
  Vec.insert(Vec.end(), Payload.begin(), Payload.end());
  return Vec;
}

// Component type nested `Depth` levels: `41 01 01` per level, `41 00` last.
std::vector<uint8_t> makeNestedComponentType(uint32_t Depth) {
  std::vector<uint8_t> Body;
  for (uint32_t I = 0; I + 1 < Depth; ++I) {
    Body.insert(Body.end(), {0x41, 0x01, 0x01});
  }
  Body.insert(Body.end(), {0x41, 0x00});
  return makeTypeSection(0x07, Body);
}

// Core module type nested `Depth` levels: `50 01 01` per level, `50 00` last.
std::vector<uint8_t> makeNestedCoreModuleType(uint32_t Depth) {
  std::vector<uint8_t> Body;
  for (uint32_t I = 0; I + 1 < Depth; ++I) {
    Body.insert(Body.end(), {0x50, 0x01, 0x01});
  }
  Body.insert(Body.end(), {0x50, 0x00});
  return makeTypeSection(0x03, Body);
}

// `Depth` components nested via component sections (0x04), innermost empty.
std::vector<uint8_t> makeNestedComponents(uint32_t Depth) {
  std::vector<uint8_t> Body;
  for (uint32_t I = 0; I < Depth; ++I) {
    std::vector<uint8_t> Content = ComponentPreamble;
    Content.insert(Content.end(), Body.begin(), Body.end());
    std::vector<uint8_t> Sec = {0x04};
    const auto Size = leb128U32(static_cast<uint32_t>(Content.size()));
    Sec.insert(Sec.end(), Size.begin(), Size.end());
    Sec.insert(Sec.end(), Content.begin(), Content.end());
    Body = std::move(Sec);
  }
  std::vector<uint8_t> Vec = ComponentPreamble;
  Vec.insert(Vec.end(), Body.begin(), Body.end());
  return Vec;
}

TEST(ComponentNameParserTest, Parse) {
  {
    auto CName = Validator::ComponentName::parse("[constructor]my-class"sv);
    ASSERT_TRUE(CName.has_value());
    EXPECT_EQ(CName->getKind(), Validator::ComponentNameKind::Constructor);
    EXPECT_EQ(CName->getDetail().get<Validator::ConstructorDetail>().Label,
              "my-class"sv);
    EXPECT_EQ(CName->getNoTagName(), "my-class"sv);
    EXPECT_EQ(CName->getOriginalName(), "[constructor]my-class"sv);
  }
  {
    auto CName =
        Validator::ComponentName::parse("[method]my-resource.my-method"sv);
    ASSERT_TRUE(CName.has_value());
    EXPECT_EQ(CName->getKind(), Validator::ComponentNameKind::Method);
    EXPECT_EQ(CName->getDetail().get<Validator::MethodDetail>().Resource,
              "my-resource"sv);
    EXPECT_EQ(CName->getDetail().get<Validator::MethodDetail>().Method,
              "my-method"sv);
    EXPECT_EQ(CName->getNoTagName(), "my-resource.my-method"sv);
    EXPECT_EQ(CName->getOriginalName(), "[method]my-resource.my-method"sv);
  }
  {
    auto CName =
        Validator::ComponentName::parse("[static]my-resource.my-method"sv);
    ASSERT_TRUE(CName.has_value());
    EXPECT_EQ(CName->getKind(), Validator::ComponentNameKind::Static);
    EXPECT_EQ(CName->getDetail().get<Validator::StaticDetail>().Resource,
              "my-resource"sv);
    EXPECT_EQ(CName->getDetail().get<Validator::StaticDetail>().Method,
              "my-method"sv);
    EXPECT_EQ(CName->getNoTagName(), "my-resource.my-method"sv);
    EXPECT_EQ(CName->getOriginalName(), "[static]my-resource.my-method"sv);
  }
  {
    auto CName = Validator::ComponentName::parse(
        "name-space:a-label/projection-label@1.2.3"sv);
    ASSERT_TRUE(CName.has_value());
    EXPECT_EQ(CName->getKind(), Validator::ComponentNameKind::InterfaceType);
    auto &D = CName->getDetail().get<Validator::InterfaceDetail>();
    EXPECT_EQ(D.Namespace, "name-space"sv);
    EXPECT_EQ(D.Package, "a-label"sv);
    EXPECT_EQ(D.Interface, "projection-label"sv);
    EXPECT_EQ(D.Version, "1.2.3"sv);
  }
  // Invalid names
  EXPECT_FALSE(Validator::ComponentName::parse("[constructor]"sv));
  EXPECT_FALSE(Validator::ComponentName::parse("[method]"sv));
  EXPECT_FALSE(Validator::ComponentName::parse("[method]resource"sv));
  EXPECT_FALSE(Validator::ComponentName::parse("[unknown]label"sv));
  EXPECT_FALSE(Validator::ComponentName::parse(""sv));
  EXPECT_FALSE(Validator::ComponentName::parse("123-abc"sv));
  // Valid labels
  {
    auto CName = Validator::ComponentName::parse("simple-label"sv);
    ASSERT_TRUE(CName.has_value());
    EXPECT_EQ(CName->getKind(), Validator::ComponentNameKind::Label);
  }
  {
    auto CName = Validator::ComponentName::parse("A"sv);
    ASSERT_TRUE(CName.has_value());
    EXPECT_EQ(CName->getKind(), Validator::ComponentNameKind::Label);
  }
}

TEST(ComponentNameParserTest, KebabLabel) {
  // Valid mixed-case kebab labels
  EXPECT_TRUE(Validator::ComponentName::parse("a"sv));
  EXPECT_TRUE(Validator::ComponentName::parse("a1"sv));
  EXPECT_TRUE(Validator::ComponentName::parse("a-1"sv));
  EXPECT_TRUE(Validator::ComponentName::parse("a-1-b-2-c-3"sv));
  EXPECT_TRUE(Validator::ComponentName::parse("a-1-c"sv));
  EXPECT_TRUE(Validator::ComponentName::parse("abc-def-ghi"sv));
  EXPECT_TRUE(Validator::ComponentName::parse("A"sv));
  EXPECT_TRUE(Validator::ComponentName::parse("B"sv));
  EXPECT_TRUE(Validator::ComponentName::parse("B1"sv));
  EXPECT_TRUE(Validator::ComponentName::parse("B-1"sv));
  EXPECT_TRUE(Validator::ComponentName::parse("B-1-C-2-D-3"sv));
  EXPECT_TRUE(Validator::ComponentName::parse("ABC-DEF-GHI"sv));
  EXPECT_TRUE(Validator::ComponentName::parse("ABC-def-GHI"sv));
  EXPECT_TRUE(Validator::ComponentName::parse("ABC-123"sv));
  EXPECT_TRUE(Validator::ComponentName::parse("ABC123-G45H"sv));
  EXPECT_TRUE(Validator::ComponentName::parse("a11-B11-123-ABC-abc"sv));

  // Invalid kebab labels
  EXPECT_FALSE(Validator::ComponentName::parse("abcDefGhi"sv));   // camelCase
  EXPECT_FALSE(Validator::ComponentName::parse("abc_def_ghi"sv)); // snake_case
  EXPECT_FALSE(Validator::ComponentName::parse("abc def ghi"sv)); // spaces
  EXPECT_FALSE(
      Validator::ComponentName::parse("Abc-Fef-Ghi"sv)); // titleCase fragments
  EXPECT_FALSE(
      Validator::ComponentName::parse("ABC123-G45h"sv)); // mixed within acronym
  EXPECT_FALSE(Validator::ComponentName::parse("Abc--Ghi"sv)); // double hyphen
  EXPECT_FALSE(Validator::ComponentName::parse("Abc-"sv));  // trailing hyphen
  EXPECT_FALSE(Validator::ComponentName::parse("-Ghi"sv));  // leading hyphen
  EXPECT_FALSE(Validator::ComponentName::parse("1-abc"sv)); // starts with digit
  EXPECT_FALSE(Validator::ComponentName::parse(""sv));      // empty
  EXPECT_FALSE(Validator::ComponentName::parse("1"sv));     // digit only
  EXPECT_FALSE(Validator::ComponentName::parse("1-a"sv));   // digit start

  // Non-ASCII
  EXPECT_FALSE(Validator::ComponentName::parse("中文字"sv));
}

TEST(ComponentNameParserTest, StronglyUniqueBasicCases) {
  Validator::ComponentContext::Context Ctx(nullptr);

  auto add = [&](std::string_view S) -> bool {
    auto CN = Validator::ComponentName::parse(S);
    if (!CN.has_value()) {
      return false;
    }
    return Ctx.AddImportedName(*CN);
  };

  EXPECT_TRUE(add("foo"sv));
  EXPECT_TRUE(add("foo-bar"sv));
  EXPECT_TRUE(add("[constructor]foo"sv));
  EXPECT_TRUE(add("[method]foo.bar"sv));
  EXPECT_TRUE(add("[method]foo.baz"sv));

  EXPECT_FALSE(add("foo"sv));
  EXPECT_FALSE(add("foo-BAR"sv));
  EXPECT_FALSE(add("[constructor]foo-BAR"sv));
  EXPECT_FALSE(add("[method]foo.foo"sv));
  EXPECT_FALSE(add("[method]foo.BAR"sv));
}

TEST(ComponentNameParserTest, StronglyUnique) {
  Validator::ComponentContext::Context Ctx(nullptr);

  auto add = [&](std::string_view S) -> bool {
    auto CN = Validator::ComponentName::parse(S);
    if (!CN.has_value()) {
      return false;
    }
    return Ctx.AddImportedName(*CN);
  };

  EXPECT_TRUE(add("[method]foo.abc"sv));
  EXPECT_TRUE(add("[constructor]foo"sv));
  EXPECT_TRUE(add("foo-bar"sv));
  EXPECT_TRUE(add("foo"sv));

  EXPECT_FALSE(add("[method]foo"sv));
  EXPECT_FALSE(add("[static]foo.abc"sv));
}

TEST(ComponentNameParserTest, StronglyUniqueExportBasicCases) {
  // Mirrors StronglyUniqueBasicCases on the export-side name set: the
  // strong-uniqueness rule must apply symmetrically to import and export
  // name sets (Explainer §Import and Export Definitions).
  Validator::ComponentContext::Context Ctx(nullptr);

  auto add = [&](std::string_view S) -> bool {
    auto CN = Validator::ComponentName::parse(S);
    if (!CN.has_value()) {
      return false;
    }
    return Ctx.AddExportedName(*CN);
  };

  EXPECT_TRUE(add("foo"sv));
  EXPECT_TRUE(add("foo-bar"sv));
  EXPECT_TRUE(add("[constructor]foo"sv));
  EXPECT_TRUE(add("[method]foo.bar"sv));
  EXPECT_TRUE(add("[method]foo.baz"sv));

  EXPECT_FALSE(add("foo"sv));
  EXPECT_FALSE(add("foo-BAR"sv));
  EXPECT_FALSE(add("[constructor]foo-BAR"sv));
  EXPECT_FALSE(add("[method]foo.foo"sv));
  EXPECT_FALSE(add("[method]foo.BAR"sv));
}

TEST(ComponentNameParserTest, StronglyUniqueExport) {
  Validator::ComponentContext::Context Ctx(nullptr);

  auto add = [&](std::string_view S) -> bool {
    auto CN = Validator::ComponentName::parse(S);
    if (!CN.has_value()) {
      return false;
    }
    return Ctx.AddExportedName(*CN);
  };

  EXPECT_TRUE(add("[method]foo.abc"sv));
  EXPECT_TRUE(add("[constructor]foo"sv));
  EXPECT_TRUE(add("foo-bar"sv));
  EXPECT_TRUE(add("foo"sv));

  EXPECT_FALSE(add("[method]foo"sv));
  EXPECT_FALSE(add("[static]foo.abc"sv));
}

TEST(ComponentNameParserTest, StronglyUniqueImportExportIndependence) {
  // Spec: import and export name sets are checked *separately* — an import
  // and an export sharing a name is not a strong-uniqueness violation.
  Validator::ComponentContext::Context Ctx(nullptr);

  auto parse = [](std::string_view S) {
    return *Validator::ComponentName::parse(S);
  };

  EXPECT_TRUE(Ctx.AddImportedName(parse("foo"sv)));
  EXPECT_TRUE(Ctx.AddExportedName(parse("foo"sv)));
  EXPECT_FALSE(Ctx.AddImportedName(parse("foo"sv)));
  EXPECT_FALSE(Ctx.AddExportedName(parse("foo"sv)));
}

TEST(ComponentNameParserTest, LockedDep) {
  // Valid: no version, no integrity.
  {
    auto CName =
        Validator::ComponentName::parse("locked-dep=<my-registry:sqlite>"sv);
    ASSERT_TRUE(CName.has_value());
    EXPECT_EQ(CName->getKind(), Validator::ComponentNameKind::LockedDep);
    auto &D = CName->getDetail().get<Validator::LockedDepDetail>();
    EXPECT_EQ(D.Namespace, "my-registry"sv);
    EXPECT_EQ(D.Package, "sqlite"sv);
    EXPECT_EQ(D.Version, ""sv);
    EXPECT_EQ(D.Integrity, ""sv);
  }
  // Valid: with version.
  {
    auto CName = Validator::ComponentName::parse(
        "locked-dep=<my-registry:sqlite@1.2.3>"sv);
    ASSERT_TRUE(CName.has_value());
    EXPECT_EQ(CName->getKind(), Validator::ComponentNameKind::LockedDep);
    auto &D = CName->getDetail().get<Validator::LockedDepDetail>();
    EXPECT_EQ(D.Namespace, "my-registry"sv);
    EXPECT_EQ(D.Package, "sqlite"sv);
    EXPECT_EQ(D.Version, "1.2.3"sv);
    EXPECT_EQ(D.Integrity, ""sv);
  }
  // Valid: with version and integrity.
  {
    auto CName = Validator::ComponentName::parse(
        "locked-dep=<my-registry:sqlite@1.2.3>,integrity=<sha256-H8BRh8j>"sv);
    ASSERT_TRUE(CName.has_value());
    EXPECT_EQ(CName->getKind(), Validator::ComponentNameKind::LockedDep);
    auto &D = CName->getDetail().get<Validator::LockedDepDetail>();
    EXPECT_EQ(D.Namespace, "my-registry"sv);
    EXPECT_EQ(D.Package, "sqlite"sv);
    EXPECT_EQ(D.Version, "1.2.3"sv);
    EXPECT_EQ(D.Integrity, "sha256-H8BRh8j"sv);
  }
  // Invalid cases.
  EXPECT_FALSE(
      Validator::ComponentName::parse("locked-dep=my-registry:sqlite"sv));
  EXPECT_FALSE(Validator::ComponentName::parse("locked-dep=<MY-REG:sqlite>"sv));
  EXPECT_FALSE(Validator::ComponentName::parse("locked-dep=<:sqlite>"sv));
  EXPECT_FALSE(Validator::ComponentName::parse("locked-dep=<my-registry:>"sv));
  EXPECT_FALSE(Validator::ComponentName::parse(
      "locked-dep=<my-registry:sqlite>,integrity=<md5-abc>"sv));
}

TEST(ComponentNameParserTest, UnlockedDep) {
  // Valid: no verrange.
  {
    auto CName =
        Validator::ComponentName::parse("unlocked-dep=<my-registry:sqlite>"sv);
    ASSERT_TRUE(CName.has_value());
    EXPECT_EQ(CName->getKind(), Validator::ComponentNameKind::UnlockedDep);
    auto &D = CName->getDetail().get<Validator::UnlockedDepDetail>();
    EXPECT_EQ(D.Namespace, "my-registry"sv);
    EXPECT_EQ(D.Package, "sqlite"sv);
    EXPECT_EQ(D.VersionRange, ""sv);
  }
  // Valid: wildcard verrange.
  {
    auto CName = Validator::ComponentName::parse(
        "unlocked-dep=<my-registry:sqlite@*>"sv);
    ASSERT_TRUE(CName.has_value());
    EXPECT_EQ(CName->getKind(), Validator::ComponentNameKind::UnlockedDep);
    auto &D = CName->getDetail().get<Validator::UnlockedDepDetail>();
    EXPECT_EQ(D.Package, "sqlite"sv);
  }
  // Valid: lower bound.
  {
    auto CName = Validator::ComponentName::parse(
        "unlocked-dep=<my-registry:imagemagick@{>=1.0.0}>"sv);
    ASSERT_TRUE(CName.has_value());
    EXPECT_EQ(CName->getKind(), Validator::ComponentNameKind::UnlockedDep);
  }
  // Valid: upper bound.
  {
    auto CName = Validator::ComponentName::parse(
        "unlocked-dep=<my-registry:imagemagick@{<2.0.0}>"sv);
    ASSERT_TRUE(CName.has_value());
    EXPECT_EQ(CName->getKind(), Validator::ComponentNameKind::UnlockedDep);
  }
  // Valid: both bounds.
  {
    auto CName = Validator::ComponentName::parse(
        "unlocked-dep=<my-registry:imagemagick@{>=1.0.0 <2.0.0}>"sv);
    ASSERT_TRUE(CName.has_value());
    EXPECT_EQ(CName->getKind(), Validator::ComponentNameKind::UnlockedDep);
  }
  // Invalid cases.
  EXPECT_FALSE(
      Validator::ComponentName::parse("unlocked-dep=<MY-REG:sqlite>"sv));
  EXPECT_FALSE(
      Validator::ComponentName::parse("unlocked-dep=my-registry:sqlite"sv));
  EXPECT_FALSE(Validator::ComponentName::parse("unlocked-dep=<:sqlite>"sv));
  EXPECT_FALSE(
      Validator::ComponentName::parse("unlocked-dep=<my-registry:>"sv));
  EXPECT_FALSE(Validator::ComponentName::parse(
      "unlocked-dep=<my-registry:sqlite@{>=bad}>"sv));
}

TEST(ComponentNameParserTest, UrlName) {
  // Valid: simple URL.
  {
    auto CName = Validator::ComponentName::parse(
        "url=<https://mycdn.com/my-component.wasm>"sv);
    ASSERT_TRUE(CName.has_value());
    EXPECT_EQ(CName->getKind(), Validator::ComponentNameKind::Url);
    auto &D = CName->getDetail().get<Validator::UrlDetail>();
    EXPECT_EQ(D.Url, "https://mycdn.com/my-component.wasm"sv);
    EXPECT_EQ(D.Integrity, ""sv);
  }
  // Valid: URL with integrity.
  {
    auto CName = Validator::ComponentName::parse(
        "url=<./other-component.wasm>,integrity=<sha256-X9ArH3k>"sv);
    ASSERT_TRUE(CName.has_value());
    EXPECT_EQ(CName->getKind(), Validator::ComponentNameKind::Url);
    auto &D = CName->getDetail().get<Validator::UrlDetail>();
    EXPECT_EQ(D.Url, "./other-component.wasm"sv);
    EXPECT_EQ(D.Integrity, "sha256-X9ArH3k"sv);
  }
  // Valid: empty URL (nonbrackets = [^<>]*).
  {
    auto CName = Validator::ComponentName::parse("url=<>"sv);
    ASSERT_TRUE(CName.has_value());
    EXPECT_EQ(CName->getKind(), Validator::ComponentNameKind::Url);
  }
  // Invalid: no angle brackets.
  EXPECT_FALSE(Validator::ComponentName::parse("url=https://example.com"sv));
  // Invalid: bad integrity.
  EXPECT_FALSE(Validator::ComponentName::parse(
      "url=<https://example.com>,integrity=<md5-abc>"sv));
}

TEST(ComponentNameParserTest, IntegrityName) {
  // Valid.
  {
    auto CName = Validator::ComponentName::parse("integrity=<sha256-abc123>"sv);
    ASSERT_TRUE(CName.has_value());
    EXPECT_EQ(CName->getKind(), Validator::ComponentNameKind::Integrity);
    EXPECT_EQ(CName->getDetail().get<Validator::IntegrityDetail>().Integrity,
              "sha256-abc123"sv);
  }
  // Invalid: unsupported algorithm.
  EXPECT_FALSE(Validator::ComponentName::parse("integrity=<md5-abc>"sv));
  // Invalid: empty.
  EXPECT_FALSE(Validator::ComponentName::parse("integrity=<>"sv));
}

TEST(ComponentNameParserTest, Semver) {
  // Valid semver via interface names.
  EXPECT_TRUE(Validator::ComponentName::parse("ns:pkg/iface@1.2.3"sv));
  EXPECT_TRUE(Validator::ComponentName::parse("ns:pkg/iface@0.1.0"sv));
  EXPECT_TRUE(Validator::ComponentName::parse("ns:pkg/iface@1.2.3-beta.1"sv));
  EXPECT_TRUE(Validator::ComponentName::parse("ns:pkg/iface@1.2.3+build"sv));
  EXPECT_TRUE(
      Validator::ComponentName::parse("ns:pkg/iface@1.2.3-beta.1+build.42"sv));
  // Valid canonversion.
  EXPECT_TRUE(Validator::ComponentName::parse("ns:pkg/iface@1"sv));
  EXPECT_TRUE(Validator::ComponentName::parse("ns:pkg/iface@0.1"sv));
  EXPECT_TRUE(Validator::ComponentName::parse("ns:pkg/iface@0.0.1"sv));
  EXPECT_TRUE(Validator::ComponentName::parse("ns:pkg/iface@42"sv));
  // Invalid versions.
  EXPECT_FALSE(Validator::ComponentName::parse("ns:pkg/iface@01.2.3"sv));
  EXPECT_FALSE(Validator::ComponentName::parse("ns:pkg/iface@abc"sv));
  EXPECT_FALSE(Validator::ComponentName::parse("a:B/c"sv));
  EXPECT_FALSE(Validator::ComponentName::parse("ns:PKG/iface"sv));
  EXPECT_FALSE(Validator::ComponentName::parse("ns:Pkg/iface"sv));
  EXPECT_FALSE(Validator::ComponentName::parse("ns:pkg/iface@"sv));
  EXPECT_TRUE(Validator::ComponentName::parse("ns:pkg/iface@0.0.0"sv));
  EXPECT_FALSE(Validator::ComponentName::parse("ns:pkg/iface@0"sv));
}

TEST(ComponentNameParserTest, SpecExamples) {
  // Examples from Explainer.md
  EXPECT_EQ(Validator::ComponentName::parse("custom-hook"sv)->getKind(),
            Validator::ComponentNameKind::Label);
  EXPECT_EQ(Validator::ComponentName::parse("wasi:http/handler"sv)->getKind(),
            Validator::ComponentNameKind::InterfaceType);
  EXPECT_EQ(Validator::ComponentName::parse(
                "url=<https://mycdn.com/my-component.wasm>"sv)
                ->getKind(),
            Validator::ComponentNameKind::Url);
  EXPECT_EQ(Validator::ComponentName::parse(
                "url=<./other-component.wasm>,integrity=<sha256-X9ArH3k>"sv)
                ->getKind(),
            Validator::ComponentNameKind::Url);
  EXPECT_EQ(
      Validator::ComponentName::parse(
          "locked-dep=<my-registry:sqlite@1.2.3>,integrity=<sha256-H8BRh8j>"sv)
          ->getKind(),
      Validator::ComponentNameKind::LockedDep);
  EXPECT_EQ(Validator::ComponentName::parse(
                "unlocked-dep=<my-registry:imagemagick@{>=1.0.0}>"sv)
                ->getKind(),
            Validator::ComponentNameKind::UnlockedDep);
  EXPECT_EQ(Validator::ComponentName::parse("integrity=<sha256-Y3BsI4l>"sv)
                ->getKind(),
            Validator::ComponentNameKind::Integrity);
  EXPECT_EQ(Validator::ComponentName::parse("get-JSON"sv)->getKind(),
            Validator::ComponentNameKind::Label);
}

TEST(ComponentNameParserTest, StronglyUniqueWithNewKinds) {
  Validator::ComponentContext::Context Ctx(nullptr);

  auto add = [&](std::string_view S) -> bool {
    auto CN = Validator::ComponentName::parse(S);
    if (!CN.has_value()) {
      return false;
    }
    return Ctx.AddImportedName(*CN);
  };

  EXPECT_TRUE(add("foo"sv));
  EXPECT_TRUE(add("locked-dep=<my-registry:sqlite@1.2.3>"sv));
  EXPECT_TRUE(add("unlocked-dep=<my-registry:imagemagick@{>=1.0.0}>"sv));
  EXPECT_TRUE(add("url=<https://example.com/pkg.wasm>"sv));
  EXPECT_TRUE(add("integrity=<sha256-abc123>"sv));
  EXPECT_TRUE(add("wasi:http/handler"sv));
}

TEST(ComponentLoaderTest, AsyncFuncType) {
  WasmEdge::Configure Conf;
  Conf.addProposal(WasmEdge::Proposal::Component);
  WasmEdge::Loader::Loader Loader(Conf);

  // Component with 1 type section containing a 0x43 async functype with no
  // params and empty result:
  //   preamble + 0x07 0x05 (type sec, size 5) + 0x01 (1 type) +
  //   0x43 0x00 (async functype, 0 params) + 0x01 0x00 (empty result)
  std::vector<uint8_t> Vec = {
      0x00, 0x61, 0x73, 0x6d, 0x0d, 0x00, 0x01, 0x00, // preamble
      0x07, 0x05, 0x01, 0x43, 0x00, 0x01, 0x00,       // type section
  };

  auto Res = Loader.parseWasmUnit(Vec);
  ASSERT_TRUE(Res);
  auto *Comp =
      std::get_if<std::unique_ptr<WasmEdge::AST::Component::Component>>(&*Res);
  ASSERT_NE(Comp, nullptr);
  ASSERT_EQ((*Comp)->getSections().size(), 1U);
  const auto &Sec = std::get<WasmEdge::AST::Component::TypeSection>(
      (*Comp)->getSections()[0]);
  ASSERT_EQ(Sec.getContent().size(), 1U);
  ASSERT_TRUE(Sec.getContent()[0].isFuncType());
  EXPECT_TRUE(Sec.getContent()[0].getFuncType().isAsync());
}

TEST(ComponentLoaderTest, MalformedResultList) {
  WasmEdge::Configure Conf;
  Conf.addProposal(WasmEdge::Proposal::Component);
  WasmEdge::Loader::Loader Loader(Conf);

  // Same payload as func.10.wasm: resultlist flag 0x01 followed by 0x01
  // (must be 0x00 per current spec) — expect MalformedDefType.
  std::vector<uint8_t> Vec = {
      0x00, 0x61, 0x73, 0x6d, 0x0d, 0x00, 0x01, 0x00, // preamble
      0x07, 0x05, 0x01, 0x40, 0x00, 0x01, 0x01,       // type section
  };

  auto Res = Loader.parseWasmUnit(Vec);
  ASSERT_FALSE(Res);
  EXPECT_EQ(Res.error().getEnum(), WasmEdge::ErrCode::Value::MalformedDefType);
}

TEST(ComponentLoaderTest, I64ResourceRepNeedsMemory64) {
  WasmEdge::Configure Conf;
  Conf.addProposal(WasmEdge::Proposal::Component);
  // Default Configure enables Memory64; explicitly disable it for this test.
  Conf.removeProposal(WasmEdge::Proposal::Memory64);
  WasmEdge::Loader::Loader Loader(Conf);

  // Resource type 0x3f 0x7e (sync dtor, i64 rep) with no destructor.
  //   preamble + 0x07 0x04 (type sec, size 4) + 0x01 (1 type) +
  //   0x3f 0x7e 0x00 (resource sync, i64 rep, no dtor)
  std::vector<uint8_t> Vec = {
      0x00, 0x61, 0x73, 0x6d, 0x0d, 0x00, 0x01,
      0x00, 0x07, 0x04, 0x01, 0x3f, 0x7e, 0x00,
  };

  auto Res = Loader.parseWasmUnit(Vec);
  ASSERT_FALSE(Res);
  EXPECT_EQ(Res.error().getEnum(), WasmEdge::ErrCode::Value::MalformedDefType);
}

TEST(ComponentLoaderTest, I64ResourceRepWithMemory64) {
  WasmEdge::Configure Conf;
  Conf.addProposal(WasmEdge::Proposal::Component);
  // Memory64 proposal is enabled by default.
  WasmEdge::Loader::Loader Loader(Conf);

  // Same payload as the previous test.
  std::vector<uint8_t> Vec = {
      0x00, 0x61, 0x73, 0x6d, 0x0d, 0x00, 0x01,
      0x00, 0x07, 0x04, 0x01, 0x3f, 0x7e, 0x00,
  };

  auto Res = Loader.parseWasmUnit(Vec);
  ASSERT_TRUE(Res);
  auto *Comp =
      std::get_if<std::unique_ptr<WasmEdge::AST::Component::Component>>(&*Res);
  ASSERT_NE(Comp, nullptr);
  ASSERT_EQ((*Comp)->getSections().size(), 1U);
  const auto &Sec = std::get<WasmEdge::AST::Component::TypeSection>(
      (*Comp)->getSections()[0]);
  ASSERT_EQ(Sec.getContent().size(), 1U);
  ASSERT_TRUE(Sec.getContent()[0].isResourceType());
  EXPECT_TRUE(Sec.getContent()[0].getResourceType().isAddrI64());
}

TEST(ComponentLoaderTest, AsyncI64ResourceRepWithMemory64) {
  WasmEdge::Configure Conf;
  Conf.addProposal(WasmEdge::Proposal::Component);
  // Memory64 proposal is enabled by default.
  WasmEdge::Loader::Loader Loader(Conf);

  // Resource type 0x3e 0x7e (async dtor, i64 rep) with dtor funcidx 0 and no
  // callback:
  //   preamble + 0x07 0x05 (type sec, size 5) + 0x01 (1 type) +
  //   0x3e 0x7e 0x00 (resource async, i64 rep, dtor funcidx 0) +
  //   0x00          (no callback via loadOption absent marker)
  std::vector<uint8_t> Vec = {
      0x00, 0x61, 0x73, 0x6d, 0x0d, 0x00, 0x01, 0x00,
      0x07, 0x05, 0x01, 0x3e, 0x7e, 0x00, 0x00,
  };

  auto Res = Loader.parseWasmUnit(Vec);
  ASSERT_TRUE(Res);
  auto *Comp =
      std::get_if<std::unique_ptr<WasmEdge::AST::Component::Component>>(&*Res);
  ASSERT_NE(Comp, nullptr);
  ASSERT_EQ((*Comp)->getSections().size(), 1U);
  const auto &Sec = std::get<WasmEdge::AST::Component::TypeSection>(
      (*Comp)->getSections()[0]);
  const auto Content = Sec.getContent();
  ASSERT_EQ(Content.size(), 1U);
  ASSERT_TRUE(Content[0].isResourceType());
  const auto &RT = Content[0].getResourceType();
  EXPECT_TRUE(RT.isAddrI64());
  // For async dtor, the destructor funcidx is mandatory.
  ASSERT_TRUE(RT.getDestructor().has_value());
  EXPECT_EQ(*RT.getDestructor(), 0U);
  // Callback is absent in this payload.
  EXPECT_FALSE(RT.getCallback().has_value());
}

TEST(ComponentLoaderTest, DeeplyNestedComponentTypeRejected) {
  WasmEdge::Configure Conf;
  Conf.addProposal(WasmEdge::Proposal::Component);
  WasmEdge::Loader::Loader Loader(Conf);

  // Over the limit: rejected, not a stack overflow.
  auto Res = Loader.parseWasmUnit(makeNestedComponentType(1000));
  ASSERT_FALSE(Res);
  EXPECT_EQ(Res.error().getEnum(),
            WasmEdge::ErrCode::Value::ComponentNestLevelExceeded);
}

TEST(ComponentLoaderTest, ModeratelyNestedComponentTypeAccepted) {
  WasmEdge::Configure Conf;
  Conf.addProposal(WasmEdge::Proposal::Component);
  WasmEdge::Loader::Loader Loader(Conf);

  // Within the limit: still loads.
  auto Res = Loader.parseWasmUnit(makeNestedComponentType(200));
  ASSERT_TRUE(Res);
  EXPECT_NE(
      std::get_if<std::unique_ptr<WasmEdge::AST::Component::Component>>(&*Res),
      nullptr);
}

TEST(ComponentLoaderTest, DeeplyNestedCoreModuleTypeRejected) {
  WasmEdge::Configure Conf;
  Conf.addProposal(WasmEdge::Proposal::Component);
  WasmEdge::Loader::Loader Loader(Conf);

  // Core module types share the guard.
  auto Res = Loader.parseWasmUnit(makeNestedCoreModuleType(1000));
  ASSERT_FALSE(Res);
  EXPECT_EQ(Res.error().getEnum(),
            WasmEdge::ErrCode::Value::ComponentNestLevelExceeded);
}

TEST(ComponentLoaderTest, DeeplyNestedComponentRejected) {
  WasmEdge::Configure Conf;
  Conf.addProposal(WasmEdge::Proposal::Component);
  WasmEdge::Loader::Loader Loader(Conf);

  // Nested components share the guard.
  auto Res = Loader.parseWasmUnit(makeNestedComponents(300));
  ASSERT_FALSE(Res);
  EXPECT_EQ(Res.error().getEnum(),
            WasmEdge::ErrCode::Value::ComponentNestLevelExceeded);
}

} // namespace
