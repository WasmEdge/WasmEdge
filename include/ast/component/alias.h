// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/ast/component/alias.h - Alias class definitions ----------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the Alias node related classes.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/component/sort.h"

#include <string>
#include <variant>
#include <vector>

namespace WasmEdge {
namespace AST {
namespace Component {

// aliastarget ::= 0x00 i:<instanceidx> n:<name>           => export i n
//               | 0x01 i:<core:instanceidx> n:<core:name> => core export i n
//               | 0x02 ct:<u32> idx:<u32>                 => outer ct idx

/// AST Component::AliasTargetExport class.
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

/// AST Component::AliasTargetOuter class.
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

/// AST Component::AliasTarget aliasing.
using AliasTarget = std::variant<AliasTargetExport, AliasTargetOuter>;

// TODO: COMPONENT - Combine the AliasTarget variant into the Alias class.

// alias ::= s:<sort> t:<aliastarget> => (alias t (s))

/// AST Component::Alias node.
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
