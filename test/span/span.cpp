// SPDX-License-Identifier: CC0-1.0
#include "experimental/span.hpp"
#include <algorithm>
#include <array>
#include <gtest/gtest.h>
#include <initializer_list>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

static_assert(!cxx20::detail::is_compatible_element_v<int, unsigned int>);
static_assert(cxx20::detail::is_generic_range_v<std::vector<int> &>);
static_assert(cxx20::detail::is_compatible_range_v<int, std::vector<int> &>);
static_assert(!cxx20::detail::is_compatible_range_v<int, int>);
static_assert(
    cxx20::detail::is_compatible_iterator_v<int, std::vector<int>::iterator>);
static_assert(
    std::is_same_v<decltype(cxx20::span(std::initializer_list<int>{1, 2})),
                   cxx20::span<const int>>);
static_assert(std::is_same_v<decltype(cxx20::span(std::array<int, 2>{1, 2})),
                             cxx20::span<const int>>);

static_assert(std::is_default_constructible_v<cxx20::span<int>>);
static_assert(std::is_default_constructible_v<cxx20::span<int, 0>>);
static_assert(!std::is_default_constructible_v<cxx20::span<int, 1>>);

TEST(SpanTest, Constructors) {
  cxx20::span<int> S;
  EXPECT_EQ(S.size(), 0U);
  EXPECT_EQ(S.data(), nullptr);

  cxx20::span<const int> CS;
  EXPECT_EQ(CS.size(), 0U);
  EXPECT_EQ(CS.data(), nullptr);
}

TEST(SpanTest, ConstructorsWithExtent) {
  cxx20::span<int, 0> S;
  EXPECT_EQ(S.size(), 0U);
  EXPECT_EQ(S.data(), nullptr);

  cxx20::span<const int, 0> CS;
  EXPECT_EQ(CS.size(), 0U);
  EXPECT_EQ(CS.data(), nullptr);
}

TEST(SpanTest, Size) {
  cxx20::span<int> S1;
  EXPECT_EQ(sizeof(S1), sizeof(int *) + sizeof(ptrdiff_t));

  int Array[1] = {1};
  cxx20::span<int, 1> S2 = Array;
  EXPECT_EQ(sizeof(S2), sizeof(int *));

  cxx20::span<int, 0> S3;
  EXPECT_EQ(sizeof(S3), sizeof(int *));
}

TEST(SpanTest, ConstructorsFromPointerAndSize) {
  int Array[4] = {1, 2, 3, 4};
  for (std::size_t Offset = 0; Offset < std::size(Array) - 1; ++Offset) {
    for (std::size_t Size = 1; Size < std::size(Array) - Offset; ++Size) {
      cxx20::span S(std::data(Array) + Offset, Size);
      EXPECT_TRUE((std::is_same_v<decltype(S)::element_type, int>));
      EXPECT_EQ(S.size(), Size);
      EXPECT_EQ(S.data(), std::data(Array) + Offset);
      EXPECT_EQ(S.empty(), Size == 0);
      for (std::size_t I = 0; I < Size; ++I) {
        EXPECT_EQ(Array[Offset + I], S[I]);
      }
      int Temp = 5;
      std::swap(Temp, S[0]);
      EXPECT_EQ(Array[Offset], S[0]);
      std::swap(Temp, S[0]);
    }
    EXPECT_EQ(Array[Offset], int(Offset) + 1);
  }
}

TEST(SpanTest, ConstructorsFromConstPointerAndSize) {
  const int Array[4] = {1, 2, 3, 4};
  for (std::size_t Offset = 0; Offset < std::size(Array) - 1; ++Offset) {
    for (std::size_t Size = 1; Size < std::size(Array) - Offset; ++Size) {
      cxx20::span S(std::data(Array) + Offset, Size);
      EXPECT_TRUE((std::is_same_v<decltype(S)::element_type, const int>));
      EXPECT_EQ(S.size(), Size);
      EXPECT_EQ(S.data(), std::data(Array) + Offset);
      EXPECT_EQ(S.empty(), Size == 0);
      for (std::size_t I = 0; I < Size; ++I) {
        EXPECT_EQ(Array[Offset + I], S[I]);
      }
    }
  }
}

TEST(SpanTest, ConstructorsFromArray) {
  int Array[] = {1, 2, 3, 4};
  cxx20::span S(Array);
  EXPECT_TRUE((std::is_same_v<decltype(S)::element_type, int>));
  EXPECT_EQ(S.size(), std::size(Array));
  EXPECT_EQ(S.data(), std::data(Array));
  for (std::size_t I = 0; I < std::size(Array); ++I) {
    EXPECT_EQ(Array[I], S[I]);
  }
  S[0] = 5;
  EXPECT_EQ(Array[0], 5);
}

