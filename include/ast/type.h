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

/// AST Limit node.
class Limit : public Base {
public:
  /// Limit type enumeration class.
  enum class LimitType : uint8_t { HasMin = 0x00, HasMinMax = 0x01 };

  Limit() noexcept = default;
  Limit(const uint32_t MinVal) noexcept
      : Type(LimitType::HasMin), Min(MinVal) {}
  Limit(const uint32_t MinVal, const uint32_t MaxVal) noexcept
      : Type(LimitType::HasMinMax), Min(MinVal), Max(MaxVal) {}

  /// Load binary from file manager.
  ///
  /// Inheritted and overrided from Base.
  /// Read limit type.
  /// Read Min and Max value of this node.
  ///
  /// \param Mgr the file manager reference.
  /// \param Conf the WasmEdge configuration reference.
  ///
  /// \returns void when success, ErrCode when failed.
  Expect<void> loadBinary(FileMgr &Mgr, const Configure &Conf) override;

  /// Getter of having max in limit.
  bool hasMax() const { return Type == LimitType::HasMinMax; }

  /// Getter of min.
  uint32_t getMin() const { return Min; }

  /// Getter of max.
  uint32_t getMax() const { return Max; }

  /// The node type should be ASTNodeAttr::Type_Limit.
  static inline constexpr const ASTNodeAttr NodeAttr = ASTNodeAttr::Type_Limit;

private:
  /// \name Data of Limit node.
  /// @{
  LimitType Type = LimitType::HasMin;
  uint32_t Min = 0;
  uint32_t Max = 0;
  /// @}
};

/// AST FunctionType node.
class FunctionType : public Base {
public:
  using Wrapper = void(void *ExecCtx, void *Function, const ValVariant *Args,
                       ValVariant *Rets);
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

  /// Getter of parameter types vector.
  Span<const ValType> getParamTypes() const { return ParamTypes; }

  /// Getter of return types vector.
  Span<const ValType> getReturnTypes() const { return ReturnTypes; }

  /// Getter of compiled symbol.
  const auto &getSymbol() const noexcept { return TypeSymbol; }
  /// Setter of compiled symbol.
  void setSymbol(Symbol<Wrapper> S) { TypeSymbol = std::move(S); }

  /// The node type should be ASTNodeAttr::Type_Function.
  static inline constexpr const ASTNodeAttr NodeAttr =
      ASTNodeAttr::Type_Function;

  friend bool operator==(const FunctionType &LHS,
                         const FunctionType &RHS) noexcept {
    return LHS.ParamTypes == RHS.ParamTypes &&
           LHS.ReturnTypes == RHS.ReturnTypes;
  }

  friend bool operator!=(const FunctionType &LHS,
                         const FunctionType &RHS) noexcept {
    return !(LHS == RHS);
  }

private:
  /// \name Data of FunctionType node.
  /// @{
  std::vector<ValType> ParamTypes;
  std::vector<ValType> ReturnTypes;
  /// @}

  Symbol<Wrapper> TypeSymbol;
};

/// AST MemoryType node.
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

  /// Getter of limit.
  const Limit &getLimit() const { return MemoryLim; }

  /// The node type should be ASTNodeAttr::Type_Memory.
  static inline constexpr const ASTNodeAttr NodeAttr = ASTNodeAttr::Type_Memory;

private:
  /// Data of MemoryType node.
  Limit MemoryLim;
};

/// AST TableType node.
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

  /// Getter of reference type.
  RefType getReferenceType() const { return Type; }

  /// Getter of limit.
  const Limit &getLimit() const { return TableLim; }

  /// The node type should be ASTNodeAttr::Type_Table.
  static inline constexpr const ASTNodeAttr NodeAttr = ASTNodeAttr::Type_Table;

private:
  /// \name Data of TableType node.
  /// @{
  RefType Type = RefType::FuncRef;
  Limit TableLim;
  /// @}
};

/// AST GlobalType node.
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
  ValType getValueType() const { return Type; }

  /// Getter of global mutation.
  ValMut getValueMutation() const { return Mut; }

  /// The node type should be ASTNodeAttr::Type_Global.
  static inline constexpr const ASTNodeAttr NodeAttr = ASTNodeAttr::Type_Global;

private:
  /// \name Data of GlobalType node.
  /// @{
  ValType Type = ValType::None;
  ValMut Mut = ValMut::Const;
  /// @}
};

} // namespace AST
} // namespace WasmEdge
