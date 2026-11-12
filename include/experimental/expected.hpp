// SPDX-License-Identifier: CC0-1.0
///
// expected - An c++17 implementation of std::expected with extensions
// Written in 2017 by Simon Brand (simonrbrand@gmail.com, @TartanLlama)
// Modified in 2020 by Shen-Ta Hsieh (ibmibmibm.tw@gmail.com, @ibmibmibm)
//
// To the extent possible under law, the author(s) have dedicated all
// copyright and related and neighboring rights to this software to the
// public domain worldwide. This software is distributed without any warranty.
//
// You should have received a copy of the CC0 Public Domain Dedication
// along with this software. If not, see
// <http://creativecommons.org/publicdomain/zero/1.0/>.
///

#pragma once
#include <functional>
#include <type_traits>
#include <utility>
#include <variant>

#if defined(_MSC_VER) && !defined(__clang__)
#define __builtin_expect(exp, c) (exp)
#endif

#if defined(__has_feature)
#if __has_feature(cxx_exceptions)
#define M_ENABLE_EXCEPTIONS 1
#else
#define M_ENABLE_EXCEPTIONS 0
#endif
#elif defined(__GNUC__)
#ifdef __EXCEPTIONS
#define M_ENABLE_EXCEPTIONS 1
#else
#define M_ENABLE_EXCEPTIONS 0
#endif
#elif defined(_MSC_VER)
#ifdef _CPPUNWIND
#define M_ENABLE_EXCEPTIONS 1
#else
#define M_ENABLE_EXCEPTIONS 0
#endif
#endif

#ifndef M_ENABLE_EXCEPTIONS
#define M_ENABLE_EXCEPTIONS 1
#endif

#if M_ENABLE_EXCEPTIONS
#define throw_exception_again throw
#else
#define try if (true)
#define catch(X) if (false)
#define throw(X) abort()
#define throw_exception_again
#endif

namespace cxx20 {
using namespace std;

template <class T, class E> class expected;
template <class E> class unexpected;
template <class E> unexpected(E) -> unexpected<E>;

template <class E> class bad_expected_access;
template <> class bad_expected_access<void> : public exception {
public:
  explicit bad_expected_access() = default;
};
template <class E>
class bad_expected_access : public bad_expected_access<void> {
public:
  explicit bad_expected_access(E e) : m_error(std::move(e)) {}
  const char *what() const noexcept override { return "Bad expected access"; }
  const E &error() const & { return m_error; }
  const E &&error() const && { return std::move(m_error); }
  E &error() & { return m_error; }
  E &&error() && { return std::move(m_error); }

private:
  E m_error;
};

struct unexpect_t {
  explicit constexpr unexpect_t() = default;
};
inline constexpr unexpect_t unexpect{};

namespace detail {

template <class T> using remove_cvref_t = remove_cv_t<remove_reference_t<T>>;

template <class A, class B>
static inline constexpr bool
    is_not_constructible_and_not_reverse_convertible_v =
        !is_constructible_v<A, B &> && !is_constructible_v<A, B &&> &&
        !is_constructible_v<A, const B &> &&
        !is_constructible_v<A, const B &&> && !is_convertible_v<B &, A> &&
        !is_convertible_v<B &&, A> && !is_convertible_v<const B &, A> &&
        !is_convertible_v<const B &&, A>;

struct no_init_t {
  explicit constexpr no_init_t() = default;
};
inline constexpr no_init_t no_init{};

template <class T> struct is_expected : false_type {};
template <class T, class E> struct is_expected<expected<T, E>> : true_type {};
template <class T>
static inline constexpr bool is_expected_v = is_expected<T>::value;

static inline bool likely(bool V) { return __builtin_expect(V, true); }

} // namespace detail

template <class E> class unexpected {
  E m_val;

public:
  constexpr unexpected(const unexpected &) = default;
  constexpr unexpected(unexpected &&) = default;
  constexpr unexpected &operator=(const unexpected &) = default;

  template <class... Args,
            enable_if_t<is_constructible_v<E, Args...>> * = nullptr,
            bool NoExcept = is_nothrow_constructible_v<E, Args...>>
  constexpr explicit unexpected(in_place_t, Args &&...args) noexcept(NoExcept)
      : m_val(std::forward<Args>(args)...) {}
  template <class U, class... Args,
            enable_if_t<is_constructible_v<E, initializer_list<U>, Args...>> * =
                nullptr,
            bool NoExcept =
                is_nothrow_constructible_v<E, initializer_list<U>, Args...>>
  constexpr explicit unexpected(in_place_t, initializer_list<U> il,
                                Args &&...args) noexcept(NoExcept)
      : m_val(il, std::forward<Args>(args)...) {}
  template <class Err = E,
            enable_if_t<is_constructible_v<E, Err> &&
                        !is_same_v<detail::remove_cvref_t<E>, in_place_t> &&
                        !is_same_v<detail::remove_cvref_t<E>, unexpected>> * =
                nullptr,
            bool NoExcept = is_nothrow_constructible_v<E, Err>>
  constexpr explicit unexpected(Err &&e) noexcept(NoExcept)
      : m_val(std::forward<Err>(e)) {}

  template <
      class Err,
      enable_if_t<is_constructible_v<E, const Err &> &&
                  detail::is_not_constructible_and_not_reverse_convertible_v<
                      E, unexpected<Err>>> * = nullptr,
      bool NoExcept = is_nothrow_constructible_v<E, const Err &>>
  constexpr unexpected(const unexpected<Err> &rhs) noexcept(NoExcept)
      : m_val(rhs.m_val) {}
  template <
      class Err,
      enable_if_t<is_constructible_v<E, Err &&> &&
                  detail::is_not_constructible_and_not_reverse_convertible_v<
                      E, unexpected<Err>>> * = nullptr,
      bool NoExcept = is_nothrow_constructible_v<E, Err &&>>
  constexpr unexpected(unexpected<Err> &&rhs) noexcept(NoExcept)
      : m_val(std::move(rhs.m_val)) {}

  template <class Err = E,
            enable_if_t<is_assignable_v<E, const Err &>> * = nullptr,
            bool NoExcept = is_nothrow_assignable_v<E, const Err &>>
  constexpr unexpected &operator=(const unexpected<Err> &e) noexcept(NoExcept) {
    m_val = e.m_val;
    return *this;
  }
  template <class Err = E, enable_if_t<is_assignable_v<E, Err &&>> * = nullptr,
            bool NoExcept = is_nothrow_assignable_v<E, Err &&>>
  constexpr unexpected &operator=(unexpected<Err> &&e) noexcept(NoExcept) {
    m_val = std::move(e.m_val);
    return *this;
  }
  constexpr const E &value() const &noexcept { return m_val; }
  constexpr E &value() &noexcept { return m_val; }
  constexpr E &&value() &&noexcept { return std::move(m_val); }
  constexpr const E &&value() const &&noexcept { return std::move(m_val); }

  template <class Err = E, enable_if_t<is_swappable_v<Err>> * = nullptr,
            bool NoExcept = is_nothrow_swappable_v<Err>>
  void swap(unexpected<Err> &rhs) noexcept(NoExcept) {
    using std::swap;
    swap(m_val, rhs.m_val);
  }

  template <class Err = E, enable_if_t<!is_swappable_v<Err>> * = nullptr>
  void swap(unexpected<Err> &rhs) = delete;
};
template <class E1, class E2,
          bool NoExcept = noexcept(declval<E1>() == declval<E2>())>
constexpr bool operator==(const unexpected<E1> &x,
                          const unexpected<E2> &y) noexcept(NoExcept) {
  return x.value() == y.value();
}
template <class E1, class E2,
          bool NoExcept = noexcept(declval<E1>() != declval<E2>())>
constexpr bool operator!=(const unexpected<E1> &x,
                          const unexpected<E2> &y) noexcept(NoExcept) {
  return x.value() != y.value();
}

namespace detail {

template <class T, class E> struct expected_traits {
  template <class U, class G>
  static inline constexpr bool enable_other_copy_constructible_v =
      is_constructible_v<T, const U &> && is_constructible_v<E, const G &> &&
      is_not_constructible_and_not_reverse_convertible_v<T, expected<U, G>> &&
      detail::is_not_constructible_and_not_reverse_convertible_v<
          unexpected<E>, expected<U, G>>;
  template <class U, class G>
  static inline constexpr bool explicit_other_copy_constructible_v =
      !is_convertible_v<const U &, T> || !is_convertible_v<const G &, E>;

  template <class U, class G>
  static inline constexpr bool enable_other_move_constructible_v =
      is_constructible_v<T, U &&> && is_constructible_v<E, G &&> &&
      is_not_constructible_and_not_reverse_convertible_v<T, expected<U, G>> &&
      detail::is_not_constructible_and_not_reverse_convertible_v<
          unexpected<E>, expected<U, G>>;
  template <class U, class G>
  static inline constexpr bool explicit_other_move_constructible_v =
      !is_convertible_v<U &&, T> || !is_convertible_v<G &&, E>;

  template <class U, class G>
  static inline constexpr bool is_nothrow_other_copy_constructible_v =
      is_nothrow_constructible_v<T, const U &> &&
      is_nothrow_constructible_v<E, const G &>;
  template <class U, class G>
  static inline constexpr bool is_nothrow_other_move_constructible_v =
      is_nothrow_constructible_v<T, U &&> &&
      is_nothrow_constructible_v<E, G &&>;
  template <class U>
  static inline constexpr bool enable_in_place_v =
      is_constructible_v<T, U> && !is_same_v<remove_cvref_t<U>, in_place_t> &&
      !is_same_v<expected<T, E>, remove_cvref_t<U>> &&
      !is_same_v<unexpected<E>, remove_cvref_t<U>>;
  static inline constexpr bool enable_emplace_value_v =
      is_nothrow_move_constructible_v<T> || is_nothrow_move_constructible_v<E>;
  template <class U>
  static inline constexpr bool enable_assign_value_v =
      !conjunction_v<is_scalar<T>, is_same<T, decay_t<U>>> &&
      is_constructible_v<T, U> && is_assignable_v<T &, U> &&
      is_nothrow_move_constructible_v<E>;
};
template <class E> struct expected_traits<void, E> {
  template <class U, class G>
  static inline constexpr bool enable_other_copy_constructible_v =
      is_void_v<U> && is_constructible_v<E, const G &> &&
      detail::is_not_constructible_and_not_reverse_convertible_v<
          unexpected<E>, expected<U, G>>;
  template <class U, class G>
  static inline constexpr bool explicit_other_copy_constructible_v =
      !is_convertible_v<const G &, E>;

  template <class U, class G>
  static inline constexpr bool enable_other_move_constructible_v =
      is_void_v<U> && is_constructible_v<E, G &&> &&
      detail::is_not_constructible_and_not_reverse_convertible_v<
          unexpected<E>, expected<U, G>>;
  template <class U, class G>
  static inline constexpr bool explicit_other_move_constructible_v =
      !is_convertible_v<G &&, E>;

  template <class U, class G>
  static inline constexpr bool is_nothrow_other_copy_constructible_v =
      is_void_v<U> && is_nothrow_constructible_v<E, const G &>;
  template <class U, class G>
  static inline constexpr bool is_nothrow_other_move_constructible_v =
      is_void_v<U> && is_nothrow_constructible_v<E, G &&>;
  template <class U> static inline constexpr bool enable_in_place_v = false;
  template <class U> static inline constexpr bool enable_assign_value_v = false;
};

template <class T, class E, bool = is_trivially_destructible_v<T>,
          bool = is_trivially_destructible_v<E>>
struct expected_storage_base {
  constexpr expected_storage_base() noexcept(
      is_nothrow_default_constructible_v<T>)
      : m_has_val(true), m_val() {}
  constexpr expected_storage_base(no_init_t) noexcept
      : m_has_val(false), m_no_init() {}
  constexpr expected_storage_base(const expected_storage_base &) = default;
  constexpr expected_storage_base &
  operator=(const expected_storage_base &) = default;

