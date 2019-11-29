// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/ast/segment.h - segment classes definition ---------*- C++ -*-===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the Segment node class and subsegment
/// node classes: GlobalSegment, ElementSegment, CodeSegment, and DataSegment.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "base.h"
#include "expression.h"
#include "instruction.h"
#include "type.h"

#include <memory>

namespace SSVM {
namespace AST {

/// Segment's base class.
class Segment : public Base {
public:
  /// Binary loading from file manager. Inheritted from Base.
  virtual Loader::ErrCode loadBinary(FileMgr &Mgr) {
    return Loader::ErrCode::InvalidGrammar;
  };

  /// Getter of locals vector.
  InstrVec &getInstrs() { return Expr->getInstrs(); }

protected:
  /// Load binary from file manager.
  ///
  /// Create the expression node and read data.
  ///
  /// \param Mgr the file manager reference.
  ///
  /// \returns ErrCode.
  Loader::ErrCode loadExpression(FileMgr &Mgr);

  /// Expression node in this segment.
  std::unique_ptr<Expression> Expr;
};

/// AST GlobalSegment node.
class GlobalSegment : public Segment {
public:
  /// Load binary from file manager.
  ///
  /// Inheritted and overrided from Base.
  /// Read the global type and expression.
  ///
  /// \param Mgr the file manager reference.
  ///
  /// \returns ErrCode.
  virtual Loader::ErrCode loadBinary(FileMgr &Mgr);

  /// Getter of locals vector.
  const GlobalType *getGlobalType() { return Global.get(); }

protected:
  /// The node type should be Attr::Seg_Global.
  Attr NodeAttr = Attr::Seg_Global;

private:
  /// \name Data of GlobalSegment node.
  /// @{
  std::unique_ptr<GlobalType> Global;
  /// @}
};

/// AST ElementSegment node.
class ElementSegment : public Segment {
public:
  /// Load binary from file manager.
  ///
  /// Inheritted and overrided from Base.
  /// Read the table index, expression, and function indices.
  ///
  /// \param Mgr the file manager reference.
  ///
  /// \returns ErrCode.
  virtual Loader::ErrCode loadBinary(FileMgr &Mgr);

  /// Getter of table index.
  const unsigned int getIdx() { return TableIdx; }

  /// Getter of function indices.
  std::vector<unsigned int> &getFuncIdxes() { return FuncIdxes; }

protected:
  /// The node type should be Attr::Seg_Element.
  Attr NodeAttr = Attr::Seg_Element;

private:
  /// \name Data of ElementSegment node.
  /// @{
  unsigned int TableIdx = 0;
  std::vector<unsigned int> FuncIdxes;
  /// @}
};

/// AST CodeSegment node.
class CodeSegment : public Segment {
public:
  /// Load binary from file manager.
  ///
  /// Inheritted and overrided from Base.
  /// Read the segment size, locals, and function body.
  ///
  /// \param Mgr the file manager reference.
  ///
  /// \returns ErrCode.
  virtual Loader::ErrCode loadBinary(FileMgr &Mgr);

  /// Getter of locals vector.
  const std::vector<std::pair<unsigned int, ValType>> &getLocals() {
    return Locals;
  }

protected:
  /// The node type should be Attr::Seg_Code.
  Attr NodeAttr = Attr::Seg_Code;

private:
  /// \name Data of CodeSegment node.
  /// @{
  unsigned int SegSize = 0;
  std::vector<std::pair<unsigned int, ValType>> Locals;
  /// @}
};

/// AST DataSegment node.
class DataSegment : public Segment {
public:
  /// Load binary from file manager.
  ///
  /// Inheritted and overrided from Base.
  /// Read the memory index, offset expression, and initialization data.
  ///
  /// \param Mgr the file manager reference.
  ///
  /// \returns ErrCode.
  virtual Loader::ErrCode loadBinary(FileMgr &Mgr);

  /// Getter of memory index.
  const unsigned int getIdx() { return MemoryIdx; }

  /// Getter of data.
  std::vector<unsigned char> &getData() { return Data; }

protected:
  /// The node type should be Attr::Seg_Data.
  Attr NodeAttr = Attr::Seg_Data;

private:
  /// \name Data of DataSegment node.
  /// @{
  unsigned int MemoryIdx = 0;
  std::vector<unsigned char> Data;
  /// @}
};

} // namespace AST
} // namespace SSVM
