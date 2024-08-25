// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/common/variant.h - Unsafe variant implementation ---------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the unsafe version of std::variant.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "span.h"

#include <cstddef>
#include <cstring>
#include <type_traits>
#include <utility>
#include <variant>

namespace WasmEdge {

template <typename... Ts> union VariadicUnion {};
template <typename FirstT, typename... RestT>
union VariadicUnion<FirstT, RestT...> {
  constexpr VariadicUnion() : Rest() {}
  ~VariadicUnion() noexcept = default;
  constexpr VariadicUnion(const VariadicUnion &) = default;
  constexpr VariadicUnion(VariadicUnion &&) = default;
  constexpr VariadicUnion &operator=(const VariadicUnion &) = default;
  constexpr VariadicUnion &operator=(VariadicUnion &&) = default;

  template <typename... Args>
  constexpr VariadicUnion(std::in_place_index_t<0>, Args &&...Values)
      : First(std::forward<Args>(Values)...) {}

  template <std::size_t N, typename... Args>
  constexpr VariadicUnion(std::in_place_index_t<N>, Args &&...Values)
      : Rest(std::in_place_index<N - 1>, std::forward<Args>(Values)...) {}

  template <typename T> constexpr const T &get() const &noexcept {
    if constexpr (std::is_same_v<T, FirstT>) {
      return First;
    } else {
      return Rest.template get<T>();
    }
  }
  template <typename T> constexpr T &get() &noexcept {
    if constexpr (std::is_same_v<T, FirstT>) {
      return First;
    } else {
      return Rest.template get<T>();
    }
  }
  template <typename T> constexpr const T &&get() const &&noexcept {
    if constexpr (std::is_same_v<T, FirstT>) {
      return std::move(First);
    } else {
      return std::move(Rest).template get<T>();
    }
  }
  template <typename T> constexpr T &&get() &&noexcept {
    if constexpr (std::is_same_v<T, FirstT>) {
      return std::move(First);
    } else {
      return std::move(Rest).template get<T>();
    }
  }

  template <typename T, typename... Args>
  constexpr T &emplace(Args &&...Values) &noexcept {
    if constexpr (std::is_same_v<T, FirstT>) {
      ::new (&First) FirstT(std::forward<Args>(Values)...);
      return *std::launder(&First);
    } else {
      return Rest.template emplace<T>(std::forward<Args>(Values)...);
    }
  }

  template <typename T, typename... Args>
  constexpr T &emplace(Args &&...Values) &&noexcept {
    if constexpr (std::is_same_v<T, FirstT>) {
      ::new (&First) FirstT(std::forward<Args>(Values)...);
      return std::move(*std::launder(&First));
    } else {
      return std::move(Rest).template emplace<T>(std::forward<Args>(Values)...);
    }
  }

