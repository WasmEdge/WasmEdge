// SPDX-License-Identifier: CC0-1.0
#include "gtest/gtest.h"
#include <experimental/expected.hpp>
#include <string>
#include <type_traits>

using cxx20::expected;
using cxx20::unexpected;

TEST(BaseTest, Triviality) {
  EXPECT_TRUE((std::is_trivially_copy_constructible_v<expected<int, int>>));
  EXPECT_TRUE((std::is_trivially_copy_assignable_v<expected<int, int>>));
  EXPECT_TRUE((std::is_trivially_move_constructible_v<expected<int, int>>));
  EXPECT_TRUE((std::is_trivially_move_assignable_v<expected<int, int>>));
  EXPECT_TRUE((std::is_trivially_destructible_v<expected<int, int>>));

  EXPECT_TRUE((std::is_trivially_copy_constructible_v<expected<void, int>>));
  EXPECT_TRUE((std::is_trivially_move_constructible_v<expected<void, int>>));
  EXPECT_TRUE((std::is_trivially_destructible_v<expected<void, int>>));

  {
    struct T {
      T(const T &) = default;
      T(T &&) = default;
      T &operator=(const T &) = default;
      T &operator=(T &&) = default;
      ~T() = default;
    };
    EXPECT_TRUE((std::is_trivially_copy_constructible_v<expected<T, int>>));
    EXPECT_TRUE((std::is_trivially_copy_assignable_v<expected<T, int>>));
    EXPECT_TRUE((std::is_trivially_move_constructible_v<expected<T, int>>));
    EXPECT_TRUE((std::is_trivially_move_assignable_v<expected<T, int>>));
    EXPECT_TRUE((std::is_trivially_destructible_v<expected<T, int>>));
  }

  {
    struct T {
      T(const T &) {}
      T(T &&){};
      T &operator=(const T &) { return *this; }
      T &operator=(T &&) { return *this; }
      ~T() {}
    };
    EXPECT_FALSE((std::is_trivially_copy_constructible_v<expected<T, int>>));
    EXPECT_FALSE((std::is_trivially_copy_assignable_v<expected<T, int>>));
    EXPECT_FALSE((std::is_trivially_move_constructible_v<expected<T, int>>));
    EXPECT_FALSE((std::is_trivially_move_assignable_v<expected<T, int>>));
    EXPECT_FALSE((std::is_trivially_destructible_v<expected<T, int>>));
  }
}

