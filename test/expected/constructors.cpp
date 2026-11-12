// SPDX-License-Identifier: CC0-1.0
#include <experimental/expected.hpp>
#include <gtest/gtest.h>
#include <initializer_list>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

struct takes_init_and_variadic {
  std::vector<int> v;
  std::tuple<int, int> t;
  template <class... Args>
  takes_init_and_variadic(std::initializer_list<int> l, Args &&...args)
      : v(l), t(std::forward<Args>(args)...) {}
};

TEST(ConstructorsTest, Constructors) {
  {
    cxx20::expected<int, int> e;
    EXPECT_TRUE(e);
    EXPECT_EQ(e, 0);
  }

  {
    cxx20::expected<int, int> e = cxx20::unexpected(0);
    EXPECT_FALSE(e);
    EXPECT_EQ(e.error(), 0);
  }

  {
    cxx20::expected<int, int> e(cxx20::unexpect, 0);
    EXPECT_FALSE(e);
    EXPECT_EQ(e.error(), 0);
  }

  {
    cxx20::expected<int, int> e(std::in_place, 42);
    EXPECT_TRUE(e);
    EXPECT_EQ(e, 42);
  }

  {
    cxx20::expected<std::vector<int>, int> e(std::in_place, {0, 1});
    EXPECT_TRUE(e);
    EXPECT_EQ((*e)[0], 0);
    EXPECT_EQ((*e)[1], 1);
  }

  {
    cxx20::expected<std::tuple<int, int>, int> e(std::in_place, 0, 1);
    EXPECT_TRUE(e);
    EXPECT_EQ(std::get<0>(*e), 0);
    EXPECT_EQ(std::get<1>(*e), 1);
  }

  {
    cxx20::expected<takes_init_and_variadic, int> e(std::in_place, {0, 1}, 2,
                                                    3);
    EXPECT_TRUE(e);
    EXPECT_EQ(e->v[0], 0);
    EXPECT_EQ(e->v[1], 1);
    EXPECT_EQ(std::get<0>(e->t), 2);
    EXPECT_EQ(std::get<1>(e->t), 3);
  }

  {
    cxx20::expected<int, int> e;
    EXPECT_TRUE(std::is_default_constructible_v<decltype(e)>);
    EXPECT_TRUE(std::is_copy_constructible_v<decltype(e)>);
    EXPECT_TRUE(std::is_move_constructible_v<decltype(e)>);
    EXPECT_TRUE(std::is_copy_assignable_v<decltype(e)>);
    EXPECT_TRUE(std::is_move_assignable_v<decltype(e)>);
    EXPECT_TRUE(std::is_trivially_copy_constructible_v<decltype(e)>);
    EXPECT_TRUE(std::is_trivially_copy_assignable_v<decltype(e)>);
    EXPECT_TRUE(std::is_trivially_move_constructible_v<decltype(e)>);
    EXPECT_TRUE(std::is_trivially_move_assignable_v<decltype(e)>);
  }

  {
    cxx20::expected<int, std::string> e;
    EXPECT_TRUE(std::is_default_constructible_v<decltype(e)>);
    EXPECT_TRUE(std::is_copy_constructible_v<decltype(e)>);
    EXPECT_TRUE(std::is_move_constructible_v<decltype(e)>);
    EXPECT_TRUE(std::is_copy_assignable_v<decltype(e)>);
    EXPECT_TRUE(std::is_move_assignable_v<decltype(e)>);
    EXPECT_FALSE(std::is_trivially_copy_constructible_v<decltype(e)>);
    EXPECT_FALSE(std::is_trivially_copy_assignable_v<decltype(e)>);
    EXPECT_FALSE(std::is_trivially_move_constructible_v<decltype(e)>);
    EXPECT_FALSE(std::is_trivially_move_assignable_v<decltype(e)>);
  }

  {
    cxx20::expected<std::string, int> e;
    EXPECT_TRUE(std::is_default_constructible_v<decltype(e)>);
    EXPECT_TRUE(std::is_copy_constructible_v<decltype(e)>);
    EXPECT_TRUE(std::is_move_constructible_v<decltype(e)>);
    EXPECT_TRUE(std::is_copy_assignable_v<decltype(e)>);
    EXPECT_TRUE(std::is_move_assignable_v<decltype(e)>);
    EXPECT_FALSE(std::is_trivially_copy_constructible_v<decltype(e)>);
    EXPECT_FALSE(std::is_trivially_copy_assignable_v<decltype(e)>);
    EXPECT_FALSE(std::is_trivially_move_constructible_v<decltype(e)>);
    EXPECT_FALSE(std::is_trivially_move_assignable_v<decltype(e)>);
  }

  {
    cxx20::expected<std::string, std::string> e;
    EXPECT_TRUE(std::is_default_constructible_v<decltype(e)>);
    EXPECT_TRUE(std::is_copy_constructible_v<decltype(e)>);
    EXPECT_TRUE(std::is_move_constructible_v<decltype(e)>);
    EXPECT_TRUE(std::is_copy_assignable_v<decltype(e)>);
    EXPECT_TRUE(std::is_move_assignable_v<decltype(e)>);
    EXPECT_FALSE(std::is_trivially_copy_constructible_v<decltype(e)>);
    EXPECT_FALSE(std::is_trivially_copy_assignable_v<decltype(e)>);
    EXPECT_FALSE(std::is_trivially_move_constructible_v<decltype(e)>);
    EXPECT_FALSE(std::is_trivially_move_assignable_v<decltype(e)>);
  }

  {
    cxx20::expected<void, int> e;
    EXPECT_TRUE(e);
  }

  {
    cxx20::expected<void, int> e(cxx20::unexpect, 42);
    EXPECT_FALSE(e);
    EXPECT_EQ(e.error(), 42);
  }
}
