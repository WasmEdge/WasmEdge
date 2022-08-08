// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//=== wasmedge/ast/component/import_section.h - Import Section class definitions
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

#include <cstdint>
#include <string>

namespace WasmEdge {
namespace AST {

class ExternDesc {};

class CoreType : public ExternDesc {
public:
  CoreType(uint32_t TypeIdx) : TypeIdx{TypeIdx} {}
  uint32_t getTypeIdx() const noexcept { return TypeIdx; }

private:
  uint32_t TypeIdx;
};

class FuncType : public ExternDesc {
public:
  FuncType(uint32_t TypeIdx) : TypeIdx{TypeIdx} {}
  uint32_t getTypeIdx() const noexcept { return TypeIdx; }

private:
  uint32_t TypeIdx;
};

class ValueType : public ExternDesc {};
class ValueTypeBool : public ValueType {};
class ValueTypeS8 : public ValueType {};
class ValueTypeU8 : public ValueType {};
class ValueTypeS16 : public ValueType {};
class ValueTypeU16 : public ValueType {};
class ValueTypeS32 : public ValueType {};
class ValueTypeU32 : public ValueType {};
class ValueTypeS64 : public ValueType {};
class ValueTypeU64 : public ValueType {};
class ValueTypeFloat32 : public ValueType {};
class ValueTypeFloat64 : public ValueType {};
class ValueTypeChar : public ValueType {};
class ValueTypeString : public ValueType {};
class ValueTypeIdx : public ValueType {
public:
  ValueTypeIdx(uint32_t TypeIdx) : TypeIdx{TypeIdx} {}
  uint32_t getTypeIdx() const noexcept { return TypeIdx; }

private:
  uint32_t TypeIdx;
};

class TypeBound : public ExternDesc {
public:
  TypeBound(uint32_t TypeIdx) : TypeIdx{TypeIdx} {}
  uint32_t getTypeIdx() const noexcept { return TypeIdx; }

private:
  uint32_t TypeIdx;
};

class InstanceType : public ExternDesc {
public:
  InstanceType(uint32_t TypeIdx) : TypeIdx{TypeIdx} {}
  uint32_t getTypeIdx() const noexcept { return TypeIdx; }

private:
  uint32_t TypeIdx;
};

class ComponentType : public ExternDesc {
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

  ExternDesc getExtern() const noexcept { return Extern; }
  void setExtern(ExternDesc E) noexcept { Extern = E; }

private:
  std::string Name;
  ExternDesc Extern;
};

} // namespace AST
} // namespace WasmEdge
