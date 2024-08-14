// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/po/option.h - Option -------------------------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
#pragma once

#include "po/helper.h"
#include "po/parser.h"

#include <cstdint>
#include <optional>
#include <string>
#include <type_traits>

namespace WasmEdge {
namespace PO {

struct Toggle {};

template <typename T, typename ParserT = Parser<std::conditional_t<
                          std::is_same_v<std::remove_pointer_t<T>, Toggle>,
                          bool, std::remove_pointer_t<T>>>>
class Option;

template <typename T> class Option<T, Parser<T>> {
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

  void default_argument() noexcept {
    Store = std::move(*Default);
    Default.reset();
  }

  cxx20::expected<void, Error> argument(std::string Argument) noexcept {
    if (auto Res = Parser<T>::parse(std::move(Argument)); !Res) {
      return cxx20::unexpected(Res.error());
    } else {
      Store = std::move(*Res);
    }
    return {};
  }

private:
  T Store{};
  std::optional<T> Default{};
  std::string_view Desc{};
  std::string_view Meta{};
  bool Hidden = false;
};

/// External storage
template <typename T> class Option<T *, Parser<T>> {
public:
  Option(T *Storage, const T *DefaultValue) noexcept
      : Store(Storage), Default(DefaultValue) {}

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
  Option(Hidden &&, ArgsT &&...Args) noexcept
      : Option(std::forward<ArgsT>(Args)...) {
    Hidden = true;
  }

  std::string_view description() const noexcept { return Desc; }

  std::string_view meta() const noexcept { return Meta; }

  bool hidden() const noexcept { return Hidden; }

  std::size_t min_narg() const noexcept { return Default != nullptr ? 0 : 1; }

  std::size_t max_narg() const noexcept { return 1; }

  const T &value() const noexcept { return *Store; }
  T &value() noexcept { return *Store; }

  void default_argument() noexcept {
    *Store = *Default;
    Default = nullptr;
  }

  cxx20::expected<void, Error> argument(std::string Argument) noexcept {
    if (auto Res = Parser<T>::parse(std::move(Argument)); !Res) {
      return cxx20::unexpected(Res.error());
    } else {
      *Store = std::move(*Res);
    }
    return {};
  }

private:
  T *Store;
  const T *Default;
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

  cxx20::expected<void, Error> argument(std::string Argument) noexcept {
    if (auto Res = Parser<bool>::parse(std::move(Argument)); !Res) {
      return cxx20::unexpected(Res.error());
    } else {
      Store = std::move(*Res);
    }
    return {};
  }

private:
  bool Store = false;
  std::string_view Desc{};
  bool Hidden = false;
};

template <> class Option<Toggle *, Parser<bool>> {
public:
  Option(bool *Storage, const bool *) noexcept : Store(Storage) {}

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

  const bool &value() const noexcept { return *Store; }
  bool &value() noexcept { return *Store; }

  void default_argument() noexcept { *Store = true; }

  cxx20::expected<void, Error> argument(std::string Argument) noexcept {
    if (auto Res = Parser<bool>::parse(std::move(Argument)); !Res) {
      return cxx20::unexpected(Res.error());
    } else {
      *Store = std::move(*Res);
    }
    return {};
  }

private:
  bool *Store;
  std::string_view Desc{};
  bool Hidden = false;
};

} // namespace PO
} // namespace WasmEdge