  template <class... Args,
            enable_if_t<is_constructible_v<T, Args...>> * = nullptr,
            bool NoExcept = is_nothrow_constructible_v<T, Args...>>
  constexpr expected_storage_base(in_place_t, Args &&...args) noexcept(NoExcept)
      : m_has_val(true), m_val(std::forward<Args>(args)...) {}
  template <class U, class... Args,
            enable_if_t<is_constructible_v<T, initializer_list<U>, Args...>> * =
                nullptr,
            bool NoExcept =
                is_nothrow_constructible_v<T, initializer_list<U>, Args...>>
  constexpr expected_storage_base(in_place_t, initializer_list<U> il,
                                  Args &&...args) noexcept(NoExcept)
      : m_has_val(true), m_val(std::move(il), std::forward<Args>(args)...) {}

  template <class... Args,
            enable_if_t<is_constructible_v<E, Args...>> * = nullptr,
            bool NoExcept = is_nothrow_constructible_v<E, Args...>>
  constexpr expected_storage_base(unexpect_t, Args &&...args) noexcept(NoExcept)
      : m_has_val(false), m_unex(in_place, std::forward<Args>(args)...) {}
  template <class U, class... Args,
            enable_if_t<is_constructible_v<E, initializer_list<U>, Args...>> * =
                nullptr,
            bool NoExcept =
                is_nothrow_constructible_v<E, initializer_list<U>, Args...>>
  constexpr expected_storage_base(unexpect_t, initializer_list<U> il,
                                  Args &&...args) noexcept(NoExcept)
      : m_has_val(false),
        m_unex(in_place, std::move(il), std::forward<Args>(args)...) {}

  ~expected_storage_base() noexcept(
      is_nothrow_destructible_v<T> &&is_nothrow_destructible_v<E>) {
    if (m_has_val) {
      destruct_value();
    } else {
      destruct_error();
    }
  }

protected:
  constexpr void destruct_value() noexcept(is_nothrow_destructible_v<T>) {
    m_val.~T();
  }
  constexpr void destruct_error() noexcept(is_nothrow_destructible_v<E>) {
    m_unex.~unexpected<E>();
  }

