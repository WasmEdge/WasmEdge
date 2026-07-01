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

private:
  ComponentValType Type;
  std::vector<Byte> Data;
};

} // namespace Component
} // namespace AST
} // namespace WasmEdge
