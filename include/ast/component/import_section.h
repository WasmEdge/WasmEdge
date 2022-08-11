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

class ExternDesc {
public:
  class CoreType;
  class FuncType;
  class ValueType;
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

class ExternDesc::ValueType : public ExternDesc {
public:
  class Bool;
  class S8;
  class U8;
  class S16;
  class U16;
  class S32;
  class U32;
  class S64;
  class U64;
  class Float32;
  class Float64;
  class Char;
  class String;
  class Idx;
};
class ExternDesc::ValueType::Bool : public ExternDesc::ValueType {};
class ExternDesc::ValueType::S8 : public ExternDesc::ValueType {};
class ExternDesc::ValueType::U8 : public ExternDesc::ValueType {};
class ExternDesc::ValueType::S16 : public ExternDesc::ValueType {};
class ExternDesc::ValueType::U16 : public ExternDesc::ValueType {};
class ExternDesc::ValueType::S32 : public ExternDesc::ValueType {};
class ExternDesc::ValueType::U32 : public ExternDesc::ValueType {};
class ExternDesc::ValueType::S64 : public ExternDesc::ValueType {};
class ExternDesc::ValueType::U64 : public ExternDesc::ValueType {};
class ExternDesc::ValueType::Float32 : public ExternDesc::ValueType {};
class ExternDesc::ValueType::Float64 : public ExternDesc::ValueType {};
class ExternDesc::ValueType::Char : public ExternDesc::ValueType {};
class ExternDesc::ValueType::String : public ExternDesc::ValueType {};
class ExternDesc::ValueType::Idx : public ExternDesc::ValueType {
public:
  Idx(uint32_t TypeIdx) : TypeIdx{TypeIdx} {}
  uint32_t getTypeIdx() const noexcept { return TypeIdx; }

private:
  uint32_t TypeIdx;
};

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

  ExternDesc getExtern() const noexcept { return Extern; }
  void setExtern(ExternDesc E) noexcept { Extern = E; }

private:
  std::string Name;
  ExternDesc Extern;
};

} // namespace AST
} // namespace WasmEdge