  bool m_has_val;
  union {
    T m_val;
    unexpected<E> m_unex;
    char m_no_init;
  };
};

template <class T, class E> struct expected_storage_base<T, E, true, true> {
  constexpr expected_storage_base() noexcept(
      is_nothrow_default_constructible_v<T>)
      : m_has_val(true), m_val() {}
  constexpr expected_storage_base(no_init_t) noexcept
      : m_has_val(false), m_no_init() {}
  constexpr expected_storage_base(const expected_storage_base &) = default;
  constexpr expected_storage_base &
  operator=(const expected_storage_base &) = default;

  template <class... Args,
            enable_if_t<is_constructible_v<T, Args...>> * = nullptr,
            bool NoExcept = is_nothrow_constructible_v<T, Args...>>
  constexpr expected_storage_base(in_place_t, Args &&...args) noexcept(NoExcept)
      : m_has_val(true), m_val(std::forward<Args>(args)...) {}
  template <class U, class... Args,
            enable_if_t<is_constructible_v<T, initializer_list<U>, Args...>> * =
                nullptr,
            bool NoExcept =
                is_nothrow_constructible_v<T, initializer_list<U>, Args...>>
  constexpr expected_storage_base(in_place_t, initializer_list<U> il,
                                  Args &&...args) noexcept(NoExcept)
      : m_has_val(true), m_val(std::move(il), std::forward<Args>(args)...) {}

  template <class... Args,
            enable_if_t<is_constructible_v<E, Args...>> * = nullptr,
            bool NoExcept = is_nothrow_constructible_v<E, Args...>>
  constexpr expected_storage_base(unexpect_t, Args &&...args) noexcept(NoExcept)
      : m_has_val(false), m_unex(in_place, std::forward<Args>(args)...) {}
  template <class U, class... Args,
            enable_if_t<is_constructible_v<E, initializer_list<U>, Args...>> * =
                nullptr,
            bool NoExcept =
                is_nothrow_constructible_v<E, initializer_list<U>, Args...>>
  constexpr expected_storage_base(unexpect_t, initializer_list<U> il,
                                  Args &&...args) noexcept(NoExcept)
      : m_has_val(false),
        m_unex(in_place, std::move(il), std::forward<Args>(args)...) {}

  ~expected_storage_base() noexcept = default;

protected:
  constexpr void destruct_value() noexcept {}
  constexpr void destruct_error() noexcept {}

  bool m_has_val;
  union {
    T m_val;
    unexpected<E> m_unex;
    char m_no_init;
  };
};

template <class T, class E> struct expected_storage_base<T, E, true, false> {
  constexpr expected_storage_base() noexcept(
      is_nothrow_default_constructible_v<T>)
      : m_has_val(true), m_val() {}
  constexpr expected_storage_base(no_init_t) noexcept
      : m_has_val(false), m_no_init() {}
  constexpr expected_storage_base(const expected_storage_base &) = default;
  constexpr expected_storage_base &
  operator=(const expected_storage_base &) = default;

  template <class... Args,
            enable_if_t<is_constructible_v<T, Args...>> * = nullptr,
            bool NoExcept = is_nothrow_constructible_v<T, Args...>>
  constexpr expected_storage_base(in_place_t, Args &&...args) noexcept(NoExcept)
      : m_has_val(true), m_val(std::forward<Args>(args)...) {}
  template <class U, class... Args,
            enable_if_t<is_constructible_v<T, initializer_list<U>, Args...>> * =
                nullptr,
            bool NoExcept =
                is_nothrow_constructible_v<T, initializer_list<U>, Args...>>
  constexpr expected_storage_base(in_place_t, initializer_list<U> il,
                                  Args &&...args) noexcept(NoExcept)
      : m_has_val(true), m_val(std::move(il), std::forward<Args>(args)...) {}

  template <class... Args,
            enable_if_t<is_constructible_v<E, Args...>> * = nullptr,
            bool NoExcept = is_nothrow_constructible_v<E, Args...>>
  constexpr expected_storage_base(unexpect_t, Args &&...args) noexcept(NoExcept)
      : m_has_val(false), m_unex(in_place, std::forward<Args>(args)...) {}
  template <class U, class... Args,
            enable_if_t<is_constructible_v<E, initializer_list<U>, Args...>> * =
                nullptr,
            bool NoExcept =
                is_nothrow_constructible_v<E, initializer_list<U>, Args...>>
  constexpr expected_storage_base(unexpect_t, initializer_list<U> il,
                                  Args &&...args) noexcept(NoExcept)
      : m_has_val(false),
        m_unex(in_place, std::move(il), std::forward<Args>(args)...) {}

  ~expected_storage_base() noexcept(is_nothrow_destructible_v<E>) {
    if (!m_has_val) {
      destruct_error();
    }
  }

protected:
  constexpr void destruct_value() noexcept {}
  constexpr void destruct_error() noexcept(is_nothrow_destructible_v<E>) {
    m_unex.~unexpected<E>();
  }

  bool m_has_val;
  union {
    T m_val;
    unexpected<E> m_unex;
    char m_no_init;
  };
};

template <class T, class E> struct expected_storage_base<T, E, false, true> {
  constexpr expected_storage_base() noexcept(
      is_nothrow_default_constructible_v<T>)
      : m_has_val(true), m_val() {}
  constexpr expected_storage_base(no_init_t) noexcept
      : m_has_val(false), m_no_init() {}
  constexpr expected_storage_base(const expected_storage_base &) = default;
  constexpr expected_storage_base &
  operator=(const expected_storage_base &) = default;

  template <class... Args,
            enable_if_t<is_constructible_v<T, Args...>> * = nullptr,
            bool NoExcept = is_nothrow_constructible_v<T, Args...>>
  constexpr expected_storage_base(in_place_t, Args &&...args) noexcept(NoExcept)
      : m_has_val(true), m_val(std::forward<Args>(args)...) {}
  template <class U, class... Args,
            enable_if_t<is_constructible_v<T, initializer_list<U>, Args...>> * =
                nullptr,
            bool NoExcept =
                is_nothrow_constructible_v<T, initializer_list<U>, Args...>>
  constexpr expected_storage_base(in_place_t, initializer_list<U> il,
                                  Args &&...args) noexcept(NoExcept)
      : m_has_val(true), m_val(std::move(il), std::forward<Args>(args)...) {}

  template <class... Args,
            enable_if_t<is_constructible_v<E, Args...>> * = nullptr,
            bool NoExcept = is_nothrow_constructible_v<E, Args...>>
  constexpr expected_storage_base(unexpect_t, Args &&...args) noexcept(NoExcept)
      : m_has_val(false), m_unex(in_place, std::forward<Args>(args)...) {}
  template <class U, class... Args,
            enable_if_t<is_constructible_v<E, initializer_list<U>, Args...>> * =
                nullptr,
            bool NoExcept =
                is_nothrow_constructible_v<E, initializer_list<U>, Args...>>
  constexpr expected_storage_base(unexpect_t, initializer_list<U> il,
                                  Args &&...args) noexcept(NoExcept)
      : m_has_val(false),
        m_unex(in_place, std::move(il), std::forward<Args>(args)...) {}

  ~expected_storage_base() noexcept(is_nothrow_destructible_v<T>) {
    if (m_has_val) {
      destruct_value();
    }
  }

protected:
  constexpr void destruct_value() noexcept(is_nothrow_destructible_v<T>) {
    m_val.~T();
  }
  constexpr void destruct_error() noexcept {}

  bool m_has_val;
  union {
    T m_val;
    unexpected<E> m_unex;
    char m_no_init;
  };
};

template <class E> struct expected_storage_base<void, E, false, false> {
  constexpr expected_storage_base() noexcept : m_has_val(true), m_no_init() {}
  constexpr expected_storage_base(no_init_t) noexcept
      : m_has_val(false), m_no_init() {}
  constexpr expected_storage_base(in_place_t) noexcept
      : m_has_val(true), m_no_init() {}
  constexpr expected_storage_base(const expected_storage_base &) = default;
  constexpr expected_storage_base &
  operator=(const expected_storage_base &) = default;

  template <class... Args,
            enable_if_t<is_constructible_v<E, Args...>> * = nullptr,
            bool NoExcept = is_nothrow_constructible_v<E, Args...>>
  constexpr expected_storage_base(unexpect_t, Args &&...args) noexcept(NoExcept)
      : m_has_val(false), m_unex(std::forward<Args>(args)...) {}
  template <class U, class... Args,
            enable_if_t<is_constructible_v<E, initializer_list<U>, Args...>> * =
                nullptr,
            bool NoExcept =
                is_nothrow_constructible_v<E, initializer_list<U>, Args...>>
  constexpr expected_storage_base(unexpect_t, initializer_list<U> il,
                                  Args &&...args) noexcept(NoExcept)
      : m_has_val(false), m_unex(std::move(il), std::forward<Args>(args)...) {}

  ~expected_storage_base() noexcept(is_nothrow_destructible_v<E>) {
    if (!m_has_val) {
      destruct_error();
    }
  }

protected:
  constexpr void destruct_value() noexcept {}
  constexpr void destruct_error() noexcept(is_nothrow_destructible_v<E>) {
    m_unex.~unexpected<E>();
  }

  bool m_has_val;
  union {
    unexpected<E> m_unex;
    char m_no_init;
  };
};

template <class E> struct expected_storage_base<void, E, false, true> {
  constexpr expected_storage_base() noexcept : m_has_val(true), m_no_init() {}
  constexpr expected_storage_base(no_init_t) noexcept
      : m_has_val(false), m_no_init() {}
  constexpr expected_storage_base(in_place_t) noexcept
      : m_has_val(true), m_no_init() {}
  constexpr expected_storage_base(const expected_storage_base &) = default;
  constexpr expected_storage_base &
  operator=(const expected_storage_base &) = default;

  template <class... Args,
            enable_if_t<is_constructible_v<E, Args...>> * = nullptr,
            bool NoExcept = is_nothrow_constructible_v<E, Args...>>
  constexpr expected_storage_base(unexpect_t, Args &&...args) noexcept(NoExcept)
      : m_has_val(false), m_unex(std::forward<Args>(args)...) {}
  template <class U, class... Args,
            enable_if_t<is_constructible_v<E, initializer_list<U>, Args...>> * =
                nullptr,
            bool NoExcept =
                is_nothrow_constructible_v<E, initializer_list<U>, Args...>>
  constexpr expected_storage_base(unexpect_t, initializer_list<U> il,
                                  Args &&...args) noexcept(NoExcept)
      : m_has_val(false), m_unex(std::move(il), std::forward<Args>(args)...) {}

  ~expected_storage_base() noexcept = default;

protected:
  constexpr void destruct_value() noexcept {}
  constexpr void destruct_error() noexcept {}

  bool m_has_val;
  union {
    unexpected<E> m_unex;
    char m_no_init;
  };
};

template <class T, class E>
struct expected_view_base : public expected_storage_base<T, E> {
  using base = expected_storage_base<T, E>;
  using base::base;

  constexpr bool has_value() const noexcept { return likely(base::m_has_val); }
  constexpr const E &error() const &noexcept { return base::m_unex.value(); }
  constexpr const E &&error() const &&noexcept {
    return std::move(base::m_unex).value();
  }
  constexpr E &error() &noexcept { return base::m_unex.value(); }
  constexpr E &&error() &&noexcept { return std::move(base::m_unex).value(); }

