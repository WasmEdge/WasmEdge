// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "po/argument_parser.h"
#include "po/list.h"
#include "po/option.h"
#include "po/subcommand.h"
#include <array>
#include <gtest/gtest.h>
#include <string_view>
#include <vector>

using namespace WasmEdge::PO;
using namespace std::literals;

TEST(Version, Simple1) {
  SubCommand S1(Description("s1"sv));
  SubCommand S2(Description("s2"sv));
  Option<Toggle> A(Description("a"sv));
  Option<Toggle> B(Description("b"sv));
  ArgumentParser Parser;
  Parser.begin_subcommand(S1, "s1"sv)
      .add_option("a"sv, A)
      .end_subcommand()
      .begin_subcommand(S2, "s2"sv)
      .add_option("b"sv, B)
      .end_subcommand();
  std::array Args = {"test", "--version"};
  EXPECT_TRUE(Parser.parse(stdout, static_cast<int>(Args.size()), Args.data()));
  EXPECT_TRUE(Parser.isVersion());
  EXPECT_FALSE(Parser.isHelp());
}

TEST(Help, Simple1) {
  SubCommand S1(Description("s1"sv));
  SubCommand S2(Description("s2"sv));
  Option<Toggle> A;
  Option<Toggle> B;
  ArgumentParser Parser;
  Parser.begin_subcommand(S1, "s1"sv)
      .add_option("a"sv, A)
      .end_subcommand()
      .begin_subcommand(S2, "s2"sv)
      .add_option("b"sv, B)
      .end_subcommand();
  std::array Args = {"test", "--help"};
  EXPECT_TRUE(Parser.parse(stdout, static_cast<int>(Args.size()), Args.data()));
  EXPECT_FALSE(Parser.isVersion());
  EXPECT_TRUE(Parser.isHelp());
}

TEST(Help, Simple2) {
  SubCommand S1(Description("s1"sv));
  SubCommand S2(Description("s2"sv));
  Option<Toggle> A;
  Option<Toggle> B;
  ArgumentParser Parser;
  Parser.begin_subcommand(S1, "s1"sv)
      .add_option("a"sv, A)
      .end_subcommand()
      .begin_subcommand(S2, "s2"sv)
      .add_option("b"sv, B)
      .end_subcommand();
  std::array Args = {"test", "s1", "--help"};
  EXPECT_TRUE(Parser.parse(stdout, static_cast<int>(Args.size()), Args.data()));
  EXPECT_FALSE(Parser.isVersion());
  EXPECT_TRUE(Parser.isHelp());
}
