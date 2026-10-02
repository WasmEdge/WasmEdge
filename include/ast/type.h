// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

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

#include "common/executable.h"
#include "common/fmt.h"
#include "common/span.h"
#include "common/symbol.h"
#include "common/types.h"

#include <algorithm>
#include <optional>
#include <set>
#include <utility>
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
    SharedNoMax = 0x02, // from threads proposal, invalid
    Shared = 0x03,      // from threads proposal
    I64HasMin = 0x04,
    I64HasMinMax = 0x05,
    I64SharedNoMax = 0x06, // from threads proposal, invalid
    I64Shared = 0x07,      // from threads proposal
  };

  /// Constructors.
  Limit() noexcept : Type(LimitType::HasMin), Min(0U), Max(0U) {}
  Limit(uint64_t MinVal, bool Is64 = false) noexcept
      : Min(MinVal), Max(MinVal) {
    if (Is64) {
      Type = LimitType::I64HasMin;
    } else {
      Type = LimitType::HasMin;
    }
  }
  Limit(uint64_t MinVal, uint64_t MaxVal, bool Is64 = false,
        bool Shared = false) noexcept
      : Min(MinVal), Max(MaxVal) {
    if (Shared) {
      if (Is64) {
        Type = LimitType::I64Shared;
      } else {
        Type = LimitType::Shared;
      }
    } else {
      if (Is64) {
        Type = LimitType::I64HasMinMax;
      } else {
        Type = LimitType::HasMinMax;
      }
    }
  }

  /// Getter and setter for limit mode.
  bool hasMax() const noexcept { return static_cast<uint8_t>(Type) & 0x01U; }
  bool isShared() const noexcept { return static_cast<uint8_t>(Type) & 0x02U; }
  bool is32() const noexcept { return static_cast<uint8_t>(Type) < 0x04U; }
  bool is64() const noexcept { return !is32(); }
  AddressType getAddrType() const noexcept {
    return is32() ? AddressType::I32 : AddressType::I64;
  }
  void setType(LimitType TargetType) noexcept { Type = TargetType; }

  /// Getter and setter for min value.
  uint64_t getMin() const noexcept { return Min; }
  void setMin(uint64_t Val) noexcept { Min = Val; }

  /// Getter and setter for max value.
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
  /// Constructors.
  FunctionType() noexcept = default;
  FunctionType(Span<const ValType> P, Span<const ValType> R) noexcept
      : ParamTypes(P.begin(), P.end()), ReturnTypes(R.begin(), R.end()) {}
  FunctionType(Span<const ValType> P, Span<const ValType> R,
               Symbol<Executable::Wrapper> S) noexcept
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

  /// Getter for param types.
  const std::vector<ValType> &getParamTypes() const noexcept {
    return ParamTypes;
  }
  std::vector<ValType> &getParamTypes() noexcept { return ParamTypes; }

  /// Getter for return types.
  const std::vector<ValType> &getReturnTypes() const noexcept {
    return ReturnTypes;
  }
  std::vector<ValType> &getReturnTypes() noexcept { return ReturnTypes; }

  /// Getter and setter for symbol.
  const auto &getSymbol() const noexcept { return WrapSymbol; }
  void setSymbol(Symbol<Executable::Wrapper> S) noexcept {
    WrapSymbol = std::move(S);
  }

private:
  /// \name Data of FunctionType.
  /// @{
  std::vector<ValType> ParamTypes;
  std::vector<ValType> ReturnTypes;
  Symbol<Executable::Wrapper> WrapSymbol;
  /// @}
};

/// AST FieldType node for GC proposal.
class FieldType {
public:
  /// Constructors.
  FieldType() noexcept = default;
  FieldType(const ValType &Type, ValMut Mut) noexcept : Type(Type), Mut(Mut) {}

  /// Getter and setter for storage type.
  const ValType &getStorageType() const noexcept { return Type; }
  void setStorageType(const ValType &VType) noexcept { Type = VType; }

  /// Getter and setter for value mutation.
  ValMut getValMut() const noexcept { return Mut; }
  void setValMut(ValMut VMut) noexcept { Mut = VMut; }

private:
  /// \name Data of FieldType.
  /// @{
  ValType Type;
  ValMut Mut;
  /// @}
};