  template <class... Args,
            enable_if_t<is_constructible_v<T, Args...> &&
                        (is_nothrow_constructible_v<T, Args...> ||
                         is_nothrow_move_constructible_v<T> ||
                         is_nothrow_move_constructible_v<E>)> * = nullptr,
            bool NoExcept = is_nothrow_constructible_v<T, Args...> &&
                is_nothrow_move_assignable_v<T> &&is_nothrow_destructible_v<E>>
  T &emplace(Args &&...args) noexcept(NoExcept) {
    if (has_value()) {
      if constexpr (is_nothrow_constructible_v<T, Args...>) {
        base::destruct_value();
        construct_value(std::forward<Args>(args)...);
      } else if constexpr (is_nothrow_move_constructible_v<T>) {
        T tmp(std::forward<Args>(args)...);
        base::destruct_value();
        construct_value(std::move(tmp));
      } else if constexpr (is_nothrow_move_assignable_v<T>) {
        val() = T(std::forward<Args>(args)...);
      } else {
        T tmp = std::move(val());
        base::destruct_value();
        try {
          construct_value(std::forward<Args>(args)...);
        } catch (...) {
          base::construct(std::move(tmp));
          throw_exception_again;
        }
      }
    } else {
      if constexpr (is_nothrow_constructible_v<T, Args...>) {
        base::destruct_error();
        construct_value(std::forward<Args>(args)...);
      } else if constexpr (is_nothrow_move_constructible_v<T>) {
        T tmp(std::forward<Args>(args)...);
        base::destruct_error();
        construct_value(std::move(tmp));
      } else {
        E tmp = std::move(error());
        base::destruct_error();
        try {
          construct_value(std::forward<Args>(args)...);
        } catch (...) {
          base::construct_error(std::move(tmp));
          throw_exception_again;
        }
      }
    }
    return val();
  }
  template <class U, class... Args,
            enable_if_t<
                is_constructible_v<T, initializer_list<U>, Args...> &&
                (is_nothrow_constructible_v<T, initializer_list<U>, Args...> ||
                 is_nothrow_move_constructible_v<T> ||
                 is_nothrow_move_constructible_v<E>)> * = nullptr,
            bool NoExcept = is_nothrow_constructible_v<T, initializer_list<U>,
                                                       Args...> &&
                is_nothrow_move_assignable_v<T> &&is_nothrow_destructible_v<E>>
  T &emplace(initializer_list<U> il, Args &&...args) noexcept(NoExcept) {
    if (has_value()) {
      if constexpr (is_nothrow_constructible_v<T, Args...>) {
        base::destruct_value();
        construct_value(il, std::forward<Args>(args)...);
      } else if constexpr (is_nothrow_move_constructible_v<T>) {
        T tmp(il, std::forward<Args>(args)...);
        base::destruct_value();
        construct_value(std::move(tmp));
      } else if constexpr (is_nothrow_move_assignable_v<T>) {
        val() = T(il, std::forward<Args>(args)...);
      } else {
        T tmp = std::move(val());
        base::destruct_value();
        try {
          construct_value(il, std::forward<Args>(args)...);
        } catch (...) {
          base::construct_value(std::move(tmp));
          throw_exception_again;
        }
      }
    } else {
      if constexpr (is_nothrow_constructible_v<T, Args...>) {
        base::destruct_error();
        construct_value(il, std::forward<Args>(args)...);
      } else if constexpr (is_nothrow_move_constructible_v<T>) {
        T tmp(il, std::forward<Args>(args)...);
        base::destruct_error();
        construct_value(std::move(tmp));
      } else {
        E tmp = std::move(error());
        base::destruct_error();
        try {
          construct_value(il, std::forward<Args>(args)...);
        } catch (...) {
          base::construct_error(std::move(tmp));
          throw_exception_again;
        }
      }
    }
    return val();
  }

protected:
  constexpr const T &val() const &noexcept { return base::m_val; }
  constexpr const T &&val() const &&noexcept { return std::move(base::m_val); }
  constexpr T &val() &noexcept { return base::m_val; }
  constexpr T &&val() &&noexcept { return std::move(base::m_val); }
  template <class... Args,
            enable_if_t<is_constructible_v<T, Args &&...>> * = nullptr,
            bool NoExcept = is_nothrow_constructible_v<T, Args &&...>>
  void construct_value(Args &&...args) noexcept(NoExcept) {
    new (addressof(base::m_val)) T(std::forward<Args>(args)...);
    base::m_has_val = true;
  }
  template <class... Args,
            enable_if_t<is_constructible_v<E, Args &&...>> * = nullptr,
            bool NoExcept = is_nothrow_constructible_v<E, Args &&...>>
  void construct_error(Args &&...args) noexcept(NoExcept) {
    new (addressof(base::m_unex)) unexpected<E>(std::forward<Args>(args)...);
    base::m_has_val = false;
  }
};

template <class E>
struct expected_view_base<void, E> : public expected_storage_base<void, E> {
  using base = expected_storage_base<void, E>;
  using base::base;

  constexpr bool has_value() const noexcept { return likely(base::m_has_val); }
  constexpr const E &error() const &noexcept { return base::m_unex.value(); }
  constexpr const E &&error() const &&noexcept {
    return std::move(base::m_unex).value();
  }
  constexpr E &error() &noexcept { return base::m_unex.value(); }
  constexpr E &&error() &&noexcept { return std::move(base::m_unex).value(); }

  void emplace() noexcept(is_nothrow_destructible_v<E>) {
    if (!has_value()) {
      base::destruct_error();
    }
    construct_value();
  }

protected:
  constexpr void val() const noexcept {}

  void construct_value() noexcept { base::m_has_val = true; }
  template <class... Args,
            enable_if_t<is_constructible_v<E, Args &&...>> * = nullptr,
            bool NoExcept = is_nothrow_constructible_v<E, Args &&...>>
  void construct_error(Args &&...args) noexcept(NoExcept) {
    new (addressof(base::m_unex)) unexpected<E>(std::forward<Args>(args)...);
    base::m_has_val = false;
  }
};

template <class T, class E>
struct expected_operations_base : public expected_view_base<T, E> {
  using expected_view_base<T, E>::expected_view_base;

protected:
  template <class U = T,
            bool NoExcept = is_nothrow_constructible_v<T, U>
                &&is_nothrow_assignable_v<add_lvalue_reference_t<T>, U>>
  void assign_value(U &&rhs) noexcept(NoExcept) {
    if (!this->has_value()) {
      if constexpr (is_nothrow_constructible_v<T, U>) {
        this->destruct_error();
        this->construct_value(std::forward<U>(rhs));
      } else {
        E tmp = this->error();
        this->destruct_error();
        try {
          this->construct_value(std::forward<U>(rhs));
        } catch (...) {
          this->construct_error(std::move(tmp));
          throw_exception_again;
        }
      }
    } else {
      this->val() = std::forward<U>(rhs);
    }
  }

  template <class G = E, bool NoExcept = is_nothrow_destructible_v<T>>
  void assign_error(const unexpected<G> &rhs) noexcept(NoExcept) {
    static_assert(is_nothrow_constructible_v<E, const G &>,
                  "E must nothrow copy constructible");
    static_assert(is_nothrow_assignable_v<E &, const G &>,
                  "E must copy assignable");
    if (this->has_value()) {
      this->destruct_value();
      this->construct_error(rhs.value());
    } else {
      this->error() = rhs.value();
    }
  }
  template <class G = E, bool NoExcept = is_nothrow_destructible_v<T>>
  void assign_error(unexpected<G> &&rhs) noexcept(NoExcept) {
    static_assert(is_nothrow_constructible_v<E, G &&>,
                  "E must nothrow move constructible");
    static_assert(is_nothrow_assignable_v<E &, G &&>, "E must move assignable");
    if (this->has_value()) {
      this->destruct_value();
      this->construct_error(std::move(rhs).value());
    } else {
      this->error() = std::move(rhs).value();
    }
  }

  void assign(const expected_operations_base &rhs) noexcept(
      is_nothrow_copy_assignable_v<T> &&is_nothrow_copy_assignable_v<E>
          &&is_nothrow_copy_assignable_v<E> &&is_nothrow_destructible_v<E> &&
      (disjunction_v<is_void<T>, is_nothrow_copy_constructible<T>>)) {
    static_assert(is_nothrow_move_constructible_v<E>,
                  "E must nothrow move constructible");
    static_assert(disjunction_v<is_void<T>, is_move_constructible<T>>,
                  "T must move constructible");
    if (!this->has_value() && rhs.has_value()) {
      if constexpr (is_void_v<T>) {
        this->destruct_error();
        this->construct_value();
      } else if constexpr (is_nothrow_copy_constructible_v<T>) {
        this->destruct_error();
        this->construct_value(rhs.val());
      } else if constexpr (is_nothrow_move_constructible_v<T>) {
        T tmp = rhs.val();
        this->destruct_error();
        this->construct_value(std::move(tmp));
      } else {
        E tmp = this->error();
        this->destruct_error();
        try {
          this->construct_value(rhs.val());
        } catch (...) {
          this->construct_error(std::move(tmp));
          throw_exception_again;
        }
      }
    } else if (this->has_value() && !rhs.has_value()) {
      if constexpr (is_void_v<T>) {
        this->construct_error(rhs.error());
      } else if constexpr (is_nothrow_copy_constructible_v<T>) {
        this->destruct_value();
        this->construct_error(rhs.error());
      } else if constexpr (is_nothrow_move_constructible_v<T>) {
        E tmp = rhs.error();
        this->destruct_value();
        this->construct_error(std::move(tmp));
      } else {
        T tmp = this->val();
        this->destruct_value();
        try {
          this->construct_error(rhs.error());
        } catch (...) {
          this->construct_value(std::move(tmp));
          throw_exception_again;
        }
      }
    } else {
      if constexpr (is_void_v<T>) {
        if (!this->has_value()) {
          this->error() = rhs.error();
        }
      } else {
        if (this->has_value()) {
          this->val() = rhs.val();
        } else {
          this->error() = rhs.error();
        }
      }
    }
  }

