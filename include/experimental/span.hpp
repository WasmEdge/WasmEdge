// SPDX-License-Identifier: CC0-1.0
///
// span - An c++20 implementation of std::span
// Written in 2020 by Shen-Ta Hsieh (ibmibmibm.tw@gmail.com, @ibmibmibm)
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

#include <array>
#include <cstddef>
#include <iterator>
#include <limits>
#include <memory>
#include <type_traits>

namespace cxx20 {
using namespace std;

inline constexpr size_t dynamic_extent = numeric_limits<size_t>::max();
template <class T, size_t Extent = dynamic_extent> struct span;

namespace detail {
template <class T, class = void> struct defined_to_address : false_type {};
template <class T>
struct defined_to_address<T, void_t<decltype(pointer_traits<T>::to_address)>>
    : true_type {};
} // namespace detail

template <class T> constexpr T *to_address(T *p) noexcept {
  static_assert(!is_function_v<T>);
  return p;
}

template <class T> constexpr auto to_address(const T &p) noexcept {
  if constexpr (detail::defined_to_address<T>::value) {
    return pointer_traits<T>::to_address(p);
  } else {
    return to_address(p.operator->());
  }
}

namespace detail {

template <class> struct is_span_or_array_impl : false_type {};
template <class T, size_t N>
struct is_span_or_array_impl<array<T, N>> : true_type {};
template <class T, size_t N>
struct is_span_or_array_impl<span<T, N>> : true_type {};

template <class, class = void> struct contiguous_range_element {
  using type = void;
};
template <class T>
struct contiguous_range_element<T, void_t<decltype(data(declval<T>()))>> {
  using type = remove_pointer_t<decltype(data(declval<T>()))>;
};

template <class T, class U>
struct is_compatible_element : is_convertible<U (*)[], T (*)[]> {};
template <class T> struct is_compatible_element<T, void> : false_type {};

template <class T>
static inline constexpr bool is_generic_range_v =
    !is_span_or_array_impl<remove_cv_t<T>>::value &&
    !is_array<remove_cv_t<T>>::value;

template <class T, class U>
static inline constexpr bool is_compatible_element_v =
    is_compatible_element<T, U>::value;
template <class T, class It>
static inline constexpr bool is_compatible_iterator_v = is_compatible_element_v<
    T, remove_pointer_t<decltype(to_address(declval<It>()))>>;
template <class T, class R>
static inline constexpr bool is_compatible_range_v = is_compatible_element_v<
    T, typename contiguous_range_element<remove_cv_t<R>>::type>;

template <class T, size_t N> class span_storage {
public:
  constexpr span_storage() noexcept = delete;
  constexpr span_storage(T *data, size_t) noexcept : m_data(data) {}

  constexpr T *data() const noexcept { return m_data; }
  constexpr size_t size() const noexcept { return N; }

private:
  T *m_data;
};

template <class T> class span_storage<T, 0> {
public:
  constexpr span_storage() noexcept : m_data(nullptr) {}
  constexpr span_storage(T *data, size_t) noexcept : m_data(data) {}

  constexpr T *data() const noexcept { return m_data; }
  constexpr size_t size() const noexcept { return 0; }

private:
  T *m_data;
};

template <class T> class span_storage<T, dynamic_extent> {
public:
  constexpr span_storage() noexcept : m_data(nullptr), m_size(0) {}
  constexpr span_storage(T *data, size_t size) noexcept
      : m_data(data), m_size(size) {}

  constexpr T *data() const noexcept { return m_data; }
  constexpr size_t size() const noexcept { return m_size; }

private:
  T *m_data;
  size_t m_size;
};

} // namespace detail

template <class T, size_t Extent>
struct span : public detail::span_storage<T, Extent> {
  using element_type = T;
  using value_type = remove_cv_t<T>;
  using size_type = size_t;
  using difference_type = ptrdiff_t;
  using pointer = T *;
  using const_pointer = const T *;
  using reference = T &;
  using const_reference = const T &;

  using iterator = pointer;
  using reverse_iterator = std::reverse_iterator<iterator>;

  static constexpr size_t extent = Extent;

  using base = detail::span_storage<T, Extent>;
  using detail::span_storage<T, Extent>::data;
  using detail::span_storage<T, Extent>::size;

