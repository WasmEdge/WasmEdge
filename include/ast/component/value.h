// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/ast/component/value.h - Value class definition -----------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the Value node class.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/span.h"
#include "common/types.h"

#include <optional>
#include <vector>

namespace WasmEdge {
namespace AST {
namespace Component {

// value ::= t:<valtype> len:<core:u32> v:<val(t)> => (value t v)
//           (where len = ||v||)

/// AST Component::Value node.
class Value {
public:
  ComponentValType &getType() noexcept { return Type; }
  const ComponentValType &getType() const noexcept { return Type; }
  std::vector<Byte> &getData() noexcept { return Data; }
  Span<const Byte> getData() const noexcept { return Data; }
  /// Payload decoded by the validator and consumed at instantiation. It is
  /// mutable because validation walks a const AST.
  const std::optional<ComponentValVariant> &getDecoded() const noexcept {
    return Decoded;
  }
  void setDecoded(ComponentValVariant V) const noexcept {
    Decoded.emplace(std::move(V));
  }

private:
  ComponentValType Type;
  std::vector<Byte> Data;
  mutable std::optional<ComponentValVariant> Decoded;
};

} // namespace Component
} // namespace AST
} // namespace WasmEdge