  void assign(expected_operations_base &&rhs) noexcept(
      is_nothrow_destructible_v<T> &&is_nothrow_destructible_v<E>
          &&is_nothrow_move_constructible_v<T>
              &&is_nothrow_move_assignable_v<E>) {
    static_assert(is_nothrow_move_constructible_v<E>,
                  "E must nothrow move constructible");
    static_assert(disjunction_v<is_void<T>, is_move_constructible<T>>,
                  "T must move constructible");
    if (!this->has_value() && rhs.has_value()) {
      if constexpr (is_void_v<T>) {
        this->destruct_error();
        this->construct_value();
      } else if constexpr (is_nothrow_move_constructible_v<T>) {
        this->destruct_error();
        this->construct_value(std::move(rhs).val());
      } else {
        E tmp = std::move(this->error());
        this->destruct_error();
        try {
          this->construct_value(std::move(rhs).val());
        } catch (...) {
          this->construct_error(std::move(tmp));
          throw_exception_again;
        }
      }
    } else if (this->has_value() && !rhs.has_value()) {
      if constexpr (is_void_v<T>) {
        this->construct_error(std::move(rhs).error());
      } else if constexpr (is_nothrow_move_constructible_v<T>) {
        this->destruct_value();
        this->construct_error(std::move(rhs).error());
      } else {
        T tmp = std::move(this->val());
        this->destruct_value();
        try {
          this->construct_error(std::move(rhs).error());
        } catch (...) {
          this->construct_value(std::move(tmp));
          throw_exception_again;
        }
      }
    } else {
      if (this->has_value()) {
        if constexpr (!is_void_v<T>) {
          this->val() = std::move(rhs).val();
        }
      } else {
        this->error() = std::move(rhs).error();
      }
    }
  }
};

template <class T, class E,
          bool = conjunction_v<
              is_trivially_copy_constructible<E>,
              disjunction<is_void<T>, is_trivially_copy_constructible<T>>>>
struct expected_copy_base : public expected_operations_base<T, E> {
  using expected_operations_base<T, E>::expected_operations_base;
};

template <class T, class E>
struct expected_copy_base<T, E, false> : public expected_operations_base<T, E> {
  using expected_operations_base<T, E>::expected_operations_base;

  constexpr expected_copy_base() = default;
  constexpr expected_copy_base(const expected_copy_base &rhs) noexcept(
      is_nothrow_copy_constructible_v<T>)
      : expected_operations_base<T, E>(no_init) {
    if (rhs.has_value()) {
      if constexpr (is_void_v<T>) {
        this->construct_value();
      } else {
        this->construct_value(rhs.val());
      }
    } else {
      this->construct_error(rhs.error());
    }
  }
  constexpr expected_copy_base(expected_copy_base &&rhs) = default;
  constexpr expected_copy_base &
  operator=(const expected_copy_base &rhs) = default;
  constexpr expected_copy_base &operator=(expected_copy_base &&rhs) = default;
};

template <class T, class E,
          bool = conjunction_v<
              is_trivially_move_constructible<E>,
              disjunction<is_void<T>, is_trivially_move_constructible<T>>>>
struct expected_move_base : public expected_copy_base<T, E> {
  using expected_copy_base<T, E>::expected_copy_base;
};

template <class T, class E>
struct expected_move_base<T, E, false> : public expected_copy_base<T, E> {
  using expected_copy_base<T, E>::expected_copy_base;

  constexpr expected_move_base() = default;
  constexpr expected_move_base(expected_move_base &&rhs) noexcept(
      is_nothrow_move_constructible_v<T>)
      : expected_copy_base<T, E>(no_init) {
    if (rhs.has_value()) {
      if constexpr (is_void_v<T>) {
        this->construct_value();
      } else {
        this->construct_value(std::move(rhs).val());
      }
    } else {
      this->construct_error(std::move(rhs).error());
    }
  }
  constexpr expected_move_base(const expected_move_base &rhs) = default;
  constexpr expected_move_base &
  operator=(const expected_move_base &rhs) = default;
  constexpr expected_move_base &operator=(expected_move_base &&rhs) = default;
};

template <
    class T, class E,
    bool = conjunction_v<
        disjunction<is_void<T>, conjunction<is_trivially_copy_assignable<T>,
                                            is_trivially_copy_constructible<T>,
                                            is_trivially_destructible<T>>>,
        is_trivially_copy_assignable<E>, is_trivially_copy_constructible<E>,
        is_trivially_destructible<E>>>
struct expected_copy_assign_base : expected_move_base<T, E> {
  using expected_move_base<T, E>::expected_move_base;
};

template <class T, class E>
struct expected_copy_assign_base<T, E, false> : expected_move_base<T, E> {
  using expected_move_base<T, E>::expected_move_base;

  constexpr expected_copy_assign_base() = default;
  constexpr expected_copy_assign_base(const expected_copy_assign_base &rhs) =
      default;
  constexpr expected_copy_assign_base(expected_copy_assign_base &&rhs) =
      default;
  constexpr expected_copy_assign_base &
  operator=(const expected_copy_assign_base &rhs) noexcept(
      is_nothrow_copy_constructible_v<T> &&is_nothrow_copy_assignable_v<E>) {
    this->assign(rhs);
    return *this;
  }
  constexpr expected_copy_assign_base &
  operator=(expected_copy_assign_base &&rhs) = default;
};

template <
    class T, class E,
    bool = conjunction_v<
        disjunction<is_void<T>, conjunction<is_trivially_destructible<T>,
                                            is_trivially_move_constructible<T>,
                                            is_trivially_move_assignable<T>>>,
        is_trivially_destructible<E>, is_trivially_move_constructible<E>,
        is_trivially_move_assignable<E>>>
struct expected_move_assign_base : expected_copy_assign_base<T, E> {
  using expected_copy_assign_base<T, E>::expected_copy_assign_base;
};

template <class T, class E>
struct expected_move_assign_base<T, E, false>
    : expected_copy_assign_base<T, E> {
  using expected_copy_assign_base<T, E>::expected_copy_assign_base;

  constexpr expected_move_assign_base() = default;
  constexpr expected_move_assign_base(const expected_move_assign_base &rhs) =
      default;
  constexpr expected_move_assign_base(expected_move_assign_base &&rhs) =
      default;
  constexpr expected_move_assign_base &
  operator=(const expected_move_assign_base &rhs) = default;
  constexpr expected_move_assign_base &
  operator=(expected_move_assign_base &&rhs) noexcept(
      is_nothrow_destructible_v<T> &&is_nothrow_destructible_v<E>
          &&is_nothrow_move_constructible_v<T>
              &&is_nothrow_move_assignable_v<E>) {
    this->assign(std::move(rhs));
    return *this;
  }
};

template <class Exp, class F, class T = typename decay_t<Exp>::value_type,
          enable_if_t<!is_void_v<T>> * = nullptr,
          class Ret = decltype(invoke(declval<F>(), *declval<Exp>()))>
constexpr auto expected_and_then_impl(Exp &&exp, F &&f) {
  static_assert(is_expected_v<Ret>, "F must return an expected");

  if (exp.has_value()) {
    return invoke(std::forward<F>(f), *std::forward<Exp>(exp));
  }
  return Ret(unexpect, std::forward<Exp>(exp).error());
}

template <class Exp, class F, class T = typename decay_t<Exp>::value_type,
          enable_if_t<is_void_v<T>> * = nullptr,
          class Ret = decltype(invoke(declval<F>()))>
constexpr auto expected_and_then_impl(Exp &&exp, F &&f) {
  static_assert(is_expected_v<Ret>, "F must return an expected");

  if (exp.has_value()) {
    return invoke(std::forward<F>(f));
  }
  return Ret(unexpect, std::forward<Exp>(exp).error());
}

template <class Exp, class F,
          class Ret = decltype(invoke(declval<F>(), declval<Exp>().error())),
          enable_if_t<!is_void_v<Ret>> * = nullptr>
constexpr auto expected_or_else_impl(Exp &&exp, F &&f) {
  static_assert(is_expected_v<Ret>, "F must return an expected");
  if (exp.has_value()) {
    return std::forward<Exp>(exp);
  }
  return invoke(std::forward<F>(f), std::forward<Exp>(exp).error());
}

template <class Exp, class F,
          class Ret = decltype(invoke(declval<F>(), declval<Exp>().error())),
          enable_if_t<is_void_v<Ret>> * = nullptr>
constexpr auto expected_or_else_impl(Exp &&exp, F &&f) {
  if (!exp.has_value()) {
    invoke(std::forward<F>(f), std::forward<Exp>(exp).error());
  }
  return std::forward<Exp>(exp);
}

template <class Exp, class F, class T = typename decay_t<Exp>::value_type,
          class E = typename decay_t<Exp>::error_type,
          enable_if_t<!is_void_v<T>> * = nullptr,
          class Ret = decltype(invoke(declval<F>(), *declval<Exp>())),
          enable_if_t<!is_void_v<Ret>> * = nullptr,
          class Result = expected<decay_t<Ret>, E>>
constexpr Result expected_map_impl(Exp &&exp, F &&f) {
  if (exp.has_value()) {
    return Result(invoke(std::forward<F>(f), *std::forward<Exp>(exp)));
  }
  return Result(unexpect, std::forward<Exp>(exp).error());
}

template <class Exp, class F, class T = typename decay_t<Exp>::value_type,
          class E = typename decay_t<Exp>::error_type,
          enable_if_t<!is_void_v<T>> * = nullptr,
          class Ret = decltype(invoke(declval<F>(), *declval<Exp>())),
          enable_if_t<is_void_v<Ret>> * = nullptr,
          class Result = expected<void, E>>
constexpr Result expected_map_impl(Exp &&exp, F &&f) {
  if (exp.has_value()) {
    invoke(std::forward<F>(f), *std::forward<Exp>(exp));
    return Result();
  }
  return Result(unexpect, std::forward<Exp>(exp).error());
}

template <class Exp, class F, class T = typename decay_t<Exp>::value_type,
          class E = typename decay_t<Exp>::error_type,
          enable_if_t<is_void_v<T>> * = nullptr,
          class Ret = decltype(invoke(declval<F>())),
          enable_if_t<!is_void_v<Ret>> * = nullptr,
          class Result = expected<decay_t<Ret>, E>>
constexpr Result expected_map_impl(Exp &&exp, F &&f) {
  if (exp.has_value()) {
    return Result(invoke(std::forward<F>(f)));
  }
  return Result(unexpect, std::forward<Exp>(exp).error());
}

template <class Exp, class F, class T = typename decay_t<Exp>::value_type,
          class E = typename decay_t<Exp>::error_type,
          enable_if_t<is_void_v<T>> * = nullptr,
          class Ret = decltype(invoke(declval<F>())),
          enable_if_t<is_void_v<Ret>> * = nullptr,
          class Result = expected<void, E>>
constexpr Result expected_map_impl(Exp &&exp, F &&f) {
  if (exp.has_value()) {
    invoke(std::forward<F>(f));
    return Result();
  }
  return Result(unexpect, std::forward<Exp>(exp).error());
}

template <class Exp, class F, class T = typename decay_t<Exp>::value_type,
          enable_if_t<!is_void_v<T>> * = nullptr,
          class Ret = decltype(invoke(declval<F>(), declval<Exp>().error())),
          enable_if_t<!is_void_v<Ret>> * = nullptr,
          class Result = expected<T, decay_t<Ret>>>
constexpr Result expected_map_error_impl(Exp &&exp, F &&f) {
  if (exp.has_value()) {
    return Result(*std::forward<Exp>(exp));
  }
  return Result(unexpect,
                invoke(std::forward<F>(f), std::forward<Exp>(exp).error()));
}

template <class Exp, class F, class T = typename decay_t<Exp>::value_type,
          enable_if_t<!is_void_v<T>> * = nullptr,
          class Ret = decltype(invoke(declval<F>(), declval<Exp>().error())),
          enable_if_t<is_void_v<Ret>> * = nullptr,
          class Result = expected<T, monostate>>
constexpr Result expected_map_error_impl(Exp &&exp, F &&f) {
  if (exp.has_value()) {
    return Result(*std::forward<Exp>(exp));
  }
  invoke(std::forward<F>(f), std::forward<Exp>(exp).error());
  return Result(unexpect);
}

template <class Exp, class F, class T = typename decay_t<Exp>::value_type,
          enable_if_t<is_void_v<T>> * = nullptr,
          class Ret = decltype(invoke(declval<F>(), declval<Exp>().error())),
          enable_if_t<!is_void_v<Ret>> * = nullptr,
          class Result = expected<T, decay_t<Ret>>>
constexpr Result expected_map_error_impl(Exp &&exp, F &&f) {
  if (exp.has_value()) {
    return Result();
  }
  return Result(unexpect,
                invoke(std::forward<F>(f), std::forward<Exp>(exp).error()));
}

template <class Exp, class F, class T = typename decay_t<Exp>::value_type,
          enable_if_t<is_void_v<T>> * = nullptr,
          class Ret = decltype(invoke(declval<F>(), declval<Exp>().error())),
          enable_if_t<is_void_v<Ret>> * = nullptr,
          class Result = expected<T, monostate>>
constexpr Result expected_map_error_impl(Exp &&exp, F &&f) {
  if (exp.has_value()) {
    return Result();
  }
  invoke(std::forward<F>(f), std::forward<Exp>(exp).error());
  return Result(unexpect);
}

template <class T, class E,
          bool = is_default_constructible_v<T> || is_void_v<T>>
struct expected_default_ctor_base {
  constexpr expected_default_ctor_base() noexcept = default;
  constexpr expected_default_ctor_base(
      expected_default_ctor_base const &) noexcept = default;
  constexpr expected_default_ctor_base(expected_default_ctor_base &&) noexcept =
      default;
  constexpr expected_default_ctor_base &
  operator=(expected_default_ctor_base const &) noexcept = default;
  constexpr expected_default_ctor_base &
  operator=(expected_default_ctor_base &&) noexcept = default;

