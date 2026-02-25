// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "loader/serialize.h"

namespace WasmEdge {
namespace Loader {

// Serialize heap type. See "include/loader/serialize.h".
Expect<void>
Serializer::serializeHeapType(const ValType &Type, ASTNodeAttr From,
                              std::vector<uint8_t> &OutVec) const noexcept {
  TypeCode Code = Type.getHeapTypeCode();
  switch (Code) {
  case TypeCode::ExternRef:
    if (unlikely(!Conf.hasProposal(Proposal::ReferenceTypes))) {
      return logNeedProposal(ErrCode::Value::MalformedElemType,
                             Proposal::ReferenceTypes, From);
    }
    [[fallthrough]];
  case TypeCode::FuncRef:
    OutVec.push_back(static_cast<uint8_t>(Code));
    return {};
  case TypeCode::NullFuncRef:
  case TypeCode::NullExternRef:
  case TypeCode::NullRef:
  case TypeCode::AnyRef:
  case TypeCode::EqRef:
  case TypeCode::I31Ref:
  case TypeCode::StructRef:
  case TypeCode::ArrayRef:
    if (!Conf.hasProposal(Proposal::GC)) {
      return logNeedProposal(ErrCode::Value::MalformedRefType, Proposal::GC,
                             From);
    }
    OutVec.push_back(static_cast<uint8_t>(Code));
    return {};
  case TypeCode::TypeIndex:
    if (unlikely(!Conf.hasProposal(Proposal::FunctionReferences))) {
      return logNeedProposal(ErrCode::Value::MalformedRefType,
                             Proposal::FunctionReferences, From);
    }
    serializeS33(static_cast<int64_t>(Type.getTypeIndex()), OutVec);
    return {};
  case TypeCode::NullExnRef:
  case TypeCode::ExnRef:
    if (unlikely(!Conf.hasProposal(Proposal::ExceptionHandling))) {
      return logNeedProposal(ErrCode::Value::MalformedRefType,
                             Proposal::ExceptionHandling, From);
    }
    OutVec.push_back(static_cast<uint8_t>(Code));
    return {};
  default:
    if (likely(Conf.hasProposal(Proposal::ReferenceTypes))) {
      return logSerializeError(ErrCode::Value::MalformedRefType, From);
    } else {
      return logSerializeError(ErrCode::Value::MalformedElemType, From);
    }
  }
}

// Serialize reference type. See "include/loader/serialize.h".
Expect<void>
Serializer::serializeRefType(const ValType &Type, ASTNodeAttr From,
                             std::vector<uint8_t> &OutVec) const noexcept {
  TypeCode Code = Type.getCode();
  switch (Code) {
  case TypeCode::Ref:
    if (unlikely(!Conf.hasProposal(Proposal::FunctionReferences))) {
      return logNeedProposal(ErrCode::Value::MalformedRefType,
                             Proposal::FunctionReferences, From);
    }
    OutVec.push_back(static_cast<uint8_t>(Code));
    return serializeHeapType(Type, From, OutVec);
  case TypeCode::RefNull:
    if (!Type.isAbsHeapType()) {
      OutVec.push_back(static_cast<uint8_t>(Code));
    }
    return serializeHeapType(Type, From, OutVec);
  default:
    if (likely(Conf.hasProposal(Proposal::ReferenceTypes))) {
      return logSerializeError(ErrCode::Value::MalformedRefType, From);
    } else {
      return logSerializeError(ErrCode::Value::MalformedElemType, From);
    }
  }
}

// Serialize value type. See "include/loader/serialize.h".
Expect<void>
Serializer::serializeValType(const ValType &Type, ASTNodeAttr From,
                             std::vector<uint8_t> &OutVec) const noexcept {
  TypeCode Code = Type.getCode();
  switch (Code) {
  case TypeCode::I32:
  case TypeCode::I64:
  case TypeCode::F32:
  case TypeCode::F64:
    OutVec.push_back(static_cast<uint8_t>(Code));
    return {};
  case TypeCode::I8:
  case TypeCode::I16:
    if (!Conf.hasProposal(Proposal::GC)) {
      return logNeedProposal(ErrCode::Value::MalformedValType, Proposal::GC,
                             From);
    }
    OutVec.push_back(static_cast<uint8_t>(Code));
    return {};
  case TypeCode::V128:
    if (unlikely(!Conf.hasProposal(Proposal::SIMD))) {
      return logNeedProposal(ErrCode::Value::MalformedValType, Proposal::SIMD,
                             From);
    }
    OutVec.push_back(static_cast<uint8_t>(Code));
    return {};
  case TypeCode::Ref:
  case TypeCode::RefNull:
    return serializeRefType(Type, From, OutVec);
  default:
    return logSerializeError(ErrCode::Value::MalformedValType, From);
  }
}

// Serialize limit. See "include/loader/serialize.h".
Expect<void>
Serializer::serializeLimit(const AST::Limit &Lim,
                           std::vector<uint8_t> &OutVec) const noexcept {
  // Limit: 0x00 + min:u32
  //       |0x01 + min:u32 + max:u32
  //       |0x02 + min:u32 (shared, invalid)
  //       |0x03 + min:u32 + max:u32 (shared)
  //       |0x04 + min:u64
  //       |0x05 + min:u64 + max:u64
  //       |0x06 + min:u64
  //       |0x07 + min:u64 + max:u64
  //
  // we encode all of them in u64
  uint8_t Flag = 0;
  if (Lim.isShared()) {
    Flag |= 0x02U;
  }
  if (Lim.hasMax()) {
    Flag |= 0x01U;
  }
  if (static_cast<AST::Limit::LimitType>(Flag) >=
      AST::Limit::LimitType::SharedNoMax) {
    if (Conf.hasProposal(Proposal::Threads)) {
      if (unlikely(!Lim.hasMax())) {
        return logSerializeError(ErrCode::Value::SharedMemoryNoMax,
                                 ASTNodeAttr::Type_Limit);
      }
    } else {
      return logSerializeError(ErrCode::Value::IntegerTooLarge,
                               ASTNodeAttr::Type_Limit);
    }
  }
  OutVec.push_back(Flag);
  serializeU64(Lim.getMin(), OutVec);
  if (Lim.hasMax()) {
    serializeU64(Lim.getMax(), OutVec);
  }
  return {};
}

// Serialize sub type. See "include/loader/serialize.h".
Expect<void>
Serializer::serializeType(const AST::SubType &SType,
                          std::vector<uint8_t> &OutVec) const noexcept {
  // Sub type: vec(typeidx)
  if (SType.getSuperTypeIndices().size() > 0) {
    if (!Conf.hasProposal(Proposal::GC)) {
      return logNeedProposal(ErrCode::Value::MalformedValType, Proposal::GC,
                             ASTNodeAttr::Type_Rec);
    }
    if (SType.isFinal()) {
      OutVec.push_back(static_cast<uint8_t>(TypeCode::SubFinal));
    } else {
      OutVec.push_back(static_cast<uint8_t>(TypeCode::Sub));
    }
    serializeU32(static_cast<uint32_t>(SType.getSuperTypeIndices().size()),
                 OutVec);
    for (const auto &Idx : SType.getSuperTypeIndices()) {
      serializeU32(Idx, OutVec);
    }
  }
  // Composite type: array | struct | func
  TypeCode CTypeCode = SType.getCompositeType().getContentTypeCode();
  OutVec.push_back(static_cast<uint8_t>(CTypeCode));
  switch (CTypeCode) {
  case TypeCode::Func:
    EXPECTED_TRY(serializeType(SType.getCompositeType().getFuncType(), OutVec));
    break;
  case TypeCode::Array:
    if (!Conf.hasProposal(Proposal::GC)) {
      return logNeedProposal(ErrCode::Value::MalformedValType, Proposal::GC,
                             ASTNodeAttr::Type_Rec);
    }
    EXPECTED_TRY(serializeType(SType.getCompositeType().getFieldTypes().front(),
                               OutVec));
    break;
  case TypeCode::Struct:
    // Struct type: vec(fieldtype)
    if (!Conf.hasProposal(Proposal::GC)) {
      return logNeedProposal(ErrCode::Value::MalformedValType, Proposal::GC,
                             ASTNodeAttr::Type_Rec);
    }
    serializeU32(
        static_cast<uint32_t>(SType.getCompositeType().getFieldTypes().size()),
        OutVec);
    for (auto FType : SType.getCompositeType().getFieldTypes()) {
      EXPECTED_TRY(serializeType(FType, OutVec));
    }
    break;
  default:
    return logSerializeError(ErrCode::Value::MalformedValType,
                             ASTNodeAttr::Type_Rec);
  }
  return {};
}

// Serialize function type. See "include/loader/serialize.h".
Expect<void>
Serializer::serializeType(const AST::FunctionType &Type,
                          std::vector<uint8_t> &OutVec) const noexcept {
  // Function type: paramtypes:vec(valtype) + returntypes:vec(valtype).
  // Param types: vec(valtype).
  serializeU32(static_cast<uint32_t>(Type.getParamTypes().size()), OutVec);
  for (auto &VType : Type.getParamTypes()) {
    EXPECTED_TRY(serializeValType(VType, ASTNodeAttr::Type_Function, OutVec));
  }
  // Return types: vec(valtype).
  if (unlikely(!Conf.hasProposal(Proposal::MultiValue)) &&
      Type.getReturnTypes().size() > 1) {
    return logNeedProposal(ErrCode::Value::MalformedValType,
                           Proposal::MultiValue, ASTNodeAttr::Type_Function);
  }
  serializeU32(static_cast<uint32_t>(Type.getReturnTypes().size()), OutVec);
  for (auto &VType : Type.getReturnTypes()) {
    EXPECTED_TRY(serializeValType(VType, ASTNodeAttr::Type_Function, OutVec));
  }
  return {};
}

// Serialize table type. See "include/loader/serialize.h".
Expect<void>
Serializer::serializeType(const AST::TableType &Type,
                          std::vector<uint8_t> &OutVec) const noexcept {
  // Table type: elemtype:valtype + limit.
  EXPECTED_TRY(
      serializeRefType(Type.getRefType(), ASTNodeAttr::Type_Table, OutVec));
  return serializeLimit(Type.getLimit(), OutVec).map_error([](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Type_Table));
    return E;
  });
}

