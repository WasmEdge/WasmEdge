// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/common/ast/segment.h - segment classes definition ------------===//
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
  Expect<void> loadBinary(FileMgr &Mgr) override {
    return Unexpect(ErrCode::InvalidGrammar);
  };

  /// Getter of locals vector.
  InstrVec &getInstrs() const { return Expr->getInstrs(); }

protected:
  /// Load binary from file manager.
  ///
  /// Create the expression node and read data.
  ///
  /// \param Mgr the file manager reference.
  ///
  /// \returns void when success, ErrMsg when failed.
  Expect<void> loadExpression(FileMgr &Mgr);

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
  /// \returns void when success, ErrMsg when failed.
  Expect<void> loadBinary(FileMgr &Mgr) override;

  /// Getter of locals vector.
  const GlobalType *getGlobalType() const { return Global.get(); }

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
  /// \returns void when success, ErrMsg when failed.
  Expect<void> loadBinary(FileMgr &Mgr) override;

  /// Getter of table index.
  uint32_t getIdx() const { return TableIdx; }

  /// Getter of function indices.
  Span<const uint32_t> getFuncIdxes() const { return FuncIdxes; }

protected:
  /// The node type should be Attr::Seg_Element.
  Attr NodeAttr = Attr::Seg_Element;

private:
  /// \name Data of ElementSegment node.
  /// @{
  uint32_t TableIdx = 0;
  std::vector<uint32_t> FuncIdxes;
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
  /// \returns void when success, ErrMsg when failed.
  Expect<void> loadBinary(FileMgr &Mgr) override;

  /// Getter of locals vector.
  Span<const std::pair<uint32_t, ValType>> getLocals() const { return Locals; }

protected:
  /// The node type should be Attr::Seg_Code.
  Attr NodeAttr = Attr::Seg_Code;

private:
  /// \name Data of CodeSegment node.
  /// @{
  uint32_t SegSize = 0;
  std::vector<std::pair<uint32_t, ValType>> Locals;
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
  /// \returns void when success, ErrMsg when failed.
  Expect<void> loadBinary(FileMgr &Mgr) override;

  /// Getter of memory index.
  uint32_t getIdx() const { return MemoryIdx; }

  /// Getter of data.
  Span<const Byte> getData() const { return Data; }

protected:
  /// The node type should be Attr::Seg_Data.
  Attr NodeAttr = Attr::Seg_Data;

private:
  /// \name Data of DataSegment node.
  /// @{
  uint32_t MemoryIdx = 0;
  std::vector<Byte> Data;
  /// @}
};

} // namespace AST
} // namespace SSVM