  constexpr explicit expected_default_ctor_base(in_place_t) {}
};

template <class T, class E> struct expected_default_ctor_base<T, E, false> {
  constexpr expected_default_ctor_base() noexcept = delete;
  constexpr expected_default_ctor_base(
      expected_default_ctor_base const &) noexcept = default;
  constexpr expected_default_ctor_base(expected_default_ctor_base &&) noexcept =
      default;
  constexpr expected_default_ctor_base &
  operator=(expected_default_ctor_base const &) noexcept = default;
  constexpr expected_default_ctor_base &
  operator=(expected_default_ctor_base &&) noexcept = default;

  constexpr explicit expected_default_ctor_base(in_place_t) {}
};

template <class T, class E,
          bool = (is_void_v<T> ||
                  is_copy_constructible_v<T>)&&is_copy_constructible_v<E>,
          bool = (is_void_v<T> ||
                  is_move_constructible_v<T>)&&is_move_constructible_v<E>>
struct expected_delete_ctor_base {
  constexpr expected_delete_ctor_base() = default;
  constexpr expected_delete_ctor_base(const expected_delete_ctor_base &) =
      default;
  constexpr expected_delete_ctor_base(expected_delete_ctor_base &&) noexcept =
      default;
  constexpr expected_delete_ctor_base &
  operator=(const expected_delete_ctor_base &) = default;
  constexpr expected_delete_ctor_base &
  operator=(expected_delete_ctor_base &&) noexcept = default;
};

template <class T, class E>
struct expected_delete_ctor_base<T, E, true, false> {
  constexpr expected_delete_ctor_base() = default;
  constexpr expected_delete_ctor_base(const expected_delete_ctor_base &) =
      default;
  constexpr expected_delete_ctor_base(expected_delete_ctor_base &&) noexcept =
      delete;
  constexpr expected_delete_ctor_base &
  operator=(const expected_delete_ctor_base &) = default;
  constexpr expected_delete_ctor_base &
  operator=(expected_delete_ctor_base &&) noexcept = default;
};

template <class T, class E>
struct expected_delete_ctor_base<T, E, false, true> {
  constexpr expected_delete_ctor_base() = default;
  constexpr expected_delete_ctor_base(const expected_delete_ctor_base &) =
      delete;
  constexpr expected_delete_ctor_base(expected_delete_ctor_base &&) noexcept =
      default;
  constexpr expected_delete_ctor_base &
  operator=(const expected_delete_ctor_base &) = default;
  constexpr expected_delete_ctor_base &
  operator=(expected_delete_ctor_base &&) noexcept = default;
};

template <class T, class E>
struct expected_delete_ctor_base<T, E, false, false> {
  constexpr expected_delete_ctor_base() = default;
  constexpr expected_delete_ctor_base(const expected_delete_ctor_base &) =
      delete;
  constexpr expected_delete_ctor_base(expected_delete_ctor_base &&) noexcept =
      delete;
  constexpr expected_delete_ctor_base &
  operator=(const expected_delete_ctor_base &) = default;
  constexpr expected_delete_ctor_base &
  operator=(expected_delete_ctor_base &&) noexcept = default;
};

template <
    class T, class E,
    bool = (is_void_v<T> ||
            (is_copy_assignable_v<T> && is_copy_constructible_v<T> &&
             (is_nothrow_move_constructible_v<E> ||
              is_nothrow_move_constructible_v<T>))) &&
           is_copy_assignable_v<E> &&is_copy_constructible_v<E>,
    bool = (is_void_v<T> ||
            (is_move_assignable_v<T> && is_move_constructible_v<T>)) &&
           is_nothrow_move_assignable_v<E> &&is_nothrow_move_constructible_v<E>>
struct expected_delete_assign_base {
  constexpr expected_delete_assign_base() = default;
  constexpr expected_delete_assign_base(const expected_delete_assign_base &) =
      default;
  constexpr expected_delete_assign_base(
      expected_delete_assign_base &&) noexcept = default;
  constexpr expected_delete_assign_base &
  operator=(const expected_delete_assign_base &) = default;
  constexpr expected_delete_assign_base &
  operator=(expected_delete_assign_base &&) noexcept = default;
};

template <class T, class E>
struct expected_delete_assign_base<T, E, true, false> {
  constexpr expected_delete_assign_base() = default;
  constexpr expected_delete_assign_base(const expected_delete_assign_base &) =
      default;
  constexpr expected_delete_assign_base(
      expected_delete_assign_base &&) noexcept = default;
  constexpr expected_delete_assign_base &
  operator=(const expected_delete_assign_base &) = default;
  constexpr expected_delete_assign_base &
  operator=(expected_delete_assign_base &&) noexcept = delete;
};

template <class T, class E>
struct expected_delete_assign_base<T, E, false, true> {
  constexpr expected_delete_assign_base() = default;
  constexpr expected_delete_assign_base(const expected_delete_assign_base &) =
      default;
  constexpr expected_delete_assign_base(
      expected_delete_assign_base &&) noexcept = default;
  constexpr expected_delete_assign_base &
  operator=(const expected_delete_assign_base &) = delete;
  constexpr expected_delete_assign_base &
  operator=(expected_delete_assign_base &&) noexcept = default;
};

template <class T, class E>
struct expected_delete_assign_base<T, E, false, false> {
  constexpr expected_delete_assign_base() = default;
  constexpr expected_delete_assign_base(const expected_delete_assign_base &) =
      default;
  constexpr expected_delete_assign_base(
      expected_delete_assign_base &&) noexcept = default;
  constexpr expected_delete_assign_base &
  operator=(const expected_delete_assign_base &) = delete;
  constexpr expected_delete_assign_base &
  operator=(expected_delete_assign_base &&) noexcept = delete;
};

} // namespace detail

template <class T, class E>
class expected : public detail::expected_move_assign_base<T, E>,
                 private detail::expected_delete_ctor_base<T, E>,
                 private detail::expected_delete_assign_base<T, E>,
                 private detail::expected_default_ctor_base<T, E> {
  using traits = detail::expected_traits<T, E>;
  using impl_base = detail::expected_move_assign_base<T, E>;
  using ctor_base = detail::expected_default_ctor_base<T, E>;
  using lvalue_reference_type = add_lvalue_reference_t<T>;
  using rvalue_reference_type = add_rvalue_reference_t<T>;
  using const_lvalue_reference_type = add_lvalue_reference_t<add_const_t<T>>;
  using const_rvalue_reference_type = add_rvalue_reference_t<add_const_t<T>>;

public:
  using value_type = T;
  using error_type = E;
  using unexpected_type = unexpected<E>;
  template <class U> using rebind = expected<U, error_type>;

  // 4.1, constructors
  constexpr expected() = default;
  constexpr expected(const expected &) = default;
  constexpr expected(expected &&) = default;

  template <
      class U, class G,
      enable_if_t<traits::template enable_other_copy_constructible_v<U, G> &&
                  !traits::template explicit_other_copy_constructible_v<U, G>>
          * = nullptr>
  constexpr expected(const expected<U, G> &rhs) noexcept(
      traits::template is_nothrow_other_copy_constructible_v<U, G>)
      : impl_base(detail::no_init), ctor_base(in_place) {
    if (rhs.has_value()) {
      this->construct_value(*rhs);
    } else {
      this->construct_error(rhs.error());
    }
  }
  template <
      class U, class G,
      enable_if_t<traits::template enable_other_copy_constructible_v<U, G> &&
                  traits::template explicit_other_copy_constructible_v<U, G>>
          * = nullptr>
  constexpr explicit expected(const expected<U, G> &rhs) noexcept(
      traits::template is_nothrow_other_copy_constructible_v<U, G>)
      : impl_base(detail::no_init), ctor_base(in_place) {
    if (rhs.has_value()) {
      this->construct_value(*rhs);
    } else {
      this->construct_error(rhs.error());
    }
  }
  template <
      class U, class G,
      enable_if_t<traits::template enable_other_move_constructible_v<U, G> &&
                  !traits::template explicit_other_move_constructible_v<U, G>>
          * = nullptr>
  constexpr expected(expected<U, G> &&rhs) noexcept(
      traits::template is_nothrow_other_move_constructible_v<U, G>)
      : impl_base(detail::no_init), ctor_base(in_place) {
    if (rhs.has_value()) {
      this->construct_value(*std::move(rhs));
    } else {
      this->construct_error(std::move(rhs).error());
    }
  }
  template <
      class U, class G,
      enable_if_t<traits::template enable_other_move_constructible_v<U, G> &&
                  traits::template explicit_other_move_constructible_v<U, G>>
          * = nullptr>
  constexpr explicit expected(expected<U, G> &&rhs) noexcept(
      traits::template is_nothrow_other_move_constructible_v<U, G>)
      : impl_base(detail::no_init), ctor_base(in_place) {
    if (rhs.has_value()) {
      this->construct_value(*std::move(rhs));
    } else {
      this->construct_error(std::move(rhs).error());
    }
  }

  template <class U = T,
            enable_if_t<traits::template enable_in_place_v<U> &&
                        is_convertible_v<U, T>> * = nullptr,
            bool NoExcept = is_nothrow_constructible_v<T, U>>
  constexpr expected(U &&v) noexcept(NoExcept)
      : expected(in_place, std::forward<U>(v)) {}
  template <class U = T,
            enable_if_t<traits::template enable_in_place_v<U> &&
                        !is_convertible_v<U, T>> * = nullptr,
            bool NoExcept = is_nothrow_constructible_v<T, U>>
  constexpr explicit expected(U &&v) noexcept(NoExcept)
      : expected(in_place, std::forward<U>(v)) {}

  template <class G = E,
            enable_if_t<is_constructible_v<E, const G &>> * = nullptr>
  constexpr expected(const unexpected<G> &e) : expected(unexpect, e.value()) {}
  template <class G = E, enable_if_t<is_constructible_v<E, G &&>> * = nullptr>
  constexpr expected(unexpected<G> &&e)
      : expected(unexpect, std::move(e.value())) {}

  template <class... Args,
            enable_if_t<is_constructible_v<T, Args...>> * = nullptr,
            bool NoExcept = is_nothrow_constructible_v<T, Args...>>
  constexpr explicit expected(in_place_t, Args &&...args) noexcept(NoExcept)
      : impl_base(in_place, std::forward<Args>(args)...), ctor_base(in_place) {}
  template <class U, class... Args,
            enable_if_t<is_constructible_v<T, initializer_list<U>, Args...>> * =
                nullptr,
            bool NoExcept =
                is_nothrow_constructible_v<T, initializer_list<U>, Args...>>
  constexpr explicit expected(in_place_t, initializer_list<U> il,
                              Args &&...args) noexcept(NoExcept)
      : impl_base(in_place, std::move(il), std::forward<Args>(args)...),
        ctor_base(in_place) {}

  template <class... Args,
            enable_if_t<is_constructible_v<E, Args...>> * = nullptr,
            bool NoExcept = is_nothrow_constructible_v<E, Args...>>
  constexpr explicit expected(unexpect_t, Args &&...args) noexcept(NoExcept)
      : impl_base(unexpect, std::forward<Args>(args)...), ctor_base(in_place) {}
  template <class U, class... Args,
            enable_if_t<is_constructible_v<E, initializer_list<U>, Args...>> * =
                nullptr,
            bool NoExcept =
                is_nothrow_constructible_v<E, initializer_list<U>, Args...>>
  constexpr explicit expected(unexpect_t, initializer_list<U> il,
                              Args &&...args) noexcept(NoExcept)
      : impl_base(unexpect, std::move(il), std::forward<Args>(args)...),
        ctor_base(in_place) {}

  // 4.2, destructor
  ~expected() = default;

  // 4.3, assignment
  constexpr expected &operator=(const expected &rhs) = default;
  constexpr expected &operator=(expected &&rhs) = default;

  template <class U = T,
            enable_if_t<traits::template enable_assign_value_v<U>> * = nullptr,
            bool NoExcept = is_nothrow_constructible_v<T, U>
                &&is_nothrow_assignable_v<lvalue_reference_type, U>>
  constexpr expected &operator=(U &&v) noexcept(NoExcept) {
    impl_base::assign_value(std::forward<U>(v));
    return *this;
  }
  template <class G = E>
  constexpr expected &
  operator=(const unexpected<G> &e) noexcept(is_nothrow_destructible_v<T>) {
    impl_base::assign_error(e);
    return *this;
  }
  template <class G = E>
  constexpr expected &
  operator=(unexpected<G> &&e) noexcept(is_nothrow_destructible_v<T>) {
    impl_base::assign_error(std::move(e));
    return *this;
  }

  // 4.4, modifiers
  using impl_base::emplace;

  // 4.5, swap
  template <
      class U = T, class G = E,
      enable_if_t<(is_void_v<U> || (is_swappable_v<U> &&
                                    (is_nothrow_move_constructible_v<U> ||
                                     is_nothrow_move_constructible_v<G>))) &&
                  is_swappable_v<G>> * = nullptr,
      bool NoExcept = is_nothrow_swappable_v<U> &&is_nothrow_swappable_v<G>
          &&is_nothrow_move_constructible_v<U>
              &&is_nothrow_move_constructible_v<G>>
  void swap(expected<U, G> &rhs) noexcept(NoExcept) {
    if (this->has_value()) {
      if (rhs.has_value()) {
        if constexpr (!is_void_v<T>) {
          using std::swap;
          swap(this->val(), rhs.val());
        }
      } else {
        rhs.swap(*this);
      }
    } else {
      if (rhs.has_value()) {
        if constexpr (is_void_v<T>) {
          this->construct_error(std::move(rhs).error());
          rhs.destruct_error();
          rhs.construct_value();
        } else if constexpr (is_nothrow_move_constructible_v<E>) {
          E tmp = std::move(rhs).error();
          rhs.destruct_error();
          if constexpr (is_nothrow_move_constructible_v<T>) {
            rhs.construct_value(std::move(*this).val());
          } else {
            try {
              rhs.construct_value(std::move(*this).val());
            } catch (...) {
              rhs.construct_error(std::move(tmp));
              throw_exception_again;
            }
          }
          this->destruct_value();
          this->construct_error(std::move(tmp));
        } else {
          static_assert(is_nothrow_move_constructible_v<T>);
          T tmp = std::move(*this).val();
          this->destruct_value();
          try {
            this->construct_error(std::move(rhs).error());
          } catch (...) {
            this->construct_value(std::move(tmp));
            throw_exception_again;
          }
          rhs.destruct_error();
          rhs.construct_value(std::move(tmp));
        }
      } else {
        using std::swap;
        swap(this->error(), rhs.error());
      }
    }
  }
  template <
      class U = T, class G = E,
      enable_if_t<(!is_void_v<U> && (!is_swappable_v<U> ||
                                     (!is_nothrow_move_constructible_v<U> &&
                                      !is_nothrow_move_constructible_v<G>))) ||
                  !is_swappable_v<G>> * = nullptr>
  void swap(expected<U, G> &rhs) = delete;

  // 4.6, observers
  constexpr const T *operator->() const { return addressof(impl_base::val()); }
  constexpr T *operator->() { return addressof(impl_base::val()); }
  constexpr const_lvalue_reference_type operator*() const & {
    return impl_base::val();
  }
  constexpr lvalue_reference_type operator*() & { return impl_base::val(); }
  constexpr const_rvalue_reference_type operator*() const && {
    return std::move(impl_base::val());
  }
  constexpr rvalue_reference_type operator*() && {
    return std::move(impl_base::val());
  }
  constexpr explicit operator bool() const noexcept { return has_value(); }
  using impl_base::error;
  using impl_base::has_value;
  constexpr const_lvalue_reference_type value() const & {
    if (!has_value()) {
      throw(bad_expected_access<E>(error()));
    }
    return impl_base::val();
  }
  constexpr const_rvalue_reference_type value() const && {
    if (!has_value()) {
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable:5272)
#endif
      throw(bad_expected_access<E>(std::move(error())));
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
    }
    return std::move(impl_base::val());
  }
  constexpr lvalue_reference_type value() & {
    if (!has_value()) {
      throw(bad_expected_access<E>(error()));
    }
    return impl_base::val();
  }
  constexpr rvalue_reference_type value() && {
    if (!has_value()) {
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable:5272)
#endif
      throw(bad_expected_access<E>(std::move(error())));
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
    }
    return std::move(impl_base::val());
  }

  template <class U> constexpr T value_or(U &&v) const &noexcept {
    static_assert(!is_copy_constructible_v<T> || is_convertible_v<U, T>,
                  "T must be copy-constructible and convertible to from U");
    return bool(*this) ? **this : static_cast<T>(std::forward<U>(v));
  }
  template <class U> constexpr T value_or(U &&v) &&noexcept {
    static_assert(!is_move_constructible_v<T> || is_convertible_v<U, T>,
                  "T must be move-constructible and convertible to from U");
    return bool(*this) ? std::move(**this) : static_cast<T>(std::forward<U>(v));
  }

  // extensions
  template <class F> constexpr auto and_then(F &&f) & {
    return detail::expected_and_then_impl(*this, std::forward<F>(f));
  }
  template <class F> constexpr auto and_then(F &&f) && {
    return detail::expected_and_then_impl(std::move(*this), std::forward<F>(f));
  }
  template <class F> constexpr auto and_then(F &&f) const & {
    return detail::expected_and_then_impl(*this, std::forward<F>(f));
  }
  template <class F> constexpr auto and_then(F &&f) const && {
    return detail::expected_and_then_impl(std::move(*this), std::forward<F>(f));
  }

  template <class F> constexpr auto or_else(F &&f) & {
    return detail::expected_or_else_impl(*this, std::forward<F>(f));
  }
  template <class F> constexpr auto or_else(F &&f) && {
    return detail::expected_or_else_impl(std::move(*this), std::forward<F>(f));
  }
  template <class F> constexpr auto or_else(F &&f) const & {
    return detail::expected_or_else_impl(*this, std::forward<F>(f));
  }
  template <class F> constexpr auto or_else(F &&f) const && {
    return detail::expected_or_else_impl(std::move(*this), std::forward<F>(f));
  }

  template <class F> constexpr auto map(F &&f) & {
    return detail::expected_map_impl(*this, std::forward<F>(f));
  }
  template <class F> constexpr auto map(F &&f) && {
    return detail::expected_map_impl(std::move(*this), std::forward<F>(f));
  }
  template <class F> constexpr auto map(F &&f) const & {
    return detail::expected_map_impl(*this, std::forward<F>(f));
  }
  template <class F> constexpr auto map(F &&f) const && {
    return detail::expected_map_impl(std::move(*this), std::forward<F>(f));
  }

  template <class F> constexpr auto map_error(F &&f) & {
    return detail::expected_map_error_impl(*this, std::forward<F>(f));
  }
  template <class F> constexpr auto map_error(F &&f) && {
    return detail::expected_map_error_impl(std::move(*this),
                                           std::forward<F>(f));
  }
  template <class F> constexpr auto map_error(F &&f) const & {
    return detail::expected_map_error_impl(*this, std::forward<F>(f));
  }
  template <class F> constexpr auto map_error(F &&f) const && {
    return detail::expected_map_error_impl(std::move(*this),
                                           std::forward<F>(f));
  }
};

