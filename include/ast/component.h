// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//===-- wasmedge/ast/component.h - Component class definition -------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the Component node class, which is the
/// component node in AST.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/component_section.h"

namespace WasmEdge {
namespace AST {

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

class Component {
public:
  /// Getter of magic vector.
  const std::vector<Byte> &getMagic() const noexcept { return Magic; }
  std::vector<Byte> &getMagic() noexcept { return Magic; }

  /// Getter of version vector.
  const std::vector<Byte> &getVersion() const noexcept { return Version; }
  std::vector<Byte> &getVersion() noexcept { return Version; }

  /// Getter of layer vector.
  const std::vector<Byte> &getLayer() const noexcept { return Layer; }
  std::vector<Byte> &getLayer() noexcept { return Layer; }

  /// Getters of references to sections.
  const ModuleSection &getModuleSection() const { return ModuleSec; }
  ModuleSection &getModuleSection() { return ModuleSec; }

  const CoreInstanceSection &getCoreInstanceSection() const {
    return CoreInstanceSec;
  }
  CoreInstanceSection &getCoreInstanceSection() { return CoreInstanceSec; }

  const CoreAliasSection &getCoreAliasSection() const { return CoreAliasSec; }
  CoreAliasSection &getCoreAliasSection() { return CoreAliasSec; }

  const CoreTypeSection &getCoreTypeSection() const { return CoreTypeSec; }
  CoreTypeSection &getCoreTypeSection() { return CoreTypeSec; }

  const ComponentSection &getComponentSection() const { return ComponentSec; }
  ComponentSection &getComponentSection() { return ComponentSec; }

  const ComponentCanonSection &getCanonSection() const { return CanonSec; }
  ComponentCanonSection &getCanonSection() { return CanonSec; }

  const ComponentStartSection &getStartSection() const { return StartSec; }
  ComponentStartSection &getStartSection() { return StartSec; }

  const ComponentImportSection &getImportSection() const { return ImportSec; }
  ComponentImportSection &getImportSection() { return ImportSec; }

  const ComponentExportSection &getExportSection() const { return ExportSec; }
  ComponentExportSection &getExportSection() { return ExportSec; }

private:
  /// \name Data of Component node.
  /// @{
  std::vector<Byte> Magic;
  std::vector<Byte> Version;
  std::vector<Byte> Layer;
  /// @}

  /// \name Section nodes of Module node.
  /// @{
  ModuleSection ModuleSec;
  /// @}

  /// \name Section nodes of Core Instance node.
  /// @{
  CoreInstanceSection CoreInstanceSec;
  /// @}

  /// \name Section nodes of Core Alias node.
  /// @{
  CoreAliasSection CoreAliasSec;
  /// @}

  /// \name Section nodes of Core Type node.
  /// @{
  CoreTypeSection CoreTypeSec;
  /// @}

  /// \name Section nodes of Component node.
  /// @{
  ComponentSection ComponentSec;
  /// @}

  /// \name Section nodes of Canon node.
  /// @{
  ComponentCanonSection CanonSec;
  /// @}

  /// \name Section nodes of Start node.
  /// @{
  ComponentStartSection StartSec;
  /// @}

  /// \name Section nodes of Import node.
  /// @{
  ComponentImportSection ImportSec;
  /// @}

  /// \name Section nodes of Export node.
  /// @{
  ComponentExportSection ExportSec;
  /// @}
};

} // namespace AST
} // namespace WasmEdge
