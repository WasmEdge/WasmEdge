// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "ast/component/component.h"
#include "common/errinfo.h"
#include "validator/component_context.h"
#include "validator/component_name.h"
#include "validator/validator.h"
#include "vm/vm.h"

#include <gtest/gtest.h>

namespace {

using namespace WasmEdge;
using namespace std::literals;

TEST(ComponentNameParser, Functions) {
  {
    std::string_view Name = "[constructor]MyClass"sv;
    std::string_view Prefix = "[constructor]"sv;
    EXPECT_TRUE(Validator::ComponentNameParser::tryRead(Prefix, Name));
    spdlog::error("Remaining name: {}", Name);
    EXPECT_EQ(Name, "MyClass");
  }
  {
    std::string_view Name = "[constructor]MyClass"sv;
    std::string_view Prefix = "[fail]"sv;
    EXPECT_FALSE(Validator::ComponentNameParser::tryRead(Prefix, Name));
    EXPECT_EQ(Name, "[constructor]MyClass");
  }
}

TEST(ComponentNameParser, Kebab) {
  EXPECT_TRUE(Validator::ComponentNameParser::isKebabString("a"sv));
  EXPECT_TRUE(Validator::ComponentNameParser::isKebabString("A"sv));
  EXPECT_TRUE(Validator::ComponentNameParser::isKebabString("abc-def-ghi"sv));
  EXPECT_TRUE(Validator::ComponentNameParser::isKebabString("ABC-DEF-GHI"sv));
  EXPECT_TRUE(Validator::ComponentNameParser::isKebabString("ABC-def-GHI"sv));
  EXPECT_TRUE(Validator::ComponentNameParser::isKebabString("ABC-123"sv));
  EXPECT_TRUE(Validator::ComponentNameParser::isKebabString("ABC123-G45H"sv));

  EXPECT_FALSE(Validator::ComponentNameParser::isKebabString("abcDefGhi"sv));
  EXPECT_FALSE(Validator::ComponentNameParser::isKebabString("abc_def_ghi"sv));
  EXPECT_FALSE(Validator::ComponentNameParser::isKebabString("abc def ghi"sv));
  EXPECT_FALSE(Validator::ComponentNameParser::isKebabString("Abc-Fef-Ghi"sv));
  EXPECT_FALSE(Validator::ComponentNameParser::isKebabString("ABC123-G45h"sv));
  EXPECT_FALSE(Validator::ComponentNameParser::isKebabString("Abc--Ghi"sv));
  EXPECT_FALSE(Validator::ComponentNameParser::isKebabString("Abc-"sv));
  EXPECT_FALSE(Validator::ComponentNameParser::isKebabString("-Ghi"sv));
  EXPECT_FALSE(Validator::ComponentNameParser::isKebabString("1-abc"sv));
  EXPECT_FALSE(Validator::ComponentNameParser::isKebabString(""sv));
  EXPECT_FALSE(Validator::ComponentNameParser::isKebabString("中文字"sv));

  EXPECT_TRUE(
      Validator::ComponentNameParser::isLowercaseKebabString("abc-def-ghi"sv));
  EXPECT_TRUE(
      Validator::ComponentNameParser::isLowercaseKebabString("abc-123"sv));

  EXPECT_FALSE(
      Validator::ComponentNameParser::isLowercaseKebabString("aBc-def-ghi"sv));
  EXPECT_FALSE(
      Validator::ComponentNameParser::isLowercaseKebabString("ABC-def-ghi"sv));

  {
    std::string_view Input = "abc-def-ghi/rest-of-string"sv;
    std::string_view Output;
    EXPECT_TRUE(Validator::ComponentNameParser::tryReadKebab(Input, Output));
    EXPECT_EQ(Output, "abc-def-ghi"sv);
    EXPECT_EQ(Input, "/rest-of-string"sv);

    EXPECT_TRUE(Validator::ComponentNameParser::readUntil(Input, '/', Output));
    EXPECT_EQ(Input, "rest-of-string"sv);
    EXPECT_EQ(Output, ""sv);
    EXPECT_TRUE(Validator::ComponentNameParser::tryReadKebab(Input, Output));
    EXPECT_EQ(Output, "rest-of-string"sv);
    EXPECT_TRUE(Validator::ComponentNameParser::isEOF(Input));
  }
}

TEST(ComponentNameParser, Parse) {
  {
    std::string_view Name = "[constructor]my-class"sv;
    Validator::ComponentName CName(Name);
    EXPECT_EQ(CName.getKind(), Validator::ComponentNameKind::Constructor);
    EXPECT_EQ(CName.getDetails().Constructor.Label, "my-class"sv);
    EXPECT_EQ(CName.getNoTagName(), "my-class"sv);
    EXPECT_EQ(CName.getOriginalName(), "[constructor]my-class"sv);
  }
  {
    std::string_view Name = "[method]my-resource.my-method"sv;
    Validator::ComponentName CName(Name);
    EXPECT_EQ(CName.getKind(), Validator::ComponentNameKind::Method);
    EXPECT_EQ(CName.getDetails().Method.Resource, "my-resource"sv);
    EXPECT_EQ(CName.getDetails().Method.Method, "my-method"sv);
    EXPECT_EQ(CName.getNoTagName(), "my-resource.my-method"sv);
    EXPECT_EQ(CName.getOriginalName(), "[method]my-resource.my-method"sv);
  }
  {
    std::string_view Name = "[static]my-resource.my-method"sv;
    Validator::ComponentName CName(Name);
    EXPECT_EQ(CName.getKind(), Validator::ComponentNameKind::Static);
    EXPECT_EQ(CName.getDetails().Static.Resource, "my-resource"sv);
    EXPECT_EQ(CName.getDetails().Static.Method, "my-method"sv);
    EXPECT_EQ(CName.getNoTagName(), "my-resource.my-method"sv);
    EXPECT_EQ(CName.getOriginalName(), "[static]my-resource.my-method"sv);
  }
  {
    std::string_view Name = "name-space:a-label/projection-label@1.2.3"sv;
    Validator::ComponentName CName(Name);
    EXPECT_EQ(CName.getKind(), Validator::ComponentNameKind::InterfaceType);
    EXPECT_EQ(CName.getDetails().Interface.Namespace, "name-space"sv);
    EXPECT_EQ(CName.getDetails().Interface.Package, "a-label"sv);
    EXPECT_EQ(CName.getDetails().Interface.Interface, "projection-label"sv);
    EXPECT_EQ(CName.getDetails().Interface.Projection, ""sv);
    EXPECT_EQ(CName.getDetails().Interface.Version, "1.2.3"sv);
  }
}

TEST(ComponentNameParser, StronglyUnique1) {
  using namespace Validator;
  ComponentContext::Context Ctx(nullptr);

  auto add = [&](std::string_view S) {
    ComponentName CN(S);
    return Ctx.AddImportedName(CN);
  };

  // Accept set: all should be strongly-unique together.
  EXPECT_TRUE(add("foo"sv));
  EXPECT_TRUE(add("foo-bar"sv));
  EXPECT_TRUE(add("[constructor]foo"sv));
  EXPECT_TRUE(add("[method]foo.bar"sv));
  EXPECT_TRUE(add("[method]foo.baz"sv));

  // Reject additions against the accepted set above.
  // Duplicate label
  EXPECT_FALSE(add("foo"sv));
  // Normalized duplicate of kebab label (foo-BAR -> foo-bar)
  EXPECT_FALSE(add("foo-BAR"sv));
  // Normalized duplicate of constructor label
  EXPECT_FALSE(add("[constructor]foo-BAR"sv));
  // l vs [*]l.l conflict
  EXPECT_FALSE(add("[method]foo.foo"sv));
  // Normalized duplicate of method (foo.BAR -> foo.bar)
  EXPECT_FALSE(add("[method]foo.BAR"sv));
}

TEST(ComponentNameParser, StronglyUnique) {
  using namespace Validator;
  ComponentContext::Context Ctx(nullptr);

  auto add = [&](std::string_view S) {
    ComponentName CN(S);
    return Ctx.AddImportedName(CN);
  };

  // Accept set: all should be strongly-unique together.
  EXPECT_TRUE(add("[method]foo.abc"sv));
  EXPECT_TRUE(add("[constructor]foo"sv));
  EXPECT_TRUE(add("foo-bar"sv));
  EXPECT_TRUE(add("foo"sv));

  // Reject additions against the accepted set above.
  EXPECT_FALSE(add("[method]foo"sv));
  EXPECT_FALSE(add("[static]foo.abc"sv));
}

} // namespace