// Serialize memory type. See "include/loader/serialize.h".
Expect<void>
Serializer::serializeType(const AST::MemoryType &Type,
                          std::vector<uint8_t> &OutVec) const noexcept {
  // Memory type: limit.
  return serializeLimit(Type.getLimit(), OutVec).map_error([](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Type_Memory));
    return E;
  });
}

// Serialize global type. See "include/loader/serialize.h".
Expect<void>
Serializer::serializeType(const AST::GlobalType &Type,
                          std::vector<uint8_t> &OutVec) const noexcept {
  // Global type: valtype + valmut.
  EXPECTED_TRY(
      serializeValType(Type.getValType(), ASTNodeAttr::Type_Global, OutVec));
  OutVec.push_back(static_cast<uint8_t>(Type.getValMut()));
  return {};
}

// Serialize field type. See "include/loader/serialize.h".
Expect<void>
Serializer::serializeType(const AST::FieldType &Type,
                          std::vector<uint8_t> &OutVec) const noexcept {
  // Field type: storage type + valmut
  EXPECTED_TRY(
      serializeValType(Type.getStorageType(), ASTNodeAttr::Type_Rec, OutVec));
  OutVec.push_back(static_cast<uint8_t>(Type.getValMut()));
  return {};
}

// Serialize field type. See "include/loader/serialize.h".
Expect<void>
Serializer::serializeType(const AST::TagType &Type,
                          std::vector<uint8_t> &OutVec) const noexcept {
  // Tag type: 0x00 + typeIdx
  OutVec.push_back(static_cast<uint8_t>(0x00U));
  serializeU32(Type.getTypeIdx(), OutVec);
  return {};
}

} // namespace Loader
} // namespace WasmEdge
