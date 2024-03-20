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
#include "ast/description.h"
#include "ast/expression.h"
#include "ast/type.h"

#include <optional>
#include <vector>

namespace WasmEdge {
namespace AST {

namespace Component {

enum class PrimValType : Byte {
  Bool = 0x7f,
  S8 = 0x7e,
  U8 = 0x7d,
  S16 = 0x7c,
  U16 = 0x7b,
  S32 = 0x7a,
  U32 = 0x79,
  S64 = 0x78,
  U64 = 0x77,
  Float32 = 0x76,
  Float64 = 0x75,
  Char = 0x74,
  String = 0x73
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
  FuncType() : ParamList{}, ResList{} {}
  FuncType(std::vector<LabelValType> P, ResultList R)
      : ParamList{P}, ResList{R} {}

  Span<const LabelValType> getParamList() const noexcept { return ParamList; }
  std::vector<LabelValType> &getParamList() noexcept { return ParamList; }
  ResultList getResultList() const noexcept { return ResList; }
  ResultList &getResultList() noexcept { return ResList; }

private:
  std::vector<LabelValType> ParamList;
  ResultList ResList;
};

enum class IndexKind : Byte {
  CoreType = 0x00,
  FuncType = 0x02,
  ComponentType = 0x04,
  InstanceType = 0x05,
};
class DescTypeIndex {
  TypeIndex TyIdx;
  IndexKind Kind;

public:
  TypeIndex &getIndex() noexcept { return TyIdx; }
  TypeIndex getIndex() const noexcept { return TyIdx; }
  IndexKind &getKind() noexcept { return Kind; }
  IndexKind getKind() const noexcept { return Kind; }
};
// use optional none as SubResource case
using TypeBound = std::optional<TypeIndex>;
using ExternDesc = std::variant<DescTypeIndex, TypeBound, ValueType>;
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

class CoreExportDecl {
  std::string Name;
  ImportDesc Desc;

public:
  std::string_view getName() const noexcept { return Name; }
  std::string &getName() noexcept { return Name; }
  ImportDesc getImportDesc() const noexcept { return Desc; }
  ImportDesc &getImportDesc() noexcept { return Desc; }
};
class CoreType;
using ModuleDecl =
    std::variant<ImportDesc, std::shared_ptr<CoreType>, Alias, CoreExportDecl>;
class ModuleType {
  std::vector<ModuleDecl> Decls;

public:
  Span<const ModuleDecl> getContent() const noexcept { return Decls; }
  std::vector<ModuleDecl> &getContent() noexcept { return Decls; }
};

// TODO: wait GC proposal
// st:<core:structtype>     => st   (GC proposal)
// at:<core:arraytype>      => at   (GC proposal)
using CoreDefType = std::variant<FunctionType, ModuleType>;
class CoreType {
public:
  CoreDefType getType() const noexcept { return T; }
  CoreDefType &getType() noexcept { return T; }

private:
  CoreDefType T;
};

class Type;
using InstanceDecl =
    std::variant<CoreType, Alias, std::shared_ptr<Type>, ExportDecl>;
class InstanceType {
  std::vector<InstanceDecl> IdList;

public:
  Span<const InstanceDecl> getContent() const noexcept { return IdList; }
  std::vector<InstanceDecl> &getContent() noexcept { return IdList; }
};

class ImportDecl {
  std::string ImportName;
  ExternDesc Desc;

public:
  std::string_view getImportName() const noexcept { return ImportName; }
  std::string &getImportName() noexcept { return ImportName; }
  ExternDesc getExternDesc() const noexcept { return Desc; }
  ExternDesc &getExternDesc() noexcept { return Desc; }
};
using ComponentDecl = std::variant<ImportDecl, InstanceDecl>;
class ComponentType {
  std::vector<ComponentDecl> CdList;

public:
  Span<const ComponentDecl> getContent() const noexcept { return CdList; }
  std::vector<ComponentDecl> &getContent() noexcept { return CdList; }
};

using DefType = std::variant<DefValType, FuncType, ComponentType, InstanceType>;
class Type {
public:
  Type(DefType V) : T{V} {}
  DefType getType() const noexcept { return T; }
  DefType &getType() noexcept { return T; }

private:
  DefType T;
};

} // namespace Component
} // namespace AST
} // namespace WasmEdge
