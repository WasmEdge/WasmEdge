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
  case TypeCode::TypeIndex:
    if (unlikely(!Conf.hasProposal(Proposal::FunctionReferences))) {
      return logNeedProposal(ErrCode::Value::MalformedRefType,
                             Proposal::FunctionReferences, From);
    }
    serializeS33(static_cast<int64_t>(Type.getTypeIndex()), OutVec);
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
  //       |0x02 + min:u32 (shared)
  //       |0x03 + min:u32 + max:u32 (shared)
  uint8_t Flag = 0;
  if (Lim.isShared()) {
    Flag = 0x02U;
  }
  if (Lim.hasMax()) {
    Flag |= 0x01U;
  }
  if (unlikely(static_cast<AST::Limit::LimitType>(Flag) ==
               AST::Limit::LimitType::SharedNoMax)) {
    if (Conf.hasProposal(Proposal::Threads)) {
      return logSerializeError(ErrCode::Value::SharedMemoryNoMax,
                               ASTNodeAttr::Type_Limit);
    }
    return logSerializeError(ErrCode::Value::IntegerTooLarge,
                             ASTNodeAttr::Type_Limit);
  }
  OutVec.push_back(Flag);
  serializeU32(Lim.getMin(), OutVec);
  if (Lim.hasMax()) {
    serializeU32(Lim.getMax(), OutVec);
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
    if (auto Res =
            serializeType(SType.getCompositeType().getFuncType(), OutVec);
        unlikely(!Res)) {
      return Unexpect(Res);
    }
    break;
  case TypeCode::Array:
  case TypeCode::Struct:
    if (!Conf.hasProposal(Proposal::GC)) {
      return logNeedProposal(ErrCode::Value::MalformedValType, Proposal::GC,
                             ASTNodeAttr::Type_Rec);
    }
    // TODO: GC - Serializer: implementation.
    [[fallthrough]];
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
    if (auto Res = serializeValType(VType, ASTNodeAttr::Type_Function, OutVec);
        unlikely(!Res)) {
      return Unexpect(Res);
    }
  }
  // Return types: vec(valtype).
  if (unlikely(!Conf.hasProposal(Proposal::MultiValue)) &&
      Type.getReturnTypes().size() > 1) {
    return logNeedProposal(ErrCode::Value::MalformedValType,
                           Proposal::MultiValue, ASTNodeAttr::Type_Function);
  }
  serializeU32(static_cast<uint32_t>(Type.getReturnTypes().size()), OutVec);
  for (auto &VType : Type.getReturnTypes()) {
    if (auto Res = serializeValType(VType, ASTNodeAttr::Type_Function, OutVec);
        unlikely(!Res)) {
      return Unexpect(Res);
    }
  }
  return {};
}

// Serialize table type. See "include/loader/serialize.h".
Expect<void>
Serializer::serializeType(const AST::TableType &Type,
                          std::vector<uint8_t> &OutVec) const noexcept {
  // Table type: elemtype:valtype + limit.
  if (auto Res =
          serializeRefType(Type.getRefType(), ASTNodeAttr::Type_Table, OutVec);
      unlikely(!Res)) {
    return Unexpect(Res);
  }
  if (auto Res = serializeLimit(Type.getLimit(), OutVec); !Res) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Type_Table));
    return Unexpect(Res);
  }
  return {};
}

// Serialize memory type. See "include/loader/serialize.h".
Expect<void>
Serializer::serializeType(const AST::MemoryType &Type,
                          std::vector<uint8_t> &OutVec) const noexcept {
  // Memory type: limit.
  if (auto Res = serializeLimit(Type.getLimit(), OutVec); unlikely(!Res)) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Type_Memory));
    return Unexpect(Res);
  }
  return {};
}

// Serialize global type. See "include/loader/serialize.h".
Expect<void>
Serializer::serializeType(const AST::GlobalType &Type,
                          std::vector<uint8_t> &OutVec) const noexcept {
  // Global type: valtype + valmut.
  if (auto Res =
          serializeValType(Type.getValType(), ASTNodeAttr::Type_Global, OutVec);
      unlikely(!Res)) {
    return Unexpect(Res);
  }
  OutVec.push_back(static_cast<uint8_t>(Type.getValMut()));
  return {};
}

} // namespace Loader
} // namespace WasmEdge
