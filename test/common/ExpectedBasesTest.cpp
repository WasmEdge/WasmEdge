// SPDX-License-Identifier: CC0-1.0
#include <experimental/expected.hpp>
#include <gtest/gtest.h>
#include <string>
#include <type_traits>

TEST(BaseTest, Triviality) {
  EXPECT_TRUE(
      (std::is_trivially_copy_constructible_v<cxx20::expected<int, int>>));
  EXPECT_TRUE((std::is_trivially_copy_assignable_v<cxx20::expected<int, int>>));
  EXPECT_TRUE(
      (std::is_trivially_move_constructible_v<cxx20::expected<int, int>>));
  EXPECT_TRUE((std::is_trivially_move_assignable_v<cxx20::expected<int, int>>));
  EXPECT_TRUE((std::is_trivially_destructible_v<cxx20::expected<int, int>>));

  EXPECT_TRUE(
      (std::is_trivially_copy_constructible_v<cxx20::expected<void, int>>));
  EXPECT_TRUE(
      (std::is_trivially_move_constructible_v<cxx20::expected<void, int>>));
  EXPECT_TRUE((std::is_trivially_destructible_v<cxx20::expected<void, int>>));

  {
    struct T {
      T(const T &) = default;
      [[maybe_unused]] T(T &&) = default;
      T &operator=(const T &) = default;
      [[maybe_unused]] T &operator=(T &&) = default;
      [[maybe_unused]] ~T() = default;
    };
    EXPECT_TRUE(
        (std::is_trivially_copy_constructible_v<cxx20::expected<T, int>>));
    EXPECT_TRUE((std::is_trivially_copy_assignable_v<cxx20::expected<T, int>>));
    EXPECT_TRUE(
        (std::is_trivially_move_constructible_v<cxx20::expected<T, int>>));
    EXPECT_TRUE((std::is_trivially_move_assignable_v<cxx20::expected<T, int>>));
    EXPECT_TRUE((std::is_trivially_destructible_v<cxx20::expected<T, int>>));
  }

  {
    struct T {
      T(const T &) {}
      [[maybe_unused]] T(T &&) {}
      T &operator=(const T &) { return *this; }
      [[maybe_unused]] T &operator=(T &&) { return *this; }
      [[maybe_unused]] ~T() {}
    };
    EXPECT_FALSE(
        (std::is_trivially_copy_constructible_v<cxx20::expected<T, int>>));
    EXPECT_FALSE(
        (std::is_trivially_copy_assignable_v<cxx20::expected<T, int>>));
    EXPECT_FALSE(
        (std::is_trivially_move_constructible_v<cxx20::expected<T, int>>));
    EXPECT_FALSE(
        (std::is_trivially_move_assignable_v<cxx20::expected<T, int>>));
    EXPECT_FALSE((std::is_trivially_destructible_v<cxx20::expected<T, int>>));
  }
}

TEST(BaseTest, Deletion) {
  EXPECT_TRUE((std::is_copy_constructible_v<cxx20::expected<int, int>>));
  EXPECT_TRUE((std::is_copy_assignable_v<cxx20::expected<int, int>>));
  EXPECT_TRUE((std::is_move_constructible_v<cxx20::expected<int, int>>));
  EXPECT_TRUE((std::is_move_assignable_v<cxx20::expected<int, int>>));
  EXPECT_TRUE((std::is_destructible_v<cxx20::expected<int, int>>));

  {
    struct T {
      [[maybe_unused]] T() = default;
    };
    EXPECT_TRUE((std::is_default_constructible_v<cxx20::expected<T, int>>));
  }

  {
    struct T {
      [[maybe_unused]] T(int) {}
    };
    EXPECT_FALSE((std::is_default_constructible_v<cxx20::expected<T, int>>));
  }

  {
    struct T {
      T(const T &) = default;
      [[maybe_unused]] T(T &&) = default;
      T &operator=(const T &) = default;
      [[maybe_unused]] T &operator=(T &&) = default;
      [[maybe_unused]] ~T() = default;
    };
    EXPECT_TRUE((std::is_copy_constructible_v<cxx20::expected<T, int>>));
    EXPECT_TRUE((std::is_copy_assignable_v<cxx20::expected<T, int>>));
    EXPECT_TRUE((std::is_move_constructible_v<cxx20::expected<T, int>>));
    EXPECT_TRUE((std::is_move_assignable_v<cxx20::expected<T, int>>));
    EXPECT_TRUE((std::is_destructible_v<cxx20::expected<T, int>>));
  }

  {
    struct T {
      T(const T &) = delete;
      T(T &&) = delete;
      T &operator=(const T &) = delete;
      T &operator=(T &&) = delete;
    };
    EXPECT_FALSE((std::is_copy_constructible_v<cxx20::expected<T, int>>));
    EXPECT_FALSE((std::is_copy_assignable_v<cxx20::expected<T, int>>));
    EXPECT_FALSE((std::is_move_constructible_v<cxx20::expected<T, int>>));
    EXPECT_FALSE((std::is_move_assignable_v<cxx20::expected<T, int>>));
  }

  {
    struct T {
      T(const T &) = delete;
      [[maybe_unused]] T(T &&) = default;
      T &operator=(const T &) = delete;
      [[maybe_unused]] T &operator=(T &&) = default;
    };
    EXPECT_FALSE((std::is_copy_constructible_v<cxx20::expected<T, int>>));
    EXPECT_FALSE((std::is_copy_assignable_v<cxx20::expected<T, int>>));
    EXPECT_TRUE((std::is_move_constructible_v<cxx20::expected<T, int>>));
    EXPECT_TRUE((std::is_move_assignable_v<cxx20::expected<T, int>>));
  }

  {
    struct T {
      T(const T &) = default;
      T(T &&) = delete;
      T &operator=(const T &) = default;
      T &operator=(T &&) = delete;
    };
    EXPECT_TRUE((std::is_copy_constructible_v<cxx20::expected<T, int>>));
    EXPECT_TRUE((std::is_copy_assignable_v<cxx20::expected<T, int>>));
  }

  {
    cxx20::expected<int, int> e;
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
    cxx20::expected<int, std::string> e;
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
    cxx20::expected<std::string, int> e;
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
    cxx20::expected<std::string, std::string> e;
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
