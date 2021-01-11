// SPDX-License-Identifier: Apache-2.0

#include "po/subcommand.h"
#include "experimental/span.hpp"
#include "po/argument_parser.h"
#include "po/list.h"
#include "po/option.h"
#include "gtest/gtest.h"
#include <numeric>
#include <vector>

using namespace SSVM::PO;
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
  EXPECT_TRUE(Parser.parse(Args.size(), Args.data()));
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
  EXPECT_TRUE(Parser.parse(Args.size(), Args.data()));
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
  EXPECT_TRUE(Parser.parse(Args.size(), Args.data()));
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
  EXPECT_FALSE(Parser.parse(Args.size(), Args.data()));
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
  EXPECT_TRUE(Parser.parse(Args.size(), Args.data()));
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
  EXPECT_TRUE(Parser.parse(Args.size(), Args.data()));
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
  EXPECT_FALSE(Parser.parse(Args.size(), Args.data()));
}
