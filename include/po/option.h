// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//===-- wasmedge/po/option.h - Option -------------------------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
#pragma once

#include "po/helper.h"
#include "po/parser.h"

#include <cstdint>
#include <iostream>
#include <optional>
#include <string>
#include <type_traits>

namespace WasmEdge {
namespace PO {

struct Toggle {};

template <typename T,
          typename ParserT =
              Parser<std::conditional_t<std::is_same_v<T, Toggle>, bool, T>>>
class Option {
public:
  Option() noexcept {}

  template <typename... ArgsT>
  Option(Description &&D, ArgsT &&...Args) noexcept
      : Option(std::forward<ArgsT>(Args)...) {
    Desc = std::move(D.Value);
  }

  template <typename... ArgsT>
  Option(MetaVar &&M, ArgsT &&...Args) noexcept
      : Option(std::forward<ArgsT>(Args)...) {
    Meta = std::move(M.Value);
  }

  template <typename... ArgsT>
  Option(DefaultValue<T> &&V, ArgsT &&...Args) noexcept
      : Option(std::forward<ArgsT>(Args)...) {
    Default.emplace(std::move(V.Value));
  }

  template <typename... ArgsT>
  Option(Hidden &&, ArgsT &&...Args) noexcept
      : Option(std::forward<ArgsT>(Args)...) {
    Hidden = true;
  }

  std::string_view description() const noexcept { return Desc; }

  std::string_view meta() const noexcept { return Meta; }

  bool hidden() const noexcept { return Hidden; }

  std::size_t min_narg() const noexcept { return Default.has_value() ? 0 : 1; }

  std::size_t max_narg() const noexcept { return 1; }

  const T &value() const noexcept { return Store; }
  T &value() noexcept { return Store; }

  void default_argument() noexcept(std::is_nothrow_move_constructible_v<T>) {
    Store = std::move(*Default);
    Default.reset();
  }

  void argument(std::string Argument) {
    Store = ParserT::parse(std::move(Argument));
  }

private:
  T Store{};
  std::optional<T> Default{};
  std::string_view Desc{};
  std::string_view Meta{};
  bool Hidden = false;
};

template <> class Option<Toggle, Parser<bool>> {
public:
  Option() {}

  template <typename... ArgsT>
  Option(Description &&D, ArgsT &&...Args) noexcept
      : Option(std::forward<ArgsT>(Args)...) {
    Desc = std::move(D.Value);
  }

  template <typename... ArgsT>
  Option(Hidden &&, ArgsT &&...Args) noexcept
      : Option(std::forward<ArgsT>(Args)...) {
    Hidden = true;
  }

  std::string_view description() const noexcept { return Desc; }

  std::string_view meta() const noexcept { return {}; }

  bool hidden() const noexcept { return Hidden; }

  std::size_t min_narg() const noexcept { return 0; }

  std::size_t max_narg() const noexcept { return 0; }

  const bool &value() const noexcept { return Store; }
  bool &value() noexcept { return Store; }

  void default_argument() noexcept { Store = true; }

  void argument(std::string Argument) {
    Store = Parser<bool>::parse(std::move(Argument));
  }

private:
  bool Store = false;
  std::string_view Desc{};
  bool Hidden = false;
};

} // namespace PO
} // namespace WasmEdge
