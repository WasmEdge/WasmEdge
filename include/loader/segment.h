//===-- ssvm/loader/segment.h - segment classes definition ------*- C++ -*-===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the Segment node class and subsegment
/// node classes: ElementSegment, CodeSegment, and DataSegment.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "expression.h"

namespace AST {

/// Segment's base class.
class Segment : public Base {
public:
  /// Binary loading from file manager. Inheritted from Base.
  virtual bool loadBinary(FileMgr &Mgr) { return false; };

protected:
  /// Load binary from file manager.
  ///
  /// Create the expression node and read data.
  ///
  /// \param Mgr the file manager reference.
  ///
  /// \returns true on success.
  bool loadExpression(FileMgr &Mgr);

  /// Expression node in this segment.
  std::unique_ptr<Expression> Expr;
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
  /// \returns true on success.
  virtual bool loadBinary(FileMgr &Mgr);

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
  /// \returns true on success.
  virtual bool loadBinary(FileMgr &Mgr);

protected:
  /// The node type should be Attr::Seg_Code.
  Attr NodeAttr = Attr::Seg_Code;

private:
  /// \name Data of CodeSegment node.
  /// @{
  unsigned int SegSize = 0;
  std::vector<std::pair<unsigned int, Base::ValType>> Locals;
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
  /// \returns true on success.
  virtual bool loadBinary(FileMgr &Mgr);

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