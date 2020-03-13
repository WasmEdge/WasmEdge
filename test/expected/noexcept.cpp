// SPDX-License-Identifier: CC0-1.0
#include "gtest/gtest.h"
#include <experimental/expected.hpp>

using std::experimental::expected;
using std::experimental::unexpect;
using std::experimental::unexpect_t;
using std::experimental::unexpected;

TEST(NoExceptTest, NoThrow) {
  using T = expected<int, int>;
  EXPECT_TRUE(std::is_nothrow_default_constructible_v<T>);
  EXPECT_TRUE((std::is_nothrow_constructible_v<T, const int &>));
  EXPECT_TRUE((std::is_nothrow_constructible_v<T, int &&>));
  EXPECT_TRUE(
      (std::is_nothrow_constructible_v<T, std::in_place_t, const int &>));
  EXPECT_TRUE((std::is_nothrow_constructible_v<T, std::in_place_t, int &&>));
  EXPECT_TRUE((std::is_nothrow_constructible_v<T, unexpect_t, const int &>));
  EXPECT_TRUE((std::is_nothrow_constructible_v<T, unexpect_t, int &&>));
  EXPECT_TRUE((std::is_nothrow_assignable_v<T &, const int &>));
  EXPECT_TRUE((std::is_nothrow_assignable_v<T &, int &&>));
  EXPECT_TRUE((std::is_nothrow_assignable_v<T &, const unexpected<int> &>));
  EXPECT_TRUE((std::is_nothrow_assignable_v<T &, unexpected<int> &&>));
  EXPECT_TRUE(std::is_nothrow_destructible_v<T>);
  EXPECT_TRUE(std::is_nothrow_swappable_v<T>);
}

TEST(NoExceptTest, ThrowAll) {
  struct throw_all {
    throw_all() noexcept(false) { throw 0; }
    throw_all(const throw_all &) noexcept(false) { throw 0; }
    throw_all(throw_all &&) noexcept(false) { throw 0; }
    ~throw_all() noexcept(false) { throw 0; }
    throw_all &operator=(const throw_all &) noexcept(false) { throw 0; }
    throw_all &operator=(throw_all &&) noexcept(false) { throw 0; }
  };
  using T = expected<throw_all, int>;
  EXPECT_FALSE(std::is_nothrow_default_constructible_v<T>);
  EXPECT_FALSE((std::is_nothrow_constructible_v<T, const throw_all &>));
  EXPECT_FALSE((std::is_nothrow_constructible_v<T, throw_all &&>));
  EXPECT_FALSE(
      (std::is_nothrow_constructible_v<T, std::in_place_t, const throw_all &>));
  EXPECT_FALSE(
      (std::is_nothrow_constructible_v<T, std::in_place_t, throw_all &&>));
  {
    const int &y = 10;
    T x(unexpect, y);
  }

  // because destructor throw
  EXPECT_FALSE((std::is_nothrow_constructible_v<T, unexpect_t, const int &>));
  EXPECT_FALSE((std::is_nothrow_constructible_v<T, unexpect_t, int &&>));

  EXPECT_FALSE((std::is_nothrow_assignable_v<T &, const throw_all &>));
  EXPECT_FALSE((std::is_nothrow_assignable_v<T &, throw_all &&>));
  EXPECT_FALSE((std::is_nothrow_assignable_v<T &, const unexpected<int> &>));
  EXPECT_FALSE((std::is_nothrow_assignable_v<T &, unexpected<int> &&>));
  EXPECT_FALSE(std::is_nothrow_destructible_v<T>);
  EXPECT_FALSE(std::is_nothrow_swappable_v<T>);
}

TEST(NoExceptTest, ThrowCopy) {
  struct throw_copy {
    throw_copy() noexcept {}
    throw_copy(const throw_copy &) noexcept(false) { throw 0; }
    throw_copy(throw_copy &&) noexcept {}
    ~throw_copy() noexcept {}
    throw_copy &operator=(const throw_copy &) noexcept(false) { throw 0; }
    throw_copy &operator=(throw_copy &&) noexcept { return *this; }
  };
  using T = expected<throw_copy, int>;
  EXPECT_TRUE(std::is_nothrow_default_constructible_v<T>);
  EXPECT_FALSE((std::is_nothrow_constructible_v<T, const throw_copy &>));
  EXPECT_TRUE((std::is_nothrow_constructible_v<T, throw_copy &&>));
  EXPECT_FALSE((
      std::is_nothrow_constructible_v<T, std::in_place_t, const throw_copy &>));
  EXPECT_TRUE(
      (std::is_nothrow_constructible_v<T, std::in_place_t, throw_copy &&>));
  EXPECT_TRUE((std::is_nothrow_constructible_v<T, unexpect_t, const int &>));
  EXPECT_TRUE((std::is_nothrow_constructible_v<T, unexpect_t, int &&>));
  EXPECT_FALSE((std::is_nothrow_assignable_v<T &, const throw_copy &>));
  EXPECT_TRUE((std::is_nothrow_assignable_v<T &, throw_copy &&>));
  EXPECT_TRUE((std::is_nothrow_assignable_v<T &, const unexpected<int> &>));
  EXPECT_TRUE((std::is_nothrow_assignable_v<T &, unexpected<int> &&>));
  EXPECT_TRUE(std::is_nothrow_destructible_v<T>);
  EXPECT_TRUE(std::is_nothrow_swappable_v<T>);
}

