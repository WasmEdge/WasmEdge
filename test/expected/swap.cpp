// SPDX-License-Identifier: CC0-1.0
#include <experimental/expected.hpp>
#include <gtest/gtest.h>
#include <string>
#include <type_traits>

struct no_throw {
  no_throw(std::string i) : i(i) {}
  std::string i;
};
struct canthrow_move {
  canthrow_move(std::string i) : i(i) {}
  canthrow_move(canthrow_move const &) = default;
  canthrow_move(canthrow_move &&other) noexcept(false) : i(other.i) {}
  canthrow_move &operator=(canthrow_move &&) = default;
  std::string i;
};

template <bool should_throw = false> struct willthrow_move {
  willthrow_move(std::string i) : i(i) {}
  willthrow_move(willthrow_move const &) = default;
  willthrow_move(willthrow_move &&other) : i(other.i) {
    if (should_throw)
      throw 0;
  }
  willthrow_move &operator=(willthrow_move &&) = default;
  std::string i;
};
static_assert(std::is_swappable_v<no_throw>, "");

template <class T1, class T2> void swap_test() {
  std::string s1 = "abcdefghijklmnopqrstuvwxyz";
  std::string s2 = "zyxwvutsrqponmlkjihgfedcba";

  using std::swap;
  {
    cxx20::expected<T1, T2> a{s1};
    cxx20::expected<T1, T2> b{s2};
    swap(a, b);
    EXPECT_EQ(a->i, s2);
    EXPECT_EQ(b->i, s1);
  }

  {
    cxx20::expected<T1, T2> a{s1};
    cxx20::expected<T1, T2> b{cxx20::unexpected<T2>(s2)};
    swap(a, b);
    EXPECT_EQ(a.error().i, s2);
    EXPECT_EQ(b->i, s1);
  }

  {
    cxx20::expected<T1, T2> a{cxx20::unexpected<T2>(s1)};
    cxx20::expected<T1, T2> b{s2};
    swap(a, b);
    EXPECT_EQ(a->i, s2);
    EXPECT_EQ(b.error().i, s1);
  }

  {
    cxx20::expected<T1, T2> a{cxx20::unexpected<T2>(s1)};
    cxx20::expected<T1, T2> b{cxx20::unexpected<T2>(s2)};
    swap(a, b);
    EXPECT_EQ(a.error().i, s2);
    EXPECT_EQ(b.error().i, s1);
  }
}

TEST(SwapTest, Swap) {
  swap_test<no_throw, no_throw>();
  swap_test<no_throw, canthrow_move>();
  swap_test<canthrow_move, no_throw>();
  // swap_test<canthrow_move, canthrow_move>();

  std::string s1 = "abcdefghijklmnopqrstuvwxyz";
  std::string s2 = "zyxwvutsrqponmlkjihgfedcbaxxx";
  cxx20::expected<no_throw, willthrow_move<true>> a{s1};
  cxx20::expected<no_throw, willthrow_move<true>> b{cxx20::unexpect, s2};

  EXPECT_ANY_THROW(swap(a, b));

  EXPECT_EQ(a->i, s1);
  EXPECT_EQ(b.error().i, s2);
}

TEST(SwapTest, Compile) {
  EXPECT_TRUE((std::is_swappable_v<cxx20::expected<no_throw, no_throw>>));
  EXPECT_TRUE((std::is_swappable_v<cxx20::expected<no_throw, canthrow_move>>));
  EXPECT_TRUE((std::is_swappable_v<cxx20::expected<canthrow_move, no_throw>>));
  EXPECT_FALSE(
      (std::is_swappable_v<cxx20::expected<canthrow_move, canthrow_move>>));
}