TEST(BaseTest, Deletion) {
  EXPECT_TRUE((std::is_copy_constructible_v<expected<int, int>>));
  EXPECT_TRUE((std::is_copy_assignable_v<expected<int, int>>));
  EXPECT_TRUE((std::is_move_constructible_v<expected<int, int>>));
  EXPECT_TRUE((std::is_move_assignable_v<expected<int, int>>));
  EXPECT_TRUE((std::is_destructible_v<expected<int, int>>));

  {
    struct T {
      T() = default;
    };
    EXPECT_TRUE((std::is_default_constructible_v<expected<T, int>>));
  }

  {
    struct T {
      T(int) {}
    };
    EXPECT_FALSE((std::is_default_constructible_v<expected<T, int>>));
  }

  {
    struct T {
      T(const T &) = default;
      T(T &&) = default;
      T &operator=(const T &) = default;
      T &operator=(T &&) = default;
      ~T() = default;
    };
    EXPECT_TRUE((std::is_copy_constructible_v<expected<T, int>>));
    EXPECT_TRUE((std::is_copy_assignable_v<expected<T, int>>));
    EXPECT_TRUE((std::is_move_constructible_v<expected<T, int>>));
    EXPECT_TRUE((std::is_move_assignable_v<expected<T, int>>));
    EXPECT_TRUE((std::is_destructible_v<expected<T, int>>));
  }

  {
    struct T {
      T(const T &) = delete;
      T(T &&) = delete;
      T &operator=(const T &) = delete;
      T &operator=(T &&) = delete;
    };
    EXPECT_FALSE((std::is_copy_constructible_v<expected<T, int>>));
    EXPECT_FALSE((std::is_copy_assignable_v<expected<T, int>>));
    EXPECT_FALSE((std::is_move_constructible_v<expected<T, int>>));
    EXPECT_FALSE((std::is_move_assignable_v<expected<T, int>>));
  }

  {
    struct T {
      T(const T &) = delete;
      T(T &&) = default;
      T &operator=(const T &) = delete;
      T &operator=(T &&) = default;
    };
    EXPECT_FALSE((std::is_copy_constructible_v<expected<T, int>>));
    EXPECT_FALSE((std::is_copy_assignable_v<expected<T, int>>));
    EXPECT_TRUE((std::is_move_constructible_v<expected<T, int>>));
    EXPECT_TRUE((std::is_move_assignable_v<expected<T, int>>));
  }

  {
    struct T {
      T(const T &) = default;
      T(T &&) = delete;
      T &operator=(const T &) = default;
      T &operator=(T &&) = delete;
    };
    EXPECT_TRUE((std::is_copy_constructible_v<expected<T, int>>));
    EXPECT_TRUE((std::is_copy_assignable_v<expected<T, int>>));
  }

  {
    expected<int, int> e;
    EXPECT_TRUE((std::is_default_constructible_v<decltype(e)>));
    EXPECT_TRUE((std::is_copy_constructible_v<decltype(e)>));
    EXPECT_TRUE((std::is_move_constructible_v<decltype(e)>));
    EXPECT_TRUE((std::is_copy_assignable_v<decltype(e)>));
    EXPECT_TRUE((std::is_move_assignable_v<decltype(e)>));
    EXPECT_TRUE((std::is_trivially_copy_constructible_v<decltype(e)>));
    EXPECT_TRUE((std::is_trivially_copy_assignable_v<decltype(e)>));
    EXPECT_TRUE((std::is_trivially_move_constructible_v<decltype(e)>));
    EXPECT_TRUE((std::is_trivially_move_assignable_v<decltype(e)>));
  }

  {
    expected<int, std::string> e;
    EXPECT_TRUE((std::is_default_constructible_v<decltype(e)>));
    EXPECT_TRUE((std::is_copy_constructible_v<decltype(e)>));
    EXPECT_TRUE((std::is_move_constructible_v<decltype(e)>));
    EXPECT_TRUE((std::is_copy_assignable_v<decltype(e)>));
    EXPECT_TRUE((std::is_move_assignable_v<decltype(e)>));
    EXPECT_FALSE((std::is_trivially_copy_constructible_v<decltype(e)>));
    EXPECT_FALSE((std::is_trivially_copy_assignable_v<decltype(e)>));
    EXPECT_FALSE((std::is_trivially_move_constructible_v<decltype(e)>));
    EXPECT_FALSE((std::is_trivially_move_assignable_v<decltype(e)>));
  }

  {
    expected<std::string, int> e;
    EXPECT_TRUE((std::is_default_constructible_v<decltype(e)>));
    EXPECT_TRUE((std::is_copy_constructible_v<decltype(e)>));
    EXPECT_TRUE((std::is_move_constructible_v<decltype(e)>));
    EXPECT_TRUE((std::is_copy_assignable_v<decltype(e)>));
    EXPECT_TRUE((std::is_move_assignable_v<decltype(e)>));
    EXPECT_FALSE((std::is_trivially_copy_constructible_v<decltype(e)>));
    EXPECT_FALSE((std::is_trivially_copy_assignable_v<decltype(e)>));
    EXPECT_FALSE((std::is_trivially_move_constructible_v<decltype(e)>));
    EXPECT_FALSE((std::is_trivially_move_assignable_v<decltype(e)>));
  }

  {
    expected<std::string, std::string> e;
    EXPECT_TRUE((std::is_default_constructible_v<decltype(e)>));
    EXPECT_TRUE((std::is_copy_constructible_v<decltype(e)>));
    EXPECT_TRUE((std::is_move_constructible_v<decltype(e)>));
    EXPECT_TRUE((std::is_copy_assignable_v<decltype(e)>));
    EXPECT_TRUE((std::is_move_assignable_v<decltype(e)>));
    EXPECT_FALSE((std::is_trivially_copy_constructible_v<decltype(e)>));
    EXPECT_FALSE((std::is_trivially_copy_assignable_v<decltype(e)>));
    EXPECT_FALSE((std::is_trivially_move_constructible_v<decltype(e)>));
    EXPECT_FALSE((std::is_trivially_move_assignable_v<decltype(e)>));
  }
}