/// AST CompositeType node for GC proposal.
class CompositeType {
public:
  /// Constructors.
  CompositeType() noexcept = default;
  CompositeType(const FunctionType &FT) noexcept
      : Type(TypeCode::Func), FType(FT) {}

  /// Getter for content.
  const FunctionType &getFuncType() const noexcept {
    return *std::get_if<FunctionType>(&FType);
  }
  FunctionType &getFuncType() noexcept {
    return *std::get_if<FunctionType>(&FType);
  }
  const std::vector<FieldType> &getFieldTypes() const noexcept {
    return *std::get_if<std::vector<FieldType>>(&FType);
  }

  /// Setter for content.
  void setArrayType(FieldType &&FT) noexcept {
    Type = TypeCode::Array;
    FType = std::vector<FieldType>{std::move(FT)};
  }
  void setStructType(std::vector<FieldType> &&VFT) noexcept {
    Type = TypeCode::Struct;
    FType = std::move(VFT);
  }
  void setFunctionType(FunctionType &&FT) noexcept {
    Type = TypeCode::Func;
    FType = std::move(FT);
  }

  /// Getter for content type.
  TypeCode getContentTypeCode() const noexcept { return Type; }

  /// Check whether this is a function type.
  bool isFunc() const noexcept { return (Type == TypeCode::Func); }

  /// Expand the composite type to its reference.
  TypeCode expand() const noexcept {
    switch (Type) {
    case TypeCode::Func:
      return TypeCode::FuncRef;
    case TypeCode::Struct:
      return TypeCode::StructRef;
    case TypeCode::Array:
      return TypeCode::ArrayRef;
    default:
      assumingUnreachable();
    }
  }

private:
  /// \name Data of CompositeType.
  /// @{
  TypeCode Type;
  std::variant<std::vector<FieldType>, FunctionType> FType;
  /// @}
};

/// AST SubType node for GC proposal.
class SubType {
public:
  /// Constructors.
  SubType() noexcept = default;
  SubType(const FunctionType &FT) noexcept
      : IsFinal(true), CompType(FT), RecTypeInfo(std::nullopt),
        TypeIndex(std::nullopt) {}

  /// Getter and setter for final flag.
  bool isFinal() const noexcept { return IsFinal; }
  void setFinal(bool F) noexcept { IsFinal = F; }

  /// Getter for type index vector.
  Span<const uint32_t> getSuperTypeIndices() const noexcept {
    return SuperTypeIndices;
  }
  std::vector<uint32_t> &getSuperTypeIndices() noexcept {
    return SuperTypeIndices;
  }

  /// Getter for composite type.
  const CompositeType &getCompositeType() const noexcept { return CompType; }
  CompositeType &getCompositeType() noexcept { return CompType; }

  /// Recursive type information.
  struct RecInfo {
    uint32_t Index;
    uint32_t RecTypeSize;
  };

  /// Getter and setter for recursive type info, {0, 1} for a lone sub type.
  RecInfo getRecursiveInfo() const noexcept {
    return RecTypeInfo.value_or(RecInfo{0, 1});
  }
  void setRecursiveInfo(uint32_t Index, uint32_t Size) noexcept {
    RecTypeInfo = RecInfo{Index, Size};
  }

  /// Check whether the sub type is written in a recursive type.
  bool isInRecType() const noexcept { return RecTypeInfo.has_value(); }

  /// Getter for type index information in a module.
  std::optional<uint32_t> getTypeIndex() const noexcept { return TypeIndex; }
  void setTypeIndex(uint32_t Index) noexcept { TypeIndex = Index; }

  /// Getter and setter for the canonical type index: the first equal type's.
  std::optional<uint32_t> getCanonicalIndex() const noexcept {
    return CanonicalIndex;
  }
  void setCanonicalIndex(uint32_t Index) noexcept { CanonicalIndex = Index; }

private:
  /// \name Data of SubType.
  /// @{
  /// Is final.
  bool IsFinal = true;
  /// List of super type indices.
  std::vector<uint32_t> SuperTypeIndices;
  /// Content of composite type.
  CompositeType CompType;
  /// @}

  /// \name Information for defined types.
  /// @{
  /// Recursive type information. Record the index in the recursive type.
  std::optional<RecInfo> RecTypeInfo;
  /// Type index in the module. Record for backward iteration.
  std::optional<uint32_t> TypeIndex;
  /// Canonical type index in the module. Set by the validator.
  std::optional<uint32_t> CanonicalIndex;
  /// @}
};

