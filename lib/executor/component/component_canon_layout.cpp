// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "executor/component/executor.h"

#include "common/errinfo.h"
#include "common/spdlog.h"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

namespace WasmEdge {
namespace Executor {

namespace {
uint32_t getFlagsSize(size_t NumLabels) noexcept {
  if (NumLabels <= 8) {
    return 1;
  }
  if (NumLabels <= 16) {
    return 2;
  }
  return 4;
}
} // namespace

// The definition behind a type index. See
// "include/executor/component/executor.h".
Expect<std::pair<const AST::Component::DefValType *,
                 const Runtime::Instance::ComponentInstance *>>
ComponentExecutor::getDefValType(
    const Runtime::Instance::ComponentInstance &TypeInst,
    const ComponentValType &Ty) {
  EXPECTED_TRY(const auto *TypeDef,
               TypeInst.getTypeDefinition(Ty.getTypeIndex()));
  if (TypeDef->Def == nullptr || !TypeDef->Def->isDefValType()) {
    const ErrCode::Value Code = getInvalidTypeCode(TypeInst);
    spdlog::error(Code);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_DefValType));
    return Unexpect(Code);
  }
  return std::make_pair(&TypeDef->Def->getDefValType(),
                        TypeDef->Owner != nullptr ? TypeDef->Owner : &TypeInst);
}

// The discriminant size. See "include/executor/component/executor.h".
uint32_t
ComponentExecutor::getDiscriminantSize(uint64_t NumCases) const noexcept {
  if (NumCases <= 256) {
    return 1;
  }
  if (NumCases <= 65536) {
    return 2;
  }
  return 4;
}

// The number of cases of a variant-like type. See
// "include/executor/component/executor.h".
uint32_t ComponentExecutor::getCaseCount(
    const AST::Component::DefValType &Ty) const noexcept {
  if (Ty.isVariantTy()) {
    return static_cast<uint32_t>(Ty.getVariant().Cases.size());
  }
  if (Ty.isEnumTy()) {
    return static_cast<uint32_t>(Ty.getEnum().Labels.size());
  }
  // An option and a result have two cases.
  return 2;
}

// The payload type of a case. See "include/executor/component/executor.h".
std::optional<ComponentValType>
ComponentExecutor::getCaseType(const AST::Component::DefValType &Ty,
                               uint32_t Case) const noexcept {
  if (Ty.isVariantTy()) {
    return Ty.getVariant().Cases[Case].second;
  }
  if (Ty.isOptionTy() && Case == 1) {
    return Ty.getOption().ValTy;
  }
  if (Ty.isResultTy()) {
    return Case == 0 ? Ty.getResult().ValTy : Ty.getResult().ErrTy;
  }
  return std::nullopt;
}

// The largest payload alignment. See "include/executor/component/executor.h".
Expect<uint32_t> ComponentExecutor::getMaxCaseAlignment(
    const Runtime::Instance::ComponentInstance &TypeInst,
    const AST::Component::DefValType &Ty, bool IsMem64,
    const Runtime::Instance::ComponentInstance *LowerInst) {
  uint32_t MaxCaseAlign = 1;
  const uint32_t NumCases = getCaseCount(Ty);
  for (uint32_t I = 0; I < NumCases; ++I) {
    if (const auto CaseTy = getCaseType(Ty, I); CaseTy.has_value()) {
      EXPECTED_TRY(const uint32_t CaseAlign,
                   getAlignment(TypeInst, *CaseTy, IsMem64, LowerInst));
      MaxCaseAlign = std::max(MaxCaseAlign, CaseAlign);
    }
  }
  return MaxCaseAlign;
}

