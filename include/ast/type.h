// SPDX-License-Identifier: Apache-2.0
//===-- wasmedge/ast/type.h - type classes definition ---------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the type classes: Limit, FunctionType,
/// MemoryType, TableType, and GlobalType.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/base.h"

#include <vector>

namespace WasmEdge {
namespace AST {

/// AST FunctionType node. TODO: Simplify this.
class FunctionType : public Base {
public:
  /// Load binary from file manager.
  ///
  /// Inheritted and overrided from Base.
  /// Read value types of parameter list and return list.
  ///
  /// \param Mgr the file manager reference.
  /// \param Conf the WasmEdge configuration reference.
  ///
  /// \returns void when success, ErrCode when failed.
  Expect<void> loadBinary(FileMgr &Mgr, const Configure &Conf) override;

  /// Getter of function type.
  const WasmEdge::FunctionType &getInner() const noexcept { return Inner; }
  WasmEdge::FunctionType &getInner() noexcept { return Inner; }

  /// The node type should be ASTNodeAttr::Type_Function.
  static inline constexpr const ASTNodeAttr NodeAttr =
      ASTNodeAttr::Type_Function;

  friend bool operator==(const FunctionType &LHS,
                         const FunctionType &RHS) noexcept {
    return LHS.Inner == RHS.Inner;
  }

  friend bool operator!=(const FunctionType &LHS,
                         const FunctionType &RHS) noexcept {
    return !(LHS == RHS);
  }

private:
  WasmEdge::FunctionType Inner;
};

/// AST MemoryType node. TODO: Simplify this.
class MemoryType : public Base {
public:
  /// Load binary from file manager.
  ///
  /// Inheritted and overrided from Base.
  /// Read the Limit data of this node.
  ///
  /// \param Mgr the file manager reference.
  /// \param Conf the WasmEdge configuration reference.
  ///
  /// \returns void when success, ErrCode when failed.
  Expect<void> loadBinary(FileMgr &Mgr, const Configure &Conf) override;

  /// Getter of memory type.
  const WasmEdge::MemoryType &getInner() const noexcept { return Inner; }

  /// The node type should be ASTNodeAttr::Type_Memory.
  static inline constexpr const ASTNodeAttr NodeAttr = ASTNodeAttr::Type_Memory;

private:
  WasmEdge::MemoryType Inner;
};

/// AST TableType node. TODO: Simplify this.
class TableType : public Base {
public:
  /// Load binary from file manager.
  ///
  /// Inheritted and overrided from Base.
  /// Read reference type and Limit data.
  ///
  /// \param Mgr the file manager reference.
  /// \param Conf the WasmEdge configuration reference.
  ///
  /// \returns void when success, ErrCode when failed.
  Expect<void> loadBinary(FileMgr &Mgr, const Configure &Conf) override;

  /// Getter of memory type.
  const WasmEdge::TableType &getInner() const noexcept { return Inner; }

  /// The node type should be ASTNodeAttr::Type_Table.
  static inline constexpr const ASTNodeAttr NodeAttr = ASTNodeAttr::Type_Table;

private:
  WasmEdge::TableType Inner;
};

/// AST GlobalType node. TODO: Simplify this.
class GlobalType : public Base {
public:
  /// Load binary from file manager.
  ///
  /// Inheritted and overrided from Base.
  /// Read value type and mutation.
  ///
  /// \param Mgr the file manager reference.
  /// \param Conf the WasmEdge configuration reference.
  ///
  /// \returns void when success, ErrCode when failed.
  Expect<void> loadBinary(FileMgr &Mgr, const Configure &Conf) override;

  /// Getter of global type.
  const WasmEdge::GlobalType &getInner() const noexcept { return Inner; }

  /// The node type should be ASTNodeAttr::Type_Global.
  static inline constexpr const ASTNodeAttr NodeAttr = ASTNodeAttr::Type_Global;

private:
  WasmEdge::GlobalType Inner;
};

} // namespace AST
} // namespace WasmEdge
