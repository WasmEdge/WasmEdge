// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "po/argument_parser.h"
#include "po/list.h"
#include "po/option.h"
#include "po/subcommand.h"
#include <array>
#include <cstdio>
#include <gtest/gtest.h>
#include <memory>
#include <string>
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

TEST(Help, WrapsLongUnbrokenDescription) {
  const std::string DescriptionText(100, 'x');
  std::string Utf8DescriptionText(77, 'y');
  Utf8DescriptionText += "\xC3\xA9";
  Utf8DescriptionText.append(21, 'y');
  Option<Toggle> A{Description(DescriptionText)};
  Option<Toggle> B{Description(Utf8DescriptionText)};
  ArgumentParser Parser;
  Parser.add_option("a"sv, A).add_option("b"sv, B);

  auto CloseFile = [](std::FILE *File) noexcept { std::fclose(File); };
  std::unique_ptr<std::FILE, decltype(CloseFile)> Out(std::tmpfile(),
                                                      CloseFile);
  ASSERT_NE(Out, nullptr);
  Parser.help(Out.get());
  ASSERT_EQ(std::fflush(Out.get()), 0);
  ASSERT_EQ(std::fseek(Out.get(), 0, SEEK_SET), 0);

  std::array<char, 1024> Buffer{};
  const std::size_t Size =
      std::fread(Buffer.data(), sizeof(char), Buffer.size(), Out.get());

  const std::string Output(Buffer.data(), Size);
  const std::size_t FirstLine = Output.find(std::string(78, 'x'));
  ASSERT_NE(FirstLine, std::string::npos);
  EXPECT_EQ(Output.find(std::string(79, 'x')), std::string::npos);
  EXPECT_NE(Output.find(std::string(22, 'x'), FirstLine + 78),
            std::string::npos);

  const std::size_t FirstUtf8Line = Output.find(std::string(77, 'y'));
  ASSERT_NE(FirstUtf8Line, std::string::npos);
  EXPECT_NE(Output.find("\xC3\xA9", FirstUtf8Line + 77), std::string::npos);
}
