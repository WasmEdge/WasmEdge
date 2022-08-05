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

private:
  /// \name Data of Component node.
  /// @{
  std::vector<Byte> Magic;
  std::vector<Byte> Version;
  std::vector<Byte> Layer;
  /// @}
};

} // namespace AST
} // namespace WasmEdge
