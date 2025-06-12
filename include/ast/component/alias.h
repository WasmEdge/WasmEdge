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

#include <cstdint>
#include <string>
#include <variant>

namespace WasmEdge {
namespace AST {
namespace Component {

// core:alias       ::= s:<core:sort> t:<core:aliastarget> => (alias t (s))
// core:aliastarget ::= 0x01 ct:<u32> idx:<u32>            => outer ct idx

/// AST Component::CoreAlias node.
class CoreAlias {
public:
  Sort &getSort() noexcept { return S; }
  const Sort &getSort() const noexcept { return S; }

  uint32_t getComponentJump() const noexcept { return CompJump; }
  void setComponentJump(const uint32_t Ct) noexcept { CompJump = Ct; }

  uint32_t getIndex() const noexcept { return Index; }
  void setIndex(const uint32_t Idx) noexcept { Index = Idx; }

private:
  Sort S;
  uint32_t CompJump;
  uint32_t Index;
};

// alias       ::= s:<sort> t:<aliastarget>                => (alias t (s))
// aliastarget ::= 0x00 i:<instanceidx> n:<name>           => export i n
//               | 0x01 i:<core:instanceidx> n:<core:name> => core export i n
//               | 0x02 ct:<u32> idx:<u32>                 => outer ct idx

/// AST Component::Alias node.
class Alias {
public:
  enum class TargetType : uint8_t {
    Export = 0x00,
    CoreExport = 0x01,
    Outer = 0x02
  };

  TargetType getTargetType() const noexcept { return Type; }
  void setTargetType(const TargetType T) noexcept { Type = T; }

  Sort &getSort() noexcept { return S; }
  const Sort &getSort() const noexcept { return S; }

  const std::pair<uint32_t, std::string> &getExport() const noexcept {
    return *std::get_if<std::pair<uint32_t, std::string>>(&Target);
  }
  void setExport(const uint32_t Idx, std::string_view Name) noexcept {
    Target.emplace<std::pair<uint32_t, std::string>>(Idx, Name);
  }

  const std::pair<uint32_t, uint32_t> &getOuter() const noexcept {
    return *std::get_if<std::pair<uint32_t, uint32_t>>(&Target);
  }
  void setOuter(const uint32_t Ct, const uint32_t Idx) noexcept {
    Target.emplace<std::pair<uint32_t, uint32_t>>(Ct, Idx);
  }

private:
  TargetType Type;
  Sort S;
  std::variant<std::pair<uint32_t, std::string>, std::pair<uint32_t, uint32_t>>
      Target;
};

} // namespace Component
} // namespace AST
} // namespace WasmEdge
