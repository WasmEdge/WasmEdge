// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/po/option.h - Option -----------------------------------------===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
#pragma once

#include "po/argument_parser.h"
#include "po/helper.h"
#include "po/parser.h"

#include <cstdint>
#include <iostream>
#include <string>

namespace SSVM {
namespace PO {

struct Toggle {};

template <typename T,
          typename ParserT =
              Parser<std::conditional_t<std::is_same_v<T, Toggle>, bool, T>>>
class Option {
public:
  Option() {}

  template <typename... ArgsT>
  Option(Description &&D, ArgsT &&... Args)
      : Option(std::forward<ArgsT>(Args)...) {
    Desc = std::move(D.Value);
  }

  template <typename... ArgsT>
  Option(MetaVar &&M, ArgsT &&... Args) : Option(std::forward<ArgsT>(Args)...) {
    Meta = std::move(M.Value);
  }

  template <typename... ArgsT>
  Option(DefaultValue<T> &&V, ArgsT &&... Args)
      : Option(std::forward<ArgsT>(Args)...) {
    Default.emplace(std::move(V.Value));
  }

  template <typename... ArgsT>
  Option(Hidden &&, ArgsT &&... Args) : Option(std::forward<ArgsT>(Args)...) {
    Hidden = true;
  }

  std::string_view description() const { return Desc; }

  std::string_view meta() const { return Meta; }

  bool hidden() const { return Hidden; }

  std::size_t min_narg() const { return Default.has_value() ? 0 : 1; }

  std::size_t max_narg() const { return 1; }

  T &value() { return Store; }

  void default_argument() {
    Store = std::move(*Default);
    Default.reset();
  }

  void argument(std::string Argument) {
    Store = ParserT::parse(std::move(Argument));
  }

private:
  T Store{};
  std::optional<T> Default{};
  std::string Desc{};
  std::string Meta{};
  bool Hidden = false;
};

template <> class Option<Toggle, Parser<bool>> {
public:
  Option() {}

  template <typename... ArgsT>
  Option(Description &&D, ArgsT &&... Args)
      : Option(std::forward<ArgsT>(Args)...) {
    Desc = std::move(D.Value);
  }

  template <typename... ArgsT>
  Option(Hidden &&, ArgsT &&... Args) : Option(std::forward<ArgsT>(Args)...) {
    Hidden = true;
  }

  std::string_view description() const { return Desc; }

  std::string_view meta() const { return {}; }

  bool hidden() const { return Hidden; }

  std::size_t min_narg() const { return 0; }

  std::size_t max_narg() const { return 0; }

  bool &value() { return Store; }

  void default_argument() { Store = true; }

  void argument(std::string Argument) {
    Store = Parser<bool>::parse(std::move(Argument));
  }

private:
  bool Store = false;
  std::string Desc{};
  bool Hidden = false;
};

} // namespace PO
} // namespace SSVM
