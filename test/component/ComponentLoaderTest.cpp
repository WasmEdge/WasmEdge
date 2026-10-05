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

#include <optional>

namespace {

using namespace WasmEdge;
using namespace std::literals;

std::optional<Validator::Component::ExternName>
parseName(std::string_view Name) {
  Validator::Component::ExternName CN;
  if (!CN.parse(Name)) {
    return std::nullopt;
  }
  return CN;
}

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
    auto CName = parseName("[constructor]my-class"sv);
    ASSERT_TRUE(CName.has_value());
    EXPECT_EQ(CName->getKind(),
              Validator::Component::ExternName::Kind::Constructor);
    EXPECT_EQ(CName->getDetail().Resource, "my-class"sv);
    EXPECT_EQ(CName->getNoTagName(), "my-class"sv);
    EXPECT_EQ(CName->getOriginalName(), "[constructor]my-class"sv);
  }
  {
    auto CName = parseName("[method]my-resource.my-method"sv);
    ASSERT_TRUE(CName.has_value());
    EXPECT_EQ(CName->getKind(), Validator::Component::ExternName::Kind::Method);
    EXPECT_EQ(CName->getDetail().Resource, "my-resource"sv);
    EXPECT_EQ(CName->getDetail().Method, "my-method"sv);
    EXPECT_EQ(CName->getNoTagName(), "my-resource.my-method"sv);
    EXPECT_EQ(CName->getOriginalName(), "[method]my-resource.my-method"sv);
  }
  {
    auto CName = parseName("[static]my-resource.my-method"sv);
    ASSERT_TRUE(CName.has_value());
    EXPECT_EQ(CName->getKind(), Validator::Component::ExternName::Kind::Static);
    EXPECT_EQ(CName->getDetail().Resource, "my-resource"sv);
    EXPECT_EQ(CName->getDetail().Method, "my-method"sv);
    EXPECT_EQ(CName->getNoTagName(), "my-resource.my-method"sv);
    EXPECT_EQ(CName->getOriginalName(), "[static]my-resource.my-method"sv);
  }
  {
    auto CName = parseName("name-space:a-label/projection-label@1.2.3"sv);
    ASSERT_TRUE(CName.has_value());
    EXPECT_EQ(CName->getKind(),
              Validator::Component::ExternName::Kind::InterfaceType);
    auto &D = CName->getDetail();
    EXPECT_EQ(D.Namespace, "name-space"sv);
    EXPECT_EQ(D.Package, "a-label"sv);
    EXPECT_EQ(D.Interface, "projection-label"sv);
    EXPECT_EQ(D.Version, "1.2.3"sv);
  }
  // Invalid names
  EXPECT_FALSE(parseName("[constructor]"sv));
  EXPECT_FALSE(parseName("[method]"sv));
  EXPECT_FALSE(parseName("[method]resource"sv));
  EXPECT_FALSE(parseName("[unknown]label"sv));
  EXPECT_FALSE(parseName(""sv));
  EXPECT_FALSE(parseName("123-abc"sv));
  // Valid labels
  {
    auto CName = parseName("simple-label"sv);
    ASSERT_TRUE(CName.has_value());
    EXPECT_EQ(CName->getKind(), Validator::Component::ExternName::Kind::Label);
  }
  {
    auto CName = parseName("A"sv);
    ASSERT_TRUE(CName.has_value());
    EXPECT_EQ(CName->getKind(), Validator::Component::ExternName::Kind::Label);
  }
}

