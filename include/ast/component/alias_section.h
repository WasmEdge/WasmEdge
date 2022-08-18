// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//===-- wasmedge/ast/component/alias_section.h - Alias Section class definitions
//
// Part of the WasmEdge Project.
//
//===------------------------------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the Alias node classes.
///
//===------------------------------------------------------------------------------------------===//
#pragma once

#include "ast/component/sort.h"

#include <cinttypes>
#include <string>

namespace WasmEdge {
namespace AST {

class AliasTarget {
public:
  class Export;
  class Outer;
};
class AliasTarget::Export : public AliasTarget {
public:
  Export(uint32_t Idx, std::string_view N) noexcept
      : InstanceIndex{Idx}, Name{N} {}

  uint32_t getInstanceIdx() const noexcept { return InstanceIndex; }
  std::string_view getName() const noexcept { return Name; }

private:
  uint32_t InstanceIndex;
  std::string Name;
};
class AliasTarget::Outer : public AliasTarget {
public:
  Outer(uint32_t CIdx, uint32_t Idx) noexcept
      : ComponentIndex{CIdx}, Index{Idx} {}

  uint32_t getComponent() const noexcept { return ComponentIndex; }
  uint32_t getIndex() const noexcept { return Index; }

private:
  uint32_t ComponentIndex;
  uint32_t Index;
};

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

} // namespace AST
} // namespace WasmEdge
