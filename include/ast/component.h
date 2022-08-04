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

private:
  /// \name Data of Component node.
  /// @{
  std::vector<Byte> Magic;
};

} // namespace AST
} // namespace WasmEdge