/// AST MemoryType node.
class MemoryType {
public:
  /// Constructors.
  MemoryType() noexcept = default;
  MemoryType(uint64_t MinVal) noexcept : Lim(MinVal) {}
  MemoryType(uint64_t MinVal, uint64_t MaxVal, bool Shared = false) noexcept
      : Lim(MinVal, MaxVal, false, Shared) {}
  MemoryType(const Limit &L) noexcept : Lim(L) {}

  /// Getter for limit.
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
  TableType() noexcept : Type(TypeCode::FuncRef), Lim() {
    assuming(Type.isRefType());
  }
  TableType(const ValType &RType, uint64_t MinVal) noexcept
      : Type(RType), Lim(MinVal) {
    assuming(Type.isRefType());
  }
  TableType(const ValType &RType, uint64_t MinVal, uint64_t MaxVal) noexcept
      : Type(RType), Lim(MinVal, MaxVal) {
    assuming(Type.isRefType());
  }
  TableType(const ValType &RType, const Limit &L) noexcept
      : Type(RType), Lim(L) {
    assuming(Type.isRefType());
  }

  /// Getter for reference type.
  const ValType &getRefType() const noexcept { return Type; }
  void setRefType(const ValType &RType) noexcept {
    assuming(RType.isRefType());
    Type = RType;
  }

  /// Getter for limit.
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

  /// Getter and setter for value type.
  const ValType &getValType() const noexcept { return Type; }
  void setValType(const ValType &VType) noexcept { Type = VType; }

  /// Getter and setter for value mutation.
  ValMut getValMut() const noexcept { return Mut; }
  void setValMut(ValMut VMut) noexcept { Mut = VMut; }

private:
  /// \name Data of GlobalType.
  /// @{
  ValType Type;
  ValMut Mut;
  /// @}
};

class TagType {
public:
  TagType() = default;
  TagType(const uint32_t TIdx, const SubType *S) noexcept
      : TypeIdx(TIdx), Type(S) {}

  /// Getter and setter for TypeIdx.
  uint32_t getTypeIdx() const noexcept { return TypeIdx; }
  void setTypeIdx(uint32_t TIdx) noexcept { TypeIdx = TIdx; }

  // Getter and setter for Defined Type.
  const SubType &getDefType() const noexcept { return *Type; }
  void setDefType(const SubType *DefType) noexcept { Type = DefType; }

  // Getter for the size of the value associated with the tag.
  uint32_t getAssocValSize() const noexcept {
    if (Type && Type->getCompositeType().isFunc()) {
      return static_cast<uint32_t>(
          Type->getCompositeType().getFuncType().getParamTypes().size());
    } else {
      return 0;
    }
  }

private:
  uint32_t TypeIdx;
  const SubType *Type;
};

/// AST Type match helper class.
class TypeMatcher {
public:
  /// Pairs of recursive types known to be equal, by their first sub types.
  using RecTypePairs = std::set<std::pair<const SubType *, const SubType *>>;

  /// Validator: Match two composite types in the same module.
  static bool matchType(Span<const SubType *const> TypeList,
                        const CompositeType &Exp,
                        const CompositeType &Got) noexcept {
    if (Exp.getContentTypeCode() != Got.getContentTypeCode()) {
      return false;
    }
    if (Exp.isFunc()) {
      // Parameters match contravariantly and results covariantly.
      const auto &ExpFType = Exp.getFuncType();
      const auto &GotFType = Got.getFuncType();
      return matchTypes(TypeList, GotFType.getParamTypes(),
                        ExpFType.getParamTypes()) &&
             matchTypes(TypeList, ExpFType.getReturnTypes(),
                        GotFType.getReturnTypes());
    }
    // A struct type may add fields, and an array type has exactly one.
    const auto &ExpFTypes = Exp.getFieldTypes();
    const auto &GotFTypes = Got.getFieldTypes();
    if (GotFTypes.size() < ExpFTypes.size()) {
      return false;
    }
    for (uint32_t I = 0; I < ExpFTypes.size(); I++) {
      if (!matchType(TypeList, ExpFTypes[I], GotFTypes[I])) {
        return false;
      }
    }
    return true;
  }

  /// Validator: Match two value types in the same module.
  static bool matchType(Span<const SubType *const> TypeList, const ValType &Exp,
                        const ValType &Got) noexcept {
    return matchType(TypeList, Exp, TypeList, Got);
  }