// 4.7, Expected equality operators
template <class T1, class E1, class T2, class E2>
constexpr bool operator==(const expected<T1, E1> &x,
                          const expected<T2, E2> &y) {
  if (bool(x) != bool(y)) {
    return false;
  }
  if (!bool(x)) {
    return x.error() == y.error();
  }
  if constexpr (is_void_v<T1> && is_void_v<T2>) {
    return true;
  } else {
    return *x == *y;
  }
}
template <class T1, class E1, class T2, class E2>
constexpr bool operator!=(const expected<T1, E1> &x,
                          const expected<T2, E2> &y) {
  if (bool(x) != bool(y)) {
    return true;
  }
  if (!bool(x)) {
    return x.error() != y.error();
  }
  if constexpr (is_void_v<T1> && is_void_v<T2>) {
    return true;
  } else {
    return *x != *y;
  }
}

// 4.8, Comparison with T
template <class T1, class E1, class T2>
constexpr enable_if_t<!is_void_v<T1> && !is_void_v<T2>, bool>
operator==(const expected<T1, E1> &x, const T2 &v) {
  return bool(x) ? *x == v : false;
}
template <class T1, class E1, class T2>
constexpr enable_if_t<!is_void_v<T1> && !is_void_v<T2>, bool>
operator==(const T2 &v, const expected<T1, E1> &x) {
  return bool(x) ? *x == v : false;
}
template <class T1, class E1, class T2>
constexpr enable_if_t<!is_void_v<T1> && !is_void_v<T2>, bool>
operator!=(const expected<T1, E1> &x, const T2 &v) {
  return bool(x) ? *x != v : false;
}
template <class T1, class E1, class T2>
constexpr enable_if_t<!is_void_v<T1> && !is_void_v<T2>, bool>
operator!=(const T2 &v, const expected<T1, E1> &x) {
  return bool(x) ? *x != v : false;
}

