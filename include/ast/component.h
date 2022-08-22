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
  const ModuleSection &getModuleSection() const noexcept { return ModuleSec; }
  ModuleSection &getModuleSection() noexcept { return ModuleSec; }

  const CoreInstanceSection &getCoreInstanceSection() const noexcept {
    return CoreInstanceSec;
  }
  CoreInstanceSection &getCoreInstanceSection() noexcept {
    return CoreInstanceSec;
  }

  const CoreAliasSection &getCoreAliasSection() const noexcept {
    return CoreAliasSec;
  }
  CoreAliasSection &getCoreAliasSection() noexcept { return CoreAliasSec; }

  const CoreTypeSection &getCoreTypeSection() const noexcept {
    return CoreTypeSec;
  }
  CoreTypeSection &getCoreTypeSection() noexcept { return CoreTypeSec; }

  const ComponentSection &getComponentSection() const noexcept {
    return ComponentSec;
  }
  ComponentSection &getComponentSection() noexcept { return ComponentSec; }

  const InstanceSection &getInstanceSection() const noexcept {
    return InstanceSec;
  }
  InstanceSection &getInstanceSection() noexcept { return InstanceSec; }

  const AliasSection &getAliasSection() const noexcept { return AliasSec; }
  AliasSection &getAliasSection() noexcept { return AliasSec; }

  const ComponentTypeSection &getTypeSection() const noexcept {
    return TypeSec;
  }
  ComponentTypeSection &getTypeSection() noexcept { return TypeSec; }

  const ComponentCanonSection &getCanonSection() const noexcept {
    return CanonSec;
  }
  ComponentCanonSection &getCanonSection() noexcept { return CanonSec; }

  const ComponentStartSection &getStartSection() const noexcept {
    return StartSec;
  }
  ComponentStartSection &getStartSection() noexcept { return StartSec; }

  const ComponentImportSection &getImportSection() const noexcept {
    return ImportSec;
  }
  ComponentImportSection &getImportSection() noexcept { return ImportSec; }

  const ComponentExportSection &getExportSection() const noexcept {
    return ExportSec;
  }
  ComponentExportSection &getExportSection() noexcept { return ExportSec; }

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

  /// \name Section nodes of Instance node.
  /// @{
  InstanceSection InstanceSec;
  /// @}

  /// \name Section nodes of Instance node.
  /// @{
  AliasSection AliasSec;
  /// @}

  /// \name Section nodes of Instance node.
  /// @{
  ComponentTypeSection TypeSec;
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
