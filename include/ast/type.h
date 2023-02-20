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
#include "expression.h"

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
    SharedNoMax = 0x02,
    Shared = 0x03
  };

  /// Constructors.
  Limit() noexcept : Type(LimitType::HasMin), Min(0U), Max(0U) {}
  Limit(uint32_t MinVal) noexcept
      : Type(LimitType::HasMin), Min(MinVal), Max(MinVal) {}
  Limit(uint32_t MinVal, uint32_t MaxVal, bool Shared = false) noexcept
      : Min(MinVal), Max(MaxVal) {
    if (Shared) {
      Type = LimitType::Shared;
    } else {
      Type = LimitType::HasMinMax;
    }
  }
  Limit(const Limit &L) noexcept : Type(L.Type), Min(L.Min), Max(L.Max) {}

  /// Getter and setter of limit mode.
  bool hasMax() const noexcept {
    return Type == LimitType::HasMinMax || Type == LimitType::Shared;
  }
  bool isShared() const noexcept { return Type == LimitType::Shared; }
  void setType(LimitType TargetType) noexcept { Type = TargetType; }

  /// Getter and setter of min value.
  uint32_t getMin() const noexcept { return Min; }
  void setMin(uint32_t Val) noexcept { Min = Val; }

  /// Getter and setter of max value.
  uint32_t getMax() const noexcept { return Max; }
  void setMax(uint32_t Val) noexcept { Max = Val; }

private:
  /// \name Data of Limit.
  /// @{
  LimitType Type;
  uint32_t Min;
  uint32_t Max;
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
  FunctionType(Span<const FullValType> P, Span<const FullValType> R)
      : ParamTypes(P.begin(), P.end()), ReturnTypes(R.begin(), R.end()) {}
  FunctionType(Span<const FullValType> P, Span<const FullValType> R,
               Symbol<Wrapper> S)
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
  const std::vector<FullValType> &getParamTypes() const noexcept {
    return ParamTypes;
  }
  std::vector<FullValType> &getParamTypes() noexcept { return ParamTypes; }

  /// Getter of return types.
  const std::vector<FullValType> &getReturnTypes() const noexcept {
    return ReturnTypes;
  }
  std::vector<FullValType> &getReturnTypes() noexcept { return ReturnTypes; }

  /// Getter and setter of symbol.
  const auto &getSymbol() const noexcept { return WrapSymbol; }
  void setSymbol(Symbol<Wrapper> S) noexcept { WrapSymbol = std::move(S); }

private:
  /// \name Data of FunctionType.
  /// @{
  std::vector<FullValType> ParamTypes;
  std::vector<FullValType> ReturnTypes;
  Symbol<Wrapper> WrapSymbol;
  /// @}
};

class StorageType {
public:
  StorageType() noexcept = default;
  StorageType(FullValType VType) noexcept : Type(VType) {
    const auto GetSize = [](FullValType VType) {
      switch (VType.getTypeCode()) {
      case ValTypeCode::I32:
        return sizeof(int32_t);
      case ValTypeCode::I64:
        return sizeof(int64_t);
      case ValTypeCode::F32:
        return sizeof(float);
      case ValTypeCode::F64:
        return sizeof(double);
      case ValTypeCode::V128:
        return sizeof(int128_t);
      case ValTypeCode::Ref:
        return sizeof(RefVariant);
      case ValTypeCode::RefNull:
        return sizeof(RefVariant);
      }
    };
    Size = GetSize(VType);
  }
  StorageType(PackedType PType) noexcept : Type(PType) {
    switch (asPackedType()) {
    case PackedType::I16:
      Size = sizeof(int16_t);
      return;
    case PackedType::I8:
      Size = sizeof(int8_t);
      return;
    }
  }

  bool isPackedType() const { return std::holds_alternative<PackedType>(Type); }

  bool isValType() const { return std::holds_alternative<FullValType>(Type); }

  PackedType asPackedType() const { return std::get<PackedType>(Type); }

  FullValType asValType() const { return std::get<FullValType>(Type); }

  uint32_t size() const {
    assuming(Size != 0);
    return Size;
  }

  FullValType unpackedType() const {
    if (isValType()) {
      return asValType();
    } else {
      return ValType::I32;
    }
  }

private:
  std::variant<FullValType, PackedType> Type;
  uint32_t Size;
};

class FieldType {
public:
  FieldType() noexcept = default;
  FieldType(ValMut Mutability, StorageType Type) noexcept
      : Mutability(Mutability), Type(Type) {}
  ValMut getMutability() const noexcept { return Mutability; }

  StorageType getStorageType() const noexcept { return Type; }

private:
  ValMut Mutability;
  StorageType Type;
};

class ArrayType {
public:
  ArrayType() noexcept = default;
  ArrayType(FieldType Type) noexcept : Type(Type) {}
  FieldType getFieldType() const noexcept { return Type; }
  bool isDefaultable() const noexcept {
    return Type.getStorageType().unpackedType().isDefaultable();
  }

private:
  FieldType Type;
};

class StructType {
public:
  StructType(std::vector<FieldType> &&TypeList) noexcept : Content(TypeList) {
    Size = 0;
    for (auto Type : TypeList) {
      Offset.push_back(Size);
      Size += Type.getStorageType().size();
    }
  }

  Span<const FieldType> getContent() const noexcept { return Content; }
  std::vector<FieldType> &getContent() noexcept { return Content; }

  Span<const uint32_t> getOffset() const noexcept { return Offset; }

