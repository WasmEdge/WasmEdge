// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/po/list.h - Option list --------------------------------------===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
#pragma once

#include "po/argument_parser.h"
#include "po/helper.h"
#include "po/parser.h"

#include <cstdint>
#include <string>
#include <vector>

namespace SSVM {
namespace PO {

template <typename T, typename ParserT = Parser<T>> class List {
public:
  List() {}

  template <typename... ArgsT>
  List(Description &&D, ArgsT &&... Args) : List(std::forward<ArgsT>(Args)...) {
    Desc = std::move(D.Value);
  }

  template <typename... ArgsT>
  List(MetaVar &&M, ArgsT &&... Args) : List(std::forward<ArgsT>(Args)...) {
    Meta = std::move(M.Value);
  }

  template <typename... ArgsT>
  List(ZeroOrMore &&, ArgsT &&... Args) : List(std::forward<ArgsT>(Args)...) {
    IsOneOrMore = false;
  }

  template <typename... ArgsT>
  List(OneOrMore &&, ArgsT &&... Args) : List(std::forward<ArgsT>(Args)...) {
    IsOneOrMore = true;
  }

  template <typename... ArgsT>
  List(DefaultValue<T> &&V, ArgsT &&... Args)
      : List(std::forward<ArgsT>(Args)...) {
    Default.push_back(std::move(V.Value));
  }

  template <typename... ArgsT>
  List(Hidden &&, ArgsT &&... Args) : List(std::forward<ArgsT>(Args)...) {
    Hidden = true;
  }

  std::string_view description() const { return Desc; }

  std::string_view meta() const { return Meta; }

  bool hidden() const { return Hidden; }

  std::size_t min_narg() const { return IsOneOrMore ? 1 : 0; }

  std::size_t max_narg() const { return -1; }

  std::vector<T> &value() { return Store; }

  void default_argument() { Store = std::move(Default); }

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
} // namespace SSVM
