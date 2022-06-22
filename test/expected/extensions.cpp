// SPDX-License-Identifier: CC0-1.0
#include <experimental/expected.hpp>
#include <gtest/gtest.h>
#include <memory>
#include <type_traits>
#include <utility>

using cxx20::expected;
using cxx20::unexpect;

TEST(ExtensionsTest, Map) {
  auto mul2 = [](int a) { return a * 2; };
  auto ret_void = [](int) {};

  {
    expected<int, int> e = 21;
    auto ret = e.map(mul2);
    EXPECT_TRUE(ret);
    EXPECT_EQ(*ret, 42);
  }

  {
    const expected<int, int> e = 21;
    auto ret = e.map(mul2);
    EXPECT_TRUE(ret);
    EXPECT_EQ(*ret, 42);
  }

  {
    expected<int, int> e = 21;
    auto ret = std::move(e).map(mul2);
    EXPECT_TRUE(ret);
    EXPECT_EQ(*ret, 42);
  }

  {
    const expected<int, int> e = 21;
    auto ret = std::move(e).map(mul2);
    EXPECT_TRUE(ret);
    EXPECT_EQ(*ret, 42);
  }

  {
    expected<int, int> e(unexpect, 21);
    auto ret = e.map(mul2);
    EXPECT_FALSE(ret);
    EXPECT_EQ(ret.error(), 21);
  }

  {
    const expected<int, int> e(unexpect, 21);
    auto ret = e.map(mul2);
    EXPECT_FALSE(ret);
    EXPECT_EQ(ret.error(), 21);
  }

  {
    expected<int, int> e(unexpect, 21);
    auto ret = std::move(e).map(mul2);
    EXPECT_FALSE(ret);
    EXPECT_EQ(ret.error(), 21);
  }

  {
    const expected<int, int> e(unexpect, 21);
    auto ret = std::move(e).map(mul2);
    EXPECT_FALSE(ret);
    EXPECT_EQ(ret.error(), 21);
  }

  {
    expected<int, int> e = 21;
    auto ret = e.map(ret_void);
    EXPECT_TRUE(ret);
    EXPECT_TRUE((std::is_same_v<decltype(ret), expected<void, int>>));
  }

  {
    const expected<int, int> e = 21;
    auto ret = e.map(ret_void);
    EXPECT_TRUE(ret);
    EXPECT_TRUE((std::is_same_v<decltype(ret), expected<void, int>>));
  }

  {
    expected<int, int> e = 21;
    auto ret = std::move(e).map(ret_void);
    EXPECT_TRUE(ret);
    EXPECT_TRUE((std::is_same_v<decltype(ret), expected<void, int>>));
  }

  {
    const expected<int, int> e = 21;
    auto ret = std::move(e).map(ret_void);
    EXPECT_TRUE(ret);
    EXPECT_TRUE((std::is_same_v<decltype(ret), expected<void, int>>));
  }

  {
    expected<int, int> e(unexpect, 21);
    auto ret = e.map(ret_void);
    EXPECT_FALSE(ret);
    EXPECT_TRUE((std::is_same_v<decltype(ret), expected<void, int>>));
  }

  {
    const expected<int, int> e(unexpect, 21);
    auto ret = e.map(ret_void);
    EXPECT_FALSE(ret);
    EXPECT_TRUE((std::is_same_v<decltype(ret), expected<void, int>>));
  }

  {
    expected<int, int> e(unexpect, 21);
    auto ret = std::move(e).map(ret_void);
    EXPECT_FALSE(ret);
    EXPECT_TRUE((std::is_same_v<decltype(ret), expected<void, int>>));
  }

  {
    const expected<int, int> e(unexpect, 21);
    auto ret = std::move(e).map(ret_void);
    EXPECT_FALSE(ret);
    EXPECT_TRUE((std::is_same_v<decltype(ret), expected<void, int>>));
  }

  // mapping functions which return references
  {
    expected<int, int> e(42);
    auto ret = e.map([](int &i) -> int & { return i; });
    EXPECT_TRUE(ret);
    EXPECT_EQ(ret, 42);
  }
}