  FirstT First;
  VariadicUnion<RestT...> Rest;
};

namespace detail {

template <typename T> struct tag {
  using type = T;
};

template <typename T>
using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;

template <typename T> struct is_in_place_tag : std::false_type {};
template <typename T>
struct is_in_place_tag<std::in_place_type_t<T>> : std::true_type {};
template <std::size_t N>
struct is_in_place_tag<std::in_place_index_t<N>> : std::true_type {};

template <typename T>
static constexpr bool not_in_place_tag =
    !is_in_place_tag<remove_cvref_t<T>>::value;

template <typename T, typename... Types>
struct index_of : std::integral_constant<std::size_t, 0> {};

template <typename T, typename... Types>
inline constexpr std::size_t index_of_v = index_of<T, Types...>::value;

template <typename T, typename FirstT, typename... RestT>
struct index_of<T, FirstT, RestT...>
    : std::integral_constant<std::size_t, std::is_same_v<T, FirstT>
                                              ? 0
                                              : index_of_v<T, RestT...> + 1> {};

template <typename... Ts> struct biggest_type;
template <typename FirstT, typename... RestT>
struct biggest_type<FirstT, RestT...> {
  using rest_type = typename biggest_type<RestT...>::type;
  using type = typename std::conditional_t<sizeof(rest_type) <= sizeof(FirstT),
                                           FirstT, rest_type>;
};
template <typename FirstT> struct biggest_type<FirstT> {
  using type = FirstT;
};

} // namespace detail

template <typename... Types> class Variant {
  static_assert(sizeof...(Types) > 0,
                "variant must have at least one alternative");
  static_assert(!(std::is_reference_v<Types> || ...),
                "variant must have no reference alternative");
  static_assert(!(std::is_void_v<Types> || ...),
                "variant must have no void alternative");
  static_assert((std::is_trivially_copyable_v<Types> && ...),
                "variant must be trivially copyable");
  static_assert((std::is_trivially_destructible_v<Types> && ...),
                "variant must be trivially destructible");

  template <typename T>
  static constexpr bool not_self =
      !std::is_same_v<detail::remove_cvref_t<T>, Variant>;
  template <typename T>
  static constexpr bool accept_type =
      (std::is_same_v<detail::remove_cvref_t<T>, Types> || ...);

  VariadicUnion<Types...> Storage;

public:
  constexpr Variant() noexcept : Variant(std::in_place_index_t<0>()) {}
  constexpr Variant(const Variant &) noexcept = default;
  constexpr Variant(Variant &&) noexcept = default;
  constexpr Variant &operator=(const Variant &) noexcept = default;
  constexpr Variant &operator=(Variant &&) noexcept = default;
  ~Variant() noexcept = default;

  template <typename T, typename = std::enable_if_t<
                            detail::not_in_place_tag<T> && not_self<T>>>
  constexpr Variant(T &&Value) noexcept
      : Variant(
            std::in_place_index_t<std::variant<detail::tag<Types>...>(
                                      detail::tag<detail::remove_cvref_t<T>>())
                                      .index()>(),
            std::forward<T>(Value)) {}

  template <std::size_t N, typename... Args>
  constexpr Variant(std::in_place_index_t<N> In, Args &&...Values) noexcept
      : Storage(In, std::forward<Args>(Values)...) {}

  template <typename T, typename = std::enable_if_t<accept_type<T>>,
            typename... Args>
  constexpr Variant(std::in_place_type_t<T>, Args &&...Values) noexcept
      : Variant(std::in_place_index_t<detail::index_of_v<T, Types...>>(),
                std::forward<Args>(Values)...) {}

  template <typename T> constexpr T &get() &noexcept {
    return Storage.template get<T>();
  }

  template <typename T> constexpr T &&get() &&noexcept {
    return std::move(Storage).template get<T>();
  }

  template <typename T> constexpr const T &get() const &noexcept {
    return Storage.template get<T>();
  }

  template <typename T> constexpr const T &&get() const &&noexcept {
    return std::move(Storage).template get<T>();
  }

  template <typename T, typename... Args>
  constexpr T &emplace(Args &&...Values) &noexcept {
    return Storage.template emplace<T>(std::forward<Args>(Values)...);
  }

  template <typename T, typename... Args>
  constexpr T &&emplace(Args &&...Values) &&noexcept {
    return std::move(Storage).template emplace<T>(
        std::forward<Args>(Values)...);
  }

  template <typename T>
  static constexpr Variant
  wrap(typename detail::biggest_type<Types...>::type Buffer) noexcept {
    static_assert(std::is_trivially_copyable_v<Variant>);
    Variant Result{T{}};
    std::memcpy(static_cast<void *>(&Result), &Buffer, sizeof(Buffer));
    return Result;
  }

  constexpr typename detail::biggest_type<Types...>::type
  unwrap() const noexcept {
    static_assert(std::is_trivially_copyable_v<Variant>);
    typename detail::biggest_type<Types...>::type Result;
    std::memcpy(&Result, static_cast<const void *>(this), sizeof(Result));
    return Result;
  }
};

} // namespace WasmEdge
