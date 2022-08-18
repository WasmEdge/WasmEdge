// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//=== wasmedge/ast/component/type_section.h - Type Section class definitions
//
// Part of the WasmEdge Project.
//
//===------------------------------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the Type node classes.
///
//===------------------------------------------------------------------------------------------===//
#pragma once

#include "ast/component/alias_section.h"
#include "ast/component/coretype_section.h"
#include "ast/component/export_section.h"
#include "ast/component/import_section.h"

#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace WasmEdge {
namespace AST {

class Type {};
class DefinedType : public Type {
public:
};

class DefinedValueType : public DefinedType {
public:
  class Prim;
  class Record;
  class Variant;
  class List;
  class Tuple;
  class Flags;
  class Enum;
  class Union;
  class Option;
  class Result;
};

enum class PrimitiveValueType : Byte {
  String = 0x73,
  Char = 0x74,
  Float64 = 0x75,
  Float32 = 0x76,
  U64 = 0x77,
  S64 = 0x78,
  U32 = 0x79,
  S32 = 0x7a,
  U16 = 0x7b,
  S16 = 0x7c,
  U8 = 0x7d,
  S8 = 0x7e,
  Bool = 0x7f
};

using ValueType = std::variant<uint32_t, PrimitiveValueType>;

class NamedValType {
public:
  void setName(std::string_view S) noexcept { Name = S; }
  std::string_view getName() const noexcept { return Name; }

  const ValueType &getType() const noexcept { return Ty; }
  ValueType &getType() noexcept { return Ty; }

private:
  std::string Name;
  ValueType Ty;
};

using FuncVec = std::variant<ValueType, std::vector<NamedValType>>;

class CaseType {
public:
  ValueType &getType() noexcept { return Ty.value(); }

private:
  std::optional<ValueType> Ty{std::nullopt};
};
class Case {
public:
  void setName(std::string_view S) noexcept { Name = S; }
  std::string_view getName() const noexcept { return Name; }

  CaseType &getType() noexcept { return OptTy.value(); }

  void setLabelIndex(uint32_t Idx) { LabelIdx = Idx; }
  uint32_t getLabelIndex() { return LabelIdx.value(); }

private:
  // (case n t?)
  std::string Name;
  std::optional<CaseType> OptTy{std::nullopt};
  // (case n t? (refines case-label[i]))
  std::optional<uint32_t> LabelIdx;
};

class DefinedValueType::Prim : public DefinedValueType {
public:
  void setValue(PrimitiveValueType V) noexcept { Value = V; }
  PrimitiveValueType getValue() const noexcept { return Value; }

private:
  PrimitiveValueType Value;
};
class DefinedValueType::Record : public DefinedValueType {
public:
  Span<const NamedValType> getFields() const noexcept { return Fields; }
  std::vector<NamedValType> &getFields() noexcept { return Fields; }

private:
  std::vector<NamedValType> Fields;
};
class DefinedValueType::Variant : public DefinedValueType {
public:
  Span<const Case> getCases() const noexcept { return Cases; }
  std::vector<Case> &getCases() noexcept { return Cases; }

private:
  std::vector<Case> Cases;
};
class DefinedValueType::List : public DefinedValueType {
public:
  const ValueType &getType() const noexcept { return Ty; }
  ValueType &getType() noexcept { return Ty; }

private:
  ValueType Ty;
};
class DefinedValueType::Tuple : public DefinedValueType {
public:
  Span<const ValueType> getTypes() const noexcept { return Types; }
  std::vector<ValueType> &getTypes() noexcept { return Types; }

private:
  std::vector<ValueType> Types;
};
class DefinedValueType::Flags : public DefinedValueType {
public:
  Span<const std::string> getNames() const noexcept { return Names; }
  std::vector<std::string> &getNames() noexcept { return Names; }

private:
  std::vector<std::string> Names;
};
class DefinedValueType::Enum : public DefinedValueType {
public:
  Span<const std::string> getNames() const noexcept { return Names; }
  std::vector<std::string> &getNames() noexcept { return Names; }

private:
  std::vector<std::string> Names;
};
class DefinedValueType::Union : public DefinedValueType {
public:
  Span<const ValueType> getTypes() const noexcept { return Types; }
  std::vector<ValueType> &getTypes() noexcept { return Types; }

private:
  std::vector<ValueType> Types;
};
class DefinedValueType::Option : public DefinedValueType {
public:
  const ValueType &getType() const noexcept { return Ty; }
  ValueType &getType() noexcept { return Ty; }

private:
  ValueType Ty;
};
class DefinedValueType::Result : public DefinedValueType {
public:
  CaseType &getResult() noexcept { return ResultTy.value(); }
  CaseType &getError() noexcept { return ErrorTy.value(); }

private:
  std::optional<CaseType> ResultTy{std::nullopt};
  std::optional<CaseType> ErrorTy{std::nullopt};
};

class FuncType : public DefinedType {
public:
  const FuncVec &getParameters() const noexcept { return Parameters; }
  FuncVec &getParameters() noexcept { return Parameters; }

  const FuncVec &getReturns() const noexcept { return Returns; }
  FuncVec &getReturns() noexcept { return Returns; }

private:
  FuncVec Parameters;
  FuncVec Returns;
};

using InstanceDecl = std::variant<CoreType, Type, Alias, ExportDecl>;
class InstanceType : public DefinedType {
public:
  Span<const InstanceDecl> getDecls() const noexcept { return Decls; }
  std::vector<InstanceDecl> &getDecls() noexcept { return Decls; }

private:
  std::vector<InstanceDecl> Decls;
};

using ComponentDecl = std::variant<ImportDecl, InstanceDecl>;
class ComponentType : public DefinedType {
public:
  Span<const ComponentDecl> getDecls() const noexcept { return Decls; }
  std::vector<ComponentDecl> &getDecls() noexcept { return Decls; }

private:
  std::vector<ComponentDecl> Decls;
};

} // namespace AST
} // namespace WasmEdge