  /// Validator: Match two type lists in the same module.
  static bool matchTypes(Span<const SubType *const> TypeList,
                         Span<const ValType> Exp,
                         Span<const ValType> Got) noexcept {
    if (Exp.size() != Got.size()) {
      return false;
    }
    for (uint32_t I = 0; I < Exp.size(); I++) {
      if (!matchType(TypeList, Exp[I], Got[I])) {
        return false;
      }
    }
    return true;
  }

  /// Matcher: Match two defined types.
  static bool matchType(Span<const SubType *const> ExpTypeList, uint32_t ExpIdx,
                        Span<const SubType *const> GotTypeList, uint32_t GotIdx,
                        RecTypePairs *Equal = nullptr) noexcept {
    // Walk up the super types: at most one each, defined before the sub type.
    while (GotIdx < GotTypeList.size()) {
      if (isDefTypeEqual(ExpTypeList, ExpIdx, GotTypeList, GotIdx, Equal)) {
        return true;
      }
      const auto SuperTypes = GotTypeList[GotIdx]->getSuperTypeIndices();
      if (SuperTypes.empty()) {
        return false;
      }
      GotIdx = SuperTypes[0];
    }
    return false;
  }

  /// Matcher: Match two value types.
  static bool matchType(Span<const SubType *const> ExpTypeList,
                        const ValType &Exp,
                        Span<const SubType *const> GotTypeList,
                        const ValType &Got,
                        RecTypePairs *Equal = nullptr) noexcept {
    if (!Exp.isRefType() || !Got.isRefType()) {
      // Number, vector, and packed types match only themselves.
      return !Exp.isRefType() && !Got.isRefType() &&
             Exp.getCode() == Got.getCode();
    }
    if (Got.isNullableRefType() && !Exp.isNullableRefType()) {
      return false;
    }
    if (Exp.isAbsHeapType() && Got.isAbsHeapType()) {
      return matchTypeCode(Exp.getHeapTypeCode(), Got.getHeapTypeCode());
    }
    if (Exp.isAbsHeapType()) {
      // A defined type matches the abstract heap types above its expansion.
      return Got.getTypeIndex() < GotTypeList.size() &&
             matchTypeCode(
                 Exp.getHeapTypeCode(),
                 GotTypeList[Got.getTypeIndex()]->getCompositeType().expand());
    }
    if (Got.isAbsHeapType()) {
      // Only the bottom type of the hierarchy matches a defined type.
      return Exp.getTypeIndex() < ExpTypeList.size() &&
             Got.getHeapTypeCode() ==
                 getBottomHeapType(ExpTypeList[Exp.getTypeIndex()]
                                       ->getCompositeType()
                                       .expand());
    }
    return matchType(ExpTypeList, Exp.getTypeIndex(), GotTypeList,
                     Got.getTypeIndex(), Equal);
  }

  /// Matcher: Match two table types.
  static bool matchType(Span<const SubType *const> ExpTypeList,
                        const TableType &Exp,
                        Span<const SubType *const> GotTypeList,
                        const TableType &Got,
                        RecTypePairs *Equal = nullptr) noexcept {
    // Matching both ways, the reference types must be equal.
    return matchLimit(Exp.getLimit(), Got.getLimit()) &&
           isValTypeEqual(ExpTypeList, Exp.getRefType(), GotTypeList,
                          Got.getRefType(), Equal);
  }

  /// Matcher: Match two global types.
  static bool matchType(Span<const SubType *const> ExpTypeList,
                        const GlobalType &Exp,
                        Span<const SubType *const> GotTypeList,
                        const GlobalType &Got,
                        RecTypePairs *Equal = nullptr) noexcept {
    return matchType(ExpTypeList, Exp.getValMut(), Exp.getValType(),
                     GotTypeList, Got.getValMut(), Got.getValType(), Equal);
  }

  /// Matcher: Match two tag types.
  static bool matchType(Span<const SubType *const> ExpTypeList,
                        const TagType &Exp,
                        Span<const SubType *const> GotTypeList,
                        const TagType &Got,
                        RecTypePairs *Equal = nullptr) noexcept {
    // Matching both ways, the defined types must be equal.
    return isDefTypeEqual(ExpTypeList, Exp.getTypeIdx(), GotTypeList,
                          Got.getTypeIdx(), Equal);
  }

