// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//=== wasmedge/ast/component/export_section.h - Export Section class definitions
//
// Part of the WasmEdge Project.
//
//===------------------------------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the Export node classes.
///
//===------------------------------------------------------------------------------------------===//
#pragma once

#include "ast/component/sort.h"
#include "ast/description.h"

#include <cstdint>
#include <string>

namespace WasmEdge {
namespace AST {

class SortIndex {};
class CoreSortIndex : public SortIndex, public ExportDesc {};
class ComponentSortIndex : public SortIndex {
public:
  ComponentSortIndex(Sort Sort, uint32_t Index) : Sort{Sort}, Index{Index} {}
  Sort getSort() const noexcept { return Sort; }
  uint32_t getIndex() const noexcept { return Index; }

private:
  Sort Sort;
  uint32_t Index;
};

class ExportDecl {
public:
  std::string_view getName() const noexcept { return Name; }
  void setName(std::string_view N) { Name = N; }

  SortIndex getExtern() const noexcept { return SortIdx; }
  void setExtern(SortIndex E) noexcept { SortIdx = E; }

private:
  std::string Name;
  SortIndex SortIdx;
};

} // namespace AST
} // namespace WasmEdge
