// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2023 Second State INC

//===-- wasmedge/ast/component/type.h - type class definitions ----===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the Type node class
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/component/alias.h"
#include "ast/expression.h"
#include "ast/type.h"

#include <vector>

namespace WasmEdge {
namespace AST {

enum class PrimValType {
  Bool,
  S8,
  U8,
  S16,
  U16,
  S32,
  U32,
  S64,
  U64,
  Float32,
  Float64,
  Char,
  String
};

using TypeIndex = uint32_t;
using ValueType = std::variant<PrimValType, TypeIndex>;

class LabelValType {
public:
  std::string_view getLabel() const noexcept { return Label; }
  std::string &getLabel() noexcept { return Label; }
  const ValueType getValType() const noexcept { return ValTy; }
  ValueType &getValType() noexcept { return ValTy; }

private:
  std::string Label;
  ValueType ValTy;
};

class Case {
public:
  std::string_view getLabel() const noexcept { return Label; }
  std::string &getLabel() noexcept { return Label; }
  const std::optional<ValueType> getValType() const noexcept { return ValTy; }
  std::optional<ValueType> &getValType() noexcept { return ValTy; }

private:
  std::string Label;
  std::optional<ValueType> ValTy;
};

class Record {
public:
  Span<const LabelValType> getLabelTypes() const noexcept { return LabelTypes; }
  std::vector<LabelValType> &getLabelTypes() noexcept { return LabelTypes; }

private:
  std::vector<LabelValType> LabelTypes;
};

class VariantTy {
public:
  Span<const Case> getCases() const noexcept { return Cases; }
  std::vector<Case> &getCases() noexcept { return Cases; }

private:
  std::vector<Case> Cases;
};

class List {
public:
  const ValueType getValType() const noexcept { return ValTy; }
  ValueType &getValType() noexcept { return ValTy; }

private:
  ValueType ValTy;
};

// Tuple is the product of given non-empty type list
// e.g. given [A, B, C], the tuple is a product A x B x C
class Tuple {
public:
  Span<const ValueType> getTypes() const noexcept { return Types; }
  std::vector<ValueType> &getTypes() noexcept { return Types; }

private:
  std::vector<ValueType> Types;
};

class Flags {
public:
  Span<const std::string> getLabels() const noexcept { return Labels; }
  std::vector<std::string> &getLabels() noexcept { return Labels; }

private:
  std::vector<std::string> Labels;
};

class Enum {
public:
  Span<const std::string> getLabels() const noexcept { return Labels; }
  std::vector<std::string> &getLabels() noexcept { return Labels; }

private:
  std::vector<std::string> Labels;
};

class Option {
public:
  const ValueType getValType() const noexcept { return ValTy; }
  ValueType &getValType() noexcept { return ValTy; }

private:
  ValueType ValTy;
};

class Result {
public:
  const std::optional<ValueType> getValType() const noexcept { return ValTy; }
  std::optional<ValueType> &getValType() noexcept { return ValTy; }

  const std::optional<ValueType> getErrorType() const noexcept { return ErrTy; }
  std::optional<ValueType> &getErrorType() noexcept { return ErrTy; }

private:
  std::optional<ValueType> ValTy;
  std::optional<ValueType> ErrTy;
};

class Own {
public:
  TypeIndex getIndex() const noexcept { return Idx; }
  TypeIndex &getIndex() noexcept { return Idx; }

private:
  TypeIndex Idx;
};

class Borrow {
public:
  TypeIndex getIndex() const noexcept { return Idx; }
  TypeIndex &getIndex() noexcept { return Idx; }

private:
  TypeIndex Idx;
};

using DefValType = std::variant<PrimValType, Record, VariantTy, List, Tuple,
                                Flags, Enum, Option, Result, Own, Borrow>;

using ResultList = std::variant<ValueType, std::vector<LabelValType>>;
class FuncType {
public:
  Span<const LabelValType> getParamList() const noexcept { return ParamList; }
  std::vector<LabelValType> &getParamList() noexcept { return ParamList; }
  ResultList getResultList() const noexcept { return ResList; }
  ResultList &getResultList() noexcept { return ResList; }

private:
  std::vector<LabelValType> ParamList;
  ResultList ResList;
};

using ExternDesc = TypeIndex;
class ExportDecl {
public:
  std::string_view getExportName() const noexcept { return ExportName; }
  std::string &getExportName() noexcept { return ExportName; }
  ExternDesc getExternDesc() const noexcept { return Desc; }
  ExternDesc &getExternDesc() noexcept { return Desc; }

private:
  std::string ExportName;
  ExternDesc Desc;
};

class Type;
// TODO: will need core:type
using InstanceDecl = std::variant<Alias, std::shared_ptr<Type>, ExportDecl>;
class InstanceType {
public:
  Span<const InstanceDecl> getIdList() const noexcept { return IdList; }
  std::vector<InstanceDecl> &getIdList() noexcept { return IdList; }

private:
  std::vector<InstanceDecl> IdList;
};

// TODO: complete these class
class ComponentType {
  // componenttype ::= 0x41 cd*:vec(<componentdecl>)   => (component cd*)
};

using DefType = std::variant<DefValType, FuncType, ComponentType, InstanceType>;
class Type {
public:
  DefType getType() const noexcept { return T; }
  DefType &getType() noexcept { return T; }

private:
  DefType T;
};

} // namespace AST
} // namespace WasmEdge