TEST(SpanTest, ConstructorsFromConstArray) {
  const int Array[] = {1, 2, 3, 4};
  cxx20::span S(Array);
  EXPECT_TRUE((std::is_same_v<decltype(S)::element_type, const int>));
  EXPECT_EQ(S.size(), std::size(Array));
  EXPECT_EQ(S.data(), std::data(Array));
  for (std::size_t I = 0; I < std::size(Array); ++I) {
    EXPECT_EQ(Array[I], S[I]);
  }
}

TEST(SpanTest, ConstructorsFromStdArray) {
  std::array Array = {1, 2, 3, 4};
  cxx20::span S(Array);
  EXPECT_TRUE((std::is_same_v<decltype(S)::element_type, int>));
  EXPECT_EQ(S.size(), std::size(Array));
  EXPECT_EQ(S.data(), std::data(Array));
  for (std::size_t I = 0; I < std::size(Array); ++I) {
    EXPECT_EQ(Array[I], S[I]);
  }
  S[0] = 5;
  EXPECT_EQ(Array[0], 5);
}

TEST(SpanTest, ConstructorsFromConstStdArray) {
  const std::array Array = {1, 2, 3, 4};
  cxx20::span S(Array);
  EXPECT_TRUE((std::is_same_v<decltype(S)::element_type, const int>));
  EXPECT_EQ(S.size(), std::size(Array));
  EXPECT_EQ(S.data(), std::data(Array));
  for (std::size_t I = 0; I < std::size(Array); ++I) {
    EXPECT_EQ(Array[I], S[I]);
  }
}

TEST(SpanTest, ConstructorsFromStdVector) {
  std::vector Vector = {1, 2, 3, 4};
  cxx20::span S(Vector);
  EXPECT_TRUE((std::is_same_v<decltype(S)::element_type, int>));
  EXPECT_EQ(S.size(), std::size(Vector));
  EXPECT_EQ(S.data(), std::data(Vector));
  for (std::size_t I = 0; I < std::size(Vector); ++I) {
    EXPECT_EQ(Vector[I], S[I]);
  }
  S[0] = 5;
  EXPECT_EQ(Vector[0], 5);
}

TEST(SpanTest, ConstructorsFromConstStdVector) {
  const std::vector Vector = {1, 2, 3, 4};
  cxx20::span S(Vector);
  EXPECT_TRUE((std::is_same_v<decltype(S)::element_type, const int>));
  EXPECT_EQ(S.size(), std::size(Vector));
  EXPECT_EQ(S.data(), std::data(Vector));
  for (std::size_t I = 0; I < std::size(Vector); ++I) {
    EXPECT_EQ(Vector[I], S[I]);
  }
}

TEST(SpanTest, ConstructorsFromStdString) {
  using namespace std::literals;
  std::string String = "hello world"s;
  cxx20::span S(String);
  EXPECT_TRUE((std::is_same_v<decltype(S)::element_type, char>));
  EXPECT_EQ(S.size(), std::size(String));
  EXPECT_EQ(S.data(), std::data(String));
  for (std::size_t I = 0; I < std::size(String); ++I) {
    EXPECT_EQ(String[I], S[I]);
  }
  S[0] = 'X';
  EXPECT_EQ(String[0], 'X');
}

TEST(SpanTest, ConstructorsFromConstStdString) {
  using namespace std::literals;
  const std::string String = "hello world"s;
  cxx20::span S(String);
  EXPECT_TRUE((std::is_same_v<decltype(S)::element_type, const char>));
  EXPECT_EQ(S.size(), std::size(String));
  EXPECT_EQ(S.data(), std::data(String));
  for (std::size_t I = 0; I < std::size(String); ++I) {
    EXPECT_EQ(String[I], S[I]);
  }
}

TEST(SpanTest, ConstructorsFromStdStringView) {
  using namespace std::literals;
  std::string_view String = "hello world"sv;
  cxx20::span S(String);
  EXPECT_TRUE((std::is_same_v<decltype(S)::element_type, const char>));
  EXPECT_EQ(S.size(), std::size(String));
  EXPECT_EQ(S.data(), std::data(String));
  for (std::size_t I = 0; I < std::size(String); ++I) {
    EXPECT_EQ(String[I], S[I]);
  }
}

TEST(SpanTest, Convertible) {
  cxx20::span<int> IS;

  cxx20::span<const int> CIS = IS;
  EXPECT_EQ(IS.size(), CIS.size());
  EXPECT_EQ(IS.data(), CIS.data());

  cxx20::span<const int> CIS2(IS);
  EXPECT_EQ(IS.size(), CIS2.size());
  EXPECT_EQ(IS.data(), CIS2.data());
}

