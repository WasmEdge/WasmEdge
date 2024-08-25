// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

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
#include "common/span.h"
#include "common/symbol.h"
#include "common/types.h"

#include <optional>
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

  /// Getter and setter of storage type.
  const ValType &getStorageType() const noexcept { return Type; }
  void setStorageType(const ValType &VType) noexcept { Type = VType; }

  /// Getter and setter of value mutation.
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

  /// Getter of content.
  const FunctionType &getFuncType() const noexcept {
    return *std::get_if<FunctionType>(&FType);
  }
  FunctionType &getFuncType() noexcept {
    return *std::get_if<FunctionType>(&FType);
  }
  const std::vector<FieldType> &getFieldTypes() const noexcept {
    return *std::get_if<std::vector<FieldType>>(&FType);
  }

  /// Setter of content.
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

  /// Getter of content type.
  TypeCode getContentTypeCode() const noexcept { return Type; }

  /// Checker if is a function type.
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

  /// Getter and setter of final flag.
  bool isFinal() const noexcept { return IsFinal; }
  void setFinal(bool F) noexcept { IsFinal = F; }

  /// Getter of type index vector.
  Span<const uint32_t> getSuperTypeIndices() const noexcept {
    return SuperTypeIndices;
  }
  std::vector<uint32_t> &getSuperTypeIndices() noexcept {
    return SuperTypeIndices;
  }

  /// Getter of composite type.
  const CompositeType &getCompositeType() const noexcept { return CompType; }
  CompositeType &getCompositeType() noexcept { return CompType; }

  /// Recursive type information.
  struct RecInfo {
    uint32_t Index;
    uint32_t RecTypeSize;
  };

  /// Getter of recursive type information.
  std::optional<RecInfo> getRecursiveInfo() const noexcept {
    return RecTypeInfo;
  }
  void setRecursiveInfo(uint32_t Index, uint32_t Size) noexcept {
    RecTypeInfo = RecInfo{Index, Size};
  }

  /// Getter of type index information in a module.
  std::optional<uint32_t> getTypeIndex() const noexcept { return TypeIndex; }
  void setTypeIndex(uint32_t Index) noexcept { TypeIndex = Index; }

private:
  /// \name Data of CompositeType.
  /// @{
  /// Is final.
  bool IsFinal;
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
  /// @}
};

/// AST Type match helper class.
class TypeMatcher {
public:
  /// Validator: Match 2 defined types in the same module.
  static bool matchType(Span<const SubType *const> TypeList, uint32_t ExpIdx,
                        uint32_t GotIdx) noexcept {
    return matchType(TypeList, ExpIdx, TypeList, GotIdx);
  }

  /// Validator: Match 2 composite types in the same module.
  static bool matchType(Span<const SubType *const> TypeList,
                        const CompositeType &Exp,
                        const CompositeType &Got) noexcept {
    auto isFieldTypeMatched = [&](const FieldType &ExpFieldType,
                                  const FieldType &GotFieldType) -> bool {
      bool IsMatch = false;
      if (ExpFieldType.getValMut() == GotFieldType.getValMut()) {
        // For both const or both var: Got storage type should match the
        // expected storage type.
        IsMatch = matchType(TypeList, ExpFieldType.getStorageType(),
                            GotFieldType.getStorageType());
        if (ExpFieldType.getValMut() == ValMut::Var) {
          // If both var: and vice versa.
          IsMatch &= matchType(TypeList, GotFieldType.getStorageType(),
                               ExpFieldType.getStorageType());
        }
      }
      return IsMatch;
    };

    if (Exp.getContentTypeCode() != Got.getContentTypeCode()) {
      return false;
    }
    switch (Exp.getContentTypeCode()) {
    case TypeCode::Func: {
      const auto &ExpFType = Exp.getFuncType();
      const auto &GotFType = Got.getFuncType();
      return matchTypes(TypeList, GotFType.getParamTypes(),
                        ExpFType.getParamTypes()) &&
             matchTypes(TypeList, ExpFType.getReturnTypes(),
                        GotFType.getReturnTypes());
    }
    case TypeCode::Struct: {
      const auto &ExpFType = Exp.getFieldTypes();
      const auto &GotFType = Got.getFieldTypes();
      if (GotFType.size() < ExpFType.size()) {
        return false;
      }
      for (uint32_t I = 0; I < ExpFType.size(); I++) {
        if (!isFieldTypeMatched(ExpFType[I], GotFType[I])) {
          return false;
        }
      }
      return true;
    }
    case TypeCode::Array: {
      const auto &ExpFType = Exp.getFieldTypes();
      const auto &GotFType = Got.getFieldTypes();
      return isFieldTypeMatched(ExpFType[0], GotFType[0]);
    }
    default:
      return false;
    }
  }