  constexpr span() noexcept = default;
  template <class It,
            enable_if_t<detail::is_compatible_iterator_v<T, It>> * = nullptr>
  constexpr span(It first, size_t count) noexcept
      : base(to_address(first), count) {}
  template <class It, enable_if_t<detail::is_compatible_iterator_v<T, It>>>
  constexpr span(It first, It last) noexcept
      : base(to_address(first), last - first) {}
  template <size_t N>
  constexpr span(T (&arr)[N]) noexcept : base(std::data(arr), N) {}
  template <class U, size_t N,
            enable_if_t<detail::is_compatible_element_v<T, U>> * = nullptr>
  constexpr span(array<U, N> &arr) noexcept : base(std::data(arr), N) {}
  template <class U, size_t N,
            enable_if_t<detail::is_compatible_element_v<T, U>> * = nullptr>
  constexpr span(const array<U, N> &arr) noexcept : base(std::data(arr), N) {}
  template <class R,
            enable_if_t<detail::is_generic_range_v<R> &&
                        detail::is_compatible_range_v<T, R>> * = nullptr>
  constexpr span(R &&r) : base(std::data(r), std::size(r)) {}
  template <class U, enable_if_t<detail::is_compatible_element_v<T, const U>>
                         * = nullptr>
  constexpr span(std::initializer_list<U> il) noexcept
      : base(std::data(il), il.size()) {}
  template <class U,
            enable_if_t<!is_same_v<T, U> &&
                        detail::is_compatible_element_v<T, U>> * = nullptr>
  constexpr span(const span<U, Extent> &s) noexcept
      : base(s.data(), s.size()) {}
  template <class U,
            enable_if_t<!is_same_v<T, U> &&
                        detail::is_compatible_element_v<T, U>> * = nullptr>
  constexpr span &operator=(const span<U, Extent> &s) noexcept {
    *this = span<T, Extent>(s);
    return *this;
  }
  constexpr span(const span &s) noexcept = default;
  constexpr span &operator=(const span &s) noexcept = default;

  constexpr size_type size_bytes() const noexcept { return size() * sizeof(T); }
  [[nodiscard]] constexpr bool empty() const noexcept { return size() == 0; }

  constexpr iterator begin() const noexcept { return iterator(data()); }
  constexpr iterator end() const noexcept { return iterator(data() + size()); }
  constexpr reverse_iterator rbegin() const noexcept {
    return reverse_iterator(end());
  }
  constexpr reverse_iterator rend() const noexcept {
    return reverse_iterator(begin());
  }
  constexpr reference front() const { return *begin(); }
  constexpr reference back() const { return *rbegin(); }
  constexpr reference operator[](size_type idx) const { return data()[idx]; }

  template <size_t Count> constexpr span<T, Count> first() const {
    static_assert(Count <= Extent);
    return span<T, Count>(data(), Count);
  }
  constexpr span<T> first(size_t Count) const { return span<T>(data(), Count); }

  template <size_t Count> constexpr span<T, Count> last() const {
    static_assert(Count <= Extent);
    return span<T, Count>(data() + (size() - Count), Count);
  }
  constexpr span<T> last(size_t Count) const {
    return span<T>(data() + (size() - Count), Count);
  }

  template <size_t Offset, size_t Count = dynamic_extent>
  constexpr auto subspan() const {
    static_assert(Offset <= Extent);
    static_assert(Count == dynamic_extent || Count <= Extent - Offset);
    constexpr size_t NewExtend =
        Count != dynamic_extent
            ? Count
            : (Extent != dynamic_extent ? (Extent - Offset) : dynamic_extent);
    const size_t NewSize = Count != dynamic_extent ? Count : size() - Offset;
    return span<T, NewExtend>(data() + Offset, NewSize);
  }
  constexpr span<T> subspan(size_t Offset,
                            size_t Count = dynamic_extent) const {
    const size_t NewSize = Count != dynamic_extent ? Count : size() - Offset;
    return span<T>(data() + Offset, NewSize);
  }
};

template <class It, class EndOrSize>
span(It, EndOrSize)
    -> span<remove_pointer_t<decltype(to_address(declval<It>()))>>;
template <class T, size_t N> span(T (&)[N]) -> span<T, N>;
template <class T, size_t N> span(array<T, N> &) -> span<T, N>;
template <class T, size_t N> span(const array<T, N> &) -> span<const T, N>;
template <class R>
span(R &&) -> span<remove_pointer_t<decltype(data(declval<R>()))>>;

template <class T, size_t N> auto as_bytes(span<T, N> s) noexcept {
  constexpr size_t NewExtend =
      (N == dynamic_extent ? dynamic_extent : sizeof(T) * N);
  return span<const byte, NewExtend>(reinterpret_cast<const byte *>(s.data()),
                                     s.size_bytes());
}

template <class T, size_t N> auto as_writable_bytes(span<T, N> s) noexcept {
  constexpr size_t NewExtend =
      (N == dynamic_extent ? dynamic_extent : sizeof(T) * N);
  return span<byte, NewExtend>(reinterpret_cast<byte *>(s.data()),
                               s.size_bytes());
}

} // namespace cxx20