TEST(SpanTest, CopyAssignment) {
  int Array[] = {1, 2, 3};

  cxx20::span<int> S1;
  EXPECT_TRUE(S1.empty());

  cxx20::span<int> S2 = Array;
  EXPECT_EQ(S2.data(), std::data(Array));
  EXPECT_EQ(S2.size(), std::size(Array));

  S2 = S1;
  EXPECT_TRUE(S2.empty());
}

TEST(SpanTest, First) {
  int Array[] = {1, 2, 3, 4, 5};

  {
    cxx20::span S = Array;
    EXPECT_EQ(S.size(), 5U);
    EXPECT_EQ(S.first<0>().size(), 0U);
    EXPECT_EQ(S.first(0).size(), 0U);
  }

  {
    cxx20::span S = Array;
    EXPECT_EQ(S.size(), 5U);
    EXPECT_EQ(S.first<2>().size(), 2U);
    EXPECT_EQ(S.first(2).size(), 2U);
  }

  {
    cxx20::span<int> S = Array;
    EXPECT_EQ(S.size(), 5U);
    EXPECT_EQ(S.first<0>().size(), 0U);
    EXPECT_EQ(S.first(0).size(), 0U);
  }

  {
    cxx20::span<int> S = Array;
    EXPECT_EQ(S.size(), 5U);
    EXPECT_EQ(S.first<2>().size(), 2U);
    EXPECT_EQ(S.first(2).size(), 2U);
  }
}

TEST(SpanTest, Last) {
  int Array[] = {1, 2, 3, 4, 5};

  {
    cxx20::span S = Array;
    EXPECT_EQ(S.size(), 5U);
    EXPECT_EQ(S.last<0>().size(), 0U);
    EXPECT_EQ(S.last(0).size(), 0U);
  }

  {
    cxx20::span S = Array;
    EXPECT_EQ(S.size(), 5U);
    EXPECT_EQ(S.last<2>().size(), 2U);
    EXPECT_EQ(S.last(2).size(), 2U);
  }

  {
    cxx20::span<int> S = Array;
    EXPECT_EQ(S.size(), 5U);
    EXPECT_EQ(S.last<0>().size(), 0U);
    EXPECT_EQ(S.last(0).size(), 0U);
  }

  {
    cxx20::span<int> S = Array;
    EXPECT_EQ(S.size(), 5U);
    EXPECT_EQ(S.last<2>().size(), 2U);
    EXPECT_EQ(S.last(2).size(), 2U);
  }
}

TEST(SpanTest, SubSpan) {
  int Array[] = {1, 2, 3, 4, 5};

  {
    cxx20::span S = Array;
    EXPECT_EQ(S.size(), 5U);
    EXPECT_EQ((S.subspan<2, 2>().size()), 2U);
    EXPECT_EQ(decltype(S.subspan<2, 2>())::extent, 2U);
    EXPECT_EQ(S.subspan(2, 2).size(), 2U);
  }

  {
    cxx20::span S = Array;
    EXPECT_EQ(S.size(), 5U);
    EXPECT_EQ((S.subspan<0, 0>().size()), 0U);
    EXPECT_EQ(decltype(S.subspan<0, 0>())::extent, 0U);
    EXPECT_EQ(S.subspan(0, 0).size(), 0U);
  }

  {
    cxx20::span<int> S = Array;
    EXPECT_EQ(S.size(), 5U);
    EXPECT_EQ((S.subspan<2, 2>().size()), 2U);
    EXPECT_EQ(decltype(S.subspan<2, 2>())::extent, 2U);
    EXPECT_EQ(S.subspan(2, 2).size(), 2U);
  }

  {
    cxx20::span<int> S = Array;
    EXPECT_EQ(S.size(), 5U);
    EXPECT_EQ((S.subspan<0, 0>().size()), 0U);
    EXPECT_EQ(decltype(S.subspan<0, 0>())::extent, 0U);
    EXPECT_EQ(S.subspan(0, 0).size(), 0U);
  }
}

TEST(SpanTest, Iterator) {
  int Array[] = {1, 2, 3, 4};
  {
    cxx20::span S = Array;
    auto It1 = S.begin();
    EXPECT_EQ(It1, It1);
    EXPECT_EQ(It1, S.begin());
    EXPECT_NE(It1, S.end());

    auto It2 = It1 + 1;
    EXPECT_NE(It2, It1);
    EXPECT_NE(It2, S.begin());
    EXPECT_NE(It2, S.end());

    EXPECT_LT(It1, It2);
    EXPECT_LE(It1, It2);
    EXPECT_LT(It2, S.end());
    EXPECT_LE(It2, S.end());
  }
  {
    cxx20::span<int> S = Array;
    auto It1 = S.begin();
    EXPECT_EQ(It1, It1);
    EXPECT_EQ(It1, S.begin());
    EXPECT_NE(It1, S.end());

    auto It2 = It1 + 1;
    EXPECT_NE(It2, It1);
    EXPECT_NE(It2, S.begin());
    EXPECT_NE(It2, S.end());

    EXPECT_LT(It1, It2);
    EXPECT_LE(It1, It2);
    EXPECT_LT(It2, S.end());
    EXPECT_LE(It2, S.end());
  }
}

