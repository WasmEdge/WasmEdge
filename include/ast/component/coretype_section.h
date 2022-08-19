// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//===---------- wasmedge/ast/component/coretype_section.h -------------===//
//
// CoreType Section class definitions
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

#include "ast/component/module_decl.h"
#include "ast/type.h"

#include <vector>

namespace WasmEdge {
namespace AST {

class FieldType {
public:
  enum Kind { I8, I16 };

  // default constructor for initialize
  FieldType() {}

  // default used constructor
  FieldType(Byte M, Kind K) : Mutability{M}, K{K} {}

  Byte getMutability() const noexcept { return Mutability; }
  Kind getKind() const noexcept { return K; }

private:
  Byte Mutability;
  Kind K;
};

class CoreType : public ModuleDecl {};
class CoreDefType : public CoreType {
public:
  class FuncType;
  class StructType;
  class ArrayType;
  class ModuleType;
};

class CoreDefType::FuncType : public CoreDefType, public FunctionType {};
class CoreDefType::StructType : public CoreDefType {
public:
  Span<const FieldType> getFieldTypes() const noexcept { return TyList; }
  std::vector<FieldType> &getFieldTypes() noexcept { return TyList; }

private:
  std::vector<FieldType> TyList{};
};
class CoreDefType::ArrayType : public CoreDefType {
public:
  const FieldType &getField() const noexcept { return Ty; }
  FieldType &getField() noexcept { return Ty; }

private:
  FieldType Ty;
};
class CoreDefType::ModuleType : public CoreDefType {
public:
  Span<const ModuleDecl> getModuleDecls() const noexcept { return Decls; }
  std::vector<ModuleDecl> &getModuleDecls() noexcept { return Decls; }

private:
  std::vector<ModuleDecl> Decls;
};

} // namespace AST
} // namespace WasmEdge
