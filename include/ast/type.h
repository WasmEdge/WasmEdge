// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//===-- wasmedge/ast/type.h - type class definitions ----------------------===//
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

#include "common/span.h"
#include "common/symbol.h"
#include "common/types.h"

#include <vector>

namespace WasmEdge {
namespace AST {

/// AST Limit node.
class Limit {
public:
  /// Limit type enumeration class.
  enum class LimitType : uint8_t {
    HasMin = 0x00,
    HasMinMax = 0x01,
    SharedNoMax = 0x02, // from threads proposal
    Shared = 0x03,      // from threads proposal
    I64HasMin = 0x04,
    I64HasMinMax = 0x05,
    I64SharedNoMax = 0x06, // from threads proposal
    I64Shared = 0x07,      // from threads proposal
  };

  /// Constructors.
  Limit() noexcept : Type(LimitType::HasMin), Min(0U), Max(0U) {}
  Limit(uint64_t MinVal) noexcept
      : Type(LimitType::HasMin), Min(MinVal), Max(MinVal) {}
  Limit(uint64_t MinVal, uint64_t MaxVal, bool Shared = false) noexcept
      : Min(MinVal), Max(MaxVal) {
    if (Shared) {
      Type = LimitType::Shared;
    } else {
      Type = LimitType::HasMinMax;
    }
  }

  /// Getter and setter of limit mode.
  bool hasMax() const noexcept {
    return Type == LimitType::HasMinMax || Type == LimitType::Shared ||
           Type == LimitType::I64HasMinMax || Type == LimitType::I64Shared;
  }
  bool isShared() const noexcept {
    return Type == LimitType::Shared || Type == LimitType::I64Shared;
  }
  bool is64() const noexcept {
    switch (Type) {
    case LimitType::I64HasMin:
    case LimitType::I64HasMinMax:
    case LimitType::I64SharedNoMax:
    case LimitType::I64Shared:
      return true;
    case LimitType::HasMin:
    case LimitType::HasMinMax:
    case LimitType::SharedNoMax:
    case LimitType::Shared:
    default:
      return false;
    }
  }
  void setType(LimitType TargetType) noexcept { Type = TargetType; }

  /// Getter and setter of min value.
  uint64_t getMin() const noexcept {
    // Hint: if one ensure it, is not 64-bit, do static_cast<uint32_t>(getMin())
    return Min;
  }
  void setMin(uint64_t Val) noexcept { Min = Val; }

  /// Getter and setter of max value.
  uint64_t getMax() const noexcept { return Max; }
  void setMax(uint64_t Val) noexcept { Max = Val; }

private:
  /// \name Data of Limit.
  /// @{
  LimitType Type;
  uint64_t Min;
  uint64_t Max;
  /// @}
};

/// AST FunctionType node.
class FunctionType {
public:
  /// Function type wrapper for symbols.
  using Wrapper = void(void *ExecCtx, void *Function, const ValVariant *Args,
                       ValVariant *Rets);

  /// Constructors.
  FunctionType() = default;
  FunctionType(Span<const ValType> P, Span<const ValType> R)
      : ParamTypes(P.begin(), P.end()), ReturnTypes(R.begin(), R.end()) {}
  FunctionType(Span<const ValType> P, Span<const ValType> R, Symbol<Wrapper> S)
      : ParamTypes(P.begin(), P.end()), ReturnTypes(R.begin(), R.end()),
        WrapSymbol(std::move(S)) {}

  /// `==` and `!=` operator overloadings.
  friend bool operator==(const FunctionType &LHS,
                         const FunctionType &RHS) noexcept {
    return LHS.ParamTypes == RHS.ParamTypes &&
           LHS.ReturnTypes == RHS.ReturnTypes;
  }

  friend bool operator!=(const FunctionType &LHS,
                         const FunctionType &RHS) noexcept {
    return !(LHS == RHS);
  }

