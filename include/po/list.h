// SPDX-License-Identifier: Apache-2.0
//===-- wasmedge/po/list.h - Option list ----------------------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
#pragma once

#include "po/helper.h"
#include "po/parser.h"

#include <cstdint>
#include <limits>
#include <string>
#include <type_traits>
#include <vector>

namespace WasmEdge {
namespace PO {

template <typename T, typename ParserT = Parser<T>> class List {
public:
  constexpr List() {}

  template <typename... ArgsT>
  List(Description &&D, ArgsT &&...Args) : List(std::forward<ArgsT>(Args)...) {
    Desc = std::move(D.Value);
  }

  template <typename... ArgsT>
  List(MetaVar &&M, ArgsT &&...Args) : List(std::forward<ArgsT>(Args)...) {
    Meta = std::move(M.Value);
  }

  template <typename... ArgsT>
  List(ZeroOrMore &&, ArgsT &&...Args) : List(std::forward<ArgsT>(Args)...) {
    IsOneOrMore = false;
  }

  template <typename... ArgsT>
  List(OneOrMore &&, ArgsT &&...Args) : List(std::forward<ArgsT>(Args)...) {
    IsOneOrMore = true;
  }

  template <typename... ArgsT>
  List(DefaultValue<T> &&V, ArgsT &&...Args)
      : List(std::forward<ArgsT>(Args)...) {
    Default.push_back(std::move(V.Value));
  }

  template <typename... ArgsT>
  List(Hidden &&, ArgsT &&...Args) : List(std::forward<ArgsT>(Args)...) {
    Hidden = true;
  }

  std::string_view description() const noexcept { return Desc; }

  std::string_view meta() const noexcept { return Meta; }

  bool hidden() const noexcept { return Hidden; }

  std::size_t min_narg() const noexcept { return IsOneOrMore ? 1 : 0; }

  std::size_t max_narg() const noexcept {
    return std::numeric_limits<std::size_t>::max();
  }

  const std::vector<T> &value() const noexcept { return Store; }
  std::vector<T> &value() noexcept { return Store; }

  void default_argument() noexcept(std::is_nothrow_move_constructible_v<T>) {
    Store = std::move(Default);
  }

  void argument(std::string Argument) {
    Store.push_back(ParserT::parse(std::move(Argument)));
  }

private:
  std::vector<T> Store;
  std::vector<T> Default;
  std::string Desc;
  std::string Meta;
  bool IsOneOrMore = false;
  bool Hidden = false;
};

} // namespace PO
} // namespace WasmEdge
