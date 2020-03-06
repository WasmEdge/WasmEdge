// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/common/ast/base.h - AST node base class definition -----------===//
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

#include "common/types.h"
#include "common/value.h"
#include "common/errcode.h"
#include "loader/filemgr.h"

namespace SSVM {
namespace AST {

/// Base class of AST node.
class Base {
public:
  /// AST node attributes enumeration class.
  enum class Attr {
    Module,
    Sec_Custom,
    Sec_Type,
    Sec_Import,
    Sec_Function,
    Sec_Table,
    Sec_Memory,
    Sec_Global,
    Sec_Export,
    Sec_Start,
    Sec_Element,
    Sec_Code,
    Sec_Data,
    Desc_Import,
    Desc_Export,
    Seg_Global,
    Seg_Element,
    Seg_Code,
    Seg_Data,
    Type_Function,
    Type_Limit,
    Type_Memory,
    Type_Table,
    Type_Global,
    Expression
  };

  Base() = default;
  virtual ~Base() = default;

  /// Binary loading from file manager.
  virtual Expect<void> loadBinary(FileMgr &Mgr) {
    return Unexpect(ErrCode::InvalidGrammar);
  };

protected:
  /// AST node attribute.
  Attr NodeAttr;
};

} // namespace AST
} // namespace SSVM
