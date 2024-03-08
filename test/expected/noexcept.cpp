// SPDX-License-Identifier: CC0-1.0
#include <experimental/expected.hpp>
#include <gtest/gtest.h>
#include <type_traits>
#include <utility>

TEST(NoExceptTest, NoThrow) {
  using T = cxx20::expected<int, int>;
  EXPECT_TRUE(std::is_nothrow_default_constructible_v<T>);
  EXPECT_TRUE((std::is_nothrow_constructible_v<T, const int &>));
  EXPECT_TRUE((std::is_nothrow_constructible_v<T, int &&>));
  EXPECT_TRUE(
      (std::is_nothrow_constructible_v<T, std::in_place_t, const int &>));
  EXPECT_TRUE((std::is_nothrow_constructible_v<T, std::in_place_t, int &&>));
  EXPECT_TRUE(
      (std::is_nothrow_constructible_v<T, cxx20::unexpect_t, const int &>));
  EXPECT_TRUE((std::is_nothrow_constructible_v<T, cxx20::unexpect_t, int &&>));
  EXPECT_TRUE((std::is_nothrow_assignable_v<T &, const int &>));
  EXPECT_TRUE((std::is_nothrow_assignable_v<T &, int &&>));
  EXPECT_TRUE(
      (std::is_nothrow_assignable_v<T &, const cxx20::unexpected<int> &>));
  EXPECT_TRUE((std::is_nothrow_assignable_v<T &, cxx20::unexpected<int> &&>));
  EXPECT_TRUE(std::is_nothrow_destructible_v<T>);
  EXPECT_TRUE(std::is_nothrow_swappable_v<T>);
}

TEST(NoExceptTest, ThrowAll) {
  struct throw_all {
    [[noreturn]] [[maybe_unused]] throw_all() noexcept(false) { throw 0; }
    [[noreturn]] throw_all(const throw_all &) noexcept(false) { throw 0; }
    [[noreturn]] [[maybe_unused]] throw_all(throw_all &&) noexcept(false) {
      throw 0;
    }
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable:4722)
#endif
    [[noreturn]] ~throw_all() noexcept(false) { throw 0; }
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
    [[noreturn]] throw_all &operator=(const throw_all &) noexcept(false) {
      throw 0;
    }
    [[noreturn]] [[maybe_unused]] throw_all &
    operator=(throw_all &&) noexcept(false) {
      throw 0;
    }
  };
  using T = cxx20::expected<throw_all, int>;
  EXPECT_FALSE(std::is_nothrow_default_constructible_v<T>);
  EXPECT_FALSE((std::is_nothrow_constructible_v<T, const throw_all &>));
  EXPECT_FALSE((std::is_nothrow_constructible_v<T, throw_all &&>));
  EXPECT_FALSE(
      (std::is_nothrow_constructible_v<T, std::in_place_t, const throw_all &>));
  EXPECT_FALSE(
      (std::is_nothrow_constructible_v<T, std::in_place_t, throw_all &&>));
  {
    const int &y = 10;
    T x(cxx20::unexpect, y);
  }

  // because destructor throw
  EXPECT_FALSE(
      (std::is_nothrow_constructible_v<T, cxx20::unexpect_t, const int &>));
  EXPECT_FALSE((std::is_nothrow_constructible_v<T, cxx20::unexpect_t, int &&>));

  EXPECT_FALSE((std::is_nothrow_assignable_v<T &, const throw_all &>));
  EXPECT_FALSE((std::is_nothrow_assignable_v<T &, throw_all &&>));
  EXPECT_FALSE(
      (std::is_nothrow_assignable_v<T &, const cxx20::unexpected<int> &>));
  EXPECT_FALSE((std::is_nothrow_assignable_v<T &, cxx20::unexpected<int> &&>));
  EXPECT_FALSE(std::is_nothrow_destructible_v<T>);
  EXPECT_FALSE(std::is_nothrow_swappable_v<T>);
}

TEST(NoExceptTest, ThrowCopy) {
  struct throw_copy {
    [[maybe_unused]] throw_copy() noexcept {}
    [[noreturn]] throw_copy(const throw_copy &) noexcept(false) { throw 0; }
    [[maybe_unused]] throw_copy(throw_copy &&) noexcept {}
    [[maybe_unused]] ~throw_copy() noexcept {}
    [[noreturn]] throw_copy &operator=(const throw_copy &) noexcept(false) {
      throw 0;
    }
    [[maybe_unused]] throw_copy &operator=(throw_copy &&) noexcept {
      return *this;
    }
  };
  using T = cxx20::expected<throw_copy, int>;
  EXPECT_TRUE(std::is_nothrow_default_constructible_v<T>);
  EXPECT_FALSE((std::is_nothrow_constructible_v<T, const throw_copy &>));
  EXPECT_TRUE((std::is_nothrow_constructible_v<T, throw_copy &&>));
  EXPECT_FALSE((
      std::is_nothrow_constructible_v<T, std::in_place_t, const throw_copy &>));
  EXPECT_TRUE(
      (std::is_nothrow_constructible_v<T, std::in_place_t, throw_copy &&>));
  EXPECT_TRUE(
      (std::is_nothrow_constructible_v<T, cxx20::unexpect_t, const int &>));
  EXPECT_TRUE((std::is_nothrow_constructible_v<T, cxx20::unexpect_t, int &&>));
  EXPECT_FALSE((std::is_nothrow_assignable_v<T &, const throw_copy &>));
  EXPECT_TRUE((std::is_nothrow_assignable_v<T &, throw_copy &&>));
  EXPECT_TRUE(
      (std::is_nothrow_assignable_v<T &, const cxx20::unexpected<int> &>));
  EXPECT_TRUE((std::is_nothrow_assignable_v<T &, cxx20::unexpected<int> &&>));
  EXPECT_TRUE(std::is_nothrow_destructible_v<T>);
  EXPECT_TRUE(std::is_nothrow_swappable_v<T>);
}