  /// Matcher: Match two limits.
  static bool matchLimit(const Limit &Exp, const Limit &Got) noexcept {
    if (Exp.getAddrType() != Got.getAddrType() ||
        Exp.isShared() != Got.isShared()) {
      return false;
    }
    if (Got.getMin() < Exp.getMin()) {
      return false;
    }
    if (Exp.hasMax()) {
      return Got.hasMax() && Got.getMax() <= Exp.getMax();
    }
    return true;
  }

  /// Matcher: Get the top type of the hierarchy of an abstract heap type.
  static TypeCode getTopHeapType(TypeCode HTCode) noexcept {
    switch (HTCode) {
    case TypeCode::NullFuncRef:
    case TypeCode::FuncRef:
      return TypeCode::FuncRef;
    case TypeCode::NullExternRef:
    case TypeCode::ExternRef:
      return TypeCode::ExternRef;
    case TypeCode::NullExnRef:
    case TypeCode::ExnRef:
      return TypeCode::ExnRef;
    default:
      return TypeCode::AnyRef;
    }
  }

  /// Matcher: Get the bottom type of the hierarchy of an abstract heap type.
  static TypeCode getBottomHeapType(TypeCode HTCode) noexcept {
    switch (getTopHeapType(HTCode)) {
    case TypeCode::FuncRef:
      return TypeCode::NullFuncRef;
    case TypeCode::ExternRef:
      return TypeCode::NullExternRef;
    case TypeCode::ExnRef:
      return TypeCode::NullExnRef;
    default:
      return TypeCode::NullRef;
    }
  }

private:
  /// Matcher: Helper for matching two field types in the same module.
  static bool matchType(Span<const SubType *const> TypeList,
                        const FieldType &Exp, const FieldType &Got) noexcept {
    return matchType(TypeList, Exp.getValMut(), Exp.getStorageType(), TypeList,
                     Got.getValMut(), Got.getStorageType());
  }

  /// Matcher: Helper for matching types with mutability, equal if mutable.
  static bool matchType(Span<const SubType *const> ExpTypeList, ValMut ExpMut,
                        const ValType &Exp,
                        Span<const SubType *const> GotTypeList, ValMut GotMut,
                        const ValType &Got,
                        RecTypePairs *Equal = nullptr) noexcept {
    if (ExpMut != GotMut) {
      return false;
    }
    if (ExpMut == ValMut::Const) {
      return matchType(ExpTypeList, Exp, GotTypeList, Got, Equal);
    }
    return isValTypeEqual(ExpTypeList, Exp, GotTypeList, Got, Equal);
  }

  /// Matcher: Helper for matching two abstract heap types.
  static bool matchTypeCode(TypeCode Exp, TypeCode Got) noexcept {
    if (Exp == Got) {
      return true;
    }
    // The abstract heap types above the got type.
    switch (Got) {
    case TypeCode::NullRef:
    case TypeCode::NullFuncRef:
    case TypeCode::NullExnRef:
    case TypeCode::NullExternRef:
      return getTopHeapType(Exp) == getTopHeapType(Got);
    case TypeCode::I31Ref:
    case TypeCode::StructRef:
    case TypeCode::ArrayRef:
      return Exp == TypeCode::EqRef || Exp == TypeCode::AnyRef;
    case TypeCode::EqRef:
      return Exp == TypeCode::AnyRef;
    default:
      return false;
    }
  }

  /// Matcher: Helper for checking the equivalence of two value types.
  static bool isValTypeEqual(Span<const SubType *const> LHSList,
                             const ValType &LHS,
                             Span<const SubType *const> RHSList,
                             const ValType &RHS, RecTypePairs *Equal) noexcept {
    if (LHS.getCode() != RHS.getCode() ||
        LHS.getHeapTypeCode() != RHS.getHeapTypeCode()) {
      return false;
    }
    return LHS.getHeapTypeCode() != TypeCode::TypeIndex ||
           isDefTypeEqual(LHSList, LHS.getTypeIndex(), RHSList,
                          RHS.getTypeIndex(), Equal);
  }

