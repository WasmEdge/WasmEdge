// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "loader/serialize.h"

namespace WasmEdge {
namespace Loader {

// Serialize heap type. See "include/loader/serialize.h".
void Serializer::serializeHeapType(
    const ValType &Type, std::vector<uint8_t> &OutVec) const noexcept {
  TypeCode Code = Type.getHeapTypeCode();
  switch (Code) {
  case TypeCode::ExternRef:
  case TypeCode::FuncRef:
  case TypeCode::NullFuncRef:
  case TypeCode::NullExternRef:
  case TypeCode::NullRef:
  case TypeCode::AnyRef:
  case TypeCode::EqRef:
  case TypeCode::I31Ref:
  case TypeCode::StructRef:
  case TypeCode::ArrayRef:
  case TypeCode::NullExnRef:
  case TypeCode::ExnRef:
    OutVec.push_back(static_cast<uint8_t>(Code));
    return;
  case TypeCode::TypeIndex:
    serializeS33(static_cast<int64_t>(Type.getTypeIndex()), OutVec);
    return;
  default:
    assumingUnreachable();
  }
}

// Serialize reference type. See "include/loader/serialize.h".
void Serializer::serializeRefType(const ValType &Type,
                                  std::vector<uint8_t> &OutVec) const noexcept {
  TypeCode Code = Type.getCode();
  switch (Code) {
  case TypeCode::Ref:
    OutVec.push_back(static_cast<uint8_t>(Code));
    return serializeHeapType(Type, OutVec);
  case TypeCode::RefNull:
    // A nullable reference to an absolute heap type has a single byte
    // shorthand, which is the canonical encoding.
    if (!Type.isAbsHeapType()) {
      OutVec.push_back(static_cast<uint8_t>(Code));
    }
    return serializeHeapType(Type, OutVec);
  default:
    assumingUnreachable();
  }
}

// Serialize value type. See "include/loader/serialize.h".
void Serializer::serializeValType(const ValType &Type,
                                  std::vector<uint8_t> &OutVec) const noexcept {
  TypeCode Code = Type.getCode();
  switch (Code) {
  case TypeCode::I32:
  case TypeCode::I64:
  case TypeCode::F32:
  case TypeCode::F64:
  case TypeCode::I8:
  case TypeCode::I16:
  case TypeCode::V128:
    OutVec.push_back(static_cast<uint8_t>(Code));
    return;
  case TypeCode::Ref:
  case TypeCode::RefNull:
    return serializeRefType(Type, OutVec);
  default:
    assumingUnreachable();
  }
}

// Serialize limit. See "include/loader/serialize.h".
void Serializer::serializeLimit(const AST::Limit &Lim,
                                std::vector<uint8_t> &OutVec) const noexcept {
  // Limit: 0x00 + min:u32
  //       |0x01 + min:u32 + max:u32
  //       |0x02 + min:u32 (shared, invalid)
  //       |0x03 + min:u32 + max:u32 (shared)
  //       |0x04 + min:u64
  //       |0x05 + min:u64 + max:u64
  //       |0x06 + min:u64 (shared, invalid)
  //       |0x07 + min:u64 + max:u64 (shared)
  //
  // Minimal LEB128 makes the u32 and u64 forms of an in-range value identical.
  uint8_t Flag = 0;
  if (Lim.is64()) {
    Flag |= 0x04U;
  }
  if (Lim.isShared()) {
    Flag |= 0x02U;
  }
  if (Lim.hasMax()) {
    Flag |= 0x01U;
  }
  OutVec.push_back(Flag);
  serializeU64(Lim.getMin(), OutVec);
  if (Lim.hasMax()) {
    serializeU64(Lim.getMax(), OutVec);
  }
}

// Serialize sub type. See "include/loader/serialize.h".
void Serializer::serializeType(const AST::SubType &SType,
                               std::vector<uint8_t> &OutVec) const noexcept {
  // Sub type: vec(typeidx). A final sub type with no super types is
  // canonically a bare composite type.
  if (SType.getSuperTypeIndices().size() > 0 || !SType.isFinal()) {
    assuming(SType.getSuperTypeIndices().size() <= UINT32_MAX);
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
    serializeType(SType.getCompositeType().getFuncType(), OutVec);
    return;
  case TypeCode::Array:
    assuming(!SType.getCompositeType().getFieldTypes().empty());
    serializeType(SType.getCompositeType().getFieldTypes().front(), OutVec);
    return;
  case TypeCode::Struct: {
    // Struct type: vec(fieldtype)
    const auto &FTypes = SType.getCompositeType().getFieldTypes();
    assuming(FTypes.size() <= UINT32_MAX);
    serializeU32(static_cast<uint32_t>(FTypes.size()), OutVec);
    for (const auto &FType : FTypes) {
      serializeType(FType, OutVec);
    }
    return;
  }
  default:
    assumingUnreachable();
  }
}

// Serialize function type. See "include/loader/serialize.h".
void Serializer::serializeType(const AST::FunctionType &Type,
                               std::vector<uint8_t> &OutVec) const noexcept {
  // Function type: paramtypes:vec(valtype) + returntypes:vec(valtype).
  assuming(Type.getParamTypes().size() <= UINT32_MAX);
  assuming(Type.getReturnTypes().size() <= UINT32_MAX);
  // Param types: vec(valtype).
  serializeU32(static_cast<uint32_t>(Type.getParamTypes().size()), OutVec);
  for (auto &VType : Type.getParamTypes()) {
    serializeValType(VType, OutVec);
  }
  // Return types: vec(valtype).
  serializeU32(static_cast<uint32_t>(Type.getReturnTypes().size()), OutVec);
  for (auto &VType : Type.getReturnTypes()) {
    serializeValType(VType, OutVec);
  }
}

// Serialize table type. See "include/loader/serialize.h".
void Serializer::serializeType(const AST::TableType &Type,
                               std::vector<uint8_t> &OutVec) const noexcept {
  // Table type: elemtype:reftype + limit.
  serializeRefType(Type.getRefType(), OutVec);
  serializeLimit(Type.getLimit(), OutVec);
}

// Serialize memory type. See "include/loader/serialize.h".
void Serializer::serializeType(const AST::MemoryType &Type,
                               std::vector<uint8_t> &OutVec) const noexcept {
  // Memory type: limit.
  serializeLimit(Type.getLimit(), OutVec);
}

// Serialize global type. See "include/loader/serialize.h".
void Serializer::serializeType(const AST::GlobalType &Type,
                               std::vector<uint8_t> &OutVec) const noexcept {
  // Global type: valtype + valmut.
  serializeValType(Type.getValType(), OutVec);
  OutVec.push_back(static_cast<uint8_t>(Type.getValMut()));
}

// Serialize field type. See "include/loader/serialize.h".
void Serializer::serializeType(const AST::FieldType &Type,
                               std::vector<uint8_t> &OutVec) const noexcept {
  // Field type: storage type + valmut
  serializeValType(Type.getStorageType(), OutVec);
  OutVec.push_back(static_cast<uint8_t>(Type.getValMut()));
}

// Serialize tag type. See "include/loader/serialize.h".
void Serializer::serializeType(const AST::TagType &Type,
                               std::vector<uint8_t> &OutVec) const noexcept {
  // Tag type: 0x00 + typeIdx
  OutVec.push_back(0x00U);
  serializeU32(Type.getTypeIdx(), OutVec);
}

} // namespace Loader
} // namespace WasmEdge
