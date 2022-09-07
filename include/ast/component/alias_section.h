// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//=== Alias Section class definitions
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
#include <variant>

namespace WasmEdge {
namespace AST {

namespace AliasTarget {

class Export {
public:
  Export() = default;
  Export(uint32_t Idx, std::string_view N) noexcept
      : InstanceIndex{Idx}, Name{N} {}

  uint32_t getInstanceIdx() const noexcept { return InstanceIndex; }
  std::string_view getName() const noexcept { return Name; }

private:
  uint32_t InstanceIndex;
  std::string Name;
};
class CoreExport {
public:
  CoreExport() = default;
  CoreExport(uint32_t Idx, std::string_view N) noexcept
      : InstanceIndex{Idx}, Name{N} {}

  uint32_t getInstanceIdx() const noexcept { return InstanceIndex; }
  std::string_view getName() const noexcept { return Name; }

private:
  uint32_t InstanceIndex;
  std::string Name;
};
class Outer {
public:
  Outer() = default;
  Outer(uint32_t CIdx, uint32_t Idx) noexcept
      : ComponentIndex{CIdx}, Index{Idx} {}

  uint32_t getComponent() const noexcept { return ComponentIndex; }
  uint32_t getIndex() const noexcept { return Index; }

private:
  uint32_t ComponentIndex;
  uint32_t Index;
};

// aliastarget ::= 0x00 i:<instanceidx> n:<name>      => export i n
//               | 0x01 i:<core:instanceidx> n:<name> => core export i n
//               | 0x02 ct:<u32> idx:<u32>            => outer ct idx
using T = std::variant<Export, CoreExport, Outer>;

} // namespace AliasTarget

class Alias {
public:
  Sort &getSort() noexcept { return S; }
  const Sort &getSort() const noexcept { return S; }

  AliasTarget::T &getTarget() noexcept { return Target; }
  const AliasTarget::T &getTarget() const noexcept { return Target; }

private:
  Sort S;
  AliasTarget::T Target;
};

} // namespace AST
} // namespace WasmEdge