TEST(ExtensionsTest, MapError) {
  auto mul2 = [](int a) { return a * 2; };
  auto ret_void = [](int) {};

  {
    expected<int, int> e = 21;
    auto ret = e.map_error(mul2);
    EXPECT_TRUE(ret);
    EXPECT_EQ(*ret, 21);
  }

  {
    const expected<int, int> e = 21;
    auto ret = e.map_error(mul2);
    EXPECT_TRUE(ret);
    EXPECT_EQ(*ret, 21);
  }

  {
    expected<int, int> e = 21;
    auto ret = std::move(e).map_error(mul2);
    EXPECT_TRUE(ret);
    EXPECT_EQ(*ret, 21);
  }

  {
    const expected<int, int> e = 21;
    auto ret = std::move(e).map_error(mul2);
    EXPECT_TRUE(ret);
    EXPECT_EQ(*ret, 21);
  }

  {
    expected<int, int> e(unexpect, 21);
    auto ret = e.map_error(mul2);
    EXPECT_FALSE(ret);
    EXPECT_EQ(ret.error(), 42);
  }

  {
    const expected<int, int> e(unexpect, 21);
    auto ret = e.map_error(mul2);
    EXPECT_FALSE(ret);
    EXPECT_EQ(ret.error(), 42);
  }

  {
    expected<int, int> e(unexpect, 21);
    auto ret = std::move(e).map_error(mul2);
    EXPECT_FALSE(ret);
    EXPECT_EQ(ret.error(), 42);
  }

  {
    const expected<int, int> e(unexpect, 21);
    auto ret = std::move(e).map_error(mul2);
    EXPECT_FALSE(ret);
    EXPECT_EQ(ret.error(), 42);
  }

  {
    expected<int, int> e = 21;
    auto ret = e.map_error(ret_void);
    EXPECT_TRUE(ret);
  }

  {
    const expected<int, int> e = 21;
    auto ret = e.map_error(ret_void);
    EXPECT_TRUE(ret);
  }

  {
    expected<int, int> e = 21;
    auto ret = std::move(e).map_error(ret_void);
    EXPECT_TRUE(ret);
  }

  {
    const expected<int, int> e = 21;
    auto ret = std::move(e).map_error(ret_void);
    EXPECT_TRUE(ret);
  }

  {
    expected<int, int> e(unexpect, 21);
    auto ret = e.map_error(ret_void);
    EXPECT_FALSE(ret);
  }

  {
    const expected<int, int> e(unexpect, 21);
    auto ret = e.map_error(ret_void);
    EXPECT_FALSE(ret);
  }

  {
    expected<int, int> e(unexpect, 21);
    auto ret = std::move(e).map_error(ret_void);
    EXPECT_FALSE(ret);
  }

  {
    const expected<int, int> e(unexpect, 21);
    auto ret = std::move(e).map_error(ret_void);
    EXPECT_FALSE(ret);
  }
}