  /// Getter of param types.
  const std::vector<ValType> &getParamTypes() const noexcept {
    return ParamTypes;
  }
  std::vector<ValType> &getParamTypes() noexcept { return ParamTypes; }

  /// Getter of return types.
  const std::vector<ValType> &getReturnTypes() const noexcept {
    return ReturnTypes;
  }
  std::vector<ValType> &getReturnTypes() noexcept { return ReturnTypes; }

  /// Getter and setter of symbol.
  const auto &getSymbol() const noexcept { return WrapSymbol; }
  void setSymbol(Symbol<Wrapper> S) noexcept { WrapSymbol = std::move(S); }

private:
  /// \name Data of FunctionType.
  /// @{
  std::vector<ValType> ParamTypes;
  std::vector<ValType> ReturnTypes;
  Symbol<Wrapper> WrapSymbol;
  /// @}
};

/// AST MemoryType node.
class MemoryType {
public:
  enum class IndexType { I32, I64 };

  /// Constructors.
  MemoryType() noexcept = default;
  MemoryType(uint64_t MinVal) noexcept : Lim(MinVal) {}
  MemoryType(uint64_t MinVal, uint64_t MaxVal, bool Shared = false) noexcept
      : Lim(MinVal, MaxVal, Shared) {}
  MemoryType(const Limit &L) noexcept : Lim(L) {}

  /// Getter of limit.
  const Limit &getLimit() const noexcept { return Lim; }
  Limit &getLimit() noexcept { return Lim; }
  IndexType getIdxType() const noexcept { return IdxType; }
  IndexType &getIdxType() noexcept { return IdxType; }
  uint64_t getPageLimit() const noexcept {
    // Maximum pages count
    switch (IdxType) {
    // 64 mode: 2^48
    case IndexType::I64:
      return UINT64_C(281474976710656);
    // 32 mode: 2^16
    case IndexType::I32:
    default:
      return UINT32_C(65536);
    }
  }

private:
  /// \name Data of MemoryType.
  /// @{
  IndexType IdxType;
  Limit Lim;
  /// @}
};

/// AST TableType node.
class TableType {
public:
  /// Constructors.
  TableType() noexcept : Type(TypeCode::FuncRef), Lim() {
    assuming(Type.isRefType());
  }
  TableType(const ValType &RType, uint32_t MinVal) noexcept
      : Type(RType), Lim(MinVal) {
    assuming(Type.isRefType());
  }
  TableType(const ValType &RType, uint32_t MinVal, uint32_t MaxVal) noexcept
      : Type(RType), Lim(MinVal, MaxVal) {
    assuming(Type.isRefType());
  }
  TableType(const ValType &RType, const Limit &L) noexcept
      : Type(RType), Lim(L) {
    assuming(Type.isRefType());
  }

  /// Getter of reference type.
  const ValType &getRefType() const noexcept { return Type; }
  void setRefType(const ValType &RType) noexcept {
    assuming(RType.isRefType());
    Type = RType;
  }

  /// Getter of limit.
  const Limit &getLimit() const noexcept { return Lim; }
  Limit &getLimit() noexcept { return Lim; }

private:
  /// \name Data of TableType.
  /// @{
  ValType Type;
  Limit Lim;
  /// @}
};

/// AST GlobalType node.
class GlobalType {
public:
  /// Constructors.
  GlobalType() noexcept : Type(TypeCode::I32), Mut(ValMut::Const) {}
  GlobalType(const ValType &VType, ValMut VMut) noexcept
      : Type(VType), Mut(VMut) {}

  /// Getter and setter of value type.
  const ValType &getValType() const noexcept { return Type; }
  void setValType(const ValType &VType) noexcept { Type = VType; }

  /// Getter and setter of value mutation.
  ValMut getValMut() const noexcept { return Mut; }
  void setValMut(ValMut VMut) noexcept { Mut = VMut; }

private:
  /// \name Data of GlobalType.
  /// @{
  ValType Type;
  ValMut Mut;
  /// @}
};

} // namespace AST
} // namespace WasmEdge