  /// Matcher: Helper for the equivalence of two defined types by indices alone.
  static std::optional<bool> isEqualByIndex(Span<const SubType *const> LHSList,
                                            uint32_t LHSIdx,
                                            Span<const SubType *const> RHSList,
                                            uint32_t RHSIdx) noexcept {
    if (LHSIdx >= LHSList.size() || RHSIdx >= RHSList.size()) {
      return false;
    }
    if (LHSList.data() != RHSList.data()) {
      return std::nullopt;
    }
    if (LHSIdx == RHSIdx) {
      return true;
    }
    const auto LCanonIdx = LHSList[LHSIdx]->getCanonicalIndex();
    const auto RCanonIdx = RHSList[RHSIdx]->getCanonicalIndex();
    if (!LCanonIdx.has_value() || !RCanonIdx.has_value()) {
      return std::nullopt;
    }
    return *LCanonIdx == *RCanonIdx;
  }

  /// Matcher: Helper for the first indices of the recursive types to compare.
  static std::optional<std::pair<uint32_t, uint32_t>>
  getRecTypePair(Span<const SubType *const> LHSList, uint32_t LHSIdx,
                 Span<const SubType *const> RHSList, uint32_t RHSIdx) noexcept {
    const auto LInfo = LHSList[LHSIdx]->getRecursiveInfo();
    const auto RInfo = RHSList[RHSIdx]->getRecursiveInfo();
    if (LInfo.Index != RInfo.Index || LInfo.RecTypeSize != RInfo.RecTypeSize) {
      return std::nullopt;
    }
    const uint32_t LStartIdx = LHSIdx - LInfo.Index;
    const uint32_t RStartIdx = RHSIdx - RInfo.Index;
    return std::make_pair(
        LHSList[LStartIdx]->getCanonicalIndex().value_or(LStartIdx),
        RHSList[RStartIdx]->getCanonicalIndex().value_or(RStartIdx));
  }

  /// Matcher: Helper for checking the equivalence of two defined types.
  static bool isDefTypeEqual(Span<const SubType *const> LHSList,
                             uint32_t LHSIdx,
                             Span<const SubType *const> RHSList,
                             uint32_t RHSIdx,
                             RecTypePairs *Equal = nullptr) noexcept {
    if (const auto IsEqual = isEqualByIndex(LHSList, LHSIdx, RHSList, RHSIdx);
        IsEqual.has_value()) {
      return *IsEqual;
    }
    auto Pair = getRecTypePair(LHSList, LHSIdx, RHSList, RHSIdx);
    if (!Pair.has_value()) {
      return false;
    }
    // Rec types refer only to earlier ones, so popping the largest pending pair
    // compares each pair once, and its repeats come out right after it.
    std::vector<std::pair<uint32_t, uint32_t>> Pending;
    std::vector<std::pair<uint32_t, uint32_t>> Compared;
    while (true) {
      if (Equal == nullptr ||
          Equal->count({LHSList[Pair->first], RHSList[Pair->second]}) == 0) {
        if (!isRecTypeEqual(LHSList, Pair->first, RHSList, Pair->second,
                            Pending)) {
          return false;
        }
        if (Equal != nullptr) {
          Compared.push_back(*Pair);
        }
      }
      while (!Pending.empty() && Pending.front() == *Pair) {
        std::pop_heap(Pending.begin(), Pending.end());
        Pending.pop_back();
      }
      if (Pending.empty()) {
        break;
      }
      std::pop_heap(Pending.begin(), Pending.end());
      Pair = Pending.back();
      Pending.pop_back();
    }
    if (Equal != nullptr) {
      for (const auto &[LStartIdx, RStartIdx] : Compared) {
        Equal->emplace(LHSList[LStartIdx], RHSList[RStartIdx]);
      }
    }
    return true;
  }

