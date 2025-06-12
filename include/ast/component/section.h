// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/ast/component/section.h - Section class definitions ------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the Component Section node classes.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/component/alias.h"
#include "ast/component/canonical.h"
#include "ast/component/export.h"
#include "ast/component/import.h"
#include "ast/component/instance.h"
#include "ast/component/start.h"
#include "ast/component/type.h"
#include "ast/module.h"
#include "ast/section.h"

namespace WasmEdge {
namespace AST {
namespace Component {

/// AST Component::CoreModuleSection node.
class CoreModuleSection : public Section {
public:
  /// Getter of content.
  const Module &getContent() const noexcept { return Content; }
  Module &getContent() noexcept { return Content; }

private:
  /// \name Data of CoreModuleSection.
  /// @{
  Module Content;
  /// @}
};

/// AST Component::CoreInstanceSection node.
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

/// AST Component::CoreTypeSection node.
class CoreTypeSection : public Section {
public:
  /// Getter of content module.
  Span<const CoreDefType> getContent() const noexcept { return Content; }
  std::vector<CoreDefType> &getContent() noexcept { return Content; }

private:
  /// \name Data of CoreTypeSection.
  /// @{
  std::vector<CoreDefType> Content;
  /// @}
};

class Component;

/// AST Component::ComponentSection node.
class ComponentSection : public Section {
public:
  /// Getter of content.
  const Component &getContent() const noexcept { return *Content; }
  std::unique_ptr<Component> &getContent() noexcept { return Content; }

private:
  /// \name Data of ComponentSection.
  /// @{
  std::unique_ptr<Component> Content;
  /// @}
};

/// AST Component::InstanceSection node.
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

/// AST Component::AliasSection node.
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

/// AST Component::TypeSection node.
class TypeSection : public Section {
public:
  /// Getter of content module.
  Span<const DefType> getContent() const noexcept { return Content; }
  std::vector<DefType> &getContent() noexcept { return Content; }

private:
  /// \name Data of TypeSection.
  /// @{
  std::vector<DefType> Content;
  /// @}
};

/// AST Component::CanonSection node.
class CanonSection : public Section {
public:
  /// Getter of content module.
  Span<const Canonical> getContent() const noexcept { return Content; }
  std::vector<Canonical> &getContent() noexcept { return Content; }

private:
  /// \name Data of CanonicalSection.
  /// @{
  std::vector<Canonical> Content;
  /// @}
};

/// AST Component::StartSection node.
class StartSection : public Section {
public:
  /// Getter of content module.
  const Start &getContent() const noexcept { return Content; }
  Start &getContent() noexcept { return Content; }

private:
  /// \name Data of StartSection.
  /// @{
  Start Content;
  /// @}
};

/// AST Component::ImportSection node.
class ImportSection : public Section {
public:
  /// Getter of content module.
  Span<const Import> getContent() const noexcept { return Content; }
  std::vector<Import> &getContent() noexcept { return Content; }

private:
  /// \name Data of ImportSection.
  /// @{
  std::vector<Import> Content;
  /// @}
};

/// AST Component::ExportSection node.
class ExportSection : public Section {
public:
  /// Getter of content module.
  Span<const Export> getContent() const noexcept { return Content; }
  std::vector<Export> &getContent() noexcept { return Content; }

private:
  /// \name Data of ExportSection.
  /// @{
  std::vector<Export> Content;
  /// @}
};

// TODO: COMPONENT - AST Component::ValueSection node.

} // namespace Component
} // namespace AST
} // namespace WasmEdge
