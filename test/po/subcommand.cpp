// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "po/subcommand.h"
#include "po/argument_parser.h"
#include "po/option.h"
#include <array>
#include <gtest/gtest.h>
#include <vector>

using namespace WasmEdge::PO;
using namespace std::literals;

TEST(SubCommands, Simple1) {
  SubCommand S1(Description("s1"sv));
  SubCommand S2(Description("s2"sv));
  ArgumentParser Parser;
  Parser.begin_subcommand(S1, "s1"sv)
      .end_subcommand()
      .begin_subcommand(S2, "s2"sv)
      .end_subcommand();
  std::array Args = {"test"};
  EXPECT_TRUE(Parser.parse(stdout, static_cast<int>(Args.size()), Args.data()));
  EXPECT_FALSE(S1.is_selected());
  EXPECT_FALSE(S2.is_selected());
}

TEST(SubCommands, Simple2) {
  SubCommand S1(Description("s1"sv));
  SubCommand S2(Description("s2"sv));
  ArgumentParser Parser;
  Parser.begin_subcommand(S1, "s1"sv)
      .end_subcommand()
      .begin_subcommand(S2, "s2"sv)
      .end_subcommand();
  std::array Args = {"test", "s1"};
  EXPECT_TRUE(Parser.parse(stdout, static_cast<int>(Args.size()), Args.data()));
  EXPECT_TRUE(S1.is_selected());
  EXPECT_FALSE(S2.is_selected());
}

TEST(SubCommands, Simple3) {
  SubCommand S1(Description("s1"sv));
  SubCommand S2(Description("s2"sv));
  ArgumentParser Parser;
  Parser.begin_subcommand(S1, "s1"sv)
      .end_subcommand()
      .begin_subcommand(S2, "s2"sv)
      .end_subcommand();
  std::array Args = {"test", "s2"};
  EXPECT_TRUE(Parser.parse(stdout, static_cast<int>(Args.size()), Args.data()));
  EXPECT_FALSE(S1.is_selected());
  EXPECT_TRUE(S2.is_selected());
}

TEST(SubCommands, Simple4) {
  SubCommand S1(Description("s1"sv));
  SubCommand S2(Description("s2"sv));
  ArgumentParser Parser;
  Parser.begin_subcommand(S1, "s1"sv)
      .end_subcommand()
      .begin_subcommand(S2, "s2"sv)
      .end_subcommand();
  std::array Args = {"test", "s1", "s2"};
  EXPECT_FALSE(
      Parser.parse(stdout, static_cast<int>(Args.size()), Args.data()));
}

TEST(SubCommands, Nested1) {
  SubCommand S1(Description("s1"sv));
  SubCommand S2(Description("s2"sv));
  ArgumentParser Parser;
  Parser.begin_subcommand(S1, "s1"sv)
      .begin_subcommand(S2, "s2"sv)
      .end_subcommand()
      .end_subcommand();
  std::array Args = {"test", "s1", "s2"};
  EXPECT_TRUE(Parser.parse(stdout, static_cast<int>(Args.size()), Args.data()));
  EXPECT_TRUE(S1.is_selected());
  EXPECT_TRUE(S2.is_selected());
}

TEST(SubCommands, Nested2) {
  SubCommand S1(Description("s1"sv));
  SubCommand S2(Description("s2"sv));
  ArgumentParser Parser;
  Parser.begin_subcommand(S1, "s1"sv)
      .begin_subcommand(S2, "s2"sv)
      .end_subcommand()
      .end_subcommand();
  std::array Args = {"test", "s1"};
  EXPECT_TRUE(Parser.parse(stdout, static_cast<int>(Args.size()), Args.data()));
  EXPECT_TRUE(S1.is_selected());
  EXPECT_FALSE(S2.is_selected());
}

TEST(SubCommands, Nested3) {
  SubCommand S1(Description("s1"sv));
  SubCommand S2(Description("s2"sv));
  ArgumentParser Parser;
  Parser.begin_subcommand(S1, "s1"sv)
      .begin_subcommand(S2, "s2"sv)
      .end_subcommand()
      .end_subcommand();
  std::array Args = {"test", "s2"};
  EXPECT_FALSE(
      Parser.parse(stdout, static_cast<int>(Args.size()), Args.data()));
}

