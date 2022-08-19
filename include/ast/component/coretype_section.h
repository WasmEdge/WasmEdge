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
  class I8;
  class I16;

  virtual Byte getMutability() const noexcept;
  virtual ~FieldType();
};
class FieldType::I8 : public FieldType {
public:
  I8(Byte M) : Mutability{M} {}

  Byte getMutability() const noexcept override { return Mutability; }

private:
  Byte Mutability;
};
class FieldType::I16 : public FieldType {
public:
  I16(Byte M) : Mutability{M} {}

  Byte getMutability() const noexcept override { return Mutability; }

private:
  Byte Mutability;
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
  std::vector<FieldType> TyList;
};
class CoreDefType::ArrayType : public CoreDefType {
public:
  ArrayType(FieldType &Ty) : Ty{Ty} {}

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
