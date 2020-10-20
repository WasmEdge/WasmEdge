// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/ast/base.h - AST node base class definition ------------------===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the AST node base class, which is the
/// base class for all of the AST nodes.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/astdef.h"
#include "common/errcode.h"
#include "loader/filemgr.h"

namespace SSVM {
namespace AST {

/// Base class of AST node.
class Base {
public:
  Base() = default;
  virtual ~Base() = default;

  /// Binary loading from file manager.
  virtual Expect<void> loadBinary(FileMgr &Mgr) {
    return Unexpect(ErrCode::InvalidGrammar);
  };

  /// AST node attribute.
  const ASTNodeAttr NodeAttr = ASTNodeAttr::Module;
};

} // namespace AST
} // namespace SSVM