TEST(SubCommands, NestedOption1) {
  Option<Toggle> T1;
  Option<Toggle> T2;
  Option<Toggle> T3;
  SubCommand S1(Description("s1"sv));
  SubCommand S2(Description("s2"sv));
  ArgumentParser Parser;
  Parser.add_option("t1"sv, T1)
      .begin_subcommand(S1, "s1"sv)
      .add_option("t2"sv, T2)
      .begin_subcommand(S2, "s2"sv)
      .add_option("t3"sv, T3)
      .end_subcommand()
      .end_subcommand();
  std::array Args = {"test", "--t1", "s1"};
  EXPECT_TRUE(Parser.parse(stdout, static_cast<int>(Args.size()), Args.data()));
  EXPECT_TRUE(T1.value());
  EXPECT_FALSE(T2.value());
  EXPECT_FALSE(T3.value());
  EXPECT_TRUE(S1.is_selected());
}

TEST(SubCommands, NestedOption2) {
  Option<Toggle> T1;
  Option<Toggle> T2;
  Option<Toggle> T3;
  SubCommand S1(Description("s1"sv));
  SubCommand S2(Description("s2"sv));
  ArgumentParser Parser;
  Parser.add_option("t1"sv, T1)
      .begin_subcommand(S1, "s1"sv)
      .add_option("t2"sv, T2)
      .begin_subcommand(S2, "s2"sv)
      .add_option("t3"sv, T3)
      .end_subcommand()
      .end_subcommand();
  std::array Args = {"test", "s1", "--t1"};
  EXPECT_FALSE(
      Parser.parse(stdout, static_cast<int>(Args.size()), Args.data()));
}

TEST(SubCommands, NestedOption3) {
  Option<Toggle> T1;
  Option<Toggle> T2;
  Option<Toggle> T3;
  SubCommand S1(Description("s1"sv));
  SubCommand S2(Description("s2"sv));
  ArgumentParser Parser;
  Parser.add_option("t1"sv, T1)
      .begin_subcommand(S1, "s1"sv)
      .add_option("t2"sv, T2)
      .begin_subcommand(S2, "s2"sv)
      .add_option("t3"sv, T3)
      .end_subcommand()
      .end_subcommand();
  std::array Args = {"test", "s1"};
  EXPECT_TRUE(Parser.parse(stdout, static_cast<int>(Args.size()), Args.data()));
  EXPECT_FALSE(T1.value());
  EXPECT_FALSE(T2.value());
  EXPECT_FALSE(T3.value());
  EXPECT_TRUE(S1.is_selected());
}

TEST(SubCommands, NestedOption4) {
  Option<Toggle> T1;
  Option<Toggle> T2;
  Option<Toggle> T3;
  SubCommand S1(Description("s1"sv));
  SubCommand S2(Description("s2"sv));
  ArgumentParser Parser;
  Parser.add_option("t1"sv, T1)
      .begin_subcommand(S1, "s1"sv)
      .add_option("t2"sv, T2)
      .begin_subcommand(S2, "s2"sv)
      .add_option("t3"sv, T3)
      .end_subcommand()
      .end_subcommand();
  std::array Args = {"test", "s1", "--t2"};
  EXPECT_TRUE(Parser.parse(stdout, static_cast<int>(Args.size()), Args.data()));
  EXPECT_FALSE(T1.value());
  EXPECT_TRUE(T2.value());
  EXPECT_FALSE(T3.value());
  EXPECT_TRUE(S1.is_selected());
}

TEST(SubCommands, NestedOption5) {
  Option<Toggle> T1;
  Option<Toggle> T2;
  Option<Toggle> T3;
  SubCommand S1(Description("s1"sv));
  SubCommand S2(Description("s2"sv));
  ArgumentParser Parser;
  Parser.add_option("t1"sv, T1)
      .begin_subcommand(S1, "s1"sv)
      .add_option("t2"sv, T2)
      .begin_subcommand(S2, "s2"sv)
      .add_option("t3"sv, T3)
      .end_subcommand()
      .end_subcommand();
  std::array Args = {"test", "--t1", "s1", "--t2"};
  EXPECT_TRUE(Parser.parse(stdout, static_cast<int>(Args.size()), Args.data()));
  EXPECT_TRUE(T1.value());
  EXPECT_TRUE(T2.value());
  EXPECT_FALSE(T3.value());
  EXPECT_TRUE(S1.is_selected());
}
