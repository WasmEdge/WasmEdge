// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include <optional>
#include <type_traits>
#include <utility>

namespace cxx20 {

template <class EF, typename = std::enable_if_t<std::is_invocable_v<EF>>>
class scope_exit {
  scope_exit(const scope_exit &) = delete;
  scope_exit &operator=(const scope_exit &) = delete;
  scope_exit &operator=(scope_exit &&) = delete;

public:
  template <class FN,
            typename = std::enable_if_t<!std::is_same_v<
                std::remove_cv_t<std::remove_reference_t<FN>>, scope_exit>>,
            typename = std::enable_if_t<std::is_constructible_v<EF, FN>>>
  constexpr explicit scope_exit(FN &&fn) noexcept(
      std::is_nothrow_constructible_v<EF, FN> ||
      std::is_nothrow_constructible_v<EF, FN &>)
      : ef(std::forward<FN>(fn)) {}
  template <
      typename = std::enable_if_t<std::is_nothrow_move_constructible_v<EF> ||
                                  std::is_copy_constructible_v<EF>>>
  constexpr scope_exit(scope_exit &&other) noexcept(
      std::is_nothrow_move_constructible_v<EF> ||
      std::is_nothrow_copy_constructible_v<EF>)
      : ef(std::move_if_noexcept(other.ef)) {
    other.release();
  }

  ~scope_exit() noexcept {
    if (ef) {
      (*ef)();
      ef.reset();
    }
  }

  constexpr void release() noexcept { ef.reset(); }

private:
  std::optional<EF> ef;
};

template <class EF> scope_exit(EF) -> scope_exit<EF>;

} // namespace cxx20