TEST(NoExceptTest, ThrowMove) {
  struct throw_move {
    throw_move() noexcept {}
    throw_move(const throw_move &) noexcept {}
    throw_move(throw_move &&) noexcept(false) { throw 0; }
    ~throw_move() noexcept {}
    throw_move &operator=(const throw_move &) noexcept { return *this; }
    throw_move &operator=(throw_move &&) noexcept(false) { throw 0; }
  };
  using T = expected<throw_move, int>;
  EXPECT_TRUE(std::is_nothrow_default_constructible_v<T>);
  EXPECT_TRUE((std::is_nothrow_constructible_v<T, const throw_move &>));
  EXPECT_FALSE((std::is_nothrow_constructible_v<T, throw_move &&>));
  EXPECT_TRUE((
      std::is_nothrow_constructible_v<T, std::in_place_t, const throw_move &>));
  EXPECT_FALSE(
      (std::is_nothrow_constructible_v<T, std::in_place_t, throw_move &&>));
  EXPECT_TRUE((std::is_nothrow_constructible_v<T, unexpect_t, const int &>));
  EXPECT_TRUE((std::is_nothrow_constructible_v<T, unexpect_t, int &&>));
  EXPECT_TRUE((std::is_nothrow_assignable_v<T &, const throw_move &>));
  EXPECT_FALSE((std::is_nothrow_assignable_v<T &, throw_move &&>));
  EXPECT_TRUE((std::is_nothrow_assignable_v<T &, const unexpected<int> &>));
  EXPECT_TRUE((std::is_nothrow_assignable_v<T &, unexpected<int> &&>));
  EXPECT_TRUE(std::is_nothrow_destructible_v<T>);
  EXPECT_FALSE(std::is_nothrow_swappable_v<T>);
}

TEST(NoExceptTest, ThrowCopyMove) {
  struct throw_copy_move {
    throw_copy_move() noexcept {}
    throw_copy_move(const throw_copy_move &) noexcept(false) { throw 0; }
    throw_copy_move(throw_copy_move &&) noexcept(false) { throw 0; }
    ~throw_copy_move() noexcept {}
    throw_copy_move &operator=(const throw_copy_move &) noexcept(false) {
      throw 0;
    }
    throw_copy_move &operator=(throw_copy_move &&) noexcept(false) { throw 0; }
  };
  using T = expected<throw_copy_move, int>;
  EXPECT_TRUE(std::is_nothrow_default_constructible_v<T>);
  EXPECT_FALSE((std::is_nothrow_constructible_v<T, const throw_copy_move &>));
  EXPECT_FALSE((std::is_nothrow_constructible_v<T, throw_copy_move &&>));
  EXPECT_FALSE((std::is_nothrow_constructible_v<T, std::in_place_t,
                                                const throw_copy_move &>));
  EXPECT_FALSE((
      std::is_nothrow_constructible_v<T, std::in_place_t, throw_copy_move &&>));
  EXPECT_TRUE((std::is_nothrow_constructible_v<T, unexpect_t, const int &>));
  EXPECT_TRUE((std::is_nothrow_constructible_v<T, unexpect_t, int &&>));
  EXPECT_FALSE((std::is_nothrow_assignable_v<T &, const throw_copy_move &>));
  EXPECT_FALSE((std::is_nothrow_assignable_v<T &, throw_copy_move &&>));
  EXPECT_TRUE((std::is_nothrow_assignable_v<T &, const unexpected<int> &>));
  EXPECT_TRUE((std::is_nothrow_assignable_v<T &, unexpected<int> &&>));
  EXPECT_TRUE(std::is_nothrow_destructible_v<T>);
  EXPECT_FALSE(std::is_nothrow_swappable_v<T>);
}