TEST(SpanTest, BeginEnd) {
  int Array1[4] = {1, 2, 3, 4};
  int Array2[4] = {5, 6, 7, 8};

  EXPECT_NE(Array1[0], Array2[0]);
  EXPECT_NE(Array1[1], Array2[1]);
  EXPECT_NE(Array1[2], Array2[2]);
  EXPECT_NE(Array1[3], Array2[3]);

  cxx20::span S1 = Array1;
  cxx20::span S2 = Array2;
  std::copy(S1.begin(), S1.end(), S2.begin());

  EXPECT_EQ(Array1[0], Array2[0]);
  EXPECT_EQ(Array1[1], Array2[1]);
  EXPECT_EQ(Array1[2], Array2[2]);
  EXPECT_EQ(Array1[3], Array2[3]);

  std::copy(S1.rbegin(), S1.rend(), S2.begin());

  EXPECT_EQ(Array1[0], Array2[3]);
  EXPECT_EQ(Array1[1], Array2[2]);
  EXPECT_EQ(Array1[2], Array2[1]);
  EXPECT_EQ(Array1[3], Array2[0]);
}

TEST(SpanTest, FrontBack) {
  int Array[4] = {1, 2, 3, 4};
  {
    cxx20::span S(Array);
    EXPECT_EQ(S.front(), *std::begin(Array));
    EXPECT_EQ(S.back(), *std::rbegin(Array));
  }
  {
    cxx20::span<int> S(Array);
    EXPECT_EQ(S.front(), *std::begin(Array));
    EXPECT_EQ(S.back(), *std::rbegin(Array));
  }
}

TEST(SpanTest, AsBytes) {
  int Array[4] = {1, 2, 3, 4};
  {
    cxx20::span S = Array;
    auto B = cxx20::as_bytes(S);
    EXPECT_EQ(B.size(), sizeof(int) * S.size());
    EXPECT_EQ(B.size(), S.size_bytes());
    EXPECT_EQ(static_cast<const void *>(B.data()),
              static_cast<const void *>(S.data()));
  }
  {
    cxx20::span<int> S = Array;
    auto B = cxx20::as_bytes(S);
    EXPECT_EQ(B.size(), sizeof(int) * S.size());
    EXPECT_EQ(B.size(), S.size_bytes());
    EXPECT_EQ(static_cast<const void *>(B.data()),
              static_cast<const void *>(S.data()));
  }
}

TEST(SpanTest, AsWritableBytes) {
  const int Array1[4] = {1, 2, 3, 4};
  {
    int Array2[4] = {5, 6, 7, 8};
    cxx20::span S1 = Array1;
    cxx20::span S2 = Array2;
    auto B1 = cxx20::as_bytes(S1);
    auto B2 = cxx20::as_writable_bytes(S2);
    EXPECT_NE(Array1[0], Array2[0]);
    EXPECT_NE(Array1[1], Array2[1]);
    EXPECT_NE(Array1[2], Array2[2]);
    EXPECT_NE(Array1[3], Array2[3]);
    std::copy(B1.begin(), B1.end(), B2.begin());
    EXPECT_EQ(Array1[0], Array2[0]);
    EXPECT_EQ(Array1[1], Array2[1]);
    EXPECT_EQ(Array1[2], Array2[2]);
    EXPECT_EQ(Array1[3], Array2[3]);
  }
  {
    int Array2[4] = {5, 6, 7, 8};
    cxx20::span<const int> S1 = Array1;
    cxx20::span<int> S2 = Array2;
    EXPECT_NE(Array1[0], Array2[0]);
    EXPECT_NE(Array1[1], Array2[1]);
    EXPECT_NE(Array1[2], Array2[2]);
    EXPECT_NE(Array1[3], Array2[3]);
    auto B1 = cxx20::as_bytes(S1);
    auto B2 = cxx20::as_writable_bytes(S2);
    std::copy(B1.begin(), B1.end(), B2.begin());
    EXPECT_EQ(Array1[0], Array2[0]);
    EXPECT_EQ(Array1[1], Array2[1]);
    EXPECT_EQ(Array1[2], Array2[2]);
    EXPECT_EQ(Array1[3], Array2[3]);
  }
}

GTEST_API_ int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