  /// Validator: Match 2 value types in the same module.
  static bool matchType(Span<const SubType *const> TypeList, const ValType &Exp,
                        const ValType &Got) noexcept {
    return matchType(TypeList, Exp, TypeList, Got);
  }

  /// Validator: Match 2 type lists in the same module.
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

  /// Matcher: Match 2 defined types.
  static bool matchType(Span<const SubType *const> ExpTypeList, uint32_t ExpIdx,
                        Span<const SubType *const> GotTypeList,
                        uint32_t GotIdx) noexcept {
    if (ExpIdx >= ExpTypeList.size() || GotIdx >= GotTypeList.size()) {
      return false;
    }
    if (isDefTypeEqual(ExpTypeList, ExpIdx, GotTypeList, GotIdx)) {
      return true;
    }
    const auto *GotType = GotTypeList[GotIdx];
    for (auto TIdx : GotType->getSuperTypeIndices()) {
      if (matchType(ExpTypeList, ExpIdx, GotTypeList, TIdx)) {
        return true;
      }
    }
    return false;
  }

  /// Matcher: Match 2 value types.
  static bool matchType(Span<const SubType *const> ExpTypeList,
                        const ValType &Exp,
                        Span<const SubType *const> GotTypeList,
                        const ValType &Got) noexcept {
    if (!Exp.isRefType() && !Got.isRefType() &&
        Exp.getCode() == Got.getCode()) {
      // Match for the non-reference type case.
      return true;
    }
    if (Exp.isRefType() && Got.isRefType()) {
      // Nullable matching.
      if (!Exp.isNullableRefType() && Got.isNullableRefType()) {
        return false;
      }

      // Match heap type.
      if (Exp.isAbsHeapType() && Got.isAbsHeapType()) {
        // Case 1: Both abstract heap type.
        return matchTypeCode(Exp.getHeapTypeCode(), Got.getHeapTypeCode());
      } else if (Exp.isAbsHeapType()) {
        // Case 2: Match a type index to abstract heap type.
        if (Got.getTypeIndex() >= GotTypeList.size()) {
          return false;
        }
        return matchTypeCode(
            Exp.getHeapTypeCode(),
            GotTypeList[Got.getTypeIndex()]->getCompositeType().expand());
      } else if (Got.isAbsHeapType()) {
        // Case 3: Match abstract heap type to a type index.
        if (Exp.getTypeIndex() >= ExpTypeList.size()) {
          return false;
        }
        TypeCode ExpandGotType =
            ExpTypeList[Exp.getTypeIndex()]->getCompositeType().expand();
        switch (Got.getHeapTypeCode()) {
        case TypeCode::NullRef:
          return matchTypeCode(TypeCode::AnyRef, ExpandGotType);
        case TypeCode::NullFuncRef:
          return matchTypeCode(TypeCode::FuncRef, ExpandGotType);
        case TypeCode::NullExternRef:
          return matchTypeCode(TypeCode::ExternRef, ExpandGotType);
        default:
          return false;
        }
      } else {
        // Case 4: Match defined types.
        return matchType(ExpTypeList, Exp.getTypeIndex(), GotTypeList,
                         Got.getTypeIndex());
      }
    }
    return false;
  }

private:
  /// Matcher: Helper for checking the equivalent of 2 defined types.
  static bool isDefTypeEqual(Span<const SubType *const> LHSList,
                             uint32_t LHSIdx,
                             Span<const SubType *const> RHSList,
                             uint32_t RHSIdx) {
    if (LHSList.data() == RHSList.data() && LHSIdx == RHSIdx) {
      // Two type indices in the same module are the same.
      return true;
    }
    const auto *LHSType = LHSList[LHSIdx];
    const auto *RHSType = RHSList[RHSIdx];
    // For GC proposal, a single subtype can be seemed as a self-recursive type.
    // That is, `(rec (type $t1 (func (param (ref $t1)))))` and
    //               `(type $t1 (func (param (ref $t1))))` are the same.
    // Therefore, use the subtype length for the recursive type size.
    const uint32_t LRecSize = LHSType->getRecursiveInfo().has_value()
                                  ? LHSType->getRecursiveInfo()->RecTypeSize
                                  : 1U;
    const uint32_t RRecSize = RHSType->getRecursiveInfo().has_value()
                                  ? RHSType->getRecursiveInfo()->RecTypeSize
                                  : 1U;
    if (LRecSize != RRecSize) {
      // 2 recursive type sizes are different. Must not be the same.
      return false;
    }
    if (LRecSize > 1) {
      // Both are in a recursive type with > 1 subtypes.
      if (LHSType->getRecursiveInfo()->Index !=
          RHSType->getRecursiveInfo()->Index) {
        // The recursive indices should be the same.
        return false;
      }
      // The recursive types should be the same.
      uint32_t LStartIdx = LHSIdx - LHSType->getRecursiveInfo()->Index;
      uint32_t RStartIdx = RHSIdx - RHSType->getRecursiveInfo()->Index;
      return isRecTypeEqual(LHSList, LStartIdx, RHSList, RStartIdx, LRecSize);
    } else {
      // Both are composite types or self-recursive types.
      return isRecTypeEqual(LHSList, LHSIdx, RHSList, RHSIdx, 1);
    }
  }

