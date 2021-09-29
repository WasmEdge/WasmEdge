// SPDX-License-Identifier: Apache-2.0
//===-- wasmedge/ast/segment.h - segment classes definition ---------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the Segment node class and subsegment
/// node classes: GlobalSegment, ElementSegment, CodeSegment, and DataSegment.
///
//===----------------------------------------------------------------------===//
#pragma once

#include <memory>

#include "common/log.h"
#include "common/symbol.h"

#include "base.h"
#include "expression.h"
#include "instruction.h"
#include "type.h"

namespace WasmEdge {
namespace AST {

/// Segment's base class.
class Segment : public Base {
public:
  /// Binary loading from file manager. Inheritted from Base.
  Expect<void> loadBinary(FileMgr &Mgr, const Configure &Conf) override = 0;

  /// Getter of locals vector.
  InstrView getInstrs() const { return Expr.getInstrs(); }

protected:
  /// Load binary from file manager.
  ///
  /// Create the expression node and read data.
  ///
  /// \param Mgr the file manager reference.
  /// \param Conf the WasmEdge configuration reference.
  ///
  /// \returns void when success, ErrCode when failed.
  Expect<void> loadExpression(FileMgr &Mgr, const Configure &Conf);

  /// Expression node in this segment.
  Expression Expr;
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
  /// \param Conf the WasmEdge configuration reference.
  ///
  /// \returns void when success, ErrCode when failed.
  Expect<void> loadBinary(FileMgr &Mgr, const Configure &Conf) override;

  /// Getter of locals vector.
  const GlobalType &getGlobalType() const { return Global; }

  /// The node type should be ASTNodeAttr::Seg_Global.
  static inline constexpr const ASTNodeAttr NodeAttr = ASTNodeAttr::Seg_Global;

private:
  /// \name Data of GlobalSegment node.
  /// @{
  GlobalType Global;
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
  /// \param Conf the WasmEdge configuration reference.
  ///
  /// \returns void when success, ErrCode when failed.
  Expect<void> loadBinary(FileMgr &Mgr, const Configure &Conf) override;

  /// Element mode enumeration.
  enum class ElemMode : uint8_t { Passive, Active, Declarative };

  /// Getter of element mode.
  ElemMode getMode() const { return Mode; }

  /// Getter of reference type.
  RefType getRefType() const { return Type; }

  /// Getter of table index.
  uint32_t getIdx() const { return TableIdx; }

  /// Getter of initialization expressions.
  Span<const Expression> getInitExprs() const { return InitExprs; }

  /// The node type should be ASTNodeAttr::Seg_Element.
  static inline constexpr const ASTNodeAttr NodeAttr = ASTNodeAttr::Seg_Element;

private:
  /// \name Data of ElementSegment node.
  /// @{
  ElemMode Mode = ElemMode::Active;
  RefType Type = RefType::FuncRef;
  uint32_t TableIdx = 0;
  std::vector<Expression> InitExprs;
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
  /// \param Conf the WasmEdge configuration reference.
  ///
  /// \returns void when success, ErrCode when failed.
  Expect<void> loadBinary(FileMgr &Mgr, const Configure &Conf) override;

  /// Getter of locals vector.
  Span<const std::pair<uint32_t, ValType>> getLocals() const { return Locals; }

  /// Getter of compiled symbol.
  const auto &getSymbol() const noexcept { return FuncSymbol; }
  /// Setter of compiled symbol.
  void setSymbol(Symbol<void> S) noexcept { FuncSymbol = std::move(S); }

  /// The node type should be ASTNodeAttr::Seg_Code.
  static inline constexpr const ASTNodeAttr NodeAttr = ASTNodeAttr::Seg_Code;

private:
  /// \name Data of CodeSegment node.
  /// @{
  uint32_t SegSize = 0;
  std::vector<std::pair<uint32_t, ValType>> Locals;
  /// @}

  Symbol<void> FuncSymbol;
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
  /// \param Conf the WasmEdge configuration reference.
  ///
  /// \returns void when success, ErrCode when failed.
  Expect<void> loadBinary(FileMgr &Mgr, const Configure &Conf) override;

  /// Data mode enumeration.
  enum class DataMode : uint8_t { Passive, Active };

  /// Getter of data mode.
  DataMode getMode() const { return Mode; }

  /// Getter of memory index.
  uint32_t getIdx() const { return MemoryIdx; }

  /// Getter of data.
  Span<const Byte> getData() const { return Data; }

  /// The node type should be ASTNodeAttr::Seg_Data.
  static inline constexpr const ASTNodeAttr NodeAttr = ASTNodeAttr::Seg_Data;

private:
  /// \name Data of DataSegment node.
  /// @{
  DataMode Mode = DataMode::Active;
  uint32_t MemoryIdx = 0;
  std::vector<Byte> Data;
  /// @}
};

} // namespace AST
} // namespace WasmEdge
