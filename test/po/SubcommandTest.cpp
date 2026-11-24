// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "po/subcommand.h"
#include "po/argument_parser.h"
#include "po/option.h"
#include <array>
#include <gtest/gtest.h>
#include <vector>

using namespace WasmEdge::PO;
using namespace std::literals;

TEST(SubcommandTest, Siblings) {
  // Test sibling subcommands.
  //
  //   1.  None selected.
  //   2.  First selected.
  //   3.  Second selected.
  //   4.  Both rejected.

  // None selected.
  {
    SubCommand S1(Description("s1"sv));
    SubCommand S2(Description("s2"sv));
    ArgumentParser Parser;
    Parser.begin_subcommand(S1, "s1"sv)
        .end_subcommand()
        .begin_subcommand(S2, "s2"sv)
        .end_subcommand();
    std::array Args = {"test"};
    EXPECT_TRUE(
        Parser.parse(stdout, static_cast<int>(Args.size()), Args.data()));
    EXPECT_FALSE(S1.is_selected());
    EXPECT_FALSE(S2.is_selected());
  }

  // First selected.
  {
    SubCommand S1(Description("s1"sv));
    SubCommand S2(Description("s2"sv));
    ArgumentParser Parser;
    Parser.begin_subcommand(S1, "s1"sv)
        .end_subcommand()
        .begin_subcommand(S2, "s2"sv)
        .end_subcommand();
    std::array Args = {"test", "s1"};
    EXPECT_TRUE(
        Parser.parse(stdout, static_cast<int>(Args.size()), Args.data()));
    EXPECT_TRUE(S1.is_selected());
    EXPECT_FALSE(S2.is_selected());
  }

  // Second selected.
  {
    SubCommand S1(Description("s1"sv));
    SubCommand S2(Description("s2"sv));
    ArgumentParser Parser;
    Parser.begin_subcommand(S1, "s1"sv)
        .end_subcommand()
        .begin_subcommand(S2, "s2"sv)
        .end_subcommand();
    std::array Args = {"test", "s2"};
    EXPECT_TRUE(
        Parser.parse(stdout, static_cast<int>(Args.size()), Args.data()));
    EXPECT_FALSE(S1.is_selected());
    EXPECT_TRUE(S2.is_selected());
  }

  // Both rejected.
  {
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
}

TEST(SubcommandTest, Nested) {
  // Test nested subcommands.
  //
  //   1.  Both selected.
  //   2.  Outer only.
  //   3.  Inner alone rejected.

  // Both selected.
  {
    SubCommand S1(Description("s1"sv));
    SubCommand S2(Description("s2"sv));
    ArgumentParser Parser;
    Parser.begin_subcommand(S1, "s1"sv)
        .begin_subcommand(S2, "s2"sv)
        .end_subcommand()
        .end_subcommand();
    std::array Args = {"test", "s1", "s2"};
    EXPECT_TRUE(
        Parser.parse(stdout, static_cast<int>(Args.size()), Args.data()));
    EXPECT_TRUE(S1.is_selected());
    EXPECT_TRUE(S2.is_selected());
  }

  // Outer only.
  {
    SubCommand S1(Description("s1"sv));
    SubCommand S2(Description("s2"sv));
    ArgumentParser Parser;
    Parser.begin_subcommand(S1, "s1"sv)
        .begin_subcommand(S2, "s2"sv)
        .end_subcommand()
        .end_subcommand();
    std::array Args = {"test", "s1"};
    EXPECT_TRUE(
        Parser.parse(stdout, static_cast<int>(Args.size()), Args.data()));
    EXPECT_TRUE(S1.is_selected());
    EXPECT_FALSE(S2.is_selected());
  }

  // Inner alone rejected.
  {
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
}

TEST(SubcommandTest, NestedOptions) {
  // Test the options of nested subcommands.
  //
  //   1.  Outer option then subcommand.
  //   2.  Outer option after subcommand rejected.
  //   3.  Subcommand leaves options unset.
  //   4.  Subcommand option.
  //   5.  Outer and subcommand options.

  // Outer option then subcommand.
  {
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
    EXPECT_TRUE(
        Parser.parse(stdout, static_cast<int>(Args.size()), Args.data()));
    EXPECT_TRUE(T1.value());
    EXPECT_FALSE(T2.value());
    EXPECT_FALSE(T3.value());
    EXPECT_TRUE(S1.is_selected());
  }

  // Outer option after subcommand rejected.
  {
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

  // Subcommand leaves options unset.
  {
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
    EXPECT_TRUE(
        Parser.parse(stdout, static_cast<int>(Args.size()), Args.data()));
    EXPECT_FALSE(T1.value());
    EXPECT_FALSE(T2.value());
    EXPECT_FALSE(T3.value());
    EXPECT_TRUE(S1.is_selected());
  }

  // Subcommand option.
  {
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
    EXPECT_TRUE(
        Parser.parse(stdout, static_cast<int>(Args.size()), Args.data()));
    EXPECT_FALSE(T1.value());
    EXPECT_TRUE(T2.value());
    EXPECT_FALSE(T3.value());
    EXPECT_TRUE(S1.is_selected());
  }

  // Outer and subcommand options.
  {
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
    EXPECT_TRUE(
        Parser.parse(stdout, static_cast<int>(Args.size()), Args.data()));
    EXPECT_TRUE(T1.value());
    EXPECT_TRUE(T2.value());
    EXPECT_FALSE(T3.value());
    EXPECT_TRUE(S1.is_selected());
  }
}