  /// Matcher: Helper for checking the equivalent of 2 recursive types.
  static bool isRecTypeEqual(Span<const SubType *const> LHSList,
                             uint32_t LStartIdx,
                             Span<const SubType *const> RHSList,
                             uint32_t RStartIdx, uint32_t RecSize) {

    auto isValTypeEqual = [&](const ValType &LType,
                              const ValType &RType) -> bool {
      if (LType.getHeapTypeCode() == TypeCode::TypeIndex &&
          RType.getHeapTypeCode() == TypeCode::TypeIndex) {
        if (LType.getCode() != RType.getCode()) {
          return false;
        }
        // Check the index is the recursive type internal index or not.
        auto LIdx = LType.getTypeIndex();
        auto RIdx = RType.getTypeIndex();
        assuming(LIdx < LHSList.size() && RIdx < RHSList.size());
        bool IsLInSelfRecType =
            (LIdx >= LStartIdx && LIdx < LStartIdx + RecSize);
        bool IsRInSelfRecType =
            (RIdx >= RStartIdx && RIdx < RStartIdx + RecSize);
        if (IsLInSelfRecType != IsRInSelfRecType) {
          // If the one index is the recursive type internal index but the other
          // isn't, the value types must be different.
          return false;
        }
        if (IsLInSelfRecType) {
          // For both are internal indices of the recursive types, the internal
          // indices must be the same.
          if (LIdx - LStartIdx == RIdx - RStartIdx) {
            return true;
          } else {
            return false;
          }
        }
        // For neither are internal indices, keep checking the equivalent of the
        // defined types.
        return isDefTypeEqual(LHSList, LIdx, RHSList, RIdx);
      } else {
        return (LType.getCode() == RType.getCode() &&
                LType.getHeapTypeCode() == RType.getHeapTypeCode());
      }
    };

    auto isFieldTypeEqual =
        [isValTypeEqual](const std::vector<FieldType> &LFieldTypes,
                         const std::vector<FieldType> &RFieldTypes) -> bool {
      if (LFieldTypes.size() != RFieldTypes.size()) {
        return false;
      }
      for (uint32_t I = 0; I < LFieldTypes.size(); I++) {
        if (LFieldTypes[I].getValMut() != RFieldTypes[I].getValMut()) {
          return false;
        }
        if (!isValTypeEqual(LFieldTypes[I].getStorageType(),
                            RFieldTypes[I].getStorageType())) {
          return false;
        }
      }
      return true;
    };

    auto isFuncTypeEqual =
        [isValTypeEqual](const FunctionType &LFuncType,
                         const FunctionType &RFuncType) -> bool {
      auto &LPTypes = LFuncType.getParamTypes();
      auto &LRTypes = LFuncType.getReturnTypes();
      auto &RPTypes = RFuncType.getParamTypes();
      auto &RRTypes = RFuncType.getReturnTypes();
      if (LPTypes.size() != RPTypes.size() ||
          LRTypes.size() != RRTypes.size()) {
        return false;
      }
      for (uint32_t I = 0; I < LPTypes.size(); I++) {
        if (!isValTypeEqual(LPTypes[I], RPTypes[I])) {
          return false;
        }
      }
      for (uint32_t I = 0; I < LRTypes.size(); I++) {
        if (!isValTypeEqual(LRTypes[I], RRTypes[I])) {
          return false;
        }
      }
      return true;
    };

    auto isCompTypeEqual = [isFuncTypeEqual, isFieldTypeEqual](
                               const CompositeType &LCompType,
                               const CompositeType &RCompType) -> bool {
      if (LCompType.expand() != RCompType.expand()) {
        return false;
      }
      switch (LCompType.expand()) {
      case TypeCode::FuncRef:
        return isFuncTypeEqual(LCompType.getFuncType(),
                               RCompType.getFuncType());
      case TypeCode::StructRef:
      case TypeCode::ArrayRef:
        return isFieldTypeEqual(LCompType.getFieldTypes(),
                                RCompType.getFieldTypes());
      default:
        assumingUnreachable();
      }
    };

    for (uint32_t I = 0; I < RecSize; I++) {
      // Every subtype in the recursive types should be equivalent.
      const auto *LHSType = LHSList[LStartIdx + I];
      const auto *RHSType = RHSList[RStartIdx + I];
      if (LHSType->isFinal() != RHSType->isFinal()) {
        return false;
      }
      auto LSuperTypes = LHSType->getSuperTypeIndices();
      auto RSuperTypes = RHSType->getSuperTypeIndices();
      if (LSuperTypes.size() != RSuperTypes.size()) {
        return false;
      }
      // TODO: GC - Fix the subtype matching.
      uint32_t SuperTypesSize = static_cast<uint32_t>(LSuperTypes.size());
      for (uint32_t J = 0; J < SuperTypesSize; J++) {
        if (!isValTypeEqual(ValType(TypeCode::Ref, LSuperTypes[J]),
                            ValType(TypeCode::Ref, RSuperTypes[J]))) {
          return false;
        }
      }
      if (!isCompTypeEqual(LHSType->getCompositeType(),
                           RHSType->getCompositeType())) {
        return false;
      }
    }
    return true;
  }

