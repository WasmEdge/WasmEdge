// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//=== Import Section class definitions
//
// Part of the WasmEdge Project.
//
//===------------------------------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the Import node classes.
///
//===------------------------------------------------------------------------------------------===//
#pragma once

#include "ast/component/value_type.h"

#include <cstdint>
#include <string>
#include <variant>

namespace WasmEdge {
namespace AST {

namespace ExternDesc {

class CoreType {
public:
  CoreType() = default;
  CoreType(uint32_t TypeIdx) : TypeIdx{TypeIdx} {}
  uint32_t getTypeIdx() const noexcept { return TypeIdx; }

private:
  uint32_t TypeIdx;
};
class FuncType {
public:
  FuncType() = default;
  FuncType(uint32_t TypeIdx) : TypeIdx{TypeIdx} {}
  uint32_t getTypeIdx() const noexcept { return TypeIdx; }

private:
  uint32_t TypeIdx;
};
class TypeBound {
public:
  TypeBound() = default;
  TypeBound(uint32_t TypeIdx) : TypeIdx{TypeIdx} {}
  uint32_t getTypeIdx() const noexcept { return TypeIdx; }

private:
  uint32_t TypeIdx;
};
class InstanceType {
public:
  InstanceType() = default;
  InstanceType(uint32_t TypeIdx) : TypeIdx{TypeIdx} {}
  uint32_t getTypeIdx() const noexcept { return TypeIdx; }

private:
  uint32_t TypeIdx;
};
class ComponentType {
public:
  ComponentType() = default;
  ComponentType(uint32_t TypeIdx) : TypeIdx{TypeIdx} {}
  uint32_t getTypeIdx() const noexcept { return TypeIdx; }

private:
  uint32_t TypeIdx;
};

using T = std::variant<CoreType, FuncType, ValueType, TypeBound, InstanceType,
                       ComponentType>;

} // namespace ExternDesc

// (import <name> <extern desc>)
class ImportDecl {
public:
  std::string_view getName() const noexcept { return Name; }
  void setName(std::string_view N) { Name = N; }

  const ExternDesc::T &getExtern() const noexcept { return Extern; }
  ExternDesc::T &getExtern() noexcept { return Extern; }

private:
  std::string Name;
  ExternDesc::T Extern;
};

} // namespace AST
} // namespace WasmEdge