TEST(ExtensionsTest, AndThen) {
  auto succeed = [](int) { return expected<int, int>(21 * 2); };
  auto fail = [](int) { return expected<int, int>(unexpect, 17); };

  {
    expected<int, int> e = 21;
    auto ret = e.and_then(succeed);
    EXPECT_TRUE(ret);
    EXPECT_EQ(*ret, 42);
  }

  {
    const expected<int, int> e = 21;
    auto ret = e.and_then(succeed);
    EXPECT_TRUE(ret);
    EXPECT_EQ(*ret, 42);
  }

  {
    expected<int, int> e = 21;
    auto ret = std::move(e).and_then(succeed);
    EXPECT_TRUE(ret);
    EXPECT_EQ(*ret, 42);
  }

  {
    const expected<int, int> e = 21;
    auto ret = std::move(e).and_then(succeed);
    EXPECT_TRUE(ret);
    EXPECT_EQ(*ret, 42);
  }

  {
    expected<int, int> e = 21;
    auto ret = e.and_then(fail);
    EXPECT_FALSE(ret);
    EXPECT_EQ(ret.error(), 17);
  }

  {
    const expected<int, int> e = 21;
    auto ret = e.and_then(fail);
    EXPECT_FALSE(ret);
    EXPECT_EQ(ret.error(), 17);
  }

  {
    expected<int, int> e = 21;
    auto ret = std::move(e).and_then(fail);
    EXPECT_FALSE(ret);
    EXPECT_EQ(ret.error(), 17);
  }

  {
    const expected<int, int> e = 21;
    auto ret = std::move(e).and_then(fail);
    EXPECT_FALSE(ret);
    EXPECT_EQ(ret.error(), 17);
  }

  {
    expected<int, int> e(unexpect, 21);
    auto ret = e.and_then(succeed);
    EXPECT_FALSE(ret);
    EXPECT_EQ(ret.error(), 21);
  }

  {
    const expected<int, int> e(unexpect, 21);
    auto ret = e.and_then(succeed);
    EXPECT_FALSE(ret);
    EXPECT_EQ(ret.error(), 21);
  }

  {
    expected<int, int> e(unexpect, 21);
    auto ret = std::move(e).and_then(succeed);
    EXPECT_FALSE(ret);
    EXPECT_EQ(ret.error(), 21);
  }

  {
    const expected<int, int> e(unexpect, 21);
    auto ret = std::move(e).and_then(succeed);
    EXPECT_FALSE(ret);
    EXPECT_EQ(ret.error(), 21);
  }

  {
    expected<int, int> e(unexpect, 21);
    auto ret = e.and_then(fail);
    EXPECT_FALSE(ret);
    EXPECT_EQ(ret.error(), 21);
  }

  {
    const expected<int, int> e(unexpect, 21);
    auto ret = e.and_then(fail);
    EXPECT_FALSE(ret);
    EXPECT_EQ(ret.error(), 21);
  }

  {
    expected<int, int> e(unexpect, 21);
    auto ret = std::move(e).and_then(fail);
    EXPECT_FALSE(ret);
    EXPECT_EQ(ret.error(), 21);
  }

  {
    const expected<int, int> e(unexpect, 21);
    auto ret = std::move(e).and_then(fail);
    EXPECT_FALSE(ret);
    EXPECT_EQ(ret.error(), 21);
  }
}

