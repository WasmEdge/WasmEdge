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

#include <cstdint>
#include <string>

namespace WasmEdge {
namespace AST {

class ExportDecl {
public:
  std::string_view getName() const noexcept { return Name; }
  void setName(std::string_view N) { Name = N; }

  const SortIndex &getExtern() const noexcept { return SortIdx; }
  SortIndex &getExtern() noexcept { return SortIdx; }

private:
  std::string Name;
  SortIndex SortIdx;
};

} // namespace AST
} // namespace WasmEdge