  /// Matcher: Helper for checking the equivalence of two recursive types.
  static bool
  isRecTypeEqual(Span<const SubType *const> LHSList, uint32_t LStartIdx,
                 Span<const SubType *const> RHSList, uint32_t RStartIdx,
                 std::vector<std::pair<uint32_t, uint32_t>> &Pending) noexcept {
    const uint32_t RecSize = LHSList[LStartIdx]->getRecursiveInfo().RecTypeSize;
    auto IsTypeIdxEqual = [&](uint32_t LIdx, uint32_t RIdx) -> bool {
      const bool IsLInRecType = LIdx >= LStartIdx && LIdx - LStartIdx < RecSize;
      const bool IsRInRecType = RIdx >= RStartIdx && RIdx - RStartIdx < RecSize;
      if (IsLInRecType || IsRInRecType) {
        return IsLInRecType && IsRInRecType &&
               LIdx - LStartIdx == RIdx - RStartIdx;
      }
      if (const auto IsEqual = isEqualByIndex(LHSList, LIdx, RHSList, RIdx);
          IsEqual.has_value()) {
        return *IsEqual;
      }
      const auto Pair = getRecTypePair(LHSList, LIdx, RHSList, RIdx);
      if (!Pair.has_value()) {
        return false;
      }
      Pending.push_back(*Pair);
      std::push_heap(Pending.begin(), Pending.end());
      return true;
    };
    auto IsValTypeEqual = [&](const ValType &LType,
                              const ValType &RType) -> bool {
      if (LType.getCode() != RType.getCode() ||
          LType.getHeapTypeCode() != RType.getHeapTypeCode()) {
        return false;
      }
      return LType.getHeapTypeCode() != TypeCode::TypeIndex ||
             IsTypeIdxEqual(LType.getTypeIndex(), RType.getTypeIndex());
    };
    auto IsValTypesEqual = [&](Span<const ValType> LTypes,
                               Span<const ValType> RTypes) -> bool {
      if (LTypes.size() != RTypes.size()) {
        return false;
      }
      for (uint32_t I = 0; I < LTypes.size(); I++) {
        if (!IsValTypeEqual(LTypes[I], RTypes[I])) {
          return false;
        }
      }
      return true;
    };

    for (uint32_t I = 0; I < RecSize; I++) {
      const auto &LType = *LHSList[LStartIdx + I];
      const auto &RType = *RHSList[RStartIdx + I];
      const auto &LCompType = LType.getCompositeType();
      const auto &RCompType = RType.getCompositeType();
      const auto LSuperTypes = LType.getSuperTypeIndices();
      const auto RSuperTypes = RType.getSuperTypeIndices();
      if (LType.isFinal() != RType.isFinal() ||
          LCompType.getContentTypeCode() != RCompType.getContentTypeCode() ||
          LSuperTypes.size() != RSuperTypes.size()) {
        return false;
      }
      if (LCompType.isFunc()) {
        const auto &LFType = LCompType.getFuncType();
        const auto &RFType = RCompType.getFuncType();
        if (!IsValTypesEqual(LFType.getParamTypes(), RFType.getParamTypes()) ||
            !IsValTypesEqual(LFType.getReturnTypes(),
                             RFType.getReturnTypes())) {
          return false;
        }
      } else {
        const auto &LFTypes = LCompType.getFieldTypes();
        const auto &RFTypes = RCompType.getFieldTypes();
        if (LFTypes.size() != RFTypes.size()) {
          return false;
        }
        for (uint32_t J = 0; J < LFTypes.size(); J++) {
          if (LFTypes[J].getValMut() != RFTypes[J].getValMut() ||
              !IsValTypeEqual(LFTypes[J].getStorageType(),
                              RFTypes[J].getStorageType())) {
            return false;
          }
        }
      }
      for (uint32_t J = 0; J < LSuperTypes.size(); J++) {
        if (!IsTypeIdxEqual(LSuperTypes[J], RSuperTypes[J])) {
          return false;
        }
      }
    }
    return true;
  }
};

} // namespace AST
} // namespace WasmEdge

template <>
struct fmt::formatter<WasmEdge::AST::FunctionType>
    : fmt::formatter<std::string_view> {
  template <typename FmtCtx>
  auto format(const WasmEdge::AST::FunctionType &Type,
              FmtCtx &Ctx) WASMEDGE_FMT_CONST noexcept -> decltype(Ctx.out()) {
    using namespace std::literals;

    fmt::memory_buffer Buffer;

    fmt::format_to(std::back_inserter(Buffer), "[ "sv);
    for (auto &P : Type.getParamTypes()) {
      fmt::format_to(std::back_inserter(Buffer), "{} "sv, P);
    }
    fmt::format_to(std::back_inserter(Buffer), "] -> [ "sv);
    for (auto &R : Type.getReturnTypes()) {
      fmt::format_to(std::back_inserter(Buffer), "{} "sv, R);
    }
    fmt::format_to(std::back_inserter(Buffer), "]"sv);

    return formatter<std::string_view>::format(
        std::string_view(Buffer.data(), Buffer.size()), Ctx);
  }
};