TEST(ExtensionsTest, OrElse) {
  using eptr = std::unique_ptr<int>;
  auto succeed = [](int) { return expected<int, int>(21 * 2); };
  auto succeedptr = [](eptr) { return expected<int, eptr>(21 * 2); };
  auto fail = [](int) { return expected<int, int>(unexpect, 17); };
  auto efail = [](eptr e) {
    *e = 17;
    return expected<int, eptr>(unexpect, std::move(e));
  };
  auto failvoid = [](int) {};
  auto failvoidptr = [](const eptr &) { /* don't consume */ };
  auto consumeptr = [](eptr) {};
  auto make_u_int = [](int n) { return std::make_unique<int>(n); };

  {
    expected<int, int> e = 21;
    auto ret = e.or_else(succeed);
    EXPECT_TRUE(ret);
    EXPECT_EQ(*ret, 21);
  }

  {
    const expected<int, int> e = 21;
    auto ret = e.or_else(succeed);
    EXPECT_TRUE(ret);
    EXPECT_EQ(*ret, 21);
  }

  {
    expected<int, int> e = 21;
    auto ret = std::move(e).or_else(succeed);
    EXPECT_TRUE(ret);
    EXPECT_EQ(*ret, 21);
  }

  {
    expected<int, eptr> e = 21;
    auto ret = std::move(e).or_else(succeedptr);
    EXPECT_TRUE(ret);
    EXPECT_EQ(*ret, 21);
  }

  {
    const expected<int, int> e = 21;
    auto ret = std::move(e).or_else(succeed);
    EXPECT_TRUE(ret);
    EXPECT_EQ(*ret, 21);
  }

  {
    expected<int, int> e = 21;
    auto ret = e.or_else(fail);
    EXPECT_TRUE(ret);
    EXPECT_EQ(*ret, 21);
  }

  {
    const expected<int, int> e = 21;
    auto ret = e.or_else(fail);
    EXPECT_TRUE(ret);
    EXPECT_EQ(*ret, 21);
  }

  {
    expected<int, int> e = 21;
    auto ret = std::move(e).or_else(fail);
    EXPECT_TRUE(ret);
    EXPECT_EQ(ret, 21);
  }

  {
    expected<int, eptr> e = 21;
    auto ret = std::move(e).or_else(efail);
    EXPECT_TRUE(ret);
    EXPECT_EQ(ret, 21);
  }

  {
    const expected<int, int> e = 21;
    auto ret = std::move(e).or_else(fail);
    EXPECT_TRUE(ret);
    EXPECT_EQ(*ret, 21);
  }

  {
    expected<int, int> e(unexpect, 21);
    auto ret = e.or_else(succeed);
    EXPECT_TRUE(ret);
    EXPECT_EQ(*ret, 42);
  }

  {
    const expected<int, int> e(unexpect, 21);
    auto ret = e.or_else(succeed);
    EXPECT_TRUE(ret);
    EXPECT_EQ(*ret, 42);
  }

  {
    expected<int, int> e(unexpect, 21);
    auto ret = std::move(e).or_else(succeed);
    EXPECT_TRUE(ret);
    EXPECT_EQ(*ret, 42);
  }

  {
    expected<int, eptr> e(unexpect, make_u_int(21));
    auto ret = std::move(e).or_else(succeedptr);
    EXPECT_TRUE(ret);
    EXPECT_EQ(*ret, 42);
  }

  {
    const expected<int, int> e(unexpect, 21);
    auto ret = std::move(e).or_else(succeed);
    EXPECT_TRUE(ret);
    EXPECT_EQ(*ret, 42);
  }

  {
    expected<int, int> e(unexpect, 21);
    auto ret = e.or_else(fail);
    EXPECT_FALSE(ret);
    EXPECT_EQ(ret.error(), 17);
  }

  {
    expected<int, int> e(unexpect, 21);
    auto ret = e.or_else(failvoid);
    EXPECT_FALSE(ret);
    EXPECT_EQ(ret.error(), 21);
  }

  {
    const expected<int, int> e(unexpect, 21);
    auto ret = e.or_else(fail);
    EXPECT_FALSE(ret);
    EXPECT_EQ(ret.error(), 17);
  }

  {
    const expected<int, int> e(unexpect, 21);
    auto ret = e.or_else(failvoid);
    EXPECT_FALSE(ret);
    EXPECT_EQ(ret.error(), 21);
  }

  {
    expected<int, int> e(unexpect, 21);
    auto ret = std::move(e).or_else(fail);
    EXPECT_FALSE(ret);
    EXPECT_EQ(ret.error(), 17);
  }

  {
    expected<int, int> e(unexpect, 21);
    auto ret = std::move(e).or_else(failvoid);
    EXPECT_FALSE(ret);
    EXPECT_EQ(ret.error(), 21);
  }

  {
    expected<int, eptr> e(unexpect, make_u_int(21));
    auto ret = std::move(e).or_else(failvoidptr);
    EXPECT_FALSE(ret);
    EXPECT_EQ(*ret.error(), 21);
  }

  {
    expected<int, eptr> e(unexpect, make_u_int(21));
    auto ret = std::move(e).or_else(consumeptr);
    EXPECT_FALSE(ret);
    EXPECT_EQ(ret.error(), nullptr);
  }

  {
    const expected<int, int> e(unexpect, 21);
    auto ret = std::move(e).or_else(fail);
    EXPECT_FALSE(ret);
    EXPECT_EQ(ret.error(), 17);
  }

  {
    const expected<int, int> e(unexpect, 21);
    auto ret = std::move(e).or_else(failvoid);
    EXPECT_FALSE(ret);
    EXPECT_EQ(ret.error(), 21);
  }
}
struct S {
  int x;
};

struct F {
  int x;
};

TEST(ExtensionsTest, Issue14) {
  auto res = expected<S, F>{unexpect, F{}};

  res.map_error([](F) {});
}

TEST(ExtensionsTest, Issue32) {
  int i = 0;
  expected<void, int> a;
  a.map([&i] { i = 42; });
  EXPECT_EQ(i, 42);

  auto x = a.map([] { return 42; });
  EXPECT_EQ(*x, 42);
}
