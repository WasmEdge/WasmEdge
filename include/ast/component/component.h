// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2025 Second State INC

//===-- wasmedge/ast/component/component.h - Component class definition ---===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the Component node class, which is the
/// component module node in AST.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/component/section.h"

#include <vector>

namespace WasmEdge {
namespace AST {
namespace Component {

/// AST Component::Component node.
class Component {
  // TODO: ValueSection
  using Section =
      std::variant<CustomSection, CoreModuleSection, CoreInstanceSection,
                   CoreTypeSection, ComponentSection, InstanceSection,
                   AliasSection, TypeSection, CanonSection, StartSection,
                   ImportSection, ExportSection>;

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

  std::vector<Section> &getSections() noexcept { return Secs; }
  Span<const Section> getSections() const noexcept { return Secs; }

private:
  /// \name Data of Component node.
  /// @{
  std::vector<Byte> Magic;
  std::vector<Byte> Version;
  std::vector<Byte> Layer;
  std::vector<Section> Secs;
  /// @}
};

} // namespace Component
} // namespace AST
} // namespace WasmEdge