// The flat type of a borrow. See "include/executor/component/executor.h".
Expect<ValType> ComponentExecutor::flattenBorrow(
    const Runtime::Instance::ComponentInstance &TypeInst, uint32_t TypeIdx,
    const Runtime::Instance::ComponentInstance *LowerInst) {
  EXPECTED_TRY(const auto *ResType, TypeInst.getTypeResource(TypeIdx));
  if (ResType == nullptr) {
    const ErrCode::Value Code = getInvalidTypeCode(TypeInst);
    spdlog::error(Code);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_ResourceType));
    return Unexpect(Code);
  }
  // The implementing instance receives the representation itself.
  if (LowerInst != nullptr && LowerInst == ResType->getImpl()) {
    return ResType->getRepType();
  }
  return ValType(TypeCode::I32);
}

// The alignment of a value type. See "include/executor/component/executor.h".
Expect<uint32_t> ComponentExecutor::getAlignment(
    const Runtime::Instance::ComponentInstance &TypeInst,
    const ComponentValType &Ty, bool IsMem64,
    const Runtime::Instance::ComponentInstance *LowerInst) {
  if (Ty.isPrimValType()) {
    switch (Ty.getPrimValType()) {
    case PrimValType::Bool:
    case PrimValType::S8:
    case PrimValType::U8:
      return UINT32_C(1);
    case PrimValType::S16:
    case PrimValType::U16:
      return UINT32_C(2);
    case PrimValType::S32:
    case PrimValType::U32:
    case PrimValType::F32:
    case PrimValType::Char:
    case PrimValType::ErrorContext:
      return UINT32_C(4);
    case PrimValType::S64:
    case PrimValType::U64:
    case PrimValType::F64:
      return UINT32_C(8);
    case PrimValType::String:
      return IsMem64 ? UINT32_C(8) : UINT32_C(4);
    default:
      assumingUnreachable();
    }
  }
  EXPECTED_TRY(auto Resolved, getDefValType(TypeInst, Ty));
  return getAlignment(*Resolved.second, *Resolved.first, IsMem64, LowerInst);
}

// The alignment of a defined value type. See
// "include/executor/component/executor.h".
Expect<uint32_t> ComponentExecutor::getAlignment(
    const Runtime::Instance::ComponentInstance &TypeInst,
    const AST::Component::DefValType &Ty, bool IsMem64,
    const Runtime::Instance::ComponentInstance *LowerInst) {
  if (Ty.isPrimValType()) {
    return getAlignment(TypeInst, ComponentValType(Ty.getPrimValType()),
                        IsMem64, LowerInst);
  }
  if (Ty.isRecordTy()) {
    uint32_t Align = 1;
    for (const auto &Field : Ty.getRecord().LabelTypes) {
      EXPECTED_TRY(
          const uint32_t FieldAlign,
          getAlignment(TypeInst, Field.getValType(), IsMem64, LowerInst));
      Align = std::max(Align, FieldAlign);
    }
    return Align;
  }
  if (Ty.isTupleTy()) {
    uint32_t Align = 1;
    for (const auto &Elem : Ty.getTuple().Types) {
      EXPECTED_TRY(const uint32_t ElemAlign,
                   getAlignment(TypeInst, Elem, IsMem64, LowerInst));
      Align = std::max(Align, ElemAlign);
    }
    return Align;
  }
  if (Ty.isListTy()) {
    if (Ty.getList().Len.has_value()) {
      return getAlignment(TypeInst, Ty.getList().ValTy, IsMem64, LowerInst);
    }
    return IsMem64 ? UINT32_C(8) : UINT32_C(4);
  }
  if (Ty.isMapTy()) {
    return IsMem64 ? UINT32_C(8) : UINT32_C(4);
  }
  if (Ty.isFlagsTy()) {
    return getFlagsSize(Ty.getFlags().Labels.size());
  }
  if (Ty.isOwnTy() || Ty.isStreamTy() || Ty.isFutureTy()) {
    return UINT32_C(4);
  }
  if (Ty.isBorrowTy()) {
    EXPECTED_TRY(const ValType Flat,
                 flattenBorrow(TypeInst, Ty.getBorrow().Idx, LowerInst));
    return Flat.getCode() == TypeCode::I64 ? UINT32_C(8) : UINT32_C(4);
  }
  if (Ty.isVariantTy() || Ty.isOptionTy() || Ty.isResultTy() || Ty.isEnumTy()) {
    EXPECTED_TRY(const uint32_t MaxCaseAlign,
                 getMaxCaseAlignment(TypeInst, Ty, IsMem64, LowerInst));
    return std::max(getDiscriminantSize(getCaseCount(Ty)), MaxCaseAlign);
  }
  const ErrCode::Value Code = getInvalidTypeCode(TypeInst);
  spdlog::error(Code);
  spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_DefValType));
  return Unexpect(Code);
}