TEST(NoExceptTest, ThrowMove) {
  struct throw_move {
    [[maybe_unused]] throw_move() noexcept {}
    throw_move(const throw_move &) noexcept {}
    [[noreturn]] [[maybe_unused]] throw_move(throw_move &&) noexcept(false) {
      throw 0;
    }
    [[maybe_unused]] ~throw_move() noexcept {}
    throw_move &operator=(const throw_move &) noexcept { return *this; }
    [[noreturn]] [[maybe_unused]] throw_move &
    operator=(throw_move &&) noexcept(false) {
      throw 0;
    }
  };
  using T = cxx20::expected<throw_move, int>;
  EXPECT_TRUE(std::is_nothrow_default_constructible_v<T>);
  EXPECT_TRUE((std::is_nothrow_constructible_v<T, const throw_move &>));
  EXPECT_FALSE((std::is_nothrow_constructible_v<T, throw_move &&>));
  EXPECT_TRUE((
      std::is_nothrow_constructible_v<T, std::in_place_t, const throw_move &>));
  EXPECT_FALSE(
      (std::is_nothrow_constructible_v<T, std::in_place_t, throw_move &&>));
  EXPECT_TRUE(
      (std::is_nothrow_constructible_v<T, cxx20::unexpect_t, const int &>));
  EXPECT_TRUE((std::is_nothrow_constructible_v<T, cxx20::unexpect_t, int &&>));
  EXPECT_TRUE((std::is_nothrow_assignable_v<T &, const throw_move &>));
  EXPECT_FALSE((std::is_nothrow_assignable_v<T &, throw_move &&>));
  EXPECT_TRUE(
      (std::is_nothrow_assignable_v<T &, const cxx20::unexpected<int> &>));
  EXPECT_TRUE((std::is_nothrow_assignable_v<T &, cxx20::unexpected<int> &&>));
  EXPECT_TRUE(std::is_nothrow_destructible_v<T>);
  EXPECT_FALSE(std::is_nothrow_swappable_v<T>);
}

TEST(NoExceptTest, ThrowCopyMove) {
  struct throw_copy_move {
    [[maybe_unused]] throw_copy_move() noexcept {}
    [[noreturn]] throw_copy_move(const throw_copy_move &) noexcept(false) {
      throw 0;
    }
    [[noreturn]] [[maybe_unused]] throw_copy_move(throw_copy_move &&) noexcept(
        false) {
      throw 0;
    }
    [[maybe_unused]] ~throw_copy_move() noexcept {}
    [[noreturn]] throw_copy_move &
    operator=(const throw_copy_move &) noexcept(false) {
      throw 0;
    }
    [[noreturn]] [[maybe_unused]] throw_copy_move &
    operator=(throw_copy_move &&) noexcept(false) {
      throw 0;
    }
  };
  using T = cxx20::expected<throw_copy_move, int>;
  EXPECT_TRUE(std::is_nothrow_default_constructible_v<T>);
  EXPECT_FALSE((std::is_nothrow_constructible_v<T, const throw_copy_move &>));
  EXPECT_FALSE((std::is_nothrow_constructible_v<T, throw_copy_move &&>));
  EXPECT_FALSE((std::is_nothrow_constructible_v<T, std::in_place_t,
                                                const throw_copy_move &>));
  EXPECT_FALSE((
      std::is_nothrow_constructible_v<T, std::in_place_t, throw_copy_move &&>));
  EXPECT_TRUE(
      (std::is_nothrow_constructible_v<T, cxx20::unexpect_t, const int &>));
  EXPECT_TRUE((std::is_nothrow_constructible_v<T, cxx20::unexpect_t, int &&>));
  EXPECT_FALSE((std::is_nothrow_assignable_v<T &, const throw_copy_move &>));
  EXPECT_FALSE((std::is_nothrow_assignable_v<T &, throw_copy_move &&>));
  EXPECT_TRUE(
      (std::is_nothrow_assignable_v<T &, const cxx20::unexpected<int> &>));
  EXPECT_TRUE((std::is_nothrow_assignable_v<T &, cxx20::unexpected<int> &&>));
  EXPECT_TRUE(std::is_nothrow_destructible_v<T>);
  EXPECT_FALSE(std::is_nothrow_swappable_v<T>);
}
