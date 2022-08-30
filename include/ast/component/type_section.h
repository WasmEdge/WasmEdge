// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//=== Type Section class definitions
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
#include "ast/component/value_type.h"

#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace WasmEdge {
namespace AST {

class Type;

class NamedValType {
public:
  NamedValType() = default;

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
  std::optional<ValueType> getType() noexcept { return Ty; }

private:
  std::optional<ValueType> Ty{std::nullopt};
};
class Case {
public:
  void setName(std::string_view S) noexcept { Name = S; }
  std::string_view getName() const noexcept { return Name; }

  std::optional<CaseType> getType() noexcept { return OptTy; }

  void setLabelIndex(uint32_t Idx) { LabelIdx = Idx; }
  uint32_t getLabelIndex() { return LabelIdx.value(); }

private:
  // (case n t?)
  std::string Name;
  std::optional<CaseType> OptTy{std::nullopt};
  // (case n t? (refines case-label[i]))
  std::optional<uint32_t> LabelIdx;
};

namespace DefinedValueType {

class Prim {
public:
  void setValue(PrimitiveValueType V) noexcept { Value = V; }
  PrimitiveValueType getValue() const noexcept { return Value; }

private:
  PrimitiveValueType Value;
};
class Record {
public:
  Span<const NamedValType> getFields() const noexcept { return Fields; }
  std::vector<NamedValType> &getFields() noexcept { return Fields; }

private:
  std::vector<NamedValType> Fields;
};
class Variant {
public:
  Span<const Case> getCases() const noexcept { return Cases; }
  std::vector<Case> &getCases() noexcept { return Cases; }

private:
  std::vector<Case> Cases;
};
class List {
public:
  const ValueType &getType() const noexcept { return Ty; }
  ValueType &getType() noexcept { return Ty; }

private:
  ValueType Ty;
};
class Tuple {
public:
  Span<const ValueType> getTypes() const noexcept { return Types; }
  std::vector<ValueType> &getTypes() noexcept { return Types; }

private:
  std::vector<ValueType> Types;
};
class Flags {
public:
  Span<const std::string> getNames() const noexcept { return Names; }
  std::vector<std::string> &getNames() noexcept { return Names; }

private:
  std::vector<std::string> Names;
};
class Enum {
public:
  Span<const std::string> getNames() const noexcept { return Names; }
  std::vector<std::string> &getNames() noexcept { return Names; }

private:
  std::vector<std::string> Names;
};
class Union {
public:
  Span<const ValueType> getTypes() const noexcept { return Types; }
  std::vector<ValueType> &getTypes() noexcept { return Types; }

private:
  std::vector<ValueType> Types;
};
class Option {
public:
  const ValueType &getType() const noexcept { return Ty; }
  ValueType &getType() noexcept { return Ty; }

private:
  ValueType Ty;
};
class Result {
public:
  std::optional<CaseType> getResult() noexcept { return ResultTy; }
  std::optional<CaseType> getError() noexcept { return ErrorTy; }

private:
  std::optional<CaseType> ResultTy{std::nullopt};
  std::optional<CaseType> ErrorTy{std::nullopt};
};

using T = std::variant<Prim, Record, Variant, List, Tuple, Flags, Enum, Union,
                       Option, Result>;
} // namespace DefinedValueType

class FuncType {
public:
  const FuncVec &getParameters() const noexcept { return Parameters; }
  FuncVec &getParameters() noexcept { return Parameters; }

  const FuncVec &getReturns() const noexcept { return Returns; }
  FuncVec &getReturns() noexcept { return Returns; }

private:
  FuncVec Parameters;
  FuncVec Returns;
};

using InstanceDecl =
    std::variant<CoreType, std::shared_ptr<Type>, Alias, ExportDecl>;
class InstanceType {
public:
  Span<const InstanceDecl> getDecls() const noexcept { return Decls; }
  std::vector<InstanceDecl> &getDecls() noexcept { return Decls; }

private:
  std::vector<InstanceDecl> Decls;
};

using ComponentDecl = std::variant<ImportDecl, InstanceDecl>;
class ComponentType {
public:
  Span<const ComponentDecl> getDecls() const noexcept { return Decls; }
  std::vector<ComponentDecl> &getDecls() noexcept { return Decls; }

private:
  std::vector<ComponentDecl> Decls;
};

using DefinedType =
    std::variant<DefinedValueType::T, FuncType, InstanceType, ComponentType>;
class Type {
public:
  DefinedType &getData() noexcept { return Data; }

private:
  DefinedType Data;
};

} // namespace AST
} // namespace WasmEdge