// The size of a value type. See "include/executor/component/executor.h".
Expect<uint64_t> ComponentExecutor::getElemSize(
    const Runtime::Instance::ComponentInstance &TypeInst,
    const ComponentValType &Ty, bool IsMem64,
    const Runtime::Instance::ComponentInstance *LowerInst) {
  if (Ty.isPrimValType()) {
    if (Ty.getPrimValType() == PrimValType::String) {
      return IsMem64 ? UINT64_C(16) : UINT64_C(8);
    }
    EXPECTED_TRY(const uint32_t Align,
                 getAlignment(TypeInst, Ty, IsMem64, LowerInst));
    return Align;
  }
  EXPECTED_TRY(auto Resolved, getDefValType(TypeInst, Ty));
  return getElemSize(*Resolved.second, *Resolved.first, IsMem64, LowerInst);
}

// The size of a defined value type. See
// "include/executor/component/executor.h".
Expect<uint64_t> ComponentExecutor::getElemSize(
    const Runtime::Instance::ComponentInstance &TypeInst,
    const AST::Component::DefValType &Ty, bool IsMem64,
    const Runtime::Instance::ComponentInstance *LowerInst) {
  if (Ty.isPrimValType()) {
    return getElemSize(TypeInst, ComponentValType(Ty.getPrimValType()), IsMem64,
                       LowerInst);
  }
  if (Ty.isRecordTy() || Ty.isTupleTy()) {
    uint64_t Size = 0;
    uint32_t Align = 1;
    auto AddField = [&](const ComponentValType &FieldTy) -> Expect<void> {
      EXPECTED_TRY(const uint32_t FieldAlign,
                   getAlignment(TypeInst, FieldTy, IsMem64, LowerInst));
      EXPECTED_TRY(const uint64_t FieldSize,
                   getElemSize(TypeInst, FieldTy, IsMem64, LowerInst));
      Size = alignTo(Size, FieldAlign) + FieldSize;
      Align = std::max(Align, FieldAlign);
      return {};
    };
    if (Ty.isRecordTy()) {
      for (const auto &Field : Ty.getRecord().LabelTypes) {
        EXPECTED_TRY(AddField(Field.getValType()));
      }
    } else {
      for (const auto &Elem : Ty.getTuple().Types) {
        EXPECTED_TRY(AddField(Elem));
      }
    }
    return alignTo(Size, Align);
  }
  if (Ty.isListTy()) {
    if (const auto &Len = Ty.getList().Len; Len.has_value()) {
      EXPECTED_TRY(
          const uint64_t ElemSize,
          getElemSize(TypeInst, Ty.getList().ValTy, IsMem64, LowerInst));
      return *Len * ElemSize;
    }
    return IsMem64 ? UINT64_C(16) : UINT64_C(8);
  }
  if (Ty.isMapTy()) {
    return IsMem64 ? UINT64_C(16) : UINT64_C(8);
  }
  if (Ty.isFlagsTy()) {
    return getFlagsSize(Ty.getFlags().Labels.size());
  }
  if (Ty.isOwnTy() || Ty.isStreamTy() || Ty.isFutureTy()) {
    return UINT64_C(4);
  }
  if (Ty.isBorrowTy()) {
    EXPECTED_TRY(const ValType Flat,
                 flattenBorrow(TypeInst, Ty.getBorrow().Idx, LowerInst));
    return Flat.getCode() == TypeCode::I64 ? UINT64_C(8) : UINT64_C(4);
  }
  if (Ty.isVariantTy() || Ty.isOptionTy() || Ty.isResultTy() || Ty.isEnumTy()) {
    // The discriminant, then the largest payload at the largest payload
    // alignment, rounded to the variant alignment.
    const uint32_t NumCases = getCaseCount(Ty);
    uint64_t Size = getDiscriminantSize(NumCases);
    EXPECTED_TRY(const uint32_t MaxCaseAlign,
                 getMaxCaseAlignment(TypeInst, Ty, IsMem64, LowerInst));
    uint64_t MaxCaseSize = 0;
    for (uint32_t I = 0; I < NumCases; ++I) {
      if (const auto CaseTy = getCaseType(Ty, I); CaseTy.has_value()) {
        EXPECTED_TRY(const uint64_t CaseSize,
                     getElemSize(TypeInst, *CaseTy, IsMem64, LowerInst));
        MaxCaseSize = std::max(MaxCaseSize, CaseSize);
      }
    }
    Size = alignTo(Size, MaxCaseAlign) + MaxCaseSize;
    EXPECTED_TRY(const uint32_t Align,
                 getAlignment(TypeInst, Ty, IsMem64, LowerInst));
    return alignTo(Size, Align);
  }
  const ErrCode::Value Code = getInvalidTypeCode(TypeInst);
  spdlog::error(Code);
  spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_DefValType));
  return Unexpect(Code);
}

