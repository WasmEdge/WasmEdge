// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/validator/component_value_decode.h - Value decoding ------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the decoder of the binary `val(t)` payload of a value
/// definition. The validator decodes each payload while checking it against
/// the declared type and caches the decoded value on the AST node, which
/// instantiation later consumes.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/component/type.h"
#include "common/errcode.h"
#include "common/types.h"

#include <cmath>
#include <cstring>
#include <string>
#include <utility>

namespace WasmEdge {
namespace Validator {

using AST::Component::DefValType;
using AST::Component::PrimValType;

/// Decoder over one value definition's payload bytes. The resolver maps a
/// type index of the enclosing component's type index space to its defined
/// value type (nullptr when unknown or not a defined value type). The whole
/// payload must be consumed exactly.
template <typename ResolverT> class ValueDecoder {
public:
  ValueDecoder(Span<const Byte> D, ResolverT &&R) noexcept
      : Data(D), Resolve(std::forward<ResolverT>(R)) {}

  Expect<ComponentValVariant> decode(const ComponentValType &Ty) noexcept {
    EXPECTED_TRY(auto V, decodeVal(Ty));
    if (Off != Data.size()) {
      return Unexpect(ErrCode::Value::ComponentMalformedValue);
    }
    return V;
  }

private:
  Expect<Byte> readByte() noexcept {
    if (Off >= Data.size()) {
      return Unexpect(ErrCode::Value::ComponentMalformedValue);
    }
    return Data[Off++];
  }

  Expect<uint64_t> readULEB(uint32_t MaxBits) noexcept {
    uint64_t Result = 0;
    uint32_t Shift = 0;
    while (true) {
      EXPECTED_TRY(auto B, readByte());
      if (Shift + 7 > MaxBits &&
          (B >> (MaxBits > Shift ? MaxBits - Shift : 0)) != 0) {
        return Unexpect(ErrCode::Value::ComponentMalformedValue);
      }
      Result |= static_cast<uint64_t>(B & 0x7FU) << Shift;
      if ((B & 0x80U) == 0) {
        return Result;
      }
      Shift += 7;
      if (Shift >= MaxBits + 7) {
        return Unexpect(ErrCode::Value::ComponentMalformedValue);
      }
    }
  }

  Expect<int64_t> readSLEB(uint32_t MaxBits) noexcept {
    int64_t Result = 0;
    uint32_t Shift = 0;
    while (true) {
      EXPECTED_TRY(auto B, readByte());
      Result |= static_cast<int64_t>(static_cast<uint64_t>(B & 0x7FU)) << Shift;
      Shift += 7;
      if ((B & 0x80U) == 0) {
        if (Shift < 64 && (B & 0x40U) != 0) {
          Result |= -(INT64_C(1) << Shift);
        }
        return Result;
      }
      if (Shift >= MaxBits + 7) {
        return Unexpect(ErrCode::Value::ComponentMalformedValue);
      }
    }
  }

  // NOLINTNEXTLINE(misc-no-recursion)
  Expect<ComponentValVariant> decodeVal(const ComponentValType &Ty) noexcept {
    if (Ty.isPrimValType()) {
      return decodePrim(static_cast<PrimValType>(Ty.getCode()));
    }
    const DefValType *D = Resolve(Ty.getTypeIndex());
    if (D == nullptr) {
      return Unexpect(ErrCode::Value::ComponentMalformedValue);
    }
    return decodeDef(*D);
  }

  Expect<ComponentValVariant> decodePrim(PrimValType P) noexcept {
    switch (P) {
    case PrimValType::Bool: {
      EXPECTED_TRY(auto B, readByte());
      if (B > 1) {
        return Unexpect(ErrCode::Value::ComponentMalformedValue);
      }
      return ComponentValVariant{B != 0};
    }
    case PrimValType::U8: {
      EXPECTED_TRY(auto B, readByte());
      return ComponentValVariant{static_cast<uint8_t>(B)};
    }
    case PrimValType::S8: {
      EXPECTED_TRY(auto B, readByte());
      return ComponentValVariant{static_cast<int8_t>(B)};
    }
    case PrimValType::U16: {
      EXPECTED_TRY(auto V, readULEB(16));
      return ComponentValVariant{static_cast<uint16_t>(V)};
    }
    case PrimValType::S16: {
      EXPECTED_TRY(auto V, readSLEB(16));
      return ComponentValVariant{static_cast<int16_t>(V)};
    }
    case PrimValType::U32: {
      EXPECTED_TRY(auto V, readULEB(32));
      return ComponentValVariant{static_cast<uint32_t>(V)};
    }
    case PrimValType::S32: {
      EXPECTED_TRY(auto V, readSLEB(32));
      return ComponentValVariant{static_cast<int32_t>(V)};
    }
    case PrimValType::U64: {
      EXPECTED_TRY(auto V, readULEB(64));
      return ComponentValVariant{V};
    }
    case PrimValType::S64: {
      EXPECTED_TRY(auto V, readSLEB(64));
      return ComponentValVariant{V};
    }
    case PrimValType::F32: {
      uint32_t Bits = 0;
      for (uint32_t I = 0; I < 4; ++I) {
        EXPECTED_TRY(auto B, readByte());
        Bits |= static_cast<uint32_t>(B) << (I * 8);
      }
      float F;
      std::memcpy(&F, &Bits, sizeof(F));
      if (std::isnan(F) && Bits != UINT32_C(0x7FC00000)) {
        return Unexpect(ErrCode::Value::ComponentMalformedValue);
      }
      return ComponentValVariant{F};
    }
    case PrimValType::F64: {
      uint64_t Bits = 0;
      for (uint32_t I = 0; I < 8; ++I) {
        EXPECTED_TRY(auto B, readByte());
        Bits |= static_cast<uint64_t>(B) << (I * 8);
      }
      double D;
      std::memcpy(&D, &Bits, sizeof(D));
      if (std::isnan(D) && Bits != UINT64_C(0x7FF8000000000000)) {
        return Unexpect(ErrCode::Value::ComponentMalformedValue);
      }
      return ComponentValVariant{D};
    }
    case PrimValType::Char: {
      EXPECTED_TRY(auto CP, readUtf8Scalar());
      return ComponentValVariant{CP};
    }
    case PrimValType::String: {
      EXPECTED_TRY(auto Len, readULEB(32));
      std::string S;
      S.reserve(static_cast<size_t>(Len));
      for (uint64_t I = 0; I < Len; ++I) {
        EXPECTED_TRY(auto B, readByte());
        S.push_back(static_cast<char>(B));
      }
      return ComponentValVariant{std::move(S)};
    }
    default:
      return Unexpect(ErrCode::Value::ComponentMalformedValue);
    }
  }

  Expect<uint32_t> readUtf8Scalar() noexcept {
    EXPECTED_TRY(auto B0, readByte());
    uint32_t CP = 0;
    uint32_t Tail = 0;
    if (B0 < 0x80U) {
      return static_cast<uint32_t>(B0);
    }
    if ((B0 & 0xE0U) == 0xC0U) {
      CP = B0 & 0x1FU;
      Tail = 1;
    } else if ((B0 & 0xF0U) == 0xE0U) {
      CP = B0 & 0x0FU;
      Tail = 2;
    } else if ((B0 & 0xF8U) == 0xF0U) {
      CP = B0 & 0x07U;
      Tail = 3;
    } else {
      return Unexpect(ErrCode::Value::ComponentMalformedValue);
    }
    for (uint32_t I = 0; I < Tail; ++I) {
      EXPECTED_TRY(auto B, readByte());
      if ((B & 0xC0U) != 0x80U) {
        return Unexpect(ErrCode::Value::ComponentMalformedValue);
      }
      CP = (CP << 6) | (B & 0x3FU);
    }
    if (CP >= 0x110000U || (CP >= 0xD800U && CP <= 0xDFFFU)) {
      return Unexpect(ErrCode::Value::ComponentMalformedValue);
    }
    return CP;
  }

  // NOLINTNEXTLINE(misc-no-recursion)
  Expect<ComponentValVariant> decodeDef(const DefValType &D) noexcept {
    if (D.isPrimValType()) {
      return decodePrim(D.getPrimValType());
    }
    if (D.isRecordTy()) {
      RecordVal R;
      for (const auto &LT : D.getRecord().LabelTypes) {
        EXPECTED_TRY(auto V, decodeVal(LT.getValType()));
        R.Fields.emplace_back(std::string(LT.getLabel()), std::move(V));
      }
      return makeComponentVal(std::move(R));
    }
    if (D.isVariantTy()) {
      const auto &Cases = D.getVariant().Cases;
      EXPECTED_TRY(auto Idx, readULEB(32));
      if (Idx >= Cases.size()) {
        return Unexpect(ErrCode::Value::ComponentMalformedValue);
      }
      VariantVal V;
      V.Case = static_cast<uint32_t>(Idx);
      V.Label = Cases[V.Case].first;
      if (Cases[V.Case].second.has_value()) {
        EXPECTED_TRY(auto P, decodeVal(*Cases[V.Case].second));
        V.Payload = std::move(P);
      }
      return makeComponentVal(std::move(V));
    }
    if (D.isListTy()) {
      const auto &L = D.getList();
      ListVal LV;
      uint64_t Count = 0;
      if (L.Len.has_value()) {
        Count = *L.Len;
      } else {
        EXPECTED_TRY(Count, readULEB(32));
      }
      for (uint64_t I = 0; I < Count; ++I) {
        EXPECTED_TRY(auto V, decodeVal(L.ValTy));
        LV.Elements.push_back(std::move(V));
      }
      return makeComponentVal(std::move(LV));
    }
    if (D.isTupleTy()) {
      TupleVal T;
      for (const auto &Ty : D.getTuple().Types) {
        EXPECTED_TRY(auto V, decodeVal(Ty));
        T.Values.push_back(std::move(V));
      }
      return makeComponentVal(std::move(T));
    }
    if (D.isFlagsTy()) {
      const auto &Labels = D.getFlags().Labels;
      FlagsVal F;
      F.Bits.resize(Labels.size(), false);
      const size_t Bytes = (Labels.size() + 7) / 8;
      for (size_t I = 0; I < Bytes; ++I) {
        EXPECTED_TRY(auto B, readByte());
        for (size_t Bit = 0; Bit < 8; ++Bit) {
          const size_t Pos = I * 8 + Bit;
          if (Pos < Labels.size() && (B & (1U << Bit)) != 0) {
            F.Bits[Pos] = true;
          }
        }
      }
      return makeComponentVal(std::move(F));
    }
    if (D.isEnumTy()) {
      const auto &Labels = D.getEnum().Labels;
      EXPECTED_TRY(auto Idx, readULEB(32));
      if (Idx >= Labels.size()) {
        return Unexpect(ErrCode::Value::ComponentMalformedValue);
      }
      return makeComponentVal(EnumVal{static_cast<uint32_t>(Idx), Labels[Idx]});
    }
    if (D.isOptionTy()) {
      EXPECTED_TRY(auto Disc, readByte());
      OptionVal O;
      if (Disc == 1) {
        EXPECTED_TRY(auto V, decodeVal(D.getOption().ValTy));
        O.Value = std::move(V);
      } else if (Disc != 0) {
        return Unexpect(ErrCode::Value::ComponentMalformedValue);
      }
      return makeComponentVal(std::move(O));
    }
    if (D.isResultTy()) {
      EXPECTED_TRY(auto Disc, readByte());
      if (Disc > 1) {
        return Unexpect(ErrCode::Value::ComponentMalformedValue);
      }
      ResultVal R;
      R.IsOk = (Disc == 0);
      const auto &PT = R.IsOk ? D.getResult().ValTy : D.getResult().ErrTy;
      if (PT.has_value()) {
        EXPECTED_TRY(auto V, decodeVal(*PT));
        R.Payload = std::move(V);
      }
      return makeComponentVal(std::move(R));
    }
    // Handles, streams, and futures cannot appear in value definitions.
    return Unexpect(ErrCode::Value::ComponentMalformedValue);
  }

  Span<const Byte> Data;
  ResolverT Resolve;
  size_t Off = 0;
};

} // namespace Validator
} // namespace WasmEdge
