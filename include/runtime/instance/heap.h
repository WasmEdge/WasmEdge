// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//===-- wasmedge/runtime/instance/function.h - Function Instance definition ==//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the function instance definition in store manager.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/type.h"
#include "module.h"
#include "common/types.h"
#include <vector>

namespace WasmEdge {
namespace Runtime {
namespace Instance {

class HeapInstance {
public:
  HeapInstance(const AST::ArrayType &Type, uint32_t Size,
               const ModuleInstance *ModInst, uint32_t TypeIdx)
      : Type(Type), Payload(Size * Type.getFieldType().getStorageType().size()),
        ModInst(ModInst), TypeIdx(TypeIdx) {}
  HeapInstance(const AST::ArrayType &Type, uint32_t Size,
               Span<const Byte> InitPayload, const ModuleInstance *ModInst,
               uint32_t TypeIdx)
      : Type(Type), Payload(InitPayload.begin(), InitPayload.end()),
        ModInst(ModInst), TypeIdx(TypeIdx) {
    assuming(Size * Type.getFieldType().getStorageType().size() ==
             InitPayload.size());
  }
  HeapInstance(const AST::StructType &Type, const ModuleInstance *ModInst,
               uint32_t TypeIdx)
      : Type(Type), Payload(Type.size()), ModInst(ModInst), TypeIdx(TypeIdx) {}
  HeapInstance(uint32_t Value)
      : Type(Value), ModInst(nullptr), TypeIdx(UINT32_MAX) {}

  bool equal(const HeapInstance *Other) const {
    assuming(Other != nullptr);
    if (TypeIdx == UINT32_MAX) {
      return Other->TypeIdx == UINT32_MAX &&
             getI31(true) == Other->getI31(true);
    }
    return this == Other;
  }

  bool isI31() const { return std::holds_alternative<uint32_t>(Type); }

  bool isStruct() const {
    return std::holds_alternative<AST::StructType>(Type);
  }

  bool isArray() const { return std::holds_alternative<AST::ArrayType>(Type); }

  const ModuleInstance* getModInst() const {
    return ModInst;
  }

  uint32_t getTypeIdx() const {
    return TypeIdx;
  }

  // Array method
  bool checkArrayIdx(uint32_t Idx) const {
    auto FieldSize =
        asType<AST::ArrayType>().getFieldType().getStorageType().size();
    return FieldSize * Idx < Payload.size();
  }
  ValVariant getArrayValue(uint32_t Idx, bool IsSigned) const {
    const auto &StorageType =
        asType<AST::ArrayType>().getFieldType().getStorageType();
    auto FieldSize = StorageType.size();
    auto Offset = FieldSize * Idx;
    return unpackPayload(Offset, StorageType, IsSigned);
  }
  void setArrayValue(ValVariant &&Val, uint32_t Idx) {
    const auto &StorageType =
        asType<AST::ArrayType>().getFieldType().getStorageType();
    auto FieldSize = StorageType.size();
    auto Offset = FieldSize * Idx;
    packPayload(Offset, StorageType, std::move(Val));
  }
  uint32_t arrayLength() const {
    auto FieldSize =
        asType<AST::ArrayType>().getFieldType().getStorageType().size();
    assuming(Payload.size() % FieldSize == 0);
    return Payload.size() / FieldSize;
  }

  // Struct method
  ValVariant getStructValue(uint32_t Idx, bool IsSigned) const {
    const auto &StructType = asType<AST::StructType>();
    const auto &FieldType = StructType.getContent()[Idx];
    const auto Offset = StructType.getOffset()[Idx];
    return unpackPayload(Offset, FieldType.getStorageType(), IsSigned);
  }

  void setStructValue(ValVariant &&Value, uint32_t Idx) {
    const auto &StructType = asType<AST::StructType>();
    const auto &FieldType = StructType.getContent()[Idx];
    const auto Offset = StructType.getOffset()[Idx];
    packPayload(Offset, FieldType.getStorageType(), std::move(Value));
  }