// The flat types of a value type. See
// "include/executor/component/executor.h".
Expect<void> ComponentExecutor::flattenType(
    const Runtime::Instance::ComponentInstance &TypeInst,
    const ComponentValType &Ty, bool IsMem64,
    const Runtime::Instance::ComponentInstance *LowerInst,
    std::vector<ValType> &Out) {
  if (Ty.isPrimValType()) {
    const ValType Ptr(IsMem64 ? TypeCode::I64 : TypeCode::I32);
    switch (Ty.getPrimValType()) {
    case PrimValType::String:
      Out.push_back(Ptr);
      Out.push_back(Ptr);
      break;
    case PrimValType::S64:
    case PrimValType::U64:
      Out.emplace_back(TypeCode::I64);
      break;
    case PrimValType::F32:
      Out.emplace_back(TypeCode::F32);
      break;
    case PrimValType::F64:
      Out.emplace_back(TypeCode::F64);
      break;
    default:
      Out.emplace_back(TypeCode::I32);
      break;
    }
    return {};
  }
  EXPECTED_TRY(auto Resolved, getDefValType(TypeInst, Ty));
  return flattenType(*Resolved.second, *Resolved.first, IsMem64, LowerInst,
                     Out);
}

// The flat types of a defined value type. See
// "include/executor/component/executor.h".
Expect<void> ComponentExecutor::flattenType(
    const Runtime::Instance::ComponentInstance &TypeInst,
    const AST::Component::DefValType &Ty, bool IsMem64,
    const Runtime::Instance::ComponentInstance *LowerInst,
    std::vector<ValType> &Out) {
  const ValType Ptr(IsMem64 ? TypeCode::I64 : TypeCode::I32);
  if (Ty.isPrimValType()) {
    return flattenType(TypeInst, ComponentValType(Ty.getPrimValType()), IsMem64,
                       LowerInst, Out);
  }
  if (Ty.isRecordTy()) {
    for (const auto &Field : Ty.getRecord().LabelTypes) {
      EXPECTED_TRY(
          flattenType(TypeInst, Field.getValType(), IsMem64, LowerInst, Out));
    }
    return {};
  }
  if (Ty.isTupleTy()) {
    for (const auto &Elem : Ty.getTuple().Types) {
      EXPECTED_TRY(flattenType(TypeInst, Elem, IsMem64, LowerInst, Out));
    }
    return {};
  }
  if (Ty.isListTy()) {
    if (const auto &Len = Ty.getList().Len; Len.has_value()) {
      // Fixed-length lists flatten to Len copies of the element.
      for (uint32_t I = 0; I < *Len && Out.size() <= MaxFlatExpand; ++I) {
        EXPECTED_TRY(
            flattenType(TypeInst, Ty.getList().ValTy, IsMem64, LowerInst, Out));
      }
      return {};
    }
    Out.push_back(Ptr);
    Out.push_back(Ptr);
    return {};
  }
  if (Ty.isMapTy()) {
    Out.push_back(Ptr);
    Out.push_back(Ptr);
    return {};
  }
  if (Ty.isFlagsTy() || Ty.isOwnTy() || Ty.isStreamTy() || Ty.isFutureTy()) {
    Out.emplace_back(TypeCode::I32);
    return {};
  }
  if (Ty.isBorrowTy()) {
    EXPECTED_TRY(const ValType Flat,
                 flattenBorrow(TypeInst, Ty.getBorrow().Idx, LowerInst));
    Out.push_back(Flat);
    return {};
  }
  if (Ty.isVariantTy() || Ty.isOptionTy() || Ty.isResultTy() || Ty.isEnumTy()) {
    // The discriminant, then the element-wise join of the payload flats.
    auto JoinFlat = [](const ValType &Lhs, const ValType &Rhs) {
      if (Lhs == Rhs) {
        return Lhs;
      }
      if ((Lhs.getCode() == TypeCode::I32 && Rhs.getCode() == TypeCode::F32) ||
          (Lhs.getCode() == TypeCode::F32 && Rhs.getCode() == TypeCode::I32)) {
        return ValType(TypeCode::I32);
      }
      return ValType(TypeCode::I64);
    };
    std::vector<ValType> Joined;
    const uint32_t NumCases = getCaseCount(Ty);
    for (uint32_t I = 0; I < NumCases; ++I) {
      const auto CaseTy = getCaseType(Ty, I);
      if (!CaseTy.has_value()) {
        continue;
      }
      std::vector<ValType> Flat;
      EXPECTED_TRY(flattenType(TypeInst, *CaseTy, IsMem64, LowerInst, Flat));
      for (size_t J = 0; J < Flat.size(); ++J) {
        if (J < Joined.size()) {
          Joined[J] = JoinFlat(Joined[J], Flat[J]);
        } else {
          Joined.push_back(Flat[J]);
        }
      }
    }
    Out.emplace_back(TypeCode::I32);
    Out.insert(Out.end(), Joined.begin(), Joined.end());
    return {};
  }
  const ErrCode::Value Code = getInvalidTypeCode(TypeInst);
  spdlog::error(Code);
  spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_DefValType));
  return Unexpect(Code);
}

