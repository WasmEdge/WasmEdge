// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/ast/component/instance.h - instance class definitions ----===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the Instance node class
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/component/sort.h"
#include "ast/component/type.h"
#include "ast/expression.h"
#include "ast/type.h"

#include <vector>

namespace WasmEdge {
namespace AST {
namespace Component {

class Import {
  std::string Name;
  ExternDesc Desc;

public:
  std::string &getName() noexcept { return Name; }
  std::string_view getName() const noexcept { return Name; }
  ExternDesc &getDesc() noexcept { return Desc; }
  const ExternDesc &getDesc() const noexcept { return Desc; }
};
class Export {
  std::string Name;
  SortIndex<Sort> Idx;
  std::optional<ExternDesc> Desc;

public:
  std::string &getName() noexcept { return Name; }
  std::string_view getName() const noexcept { return Name; }
  SortIndex<Sort> &getSortIndex() noexcept { return Idx; }
  const SortIndex<Sort> &getSortIndex() const noexcept { return Idx; }
  std::optional<ExternDesc> &getDesc() noexcept { return Desc; }
  const std::optional<ExternDesc> getDesc() const noexcept { return Desc; }
};

} // namespace Component
} // namespace AST
} // namespace WasmEdge
