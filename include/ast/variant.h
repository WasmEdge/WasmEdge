//===-- ssvm/ast/variant.h - Unsafe variant implement -----------*- C++ -*-===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the unsafe version of std::variant
///
//===----------------------------------------------------------------------===//
#pragma once

#include <cstddef>
#include <cstring>
#include <type_traits>
#include <utility>
#include <variant>

namespace SSVM {
namespace AST {

template <typename... Ts> union VariadicUnion {};
template <typename FirstT, typename... RestT>
union VariadicUnion<FirstT, RestT...> {
  constexpr VariadicUnion() : Rest() {}

  template <typename... Args>
  constexpr VariadicUnion(std::in_place_index_t<0>, Args &&... Values)
      : First() {
    ::new (&First) FirstT(std::forward<Args>(Values)...);
  }

  template <std::size_t N, typename... Args>
  constexpr VariadicUnion(std::in_place_index_t<N>, Args &&... Values)
      : Rest(std::in_place_index<N - 1>, std::forward<Args>(Values)...) {}

  template <typename T>
  constexpr const T &get() const &noexcept {
    if constexpr (std::is_same_v<T, FirstT>) {
      return *reinterpret_cast<const FirstT *>(&First);
    } else {
      return Rest.template get<T>();
    }
  }
  template <typename T>
  constexpr T &get() & noexcept {
    if constexpr (std::is_same_v<T, FirstT>) {
      return *reinterpret_cast<FirstT *>(&First);
    } else {
      return Rest.template get<T>();
    }
  }
  template <typename T>
  constexpr const T &&get() const &&noexcept {
    if constexpr (std::is_same_v<T, FirstT>) {
      return std::move(*reinterpret_cast<const FirstT *>(&First));
    } else {
      return std::move(Rest).template get<T>();
    }
  }
  template <typename T>
  constexpr T &&get() && noexcept {
    if constexpr (std::is_same_v<T, FirstT>) {
      return std::move(*reinterpret_cast<FirstT *>(&First));
    } else {
      return std::move(Rest).template get<T>();
    }
  }

  std::aligned_storage_t<sizeof(FirstT), alignof(FirstT)> First;
  VariadicUnion<RestT...> Rest;
};

template <typename T> struct tag { using type = T; };

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

template <typename... Types> class Variant {
  static_assert(sizeof...(Types) > 0,
                "variant must have at least one alternative");
  static_assert(!(std::is_reference_v<Types> || ...),
                "variant must have no reference alternative");
  static_assert(!(std::is_void_v<Types> || ...),
                "variant must have no void alternative");
  static_assert((std::is_trivially_destructible_v<Types> && ...),
                "variant must be trivially destructible");

  template <typename T>
  static constexpr bool not_self = !std::is_same_v<remove_cvref_t<T>, Variant>;
  template <typename T>
  static constexpr bool
      accept_type = (std::is_same_v<remove_cvref_t<T>, Types> || ...);

  VariadicUnion<Types...> Storage;

public:
  constexpr Variant() noexcept : Variant(std::in_place_index_t<0>()) {}
  constexpr Variant(const Variant &) noexcept = default;
  constexpr Variant(Variant &&) noexcept = default;
  constexpr Variant &operator=(const Variant &) noexcept = default;
  constexpr Variant &operator=(Variant &&) noexcept = default;
  ~Variant() noexcept = default;

  template <typename T,
            typename = std::enable_if_t<not_in_place_tag<T> && not_self<T>>>
  constexpr Variant(T &&Value) noexcept
      : Variant(std::in_place_index_t<std::variant<tag<Types>...>(
                                          tag<remove_cvref_t<T>>())
                                          .index()>(),
                std::forward<T>(Value)) {}

  template <std::size_t N, typename... Args>
  constexpr Variant(std::in_place_index_t<N> In, Args &&... Values) noexcept
      : Storage(In, std::forward<Args>(Values)...) {}

  template <typename T, typename = std::enable_if_t<accept_type<T>>,
            typename... Args>
  constexpr Variant(std::in_place_type_t<T>, Args &&... Values) noexcept
      : Variant(std::in_place_index_t<index_of_v<T, Types...>>(),
                std::forward<Args>(Values)...) {}

  template <typename T> constexpr T &get() & noexcept {
    return Storage.template get<T>();
  }

  template <typename T> constexpr T &&get() && noexcept {
    return std::move(Storage).template get<T>();
  }

  template <typename T> constexpr const T &get() const &noexcept {
    return Storage.template get<T>();
  }

  template <typename T> constexpr const T &&get() const &&noexcept {
    return std::move(Storage).template get<T>();
  }
};

} // namespace AST
} // namespace SSVM

namespace std {

template <typename T, typename... Types>
constexpr T &get(SSVM::AST::Variant<Types...> &Variant) {
  return Variant.template get<T>();
}

template <typename T, typename... Types>
constexpr T &&get(SSVM::AST::Variant<Types...> &&Variant) {
  return std::move(Variant).template get<T>();
}

template <typename T, typename... Types>
constexpr const T &get(const SSVM::AST::Variant<Types...> &Variant) {
  return Variant.template get<T>();
}

template <typename T, typename... Types>
constexpr const T &&get(const SSVM::AST::Variant<Types...> &&Variant) {
  return std::move(Variant).template get<T>();
}

template <std::size_t I, typename... Types>
constexpr typename std::variant_alternative_t<I, std::variant<Types...>> &
get(SSVM::AST::Variant<Types...> &Variant) {
  return Variant
      .template get<std::variant_alternative_t<I, std::variant<Types...>>>();
}

template <std::size_t I, typename... Types>
constexpr typename std::variant_alternative_t<I, std::variant<Types...>> &
get(SSVM::AST::Variant<Types...> &&Variant) {
  return std::move(Variant)
      .template get<std::variant_alternative_t<I, std::variant<Types...>>>();
}

template <std::size_t I, typename... Types>
constexpr const typename std::variant_alternative_t<I, std::variant<Types...>> &
get(const SSVM::AST::Variant<Types...> &Variant) {
  return Variant
      .template get<std::variant_alternative_t<I, std::variant<Types...>>>();
}

template <std::size_t I, typename... Types>
constexpr const typename std::variant_alternative_t<I, std::variant<Types...>> &
get(const SSVM::AST::Variant<Types...> &&Variant) {
  return std::move(Variant)
      .template get<std::variant_alternative_t<I, std::variant<Types...>>>();
}


} // namespace std