TEST(ComponentNameParserTest, KebabLabel) {
  // Valid mixed-case kebab labels
  EXPECT_TRUE(parseName("a"sv));
  EXPECT_TRUE(parseName("a1"sv));
  EXPECT_TRUE(parseName("a-1"sv));
  EXPECT_TRUE(parseName("a-1-b-2-c-3"sv));
  EXPECT_TRUE(parseName("a-1-c"sv));
  EXPECT_TRUE(parseName("abc-def-ghi"sv));
  EXPECT_TRUE(parseName("A"sv));
  EXPECT_TRUE(parseName("B"sv));
  EXPECT_TRUE(parseName("B1"sv));
  EXPECT_TRUE(parseName("B-1"sv));
  EXPECT_TRUE(parseName("B-1-C-2-D-3"sv));
  EXPECT_TRUE(parseName("ABC-DEF-GHI"sv));
  EXPECT_TRUE(parseName("ABC-def-GHI"sv));
  EXPECT_TRUE(parseName("ABC-123"sv));
  EXPECT_TRUE(parseName("ABC123-G45H"sv));
  EXPECT_TRUE(parseName("a11-B11-123-ABC-abc"sv));

  // Invalid kebab labels
  EXPECT_FALSE(parseName("abcDefGhi"sv));   // camelCase
  EXPECT_FALSE(parseName("abc_def_ghi"sv)); // snake_case
  EXPECT_FALSE(parseName("abc def ghi"sv)); // spaces
  EXPECT_FALSE(parseName("Abc-Fef-Ghi"sv)); // titleCase fragments
  EXPECT_FALSE(parseName("ABC123-G45h"sv)); // mixed within acronym
  EXPECT_FALSE(parseName("Abc--Ghi"sv));    // double hyphen
  EXPECT_FALSE(parseName("Abc-"sv));        // trailing hyphen
  EXPECT_FALSE(parseName("-Ghi"sv));        // leading hyphen
  EXPECT_FALSE(parseName("1-abc"sv));       // starts with digit
  EXPECT_FALSE(parseName(""sv));            // empty
  EXPECT_FALSE(parseName("1"sv));           // digit only
  EXPECT_FALSE(parseName("1-a"sv));         // digit start

  // Non-ASCII
  EXPECT_FALSE(parseName("中文字"sv));
}

TEST(ComponentNameParserTest, StronglyUniqueBasicCases) {
  Validator::Component::Context Ctx;
  Ctx.pushScope(Validator::Component::ScopeKind::Component);
  std::vector<Validator::Component::NameRecord> Names;

  auto add = [&](std::string_view S) -> bool {
    auto CN = parseName(S);
    if (!CN.has_value()) {
      return false;
    }
    return Ctx
        .addUniqueName(Names, Validator::Component::NameRecord(*CN), false)
        .has_value();
  };

  EXPECT_TRUE(add("foo"sv));
  EXPECT_TRUE(add("foo-bar"sv));
  EXPECT_TRUE(add("[constructor]foo"sv));
  EXPECT_TRUE(add("[method]foo.bar"sv));
  EXPECT_TRUE(add("[method]foo.baz"sv));

  EXPECT_FALSE(add("foo"sv));
  EXPECT_FALSE(add("foo-BAR"sv));
  EXPECT_FALSE(add("foobar"sv));
  EXPECT_FALSE(add("[constructor]FOO"sv));
  EXPECT_FALSE(add("[method]foo.foo"sv));
  EXPECT_FALSE(add("[method]foo.BAR"sv));
  EXPECT_FALSE(add("[static]foo-bar.FOOB-ar"sv));
}

