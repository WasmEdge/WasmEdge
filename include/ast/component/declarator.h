// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2025 Second State INC

//===-- wasmedge/ast/component/declarator.h - Declarator class definitions ===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the Declarator related class.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/component/alias.h"
#include "ast/component/descriptor.h"
#include "ast/type.h"

#include <memory>
#include <string>
#include <string_view>
#include <variant>

namespace WasmEdge {
namespace AST {
namespace Component {

// Need the forward declaration.
class CoreDefType;
class DefType;

// core:importdecl ::= m:<core:name> n:<core:name> d:<core:importdesc>
//                   => (import m n d)

/// AST Component::CoreImportDecl node.
class CoreImportDecl {
public:
  std::string_view getModuleName() const noexcept { return ModName; }
  std::string &getModuleName() noexcept { return ModName; }
  std::string_view getName() const noexcept { return Name; }
  std::string &getName() noexcept { return Name; }
  CoreImportDesc getImportDesc() const noexcept { return Desc; }
  CoreImportDesc &getImportDesc() noexcept { return Desc; }

private:
  std::string ModName;
  std::string Name;
  CoreImportDesc Desc;
};

// core:exportdecl ::= n:<core:name> d:<core:importdesc> => (export n d)

/// AST Component::CoreExportDecl node.
class CoreExportDecl {
public:
  std::string_view getName() const noexcept { return Name; }
  std::string &getName() noexcept { return Name; }
  CoreImportDesc getImportDesc() const noexcept { return Desc; }
  CoreImportDesc &getImportDesc() noexcept { return Desc; }

private:
  std::string Name;
  CoreImportDesc Desc;
};

// core:moduledecl ::= 0x00 i:<core:importdecl> => i
//                   | 0x01 t:<core:type>       => t
//                   | 0x02 a:<core:alias>      => a
//                   | 0x03 e:<core:exportdecl> => e

/// AST Component::CoreModuleDecl node.
class CoreModuleDecl {
public:
  const CoreImportDecl &getImport() const noexcept {
    return *std::get_if<CoreImportDecl>(&Decl);
  }
  void setImport(CoreImportDecl &&Imp) noexcept {
    Decl.emplace<CoreImportDecl>(std::move(Imp));
  }

  const CoreDefType *getType() const noexcept {
    return std::get_if<std::unique_ptr<CoreDefType>>(&Decl)->get();
  }
  void setType(std::unique_ptr<CoreDefType> &&Imp) noexcept {
    Decl.emplace<std::unique_ptr<CoreDefType>>(std::move(Imp));
  }

  const CoreAlias &getAlias() const noexcept {
    return *std::get_if<CoreAlias>(&Decl);
  }
  void setAlias(CoreAlias &&A) noexcept {
    Decl.emplace<CoreAlias>(std::move(A));
  }

  const CoreExportDecl &getExport() const noexcept {
    return *std::get_if<CoreExportDecl>(&Decl);
  }
  void setExport(CoreExportDecl &&Exp) noexcept {
    Decl.emplace<CoreExportDecl>(std::move(Exp));
  }

private:
  std::variant<CoreImportDecl, std::unique_ptr<CoreDefType>, CoreAlias,
               CoreExportDecl>
      Decl;
};

// exportdecl  ::= en:<exportname'> ed:<externdesc> => (export en ed)
// exportname' ::= 0x00 len:<u32> en:<exportname>   => en (if len = |en|)
// importdecl  ::= in:<importname'> ed:<externdesc> => (import in ed)
// importname' ::= 0x00 len:<u32> in:<importname>   => in (if len = |in|)

/// Base class of Component::ImportDecl and Component::ExportDecl node.
class ExternDecl {
public:
  std::string_view getName() const noexcept { return Name; }
  std::string &getName() noexcept { return Name; }
  ExternDesc getExternDesc() const noexcept { return Desc; }
  ExternDesc &getExternDesc() noexcept { return Desc; }

private:
  std::string Name;
  ExternDesc Desc;
};

/// AST Component::ImportDecl node.
class ImportDecl : public ExternDecl {};

/// AST Component::ExportDecl node.
class ExportDecl : public ExternDecl {};

// instancedecl ::= 0x00 t:<core:type>   => t
//                | 0x01 t:<type>        => t
//                | 0x02 a:<alias>       => a
//                | 0x04 ed:<exportdecl> => ed

/// AST Component::InstanceDecl node.
class InstanceDecl {
public:
  const CoreDefType *getCoreType() const noexcept {
    return std::get_if<std::unique_ptr<CoreDefType>>(&Decl)->get();
  }
  void setCoreType(std::unique_ptr<CoreDefType> &&T) noexcept {
    Decl.emplace<std::unique_ptr<CoreDefType>>(std::move(T));
  }

  const DefType *getType() const noexcept {
    return std::get_if<std::unique_ptr<DefType>>(&Decl)->get();
  }
  void setType(std::unique_ptr<DefType> &&T) noexcept {
    Decl.emplace<std::unique_ptr<DefType>>(std::move(T));
  }

  const Alias &getAlias() const noexcept { return *std::get_if<Alias>(&Decl); }
  void setAlias(Alias &&A) noexcept { Decl.emplace<Alias>(std::move(A)); }

  const ExportDecl &getExport() const noexcept {
    return *std::get_if<ExportDecl>(&Decl);
  }
  void setExport(ExportDecl &&Exp) noexcept {
    Decl.emplace<ExportDecl>(std::move(Exp));
  }

  bool isCoreType() const noexcept {
    return std::holds_alternative<std::unique_ptr<CoreDefType>>(Decl);
  }
  bool isType() const noexcept {
    return std::holds_alternative<std::unique_ptr<DefType>>(Decl);
  }
  bool isAlias() const noexcept { return std::holds_alternative<Alias>(Decl); }
  bool isExportDecl() const noexcept {
    return std::holds_alternative<ExportDecl>(Decl);
  }

private:
  std::variant<std::unique_ptr<CoreDefType>, std::unique_ptr<DefType>, Alias,
               ExportDecl>
      Decl;
};

// componentdecl ::= 0x03 id:<importdecl> => id
//                 | id:<instancedecl>    => id

/// AST Component::ComponentDecl node.
class ComponentDecl {
public:
  const ImportDecl &getImport() const noexcept {
    return *std::get_if<ImportDecl>(&Decl);
  }
  void setImport(ImportDecl &&Imp) noexcept {
    Decl.emplace<ImportDecl>(std::move(Imp));
  }

  const InstanceDecl &getInstance() const noexcept {
    return *std::get_if<InstanceDecl>(&Decl);
  }
  void setInstance(InstanceDecl &&Inst) noexcept {
    Decl.emplace<InstanceDecl>(std::move(Inst));
  }

  bool isImportDecl() const noexcept {
    return std::holds_alternative<ImportDecl>(Decl);
  }
  bool isInstanceDecl() const noexcept {
    return std::holds_alternative<InstanceDecl>(Decl);
  }

private:
  std::variant<ImportDecl, InstanceDecl> Decl;
};

} // namespace Component
} // namespace AST
} // namespace WasmEdge