  // I31 method
  uint32_t getI31(bool IsSigned) const {
    (void)ModInst;
    auto Value = asType<uint32_t>();
    const uint32_t HIGHEST_MASK = 1U << 31;
    const uint32_t SIGNED_MASK = 1U << 30;
    if (IsSigned) {
      uint32_t Signed = Value & SIGNED_MASK;
      return (Signed) ? (Value | HIGHEST_MASK) : (Value & (~HIGHEST_MASK));
    } else {
      return Value & (~HIGHEST_MASK);
    }
  }

  uint32_t payloadSize() const { return Payload.size(); }

private:
  template <typename T> const T &asType() const { return std::get<T>(Type); }
  const AST::ArrayType &getArrayType() const {
    return std::get<AST::ArrayType>(Type);
  }
  template <typename T> void setPayload(uint32_t Offset, T Value) {
    auto Size = sizeof(T);
    assuming(Offset + Size <= Payload.size());
    std::memcpy(&Payload[Offset], &Value, sizeof(T));
  }
  template <typename T> T getPayload(uint32_t Offset) const {
    auto Size = sizeof(T);
    assuming(Offset + Size <= Payload.size());
    T Value;
    std::memcpy(&Value, &Payload[Offset], sizeof(T));
    return Value;
  }
  void packPayload(uint32_t Offset, const AST::StorageType &StorageType,
                   ValVariant &&Value) {
    if (StorageType.isValType()) {
      auto ValType = StorageType.asValType();
      switch (ValType.getTypeCode()) {
      case ValTypeCode::I32:
        setPayload(Offset, Value.get<int32_t>());
        return;
      case ValTypeCode::I64:
        setPayload(Offset, Value.get<int64_t>());
        return;
      case ValTypeCode::F32:
        setPayload(Offset, Value.get<float>());
        return;
      case ValTypeCode::F64:
        setPayload(Offset, Value.get<double>());
        return;
      case ValTypeCode::V128:
        setPayload(Offset, Value.get<int128_t>());
        return;
      case ValTypeCode::Ref:
        setPayload(Offset, Value.get<RefVariant>());
        return;
      case ValTypeCode::RefNull:
        setPayload(Offset, Value.get<RefVariant>());
        return;
      }
    } else {
      switch (StorageType.asPackedType()) {
      case PackedType::I16: {
        int16_t PackedValue = Value.get<int32_t>();
        setPayload(Offset, PackedValue);
        return;
      }
      case PackedType::I8: {
        int8_t PackedValue = Value.get<int32_t>();
        setPayload(Offset, PackedValue);
        return;
      }
      }
    }
  }
  ValVariant unpackPayload(uint32_t Offset, const AST::StorageType &StorageType,
                           bool IsSigned) const {
    if (StorageType.isValType()) {
      auto ValType = StorageType.asValType();
      switch (ValType.getTypeCode()) {
      case ValTypeCode::I32:
        return getPayload<int32_t>(Offset);
      case ValTypeCode::I64:
        return getPayload<int64_t>(Offset);
      case ValTypeCode::F32:
        return getPayload<float>(Offset);
      case ValTypeCode::F64:
        return getPayload<double>(Offset);
      case ValTypeCode::V128:
        return getPayload<int128_t>(Offset);
      case ValTypeCode::Ref:
        return getPayload<RefVariant>(Offset);
      case ValTypeCode::RefNull:
        return getPayload<RefVariant>(Offset);
      }
    } else {
      switch (StorageType.asPackedType()) {
      case PackedType::I16: {
        if (IsSigned) {
          return (int32_t)getPayload<int16_t>(Offset);
        } else {
          return (uint32_t)getPayload<uint16_t>(Offset);
        }
      }
      case PackedType::I8: {
        if (IsSigned) {
          return (int32_t)getPayload<int8_t>(Offset);
        } else {
          return (uint32_t)getPayload<uint8_t>(Offset);
        }
      }
      }
    }
  }
  std::variant<AST::ArrayType, AST::StructType, uint32_t> Type;
  std::vector<Byte> Payload;
  const ModuleInstance *ModInst;
  // if uint32_t_MAX, it means i31
  uint32_t TypeIdx;
};

} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge