// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

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

TEST(HelpTest, Version) {
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

TEST(HelpTest, HelpSimple1) {
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

TEST(HelpTest, HelpSimple2) {
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

TEST(HelpTest, OptionFormatting) {
  Option<Toggle> A(Description("option a description"sv));
  ArgumentParser Parser;
  Parser.add_option("opt-a"sv, A);

  std::FILE *Fp = std::tmpfile();
  ASSERT_NE(Fp, nullptr);
  Parser.help(Fp);
  std::rewind(Fp);
  std::string Out;
  char Buf[256];
  while (std::fgets(Buf, sizeof(Buf), Fp)) {
    Out += Buf;
  }
  std::fclose(Fp);

  // The option flag should be indented on the same line and not start on an
  // unindented newline
  EXPECT_EQ(Out.find("\n--opt-a"), std::string::npos);
  EXPECT_NE(Out.find("--opt-a"), std::string::npos);
  EXPECT_NE(Out.find("\t\toption a description\n"), std::string::npos);
  EXPECT_NE(Out.find("Show this help message\n"), std::string::npos);
}
