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
#include "common/span.h"

#include <variant>
#include <vector>

namespace WasmEdge {
namespace AST {
namespace Component {

// component ::= <preamble> s*:<section>*            => (component flatten(s*))
// preamble  ::= <magic> <version> <layer>
// magic     ::= 0x00 0x61 0x73 0x6D
// version   ::= 0x0d 0x00
// layer     ::= 0x01 0x00
// section   ::=    section_0(<core:custom>)         => ϵ
//             | m: section_1(<core:module>)         => [core-prefix(m)]
//             | i*:section_2(vec(<core:instance>))  => core-prefix(i)*
//             | t*:section_3(vec(<core:type>))      => core-prefix(t)*
//             | c: section_4(<component>)           => [c]
//             | i*:section_5(vec(<instance>))       => i*
//             | a*:section_6(vec(<alias>))          => a*
//             | t*:section_7(vec(<type>))           => t*
//             | c*:section_8(vec(<canon>))          => c*
//             | s: section_9(<start>)               => [s]
//             | i*:section_10(vec(<import>))        => i*
//             | e*:section_11(vec(<export>))        => e*
//             | v*:section_12(vec(<value>))         => v* 🪙

/// AST Component::Component node.
class Component {
  // TODO: COMPONENT - ValueSection
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