  /// Matcher: Helper for matching 2 type codes.
  static bool matchTypeCode(TypeCode Exp, TypeCode Got) noexcept {
    // Handle the equal cases first.
    if (Exp == Got) {
      return true;
    }

    // Match the func types: nofunc <= func
    if (Exp == TypeCode::FuncRef || Exp == TypeCode::NullFuncRef) {
      return Got == TypeCode::NullFuncRef;
    }
    if (Got == TypeCode::FuncRef || Got == TypeCode::NullFuncRef) {
      return false;
    }

    // Match the extern types: noextern <= extern
    if (Exp == TypeCode::ExternRef || Exp == TypeCode::NullExternRef) {
      return Got == TypeCode::NullExternRef;
    }
    if (Got == TypeCode::ExternRef || Got == TypeCode::NullExternRef) {
      return false;
    }

    // Match the other types: none <= i31 | struct | array <= eq <= any
    switch (Exp) {
    case TypeCode::I31Ref:
    case TypeCode::StructRef:
    case TypeCode::ArrayRef:
      // This will filter out the i31/struct/array unmatch cases.
      return Got == TypeCode::NullRef;
    case TypeCode::EqRef:
      return Got != TypeCode::AnyRef;
    case TypeCode::AnyRef:
      return true;
    default:
      break;
    }
    return false;
  }
};

/// AST MemoryType node.
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

class TagType {
public:
  TagType() = default;
  TagType(const uint32_t TIdx, const SubType *S) noexcept
      : TypeIdx(TIdx), Type(S) {}

  /// Getter and setter of TypeIdx.
  uint32_t getTypeIdx() const noexcept { return TypeIdx; }
  void setTypeIdx(uint32_t TIdx) noexcept { TypeIdx = TIdx; }

  // Getter and setter of Defined Type.
  const SubType &getDefType() const noexcept { return *Type; }
  void setDefType(const SubType *DefType) noexcept { Type = DefType; }

  // Getter of the size of value that is associated with the tag.
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

} // namespace AST
} // namespace WasmEdge

template <>
struct fmt::formatter<WasmEdge::AST::FunctionType>
    : fmt::formatter<std::string_view> {
  fmt::format_context::iterator
  format(const WasmEdge::AST::FunctionType &Type,
         fmt::format_context &Ctx) const noexcept {
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
