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

namespace WasmEdge {
namespace AST {

class ExternDesc {
public:
  class CoreType;
  class FuncType;
  class ValType;
  class TypeBound;
  class InstanceType;
  class ComponentType;
};

class ExternDesc::CoreType : public ExternDesc {
public:
  CoreType(uint32_t TypeIdx) : TypeIdx{TypeIdx} {}
  uint32_t getTypeIdx() const noexcept { return TypeIdx; }

private:
  uint32_t TypeIdx;
};

class ExternDesc::FuncType : public ExternDesc {
public:
  FuncType(uint32_t TypeIdx) : TypeIdx{TypeIdx} {}
  uint32_t getTypeIdx() const noexcept { return TypeIdx; }

private:
  uint32_t TypeIdx;
};

class ExternDesc::ValType : public ExternDesc, public ValueType {};

class ExternDesc::TypeBound : public ExternDesc {
public:
  TypeBound(uint32_t TypeIdx) : TypeIdx{TypeIdx} {}
  uint32_t getTypeIdx() const noexcept { return TypeIdx; }

private:
  uint32_t TypeIdx;
};

class ExternDesc::InstanceType : public ExternDesc {
public:
  InstanceType(uint32_t TypeIdx) : TypeIdx{TypeIdx} {}
  uint32_t getTypeIdx() const noexcept { return TypeIdx; }

private:
  uint32_t TypeIdx;
};

class ExternDesc::ComponentType : public ExternDesc {
public:
  ComponentType(uint32_t TypeIdx) : TypeIdx{TypeIdx} {}
  uint32_t getTypeIdx() const noexcept { return TypeIdx; }

private:
  uint32_t TypeIdx;
};

// (import <name> <extern desc>)
class ImportDecl {
public:
  std::string_view getName() const noexcept { return Name; }
  void setName(std::string_view N) { Name = N; }

  const ExternDesc &getExtern() const noexcept { return Extern; }
  ExternDesc &getExtern() noexcept { return Extern; }

private:
  std::string Name;
  ExternDesc Extern;
};

} // namespace AST
} // namespace WasmEdge