// Whether a type is a number type. See
// "include/executor/component/executor.h".
Expect<bool> ComponentExecutor::isNumberType(
    const Runtime::Instance::ComponentInstance &TypeInst,
    const ComponentValType &Ty) {
  if (Ty.isPrimValType()) {
    switch (Ty.getPrimValType()) {
    case PrimValType::S8:
    case PrimValType::U8:
    case PrimValType::S16:
    case PrimValType::U16:
    case PrimValType::S32:
    case PrimValType::U32:
    case PrimValType::S64:
    case PrimValType::U64:
    case PrimValType::F32:
    case PrimValType::F64:
      return true;
    default:
      return false;
    }
  }
  EXPECTED_TRY(auto Resolved, getDefValType(TypeInst, Ty));
  if (Resolved.first->isPrimValType()) {
    return isNumberType(*Resolved.second,
                        ComponentValType(Resolved.first->getPrimValType()));
  }
  return false;
}

// Whether a type is `u8`. See "include/executor/component/executor.h".
Expect<bool> ComponentExecutor::isByteType(
    const Runtime::Instance::ComponentInstance &TypeInst,
    const ComponentValType &Ty) {
  if (Ty.isPrimValType()) {
    return Ty.getPrimValType() == PrimValType::U8;
  }
  EXPECTED_TRY(auto Resolved, getDefValType(TypeInst, Ty));
  return Resolved.first->isPrimValType() &&
         Resolved.first->getPrimValType() == PrimValType::U8;
}

} // namespace Executor
} // namespace WasmEdge
