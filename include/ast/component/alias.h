// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/ast/component/alias.h - alias class definitions ----------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the Alias node class
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/component/sort.h"
#include "ast/expression.h"
#include "ast/type.h"

#include <vector>

namespace WasmEdge {
namespace AST {
namespace Component {

class AliasTargetExport {
public:
  uint32_t getInstanceIdx() const noexcept { return InstanceIndex; }
  uint32_t &getInstanceIdx() noexcept { return InstanceIndex; }
  std::string_view getName() const noexcept { return Name; }
  std::string &getName() noexcept { return Name; }

private:
  uint32_t InstanceIndex;
  std::string Name;
};
class AliasTargetOuter {
public:
  uint32_t getComponent() const noexcept { return ComponentIndex; }
  uint32_t &getComponent() noexcept { return ComponentIndex; }
  uint32_t getIndex() const noexcept { return Index; }
  uint32_t &getIndex() noexcept { return Index; }

private:
  uint32_t ComponentIndex;
  uint32_t Index;
};

using AliasTarget = std::variant<AliasTargetExport, AliasTargetOuter>;

/// Alias
class Alias {
public:
  Sort &getSort() noexcept { return S; }
  const Sort &getSort() const noexcept { return S; }

  AliasTarget &getTarget() noexcept { return Target; }
  const AliasTarget &getTarget() const noexcept { return Target; }

private:
  Sort S;
  AliasTarget Target;
};

} // namespace Component
} // namespace AST
} // namespace WasmEdge
