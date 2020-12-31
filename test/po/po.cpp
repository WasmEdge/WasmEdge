// SPDX-License-Identifier: Apache-2.0

#include "experimental/span.hpp"
#include "po/argument_parser.h"
#include "po/list.h"
#include "po/option.h"
#include "gtest/gtest.h"
#include <numeric>
#include <vector>

using namespace SSVM::PO;

struct Param {
  bool R;
  bool A;
  int B;
  std::vector<const char *> F;
  std::vector<const char *> Args;
  Param(bool R, bool A, int B, std::vector<const char *> F,
        std::vector<const char *> Args)
      : R(R), A(A), B(B), F(std::move(F)), Args(std::move(Args)) {}
  Param(bool R, std::vector<const char *> Args) : R(R), Args(std::move(Args)) {}
};

class GeneralOptions : public ::testing::TestWithParam<Param> {
public:
  GeneralOptions()
      : A(Description("a option"sv)),
        B(Description("b option"sv), DefaultValue<int>(-1)),
        F(Description("f option"sv), ZeroOrMore()) {
    Parser.add_option("a"sv, A).add_option("b"sv, B).add_option(F);
  }
  Option<Toggle> A;
  Option<int> B;
  List<std::string> F;
  ArgumentParser Parser;
};

TEST_P(GeneralOptions, Test) {
  auto P = GetParam();
  EXPECT_EQ(P.R, Parser.parse(P.Args.size(), P.Args.data()));
  if (P.R) {
    EXPECT_EQ(P.A, A.value());
    EXPECT_EQ(P.B, B.value());
    EXPECT_EQ(P.F.size(), F.value().size());
    for (size_t I = 0; I < P.F.size(); ++I) {
      SCOPED_TRACE(I);
      EXPECT_EQ(P.F[I], F.value()[I]);
    }
  }
}

INSTANTIATE_TEST_SUITE_P(
    InstantiationName, GeneralOptions,
    testing::Values(Param(true, false, 0, {}, {"test", "--"}),
                    Param(true, false, 0, {"-a"}, {"test", "--", "-a"}),
                    Param(true, true, 0, {}, {"test", "-a"}),
                    Param(true, false, -1, {}, {"test", "-b"}),
                    Param(false, {"test", "-b", "-2"}),
                    Param(true, false, -2, {}, {"test", "--b=-2"})),
    [](const testing::TestParamInfo<GeneralOptions::ParamType> &Info) {
      std::string Name;
      for (const auto &Arg : Info.param.Args) {
        Name += Arg;
        Name += ' ';
      }
      Name.pop_back();
      for (auto &C : Name) {
        if (!std::isalnum(C))
          C = '_';
      }
      return Name;
    });
