// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/plugins/wasi_crypto/utils/handles_manager.h - OptionalRef ===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the definition of the OptionalRef and some helper
/// functions.
///
//===----------------------------------------------------------------------===//

#pragma once

#include "utils/error.h"

#include <functional>
#include <optional>
#include <type_traits>
#include <utility>

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {

template <typename T> using OptionalRef = T *;

namespace detail {

template <typename> struct IsOptional : std::false_type {};
template <typename T> struct IsOptional<std::optional<T>> : std::true_type {
  using Type = T;
};

template <typename> struct IsExpected : std::false_type {};
template <typename T> struct IsExpected<WasiCryptoExpect<T>> : std::true_type {
  using Type = T;
};

template <typename> struct IsOptionalRef : std::false_type {};
template <typename T> struct IsOptionalRef<OptionalRef<T>> : std::true_type {
  using Type = T;
};

template <typename> struct IsExpectedOptionalRef : std::false_type {};
template <typename T>
struct IsExpectedOptionalRef<WasiCryptoExpect<OptionalRef<T>>>
    : std::true_type {
  using Type = T;
};

} // namespace detail

/// std::optional<T> -> (T -> WasiCrypto<R>) ->
/// WasiCryptoExpect<std::optional<R>>
template <typename F>
inline auto mapAndTransposeOptional(const __wasi_opt_options_t Optional,
                                    F &&Function) noexcept
    -> std::enable_if_t<
        detail::IsExpected<std::decay_t<decltype(std::invoke(
            std::forward<F>(Function), Optional.u.some))>>::value,
        WasiCryptoExpect<std::optional<
            typename detail::IsExpected<std::decay_t<decltype(std::invoke(
                std::forward<F>(Function), Optional.u.some))>>::Type>>> {
  if (Optional.tag == __WASI_OPT_OPTIONS_U_NONE)
    return std::nullopt;

  return std::invoke(std::forward<F>(Function), Optional.u.some);
}

template <typename F>
inline auto mapAndTransposeOptional(const __wasi_opt_symmetric_key_t Optional,
                                    F &&Function) noexcept
    -> std::enable_if_t<
        detail::IsExpected<std::decay_t<decltype(std::invoke(
            std::forward<F>(Function), Optional.u.some))>>::value,
        WasiCryptoExpect<std::optional<
            typename detail::IsExpected<std::decay_t<decltype(std::invoke(
                std::forward<F>(Function), Optional.u.some))>>::Type>>> {
  if (Optional.tag == __WASI_OPT_SYMMETRIC_KEY_U_NONE)
    return std::nullopt;

  return std::invoke(std::forward<F>(Function), Optional.u.some);
}

/// std::optional<T> -> (T -> WasiCryptoExpect<OptionalRef<R>>) ->
/// WasiCryptoExpect<OptionalRef<R>>
template <
    typename O, typename F,
    typename = std::enable_if_t<detail::IsOptional<std::decay_t<O>>::value>>
inline auto transposeOptionalToRef(O &&Optional, F &&Function) noexcept
    -> WasiCryptoExpect<OptionalRef<typename detail::IsExpectedOptionalRef<
        std::decay_t<decltype(std::invoke(
            std::forward<F>(Function), *std::forward<O>(Optional)))>>::Type>> {
  if (!Optional)
    return nullptr;

  return std::invoke(std::forward<F>(Function), *Optional);
}

/// OptionalRef<T> -> (T -> WasiCryptoExpect<OptionalRef<R>>) ->
/// WasiCryptoExpect<OptionalRef<R>>
template <
    typename O, typename F,
    typename = std::enable_if_t<detail::IsOptionalRef<std::decay_t<O>>::value>>
inline auto transposeOptionalRef(O &&Optional, F &&Function) noexcept
    -> WasiCryptoExpect<OptionalRef<typename detail::IsExpectedOptionalRef<
        std::decay_t<decltype(std::invoke(
            std::forward<F>(Function), *std::forward<O>(Optional)))>>::Type>> {
  if (!Optional)
    return nullptr;

  return std::invoke(std::forward<F>(Function), *Optional);
}

/// std::optional<T> -> OptionalRef<T>
template <typename O, typename = std::enable_if_t<
                          detail::IsOptional<std::decay_t<O>>::value>>
inline auto asOptionalRef(O &&Optional) noexcept
    -> OptionalRef<typename detail::IsOptional<std::decay_t<O>>::Type> {
  if (!Optional)
    return nullptr;

  return &*Optional;
}

} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
