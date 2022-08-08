// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//=== wasmedge/ast/component_section.h - Component Section class definitions
//
// Part of the WasmEdge Project.
//
//===------------------------------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the Component Section node classes.
///
//===------------------------------------------------------------------------------------------===//
#pragma once

#include "ast/component/export_section.h"
#include "ast/component/import_section.h"
#include "ast/module.h"

#include <vector>

namespace WasmEdge {
namespace AST {

/// AST ModuleSection node.
class ModuleSection : public Section {
public:
  /// Getter of content module.
  Span<const std::unique_ptr<Module>> getContent() const noexcept {
    return Content;
  }
  std::vector<std::unique_ptr<Module>> &getContent() noexcept {
    return Content;
  }

private:
  /// \name Data of ModuleSection.
  /// @{
  std::vector<std::unique_ptr<Module>> Content;
  /// @}
};

/// AST ComponentImportSection node.
class ComponentImportSection : public Section {
public:
  /// Getter of content.
  Span<const ImportDecl> getContent() const noexcept { return Content; }
  std::vector<ImportDecl> &getContent() noexcept { return Content; }

private:
  /// \name Data of ComponentImportSection.
  /// @{
  std::vector<ImportDecl> Content;
  /// @}
};

/// AST ComponentExportSection node.
class ComponentExportSection : public Section {
public:
  /// Getter of content.
  Span<const ExportDecl> getContent() const noexcept { return Content; }
  std::vector<ExportDecl> &getContent() noexcept { return Content; }

private:
  /// \name Data of ComponentExportSection.
  /// @{
  std::vector<ExportDecl> Content;
  /// @}
};

} // namespace AST
} // namespace WasmEdge