// 4.9, Comparison with unexpected<E>
template <class T1, class E1, class E2>
constexpr bool operator==(const expected<T1, E1> &x, const unexpected<E2> &e) {
  return bool(x) ? false : x.error() == e.value();
}
template <class T1, class E1, class E2>
constexpr bool operator==(const unexpected<E2> &e, const expected<T1, E1> &x) {
  return bool(x) ? false : x.error() == e.value();
}
template <class T1, class E1, class E2>
constexpr bool operator!=(const expected<T1, E1> &x, const unexpected<E2> &e) {
  return bool(x) ? true : x.error() != e.value();
}
template <class T1, class E1, class E2>
constexpr bool operator!=(const unexpected<E2> &e, const expected<T1, E1> &x) {
  return bool(x) ? true : x.error() != e.value();
}

// 4.10, Specialized algorithms
template <
    class T1, class E1,
    enable_if_t<(is_void_v<T1> || (is_swappable_v<T1> &&
                                   (is_nothrow_move_constructible_v<T1> ||
                                    is_nothrow_move_constructible_v<E1>))) &&
                is_swappable_v<E1>> * = nullptr,
    bool NoExcept = is_nothrow_swappable_v<T1> &&is_nothrow_swappable_v<E1>
        &&is_nothrow_move_constructible_v<T1>
            &&is_nothrow_move_constructible_v<E1>>
void swap(expected<T1, E1> &x, expected<T1, E1> &y) noexcept(NoExcept) {
  x.swap(y);
}
template <
    class T1, class E1,
    enable_if_t<(!is_void_v<T1> && (!is_swappable_v<T1> ||
                                    (!is_nothrow_move_constructible_v<T1> &&
                                     !is_nothrow_move_constructible_v<E1>))) ||
                !is_swappable_v<E1>> * = nullptr>
void swap(expected<T1, E1> &x, expected<T1, E1> &y) = delete;

template <class E1, enable_if_t<is_swappable_v<E1>> * = nullptr,
          bool NoExcept = is_nothrow_swappable_v<E1>>
void swap(unexpected<E1> &x, unexpected<E1> &y) noexcept(NoExcept) {
  x.swap(y);
}
template <class E1, enable_if_t<!is_swappable_v<E1>> * = nullptr>
void swap(unexpected<E1> &x, unexpected<E1> &y) = delete;

} // namespace cxx20

#undef try
#undef catch
#undef throw
#undef throw_exception_again
#undef M_ENABLE_EXCEPTIONS