TEST(ComponentNameParserTest, StronglyUnique) {
  Validator::Component::Context Ctx;
  Ctx.pushScope(Validator::Component::ScopeKind::Component);
  std::vector<Validator::Component::NameRecord> Names;

  auto add = [&](std::string_view S) -> bool {
    auto CN = parseName(S);
    if (!CN.has_value()) {
      return false;
    }
    return Ctx
        .addUniqueName(Names, Validator::Component::NameRecord(*CN), false)
        .has_value();
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
  Validator::Component::Context Ctx;
  Ctx.pushScope(Validator::Component::ScopeKind::Component);
  std::vector<Validator::Component::NameRecord> Names;

  auto add = [&](std::string_view S) -> bool {
    auto CN = parseName(S);
    if (!CN.has_value()) {
      return false;
    }
    return Ctx
        .addUniqueName(Names, Validator::Component::NameRecord(*CN), false)
        .has_value();
  };

  EXPECT_TRUE(add("foo"sv));
  EXPECT_TRUE(add("foo-bar"sv));
  EXPECT_TRUE(add("[constructor]foo"sv));
  EXPECT_TRUE(add("[method]foo.bar"sv));
  EXPECT_TRUE(add("[method]foo.baz"sv));

  EXPECT_FALSE(add("foo"sv));
  EXPECT_FALSE(add("foo-BAR"sv));
  EXPECT_FALSE(add("foobar"sv));
  EXPECT_FALSE(add("[constructor]FOO"sv));
  EXPECT_FALSE(add("[method]foo.foo"sv));
  EXPECT_FALSE(add("[method]foo.BAR"sv));
  EXPECT_FALSE(add("[static]foo-bar.FOOB-ar"sv));
}

TEST(ComponentNameParserTest, StronglyUniqueExport) {
  Validator::Component::Context Ctx;
  Ctx.pushScope(Validator::Component::ScopeKind::Component);
  std::vector<Validator::Component::NameRecord> Names;

  auto add = [&](std::string_view S) -> bool {
    auto CN = parseName(S);
    if (!CN.has_value()) {
      return false;
    }
    return Ctx
        .addUniqueName(Names, Validator::Component::NameRecord(*CN), false)
        .has_value();
  };

  EXPECT_TRUE(add("[method]foo.abc"sv));
  EXPECT_TRUE(add("[constructor]foo"sv));
  EXPECT_TRUE(add("foo-bar"sv));
  EXPECT_TRUE(add("foo"sv));

  EXPECT_FALSE(add("[method]foo"sv));
  EXPECT_FALSE(add("[static]foo.abc"sv));
}

TEST(ComponentNameParserTest, OnlyPlainAndInterfaceNames) {
  // externname ::= <plainname> | <interfacename>: the former dependency, url
  // and integrity forms are not extern names.
  EXPECT_FALSE(parseName("locked-dep=<my-registry:sqlite>"sv));
  EXPECT_FALSE(parseName("unlocked-dep=<my-registry:sqlite@*>"sv));
  EXPECT_FALSE(parseName("url=<https://mycdn.com/my-component.wasm>"sv));
  EXPECT_FALSE(parseName("integrity=<sha256-H8BRh8j>"sv));
}

TEST(ComponentNameParserTest, StronglyUniqueImportExportIndependence) {
  // Spec: import and export name sets are checked *separately* — an import
  // and an export sharing a name is not a strong-uniqueness violation.
  Validator::Component::Context Ctx;
  Ctx.pushScope(Validator::Component::ScopeKind::Component);
  std::vector<Validator::Component::NameRecord> Imports, Exports;

  auto add = [&Ctx](std::vector<Validator::Component::NameRecord> &Names,
                    std::string_view S) {
    return Ctx
        .addUniqueName(Names, Validator::Component::NameRecord(*parseName(S)),
                       false)
        .has_value();
  };

  EXPECT_TRUE(add(Imports, "foo"sv));
  EXPECT_TRUE(add(Exports, "foo"sv));
  EXPECT_FALSE(add(Imports, "foo"sv));
  EXPECT_FALSE(add(Exports, "foo"sv));
}

TEST(ComponentNameParserTest, Semver) {
  // Valid semver via interface names.
  EXPECT_TRUE(parseName("ns:pkg/iface@1.2.3"sv));
  EXPECT_TRUE(parseName("ns:pkg/iface@0.1.0"sv));
  EXPECT_TRUE(parseName("ns:pkg/iface@1.2.3-beta.1"sv));
  EXPECT_TRUE(parseName("ns:pkg/iface@1.2.3+build"sv));
  EXPECT_TRUE(parseName("ns:pkg/iface@1.2.3-beta.1+build.42"sv));
  // Valid canonversion.
  EXPECT_TRUE(parseName("ns:pkg/iface@1"sv));
  EXPECT_TRUE(parseName("ns:pkg/iface@0.1"sv));
  EXPECT_TRUE(parseName("ns:pkg/iface@0.0.1"sv));
  EXPECT_TRUE(parseName("ns:pkg/iface@42"sv));
  // Invalid versions.
  EXPECT_FALSE(parseName("ns:pkg/iface@01.2.3"sv));
  EXPECT_FALSE(parseName("ns:pkg/iface@abc"sv));
  EXPECT_FALSE(parseName("a:B/c"sv));
  EXPECT_FALSE(parseName("ns:PKG/iface"sv));
  EXPECT_FALSE(parseName("ns:Pkg/iface"sv));
  EXPECT_FALSE(parseName("ns:pkg/iface@"sv));
  EXPECT_TRUE(parseName("ns:pkg/iface@0.0.0"sv));
  EXPECT_FALSE(parseName("ns:pkg/iface@0"sv));
}

TEST(ComponentNameParserTest, SpecExamples) {
  // Examples from Explainer.md
  EXPECT_EQ(parseName("custom-hook"sv)->getKind(),
            Validator::Component::ExternName::Kind::Label);
  EXPECT_EQ(parseName("wasi:http/handler"sv)->getKind(),
            Validator::Component::ExternName::Kind::InterfaceType);
  EXPECT_EQ(parseName("get-JSON"sv)->getKind(),
            Validator::Component::ExternName::Kind::Label);
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

TEST(ComponentLoaderTest, ValueSection) {
  WasmEdge::Configure Conf;
  Conf.addProposal(WasmEdge::Proposal::Component);
  WasmEdge::Loader::Loader Loader(Conf);

  // Component with 1 value section containing one `bool true` value:
  //   0x0c = value section id (12), 0x04 = content size,
  //   0x01 = vec count (1 value), 0x7f = bool valtype,
  //   0x01 = len (1 byte), 0x01 = val(bool) true.
  std::vector<uint8_t> Vec = {
      0x00, 0x61, 0x73, 0x6d, 0x0d, 0x00, 0x01, 0x00, // preamble
      0x0c, 0x04, 0x01, 0x7f, 0x01, 0x01,             // value section
  };

  auto Res = Loader.parseWasmUnit(Vec);
  ASSERT_TRUE(Res);
  auto *Comp =
      std::get_if<std::unique_ptr<WasmEdge::AST::Component::Component>>(&*Res);
  ASSERT_NE(Comp, nullptr);
  ASSERT_EQ((*Comp)->getSections().size(), 1U);
  const auto &Sec = std::get<WasmEdge::AST::Component::ValueSection>(
      (*Comp)->getSections()[0]);
  ASSERT_EQ(Sec.getContent().size(), 1U);
  EXPECT_EQ(Sec.getContent()[0].getType().getCode(),
            WasmEdge::ComponentTypeCode::Bool);
  ASSERT_EQ(Sec.getContent()[0].getData().size(), 1U);
  EXPECT_EQ(Sec.getContent()[0].getData()[0], 0x01);
}

TEST(ComponentLoaderTest, ValueSectionLengthExceedsInput) {
  WasmEdge::Configure Conf;
  Conf.addProposal(WasmEdge::Proposal::Component);
  WasmEdge::Loader::Loader Loader(Conf);

  // Value section declaring a value whose length exceeds the input:
  //   0x0c = value section id (12), 0x00 = content size,
  //   0x01 = vec count (1 value), 0x20 = valtype (type index 32),
  //   0xffffffff0e = len (0xefffffff bytes).
  std::vector<uint8_t> Vec = {
      0x00, 0x61, 0x73, 0x6d, 0x0d, 0x00, 0x01, 0x00, // preamble
      0x0c, 0x00, 0x01, 0x20,                         // value section
      0xff, 0xff, 0xff, 0xff, 0x0e,                   // value length
  };

  auto Res = Loader.parseWasmUnit(Vec);
  ASSERT_FALSE(Res);
  EXPECT_EQ(Res.error().getEnum(), WasmEdge::ErrCode::Value::UnexpectedEnd);
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

TEST(ComponentLoaderTest, MalformedTaskReturnResultList) {
  WasmEdge::Configure Conf;
  Conf.addProposal(WasmEdge::Proposal::Component);
  WasmEdge::Loader::Loader Loader(Conf);

  // task.return takes the functype resultlist: after 0x01 only 0x00 follows.
  std::vector<uint8_t> Vec = {
      0x00, 0x61, 0x73, 0x6d, 0x0d, 0x00, 0x01, 0x00, // preamble
      0x08, 0x04, 0x01, 0x09, 0x01, 0x01,             // canon section
  };

  auto Res = Loader.parseWasmUnit(Vec);
  ASSERT_FALSE(Res);
  EXPECT_EQ(Res.error().getEnum(),
            WasmEdge::ErrCode::Value::MalformedCanonical);
}

TEST(ComponentLoaderTest, ContextTypeIsACoreValType) {
  WasmEdge::Configure Conf;
  Conf.addProposal(WasmEdge::Proposal::Component);
  WasmEdge::Loader::Loader Loader(Conf);

  // Component with one canon section holding `context.get i64 0` and
  // `context.set f32 0`. The type immediate is a core:valtype, so both
  // decode; rejecting anything but i32/i64 is a validation rule.
  std::vector<uint8_t> Vec = {
      0x00, 0x61, 0x73, 0x6d, 0x0d, 0x00, 0x01, 0x00, // preamble
      0x08, 0x07, 0x02, 0x0a, 0x7e, 0x00, 0x0b, 0x7d, // canon section
      0x00,
  };

  auto Res = Loader.parseWasmUnit(Vec);
  ASSERT_TRUE(Res);
  auto *Comp =
      std::get_if<std::unique_ptr<WasmEdge::AST::Component::Component>>(&*Res);
  ASSERT_NE(Comp, nullptr);
  ASSERT_EQ((*Comp)->getSections().size(), 1U);
  const auto &Sec = std::get<WasmEdge::AST::Component::CanonSection>(
      (*Comp)->getSections()[0]);
  ASSERT_EQ(Sec.getContent().size(), 2U);
  EXPECT_EQ(Sec.getContent()[0].getContextType(),
            WasmEdge::ValType(WasmEdge::TypeCode::I64));
  EXPECT_EQ(Sec.getContent()[1].getContextType(),
            WasmEdge::ValType(WasmEdge::TypeCode::F32));
}

TEST(ComponentLoaderTest, NameAttributesRejectDuplicateKinds) {
  WasmEdge::Configure Conf;
  Conf.addProposal(WasmEdge::Proposal::Component);
  WasmEdge::Loader::Loader Loader(Conf);

  // Component importing a func named "f" with two attributes of the given
  // kind: 0x00 implements, 0x01 versionsuffix, 0x02 external-id.
  auto Load = [&Loader](uint8_t Kind) {
    std::vector<uint8_t> Vec = {
        0x00, 0x61, 0x73, 0x6d, 0x0d, 0x00, 0x01, 0x00, // preamble
        0x07, 0x05, 0x01, 0x40, 0x00, 0x01, 0x00,       // functype at index 0
        0x0a, 0x0d, 0x01, 0x02, 0x01, 0x66, 0x02, Kind, // import section
        0x01, 0x61, Kind, 0x01, 0x62, 0x01, 0x00,
    };
    return Loader.parseWasmUnit(Vec);
  };

  auto Res = Load(0x00);
  ASSERT_FALSE(Res);
  EXPECT_EQ(Res.error(),
            WasmEdge::ErrCode::Value::ComponentImplementsDuplicate);
  Res = Load(0x01);
  ASSERT_FALSE(Res);
  EXPECT_EQ(Res.error(),
            WasmEdge::ErrCode::Value::ComponentVersionSuffixDuplicate);
  Res = Load(0x02);
  ASSERT_FALSE(Res);
  EXPECT_EQ(Res.error(),
            WasmEdge::ErrCode::Value::ComponentExternalIdDuplicate);
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

TEST(ComponentLoaderTest, UnallocatedResourceTypeOpcodeRejected) {
  WasmEdge::Configure Conf;
  Conf.addProposal(WasmEdge::Proposal::Component);
  WasmEdge::Loader::Loader Loader(Conf);

  // 0x3e is not allocated by the specification: `resourcetype` has only the
  // 0x3f form. An earlier draft spelled an async destructor here.
  //   preamble + 0x07 0x05 (type sec, size 5) + 0x01 (1 type) +
  //   0x3e 0x7e 0x00 0x00
  std::vector<uint8_t> Vec = {
      0x00, 0x61, 0x73, 0x6d, 0x0d, 0x00, 0x01, 0x00,
      0x07, 0x05, 0x01, 0x3e, 0x7e, 0x00, 0x00,
  };

  auto Res = Loader.parseWasmUnit(Vec);
  ASSERT_FALSE(Res);
  EXPECT_EQ(Res.error().getEnum(), WasmEdge::ErrCode::Value::MalformedDefType);
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

TEST(ComponentLoaderTest, NestedCoreModuleTypeRejected) {
  WasmEdge::Configure Conf;
  Conf.addProposal(WasmEdge::Proposal::Component);
  WasmEdge::Loader::Loader Loader(Conf);

  // A module type cannot declare a nested module type.
  auto Res = Loader.parseWasmUnit(makeNestedCoreModuleType(2));
  ASSERT_FALSE(Res);
  EXPECT_EQ(Res.error().getEnum(),
            WasmEdge::ErrCode::Value::MalformedModuleType);
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
