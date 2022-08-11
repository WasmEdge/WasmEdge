// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//===---------- wasmedge/ast/component/corealias_section.h -------------===//
//
// CoreAlias Section class definitions
//
// Part of the WasmEdge Project.
//
//===------------------------------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the Core Alias node classes.
///
//===------------------------------------------------------------------------------------------===//
#pragma once

#include "ast/component/module_decl.h"
#include "ast/component/sort.h"

#include <cinttypes>
#include <string>

namespace WasmEdge {
namespace AST {

class CoreAliasTarget {
public:
  class Export;
  class Outer;
};
class CoreAliasTarget::Export : public CoreAliasTarget {
public:
  Export(uint32_t Idx, std::string_view N) noexcept
      : InstanceIdx{Idx}, Name{N} {}

  uint32_t getInstanceIdx() const noexcept { return InstanceIdx; }
  std::string_view getName() const noexcept { return Name; }

private:
  uint32_t InstanceIdx;
  std::string Name;
};
class CoreAliasTarget::Outer : public CoreAliasTarget {
public:
  Outer(uint32_t C, uint32_t Idx) noexcept : Component{C}, Index{Idx} {}

  uint32_t getComponent() const noexcept { return Component; }
  uint32_t getIndex() const noexcept { return Index; }

private:
  uint32_t Component;
  uint32_t Index;
};

class CoreAlias : public ModuleDecl {
public:
  void setSort(CoreSort S) noexcept { Sort = S; }
  CoreSort getSort() const noexcept { return Sort; }
  void setTarget(CoreAliasTarget T) noexcept { Target = T; }
  const CoreAliasTarget &getTarget() const noexcept { return Target; }

private:
  CoreSort Sort;
  CoreAliasTarget Target;
};

} // namespace AST
} // namespace WasmEdge