  uint32_t size() const { return Size; }

private:
  std::vector<FieldType> Content;
  std::vector<uint32_t> Offset;
  uint32_t Size;
};

class StructureType {
public:
  StructureType() noexcept = default;
  StructureType(FunctionType &&Type) : VariantType(Type) {}
  StructureType(StructType &&Type) : VariantType(Type) {}
  StructureType(ArrayType &&Type) : VariantType(Type) {}
  template <typename T> const T &asType() const {
    return std::get<T>(VariantType);
  }
  template <typename T> T &asType() { return std::get<T>(VariantType); }

  template <typename T> bool isType() const {
    return std::holds_alternative<T>(VariantType);
  }

private:
  std::variant<FunctionType, StructType, ArrayType> VariantType;
};

class DefinedType {
public:
  DefinedType() noexcept = default;
  DefinedType(StructType &&Type) noexcept
      : IsFinal(true), ParentTypeIdx(), Type(std::move(Type)) {}
  DefinedType(ArrayType &&Type) noexcept
      : IsFinal(true), ParentTypeIdx(), Type(std::move(Type)) {}
  DefinedType(FunctionType &&Type) noexcept
      : IsFinal(true), ParentTypeIdx(), Type(std::move(Type)) {}
  //  template <typename T>
  //  DefinedType(T &&Type) noexcept
  //      : IsFinal(true), ParentTypeIdx(), Type(std::move<T>(Type)) {}
  DefinedType(bool IsFinal, std::vector<uint32_t> &&ParentTypeIdx,
              StructureType &&Type) noexcept
      : IsFinal(IsFinal), ParentTypeIdx(ParentTypeIdx), Type(Type) {}

  const FunctionType &asFunctionType() const {
    // TODO: check all usage of `asFunctionType` that each should ensure that
    return Type.asType<FunctionType>();
  }

  FunctionType &asFunctionType() { return Type.asType<FunctionType>(); }

  bool isFinal() const { return IsFinal; }

  template <typename T> bool isType() const {
    return Type.template isType<T>();
  }

  Span<const uint32_t> getParentTypeIdx() const { return ParentTypeIdx; }

  const ArrayType &asArrayType() const { return Type.asType<ArrayType>(); }
  const StructType &asStructType() const { return Type.asType<StructType>(); }

  const StructureType &getType() const { return Type; }

private:
  bool IsFinal;
  std::vector<uint32_t> ParentTypeIdx;
  StructureType Type;
};

/// AST  MemoryType node.
class MemoryType {
public:
  /// Constructors.
  MemoryType() noexcept = default;
  MemoryType(uint32_t MinVal) noexcept : Lim(MinVal) {}
  MemoryType(uint32_t MinVal, uint32_t MaxVal, bool Shared = false) noexcept
      : Lim(MinVal, MaxVal, Shared) {}
  MemoryType(const Limit &L) noexcept : Lim(L) {}

  /// Getter of limit.
  const Limit &getLimit() const noexcept { return Lim; }
  Limit &getLimit() noexcept { return Lim; }

private:
  /// \name Data of MemoryType.
  /// @{
  Limit Lim;
  /// @}
};

/// AST TableType node.
class TableType {
public:
  /// Constructors.
  TableType() noexcept : Type(RefType::FuncRef), Lim() {}
  TableType(FullRefType RType, uint32_t MinVal) noexcept
      : Type(RType), Lim(MinVal) {}
  TableType(FullRefType RType, uint32_t MinVal, uint32_t MaxVal) noexcept
      : Type(RType), Lim(MinVal, MaxVal) {}
  TableType(FullRefType RType, const Limit &L) noexcept : Type(RType), Lim(L) {}

  /// Getter of reference type.
  FullRefType getRefType() const noexcept { return Type; }
  void setRefType(FullRefType RType) noexcept { Type = RType; }

  /// Getter of limit.
  const Limit &getLimit() const noexcept { return Lim; }
  Limit &getLimit() noexcept { return Lim; }

private:
  /// \name Data of TableType.
  /// @{
  FullRefType Type;
  Limit Lim;
  /// @}
};

class Table {
public:
  Table() noexcept : TType(), InitExpr() {}
  const TableType &getTableType() const noexcept { return TType; }
  TableType &getTableType() noexcept { return TType; }
  const Expression &getInitExpr() const noexcept { return InitExpr; }
  Expression &getInitExpr() noexcept { return InitExpr; }

private:
  TableType TType;
  Expression InitExpr;
};

/// AST GlobalType node.
class GlobalType {
public:
  /// Constructors.
  GlobalType() noexcept : Type(ValType::I32), Mut(ValMut::Const) {}
  GlobalType(FullValType VType, ValMut VMut) noexcept
      : Type(VType), Mut(VMut) {}

  /// `==` and `!=` operator overloadings.
  friend bool operator==(const GlobalType &LHS,
                         const GlobalType &RHS) noexcept {
    return LHS.Type == RHS.Type && LHS.Mut == RHS.Mut;
  }

  friend bool operator!=(const GlobalType &LHS,
                         const GlobalType &RHS) noexcept {
    return !(LHS == RHS);
  }

  /// Getter and setter of value type.
  FullValType getValType() const noexcept { return Type; }
  void setValType(FullValType VType) noexcept { Type = VType; }

  /// Getter and setter of value mutation.
  ValMut getValMut() const noexcept { return Mut; }
  void setValMut(ValMut VMut) noexcept { Mut = VMut; }

private:
  /// \name Data of GlobalType.
  /// @{
  FullValType Type;
  ValMut Mut;
  /// @}
};

} // namespace AST
} // namespace WasmEdge
