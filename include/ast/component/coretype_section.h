// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//=== CoreType Section class definitions
//
// Part of the WasmEdge Project.
//
//===------------------------------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the Core Type node classes.
///
//===------------------------------------------------------------------------------------------===//
#pragma once

#include "ast/component/corealias_section.h"
#include "ast/description.h"
#include "ast/type.h"

#include <variant>
#include <vector>

namespace WasmEdge {
namespace AST {

enum class FieldStorageType : Byte { I8 = 0x06, I16 = 0x07 };

class FieldType {
public:
  // default constructor for initialize
  FieldType() = default;

  // default used constructor
  FieldType(FieldStorageType T, Byte M) : Ty{T}, Mutability{M} {}

  Byte getMutability() const noexcept { return Mutability; }
  FieldStorageType getKind() const noexcept { return Ty; }

private:
  FieldStorageType Ty;
  Byte Mutability;
};

class ModuleDecl;

namespace CoreDefType {

class FuncType : public ::WasmEdge::AST::FunctionType {};

class StructType {
public:
  Span<const FieldType> getFieldTypes() const noexcept { return TyList; }
  std::vector<FieldType> &getFieldTypes() noexcept { return TyList; }

private:
  std::vector<FieldType> TyList{};
};

class ArrayType {
public:
  const FieldType &getField() const noexcept { return Ty; }
  FieldType &getField() noexcept { return Ty; }

private:
  FieldType Ty;
};

class ModuleType {
public:
  Span<const ModuleDecl> getModuleDecls() const noexcept { return Decls; }
  std::vector<ModuleDecl> &getModuleDecls() noexcept { return Decls; }

private:
  std::vector<ModuleDecl> Decls;
};

using T = std::variant<FuncType, StructType, ArrayType, ModuleType>;

} // namespace CoreDefType

using CoreType = CoreDefType::T;
class ModuleDecl {
  using T = std::variant<CoreType, CoreAlias, ExportDesc, ImportDesc>;
  T _T;

public:
  T &getContent() noexcept { return _T; }
};

} // namespace AST
} // namespace WasmEdge
