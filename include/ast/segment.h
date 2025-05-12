// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/ast/segment.h - segment class definitions ----------------===//
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

#include "ast/expression.h"
#include "ast/type.h"

#include <vector>

namespace WasmEdge {
namespace AST {

/// Segment's base class.
class Segment {
public:
  /// Getter of expression.
  const Expression &getExpr() const noexcept { return Expr; }
  Expression &getExpr() noexcept { return Expr; }

protected:
  /// Expression node in this segment.
  Expression Expr;
};

/// AST GlobalSegment node.
class GlobalSegment : public Segment {
public:
  /// Getter of global type.
  const GlobalType &getGlobalType() const noexcept { return Global; }
  GlobalType &getGlobalType() noexcept { return Global; }

private:
  /// \name Data of GlobalSegment node.
  /// @{
  GlobalType Global;
  /// @}
};

/// AST ElementSegment node.
class ElementSegment : public Segment {
public:
  /// Element mode enumeration.
  enum class ElemMode : uint8_t { Passive, Active, Declarative };

  /// Getter and setter of element mode.
  ElemMode getMode() const noexcept { return Mode; }
  void setMode(ElemMode EMode) noexcept { Mode = EMode; }

  /// Getter of reference type.
  const ValType &getRefType() const noexcept { return Type; }
  void setRefType(const ValType &RType) noexcept {
    assuming(RType.isRefType());
    Type = RType;
  }

  /// Getter of table index.
  uint32_t getIdx() const noexcept { return TableIdx; }
  void setIdx(uint32_t Idx) noexcept { TableIdx = Idx; }

  /// Getter of initialization expressions.
  Span<const Expression> getInitExprs() const noexcept { return InitExprs; }
  std::vector<Expression> &getInitExprs() noexcept { return InitExprs; }

private:
  /// \name Data of ElementSegment node.
  /// @{
  ElemMode Mode = ElemMode::Active;
  ValType Type = TypeCode::FuncRef;
  uint32_t TableIdx = 0;
  std::vector<Expression> InitExprs;
  /// @}
};

/// AST TableSegment node.
class TableSegment : public Segment {
public:
  /// Getter of table type.
  const TableType &getTableType() const noexcept { return TType; }
  TableType &getTableType() noexcept { return TType; }

private:
  /// \name Data of TableSegment node.
  /// @{
  TableType TType;
  /// @}
};

/// AST CodeSegment node.
class CodeSegment : public Segment {
public:
  /// Getter and setter of segment size.
  uint32_t getSegSize() const noexcept { return SegSize; }
  void setSegSize(uint32_t Size) noexcept { SegSize = Size; }

  /// Getter of locals vector.
  Span<const std::pair<uint32_t, ValType>> getLocals() const noexcept {
    return Locals;
  }
  std::vector<std::pair<uint32_t, ValType>> &getLocals() noexcept {
    return Locals;
  }

  /// Getter and setter of compiled symbol.
  const auto &getSymbol() const noexcept { return FuncSymbol; }
  void setSymbol(Symbol<void> S) noexcept { FuncSymbol = std::move(S); }

private:
  /// \name Data of CodeSegment node.
  /// @{
  uint32_t SegSize = 0;
  std::vector<std::pair<uint32_t, ValType>> Locals;
  Symbol<void> FuncSymbol;
  /// @}
};

/// AST DataSegment node.
class DataSegment : public Segment {
public:
  /// Data mode enumeration.
  enum class DataMode : uint8_t { Passive, Active };

  /// Getter and setter of data mode.
  DataMode getMode() const noexcept { return Mode; }
  void setMode(DataMode DMode) noexcept { Mode = DMode; }

  /// Getter and setter of memory index.
  uint32_t getIdx() const noexcept { return MemoryIdx; }
  void setIdx(uint32_t Idx) noexcept { MemoryIdx = Idx; }

  /// Getter of data.
  Span<const Byte> getData() const noexcept { return Data; }
  std::vector<Byte> &getData() noexcept { return Data; }

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
