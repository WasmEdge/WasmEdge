// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//=== Instance Section class definitions
//
// Part of the WasmEdge Project.
//
//===------------------------------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the Instance node classes.
///
//===------------------------------------------------------------------------------------------===//
#pragma once

#include "ast/component/export_section.h"

#include <variant>
#include <vector>

namespace WasmEdge {
namespace AST {

class InstantiateArg {
public:
  std::string_view getName() const noexcept { return Name; }
  void setName(std::string_view N) noexcept { Name = N; }

  const SortIndex &getSortIndex() const noexcept { return SortIdx; }
  SortIndex &getSortIndex() noexcept { return SortIdx; }

private:
  std::string Name;
  SortIndex SortIdx;
};

namespace InstanceExpr {

class Instantiate {
public:
  void setIndex(uint32_t I) noexcept { ComponentIndex = I; }
  const uint32_t &getIndex() const noexcept { return ComponentIndex; }

  Span<const InstantiateArg> getArgs() const noexcept { return Args; }
  std::vector<InstantiateArg> &getArgs() noexcept { return Args; }

private:
  uint32_t ComponentIndex;
  std::vector<InstantiateArg> Args;
};
class Export {
public:
  Span<const ExportDecl> getExports() const noexcept { return List; }
  std::vector<ExportDecl> &getExports() noexcept { return List; }

private:
  std::vector<ExportDecl> List;
};

using T = std::variant<Instantiate, Export>;

} // namespace InstanceExpr

using Instance = InstanceExpr::T;

} // namespace AST
} // namespace WasmEdge
