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

#include "ast/component/alias_section.h"
#include "ast/component/canon_section.h"
#include "ast/component/corealias_section.h"
#include "ast/component/coreinstance_section.h"
#include "ast/component/coretype_section.h"
#include "ast/component/export_section.h"
#include "ast/component/import_section.h"
#include "ast/component/instance_section.h"
#include "ast/component/start_section.h"
#include "ast/component/type_section.h"
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

/// AST CoreInstanceSection node.
class CoreInstanceSection : public Section {
public:
  /// Getter of content module.
  Span<const CoreInstance> getContent() const noexcept { return Content; }
  std::vector<CoreInstance> &getContent() noexcept { return Content; }

private:
  /// \name Data of CoreInstanceSection.
  /// @{
  std::vector<CoreInstance> Content;
  /// @}
};

class CoreAliasSection : public Section {
public:
  /// Getter of content.
  Span<const CoreAlias> getContent() const noexcept { return Content; }
  std::vector<CoreAlias> &getContent() noexcept { return Content; }

private:
  std::vector<CoreAlias> Content;
};

class CoreTypeSection : public Section {
public:
  /// Getter of content.
  Span<const CoreType> getContent() const noexcept { return Content; }
  std::vector<CoreType> &getContent() noexcept { return Content; }

private:
  std::vector<CoreType> Content;
};

class Component;

class ComponentSection : public Section {
public:
  /// Getter of content.
  Span<const std::unique_ptr<Component>> getContent() const noexcept {
    return Content;
  }
  std::vector<std::unique_ptr<Component>> &getContent() noexcept {
    return Content;
  }

private:
  std::vector<std::unique_ptr<Component>> Content;
};

class InstanceSection : public Section {
public:
  /// Getter of content module.
  Span<const Instance> getContent() const noexcept { return Content; }
  std::vector<Instance> &getContent() noexcept { return Content; }

private:
  /// \name Data of InstanceSection.
  /// @{
  std::vector<Instance> Content;
  /// @}
};

class AliasSection : public Section {
public:
  /// Getter of content module.
  Span<const Alias> getContent() const noexcept { return Content; }
  std::vector<Alias> &getContent() noexcept { return Content; }

private:
  /// \name Data of AliasSection.
  /// @{
  std::vector<Alias> Content;
  /// @}
};

class ComponentTypeSection : public Section {
public:
  /// Getter of content module.
  Span<const Type> getContent() const noexcept { return Content; }
  std::vector<Type> &getContent() noexcept { return Content; }

private:
  /// \name Data of ComponentTypeSection.
  /// @{
  std::vector<Type> Content;
  /// @}
};

/// AST ComponentCanonSection node.
class ComponentCanonSection : public Section {
public:
  /// Getter of content.
  Span<const Canon> getContent() const noexcept { return Content; }
  std::vector<Canon> &getContent() noexcept { return Content; }

private:
  /// \name Data of ComponentCanonSection.
  /// @{
  std::vector<Canon> Content;
  /// @}
};

/// AST ComponentStartSection node.
class ComponentStartSection : public Section {
public:
  /// Getter and setter of funcidx.
  uint32_t getFuncIdx() const { return FuncIdx; }
  void setFuncIdx(uint32_t Val) noexcept { FuncIdx = Val; }

  /// Getter of content.
  Span<const StartValueIdx> getContent() const noexcept { return Content; }
  std::vector<StartValueIdx> &getContent() noexcept { return Content; }

private:
  /// \name Data of ComponentStartSection.
  /// @{
  uint32_t FuncIdx;
  std::vector<StartValueIdx> Content;
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
