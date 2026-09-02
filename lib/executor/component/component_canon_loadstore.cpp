// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "executor/component/canonical_abi.h"

#include "common/component_valtype.h"
#include "common/component_variant.h"
#include "common/errinfo.h"
#include "common/spdlog.h"
#include "executor/component/executor.h"
#include "executor/executor.h"
#include "runtime/component/canonopt.h"
#include "runtime/component/taskmgr.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace std::literals;

namespace WasmEdge {
namespace Executor {
namespace Component {

namespace {
// The position of Label among the labels of an enum or flags, if any.
std::optional<uint32_t> labelIndex(Span<const std::string> Labels,
                                   std::string_view Label) noexcept {
  for (size_t I = 0; I < Labels.size(); ++I) {
    if (Labels[I] == Label) {
      return static_cast<uint32_t>(I);
    }
  }
  return std::nullopt;
}
} // namespace

uint32_t LiftLowerContext::resolveVariantCase(
    const VariantVal &V, const AST::Component::VariantTy &T) noexcept {
  if (!V.Label.empty()) {
    for (size_t I = 0; I < T.Cases.size(); ++I) {
      if (T.Cases[I].first == V.Label) {
        return static_cast<uint32_t>(I);
      }
    }
  }
  return V.Case;
}

uint32_t
LiftLowerContext::resolveEnumCase(const EnumVal &E,
                                  const AST::Component::EnumTy &T) noexcept {
  if (!E.Label.empty()) {
    return labelIndex(T.Labels, E.Label).value_or(E.Case);
  }
  return E.Case;
}

uint64_t
LiftLowerContext::packFlags(const FlagsVal &F,
                            const AST::Component::FlagsTy &T) noexcept {
  uint64_t Packed = 0;
  if (F.Bits.empty() && !F.SetLabels.empty()) {
    for (const auto &Label : F.SetLabels) {
      if (const auto I = labelIndex(T.Labels, Label); I.has_value()) {
        Packed |= (1ull << *I);
      }
    }
    return Packed;
  }
  for (size_t I = 0; I < F.Bits.size(); ++I) {
    if (F.Bits[I]) {
      Packed |= (1ull << I);
    }
  }
  return Packed;
}

float LiftLowerContext::canonicalizeNaN32(float F) noexcept {
  if (std::isnan(F)) {
    float Canonical = 0.f;
    std::memcpy(&Canonical, &CanonicalF32NaNBits, sizeof(Canonical));
    return Canonical;
  }
  return F;
}

double LiftLowerContext::canonicalizeNaN64(double F) noexcept {
  if (std::isnan(F)) {
    double Canonical = 0.;
    std::memcpy(&Canonical, &CanonicalF64NaNBits, sizeof(Canonical));
    return Canonical;
  }
  return F;
}

namespace {

// loadValue<T,N> with a runtime width, kept out of EXPECTED_TRY.
template <typename T>
Expect<void> loadN(Runtime::Instance::MemoryInstance &Mem, uint32_t Bytes,
                   T &Val, uint64_t Off) noexcept {
  switch (Bytes) {
  case 1:
    return Mem.template loadValue<T, 1>(Val, Off);
  case 2:
    return Mem.template loadValue<T, 2>(Val, Off);
  case 3:
    return Mem.template loadValue<T, 3>(Val, Off);
  case 4:
    return Mem.template loadValue<T, 4>(Val, Off);
  default:
    assumingUnreachable();
  }
}

template <typename T>
Expect<void> storeN(Runtime::Instance::MemoryInstance &Mem, uint32_t Bytes,
                    T Val, uint64_t Off) noexcept {
  switch (Bytes) {
  case 1:
    return Mem.template storeValue<T, 1>(Val, Off);
  case 2:
    return Mem.template storeValue<T, 2>(Val, Off);
  case 3:
    return Mem.template storeValue<T, 3>(Val, Off);
  case 4:
    return Mem.template storeValue<T, 4>(Val, Off);
  default:
    assumingUnreachable();
  }
}
} // namespace

// Trap with a diagnostic naming the offending region.
Expect<uint64_t> LiftLowerContext::loadPtr(uint64_t Ptr) const noexcept {
  assuming(Mem != nullptr);
  if (isMemory64()) {
    uint64_t V = 0;
    EXPECTED_TRY(Mem->loadValue<uint64_t>(V, Ptr));
    return V;
  }
  uint32_t V = 0;
  EXPECTED_TRY(Mem->loadValue<uint32_t>(V, Ptr));
  return static_cast<uint64_t>(V);
}

Expect<void> LiftLowerContext::storePtr(uint64_t V,
                                        uint64_t Ptr) const noexcept {
  assuming(Mem != nullptr);
  if (isMemory64()) {
    return Mem->storeValue<uint64_t>(V, Ptr);
  }
  return Mem->storeValue<uint32_t>(static_cast<uint32_t>(V), Ptr);
}

Expect<void> LiftLowerContext::trapMemoryOOB(uint64_t Ptr,
                                             uint64_t Len) const noexcept {
  spdlog::error(ErrCode::Value::MemoryOutOfBounds);
  spdlog::error(ErrInfo::InfoBoundary(
      Ptr, Len,
      getMemory() != nullptr ? getMemory()->getPageSize() *
                                   Runtime::Instance::MemoryInstance::kPageSize
                             : 0));
  return Unexpect(ErrCode::Value::MemoryOutOfBounds);
}

Expect<void> LiftLowerContext::trapDataInvalid(std::string_view Detail,
                                               ErrCode::Value Code) noexcept {
  spdlog::error(Code);
  spdlog::error("    canonical ABI: {}"sv, Detail);
  return Unexpect(Code);
}

Expect<void> LiftLowerContext::validateUSV(uint32_t Value) noexcept {
  if (Value >= 0x110000U) {
    return trapDataInvalid("code point out of range",
                           ErrCode::Value::ComponentCharInvalid);
  }
  if (Value >= 0xD800U && Value <= 0xDFFFU) {
    return trapDataInvalid("surrogate code point",
                           ErrCode::Value::ComponentCharInvalid);
  }
  return {};
}

void LiftLowerContext::assumeValidUSV(uint32_t Value) noexcept {
  assuming(Value < 0x110000U && (Value < 0xD800U || Value > 0xDFFFU));
}

Expect<ComponentValVariant>
LiftLowerContext::load(uint64_t Ptr, PrimValType T) const noexcept {
  assuming(getMemory() != nullptr);
  switch (T) {
  case PrimValType::Bool: {
    // 0 -> false, else true.
    uint32_t I = 0;
    EXPECTED_TRY(loadN<uint32_t>(*getMemory(), 1, I, Ptr));
    return ComponentValVariant{I != 0};
  }
  case PrimValType::S8: {
    int32_t V = 0;
    EXPECTED_TRY(loadN<int32_t>(*getMemory(), 1, V, Ptr));
    return ComponentValVariant{static_cast<int8_t>(V)};
  }
  case PrimValType::U8: {
    uint32_t V = 0;
    EXPECTED_TRY(loadN<uint32_t>(*getMemory(), 1, V, Ptr));
    return ComponentValVariant{static_cast<uint8_t>(V)};
  }
  case PrimValType::S16: {
    int32_t V = 0;
    EXPECTED_TRY(loadN<int32_t>(*getMemory(), 2, V, Ptr));
    return ComponentValVariant{static_cast<int16_t>(V)};
  }
  case PrimValType::U16: {
    uint32_t V = 0;
    EXPECTED_TRY(loadN<uint32_t>(*getMemory(), 2, V, Ptr));
    return ComponentValVariant{static_cast<uint16_t>(V)};
  }
  case PrimValType::S32: {
    int32_t V = 0;
    EXPECTED_TRY(getMemory()->loadValue<int32_t>(V, Ptr));
    return ComponentValVariant{V};
  }
  case PrimValType::U32: {
    uint32_t V = 0;
    EXPECTED_TRY(getMemory()->loadValue<uint32_t>(V, Ptr));
    return ComponentValVariant{V};
  }
  case PrimValType::S64: {
    int64_t V = 0;
    EXPECTED_TRY(getMemory()->loadValue<int64_t>(V, Ptr));
    return ComponentValVariant{V};
  }
  case PrimValType::U64: {
    uint64_t V = 0;
    EXPECTED_TRY(getMemory()->loadValue<uint64_t>(V, Ptr));
    return ComponentValVariant{V};
  }
  case PrimValType::F32: {
    // Canonicalize NaN on load.
    float V = 0.f;
    EXPECTED_TRY(getMemory()->loadValue<float>(V, Ptr));
    return ComponentValVariant{canonicalizeNaN32(V)};
  }
  case PrimValType::F64: {
    // Canonicalize NaN on load.
    double V = 0.;
    EXPECTED_TRY(getMemory()->loadValue<double>(V, Ptr));
    return ComponentValVariant{canonicalizeNaN64(V)};
  }
  case PrimValType::Char: {
    uint32_t V = 0;
    EXPECTED_TRY(getMemory()->loadValue<uint32_t>(V, Ptr));
    EXPECTED_TRY(validateUSV(V));
    return ComponentValVariant{V};
  }
  case PrimValType::String: {
    // Read (begin, tagged_code_units), then decode per the encoding.
    EXPECTED_TRY(auto Begin, loadPtr(Ptr));
    EXPECTED_TRY(auto Tagged, loadPtr(Ptr + getPtrSize()));
    EXPECTED_TRY(auto Str, decodeString(Begin, Tagged));
    return ComponentValVariant{std::move(Str)};
  }
  case PrimValType::ErrorContext: {
    uint32_t Idx = 0;
    EXPECTED_TRY(getMemory()->loadValue<uint32_t>(Idx, Ptr));
    return liftErrorContext(Idx);
  }
  default:
    spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
    spdlog::error("    canonical ABI: load of unknown prim 0x{:02x}"sv,
                  static_cast<uint8_t>(T));
    return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
  }
}

Expect<ComponentValVariant>
LiftLowerContext::load(uint64_t Ptr, const ComponentValType &T) const noexcept {
  if (T.isPrimValType()) {
    return load(Ptr, T.getPrimValType());
  }
  const auto Idx = T.getTypeIndex();
  EXPECTED_TRY(const auto *D, getDefValType(Idx));
  return contextOf(Idx).load(Ptr, *D);
}

Expect<ComponentValVariant>
LiftLowerContext::load(uint64_t Ptr,
                       const AST::Component::DefValType &T) const noexcept {
  if (T.isPrimValType()) {
    return load(Ptr, T.getPrimValType());
  }

  if (T.isRecordTy()) {
    RecordVal R;
    uint32_t Off = Ptr;
    for (const auto &F : T.getRecord().LabelTypes) {
      EXPECTED_TRY(auto A, alignment(F.getValType()));
      Off = alignTo(Off, A);
      EXPECTED_TRY(auto V, load(Off, F.getValType()));
      R.Fields.emplace_back(std::string(F.getLabel()), std::move(V));
      EXPECTED_TRY(auto S, elemSize(F.getValType()));
      Off += S;
    }
    return makeComponentVal(std::move(R));
  }

  if (T.isTupleTy()) {
    TupleVal Tu;
    uint32_t Off = Ptr;
    for (const auto &V : T.getTuple().Types) {
      EXPECTED_TRY(auto A, alignment(V));
      Off = alignTo(Off, A);
      EXPECTED_TRY(auto Val, load(Off, V));
      Tu.Values.push_back(std::move(Val));
      EXPECTED_TRY(auto S, elemSize(V));
      Off += S;
    }
    return makeComponentVal(std::move(Tu));
  }

  if (T.isVariantTy()) {
    const auto &VariantType = T.getVariant();
    const uint32_t NumCases = static_cast<uint32_t>(VariantType.Cases.size());
    const uint32_t DiscSize = LiftLowerContext::discriminantSize(NumCases);
    uint32_t Case = 0;
    EXPECTED_TRY(loadN<uint32_t>(*Mem, DiscSize, Case, Ptr));
    if (Case >= NumCases) {
      EXPECTED_TRY(
          trapDataInvalid("invalid variant discriminant",
                          ErrCode::Value::ComponentDiscriminantInvalid));
    }
    VariantVal VV;
    VV.Case = Case;
    if (VariantType.Cases[Case].second.has_value()) {
      EXPECTED_TRY(auto MaxAlign, maxCaseAlignment(VariantType.Cases));
      const uint32_t PayloadOff = alignTo(Ptr + DiscSize, MaxAlign);
      EXPECTED_TRY(auto PV, load(PayloadOff, *VariantType.Cases[Case].second));
      VV.Payload = std::move(PV);
    }
    return makeComponentVal(std::move(VV));
  }

  if (T.isOptionTy()) {
    // option<T> = variant{none(0) | some(T)(1)}: disc 1B.
    uint32_t Disc = 0;
    EXPECTED_TRY(loadN<uint32_t>(*Mem, 1, Disc, Ptr));
    if (Disc >= 2u) {
      EXPECTED_TRY(
          trapDataInvalid("invalid variant discriminant for option",
                          ErrCode::Value::ComponentDiscriminantInvalid));
    }
    OptionVal OV;
    if (Disc == 1u) {
      EXPECTED_TRY(auto A, alignment(T.getOption().ValTy));
      const uint32_t PayloadOff = alignTo(Ptr + 1u, A);
      EXPECTED_TRY(auto PV, load(PayloadOff, T.getOption().ValTy));
      OV.Value = std::move(PV);
    }
    return makeComponentVal(std::move(OV));
  }

  if (T.isResultTy()) {
    // result<T,E> = variant{ok(0)(T?) | err(1)(E?)}: disc 1B.
    const auto &R = T.getResult();
    uint32_t Disc = 0;
    EXPECTED_TRY(loadN<uint32_t>(*Mem, 1, Disc, Ptr));
    if (Disc >= 2u) {
      EXPECTED_TRY(
          trapDataInvalid("invalid variant discriminant for result",
                          ErrCode::Value::ComponentDiscriminantInvalid));
    }
    ResultVal RV;
    RV.IsOk = (Disc == 0u);
    const std::optional<ComponentValType> &PT = RV.IsOk ? R.ValTy : R.ErrTy;
    if (PT.has_value()) {
      uint32_t MaxAlign = 1u;
      if (R.ValTy.has_value()) {
        EXPECTED_TRY(auto A, alignment(*R.ValTy));
        MaxAlign = std::max(MaxAlign, A);
      }
      if (R.ErrTy.has_value()) {
        EXPECTED_TRY(auto A, alignment(*R.ErrTy));
        MaxAlign = std::max(MaxAlign, A);
      }
      const uint32_t PayloadOff = alignTo(Ptr + 1u, MaxAlign);
      EXPECTED_TRY(auto PV, load(PayloadOff, *PT));
      RV.Payload = std::move(PV);
    }
    return makeComponentVal(std::move(RV));
  }

  if (T.isListTy()) {
    // with-len loads len elements in place at Ptr, with no header.
    if (T.getList().Len.has_value()) {
      return loadList(Ptr, *T.getList().Len, T.getList().ValTy);
    }
    EXPECTED_TRY(auto Begin, loadPtr(Ptr));
    EXPECTED_TRY(auto Length, loadPtr(Ptr + getPtrSize()));
    EXPECTED_TRY(auto ElemAlign, alignment(T.getList().ValTy));
    EXPECTED_TRY(auto ElemSz, elemSize(T.getList().ValTy));
    // Trap when length * element size exceeds the maximum.
    uint64_t ByteLen64 =
        static_cast<uint64_t>(Length) * static_cast<uint64_t>(ElemSz);
    if (ByteLen64 > static_cast<uint64_t>(MaxCanonByteLength)) {
      EXPECTED_TRY(trapDataInvalid("list byte length exceeds MAX"));
    }
    if (Begin != alignTo(Begin, ElemAlign)) {
      EXPECTED_TRY(trapDataInvalid("unaligned pointer for list",
                                   ErrCode::Value::ComponentPtrUnaligned));
    }
    const uint32_t ByteLen = static_cast<uint32_t>(ByteLen64);
    if (Length > 0u && !Mem->checkAccessBound(Begin, ByteLen)) {
      EXPECTED_TRY(trapMemoryOOB(Begin, ByteLen));
    }
    ListVal LV;
    LV.Elements.reserve(Length);
    for (uint32_t I = 0; I < Length; ++I) {
      EXPECTED_TRY(auto E, load(Begin + I * ElemSz, T.getList().ValTy));
      LV.Elements.push_back(std::move(E));
    }
    return makeComponentVal(std::move(LV));
  }

  if (T.isMapTy()) {
    EXPECTED_TRY(auto Begin, loadPtr(Ptr));
    EXPECTED_TRY(auto Length, loadPtr(Ptr + getPtrSize()));
    return loadList(Begin, Length, mapEntryType(T.getMap()));
  }

  if (T.isFlagsTy()) {
    const auto &F = T.getFlags();
    const uint32_t Labels = static_cast<uint32_t>(F.Labels.size());
    const uint32_t Bytes = (Labels + 7u) / 8u;
    uint64_t Raw = 0;
    if (Bytes > 0u) {
      // Labels are capped at 32, so at most 4 bytes.
      assuming(Bytes <= 4u);
      uint32_t V = 0;
      EXPECTED_TRY(loadN<uint32_t>(*Mem, Bytes, V, Ptr));
      Raw = V;
    }
    FlagsVal FV;
    FV.Bits.resize(Labels);
    for (uint32_t I = 0; I < Labels; ++I) {
      FV.Bits[I] = ((Raw >> I) & 1ull) != 0ull;
      if (FV.Bits[I]) {
        FV.SetLabels.push_back(F.Labels[I]);
      }
    }
    return makeComponentVal(std::move(FV));
  }

  if (T.isEnumTy()) {
    const uint32_t NumCases = static_cast<uint32_t>(T.getEnum().Labels.size());
    const uint32_t DiscSize = LiftLowerContext::discriminantSize(NumCases);
    uint32_t Case = 0;
    EXPECTED_TRY(loadN<uint32_t>(*Mem, DiscSize, Case, Ptr));
    if (Case >= NumCases) {
      EXPECTED_TRY(
          trapDataInvalid("invalid variant discriminant for enum",
                          ErrCode::Value::ComponentDiscriminantInvalid));
    }
    return makeComponentVal(EnumVal{Case, {}});
  }

  // The value carries the representation; own leaves, borrow stays.
  if (T.isOwnTy()) {
    uint32_t H = 0;
    EXPECTED_TRY(Mem->loadValue<uint32_t>(H, Ptr));
    EXPECTED_TRY(uint64_t Rep, liftHandle(T.getOwn().Idx, H, true));
    return makeComponentVal(OwnVal{Rep});
  }

  if (T.isBorrowTy()) {
    uint32_t H = 0;
    EXPECTED_TRY(Mem->loadValue<uint32_t>(H, Ptr));
    EXPECTED_TRY(uint64_t Rep, liftHandle(T.getBorrow().Idx, H, false));
    return makeComponentVal(BorrowVal{Rep});
  }

  if (T.isStreamTy() || T.isFutureTy()) {
    const bool IsStream = T.isStreamTy();
    uint32_t H = 0;
    EXPECTED_TRY(Mem->loadValue<uint32_t>(H, Ptr));
    EXPECTED_TRY(auto Shared, liftTransmitEnd(IsStream, H));
    return makeComponentVal(StreamFutureVal{std::move(Shared), IsStream});
  }

  spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
  spdlog::error("    canonical ABI: load of gated value type"sv);
  return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
}

Expect<void> LiftLowerContext::store(const ComponentValVariant &V,
                                     PrimValType T,
                                     uint64_t Ptr) const noexcept {
  assuming(getMemory() != nullptr);
  switch (T) {
  case PrimValType::Bool: {
    const uint32_t B = std::get<bool>(V) ? 1u : 0u;
    return storeN<uint32_t>(*getMemory(), 1, B, Ptr);
  }
  case PrimValType::S8: {
    const uint32_t Bits =
        static_cast<uint32_t>(static_cast<uint8_t>(std::get<int8_t>(V)));
    return storeN<uint32_t>(*getMemory(), 1, Bits, Ptr);
  }
  case PrimValType::U8:
    return storeN<uint32_t>(*getMemory(), 1, std::get<uint8_t>(V), Ptr);
  case PrimValType::S16: {
    const uint32_t Bits =
        static_cast<uint32_t>(static_cast<uint16_t>(std::get<int16_t>(V)));
    return storeN<uint32_t>(*getMemory(), 2, Bits, Ptr);
  }
  case PrimValType::U16:
    return storeN<uint32_t>(*getMemory(), 2, std::get<uint16_t>(V), Ptr);
  case PrimValType::S32:
    return getMemory()->storeValue<uint32_t>(
        static_cast<uint32_t>(std::get<int32_t>(V)), Ptr);
  case PrimValType::U32:
    return getMemory()->storeValue<uint32_t>(std::get<uint32_t>(V), Ptr);
  case PrimValType::S64:
    return getMemory()->storeValue<uint64_t>(
        static_cast<uint64_t>(std::get<int64_t>(V)), Ptr);
  case PrimValType::U64:
    return getMemory()->storeValue<uint64_t>(std::get<uint64_t>(V), Ptr);
  case PrimValType::F32:
    // Canonicalize NaN on store.
    return getMemory()->storeValue<float>(canonicalizeNaN32(std::get<float>(V)),
                                          Ptr);
  case PrimValType::F64:
    // Canonicalize NaN on store.
    return getMemory()->storeValue<double>(
        canonicalizeNaN64(std::get<double>(V)), Ptr);
  case PrimValType::Char: {
    const uint32_t I = std::get<uint32_t>(V);
    assumeValidUSV(I);
    return getMemory()->storeValue<uint32_t>(I, Ptr);
  }
  case PrimValType::String: {
    // Encode per the string option, then store (begin, tagged_code_units).
    EXPECTED_TRY(auto Encoded, encodeString(std::get<std::string>(V)));
    EXPECTED_TRY(storePtr(Encoded.first, Ptr));
    EXPECTED_TRY(storePtr(Encoded.second, Ptr + getPtrSize()));
    return {};
  }
  case PrimValType::ErrorContext: {
    EXPECTED_TRY(auto Idx, lowerErrorContext(V));
    return getMemory()->storeValue<uint32_t>(Idx, Ptr);
  }
  default:
    spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
    spdlog::error("    canonical ABI: store of unknown prim 0x{:02x}"sv,
                  static_cast<uint8_t>(T));
    return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
  }
}

Expect<void> LiftLowerContext::store(const ComponentValVariant &V,
                                     const ComponentValType &T,
                                     uint64_t Ptr) const noexcept {
  if (T.isPrimValType()) {
    return store(V, T.getPrimValType(), Ptr);
  }
  const auto Idx = T.getTypeIndex();
  EXPECTED_TRY(const auto *D, getDefValType(Idx));
  return contextOf(Idx).store(V, *D, Ptr);
}

Expect<void> LiftLowerContext::store(const ComponentValVariant &V,
                                     const AST::Component::DefValType &T,
                                     uint64_t Ptr) const noexcept {
  if (T.isPrimValType()) {
    return store(V, T.getPrimValType(), Ptr);
  }

  if (T.isRecordTy()) {
    const auto &Record = getComponentVal<RecordVal>(V);
    const auto &Fields = T.getRecord().LabelTypes;
    assuming(Record.Fields.size() == Fields.size());
    uint32_t Off = Ptr;
    for (size_t I = 0; I < Fields.size(); ++I) {
      EXPECTED_TRY(auto A, alignment(Fields[I].getValType()));
      Off = alignTo(Off, A);
      EXPECTED_TRY(store(Record.Fields[I].second, Fields[I].getValType(), Off));
      EXPECTED_TRY(auto S, elemSize(Fields[I].getValType()));
      Off += S;
    }
    return {};
  }

  if (T.isTupleTy()) {
    const auto &Tuple = getComponentVal<TupleVal>(V);
    const auto &Types = T.getTuple().Types;
    assuming(Tuple.Values.size() == Types.size());
    uint32_t Off = Ptr;
    for (size_t I = 0; I < Types.size(); ++I) {
      EXPECTED_TRY(auto A, alignment(Types[I]));
      Off = alignTo(Off, A);
      EXPECTED_TRY(store(Tuple.Values[I], Types[I], Off));
      EXPECTED_TRY(auto S, elemSize(Types[I]));
      Off += S;
    }
    return {};
  }

  if (T.isVariantTy()) {
    const auto &Variant = getComponentVal<VariantVal>(V);
    const auto &VariantType = T.getVariant();
    const uint32_t Case = resolveVariantCase(Variant, VariantType);
    assuming(Case < VariantType.Cases.size());
    const uint32_t DiscSize = LiftLowerContext::discriminantSize(
        static_cast<uint32_t>(VariantType.Cases.size()));
    EXPECTED_TRY(storeN<uint32_t>(*Mem, DiscSize, Case, Ptr));
    if (VariantType.Cases[Case].second.has_value()) {
      assuming(Variant.Payload.has_value());
      EXPECTED_TRY(auto MaxAlign, maxCaseAlignment(VariantType.Cases));
      const uint32_t PayloadOff = alignTo(Ptr + DiscSize, MaxAlign);
      EXPECTED_TRY(
          store(*Variant.Payload, *VariantType.Cases[Case].second, PayloadOff));
    }
    return {};
  }

  if (T.isOptionTy()) {
    const auto &Option = getComponentVal<OptionVal>(V);
    const uint32_t Disc = Option.Value.has_value() ? 1u : 0u;
    EXPECTED_TRY(storeN<uint32_t>(*Mem, 1, Disc, Ptr));
    if (Option.Value.has_value()) {
      EXPECTED_TRY(auto A, alignment(T.getOption().ValTy));
      const uint32_t PayloadOff = alignTo(Ptr + 1u, A);
      EXPECTED_TRY(store(*Option.Value, T.getOption().ValTy, PayloadOff));
    }
    return {};
  }

  if (T.isResultTy()) {
    const auto &Result = getComponentVal<ResultVal>(V);
    const auto &ResTy = T.getResult();
    const uint32_t Disc = Result.IsOk ? 0u : 1u;
    EXPECTED_TRY(storeN<uint32_t>(*Mem, 1, Disc, Ptr));
    const std::optional<ComponentValType> &PT =
        Result.IsOk ? ResTy.ValTy : ResTy.ErrTy;
    if (PT.has_value()) {
      assuming(Result.Payload.has_value());
      uint32_t MaxAlign = 1u;
      if (ResTy.ValTy.has_value()) {
        EXPECTED_TRY(auto A, alignment(*ResTy.ValTy));
        MaxAlign = std::max(MaxAlign, A);
      }
      if (ResTy.ErrTy.has_value()) {
        EXPECTED_TRY(auto A, alignment(*ResTy.ErrTy));
        MaxAlign = std::max(MaxAlign, A);
      }
      const uint32_t PayloadOff = alignTo(Ptr + 1u, MaxAlign);
      EXPECTED_TRY(store(*Result.Payload, *PT, PayloadOff));
    }
    return {};
  }

  if (T.isListTy()) {
    // with-len stores each element in place at Ptr, with no header.
    if (T.getList().Len.has_value()) {
      const auto &L = T.getList();
      const uint32_t Len = *L.Len;
      const auto &List = getComponentVal<ListVal>(V);
      assuming(List.Elements.size() == Len);
      const auto &ElemT = L.ValTy;
      EXPECTED_TRY(auto ElemSz, elemSize(ElemT));
      for (uint32_t I = 0; I < Len; ++I) {
        EXPECTED_TRY(store(List.Elements[I], ElemT, Ptr + I * ElemSz));
      }
      return {};
    }
    EXPECTED_TRY(auto Range,
                 storeList(getComponentVal<ListVal>(V), T.getList().ValTy));
    EXPECTED_TRY(storePtr(Range.first, Ptr));
    EXPECTED_TRY(storePtr(Range.second, Ptr + getPtrSize()));
    return {};
  }

  if (T.isMapTy()) {
    EXPECTED_TRY(auto Range, storeList(getComponentVal<ListVal>(V),
                                       mapEntryType(T.getMap())));
    EXPECTED_TRY(storePtr(Range.first, Ptr));
    EXPECTED_TRY(storePtr(Range.second, Ptr + getPtrSize()));
    return {};
  }

  if (T.isFlagsTy()) {
    const auto &Flags = getComponentVal<FlagsVal>(V);
    const auto &FlagsType = T.getFlags();
    assuming(Flags.Bits.empty() ||
             Flags.Bits.size() == FlagsType.Labels.size());
    const uint32_t Bytes =
        static_cast<uint32_t>((FlagsType.Labels.size() + 7) / 8);
    const uint64_t Packed = packFlags(Flags, FlagsType);
    if (Bytes > 0u) {
      assuming(Bytes <= 4u);
      EXPECTED_TRY(
          storeN<uint32_t>(*Mem, Bytes, static_cast<uint32_t>(Packed), Ptr));
    }
    return {};
  }

  if (T.isEnumTy()) {
    const auto &Enum = getComponentVal<EnumVal>(V);
    const auto &EnumType = T.getEnum();
    const uint32_t Case = resolveEnumCase(Enum, EnumType);
    assuming(Case < EnumType.Labels.size());
    const uint32_t DiscSize = LiftLowerContext::discriminantSize(
        static_cast<uint32_t>(EnumType.Labels.size()));
    return storeN<uint32_t>(*Mem, DiscSize, Case, Ptr);
  }

  if (T.isOwnTy()) {
    const auto &Own = getComponentVal<OwnVal>(V);
    return Mem->storeValue<uint32_t>(
        lowerHandle(T.getOwn().Idx, Own.Handle, true), Ptr);
  }

  if (T.isBorrowTy()) {
    const auto &Borrow = getComponentVal<BorrowVal>(V);
    return Mem->storeValue<uint32_t>(
        lowerHandle(T.getBorrow().Idx, Borrow.Handle, false), Ptr);
  }

  if (T.isStreamTy() || T.isFutureTy()) {
    const auto &Transmit = getComponentVal<StreamFutureVal>(V);
    EXPECTED_TRY(uint32_t Idx,
                 lowerTransmitEnd(T.isStreamTy(), Transmit.Shared));
    return Mem->storeValue<uint32_t>(Idx, Ptr);
  }

  spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
  spdlog::error("    canonical ABI: store of gated value type"sv);
  return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
}

Expect<ComponentValVariant>
LiftLowerContext::loadList(uint64_t Begin, uint64_t Length,
                           PrimValType ElemT) const noexcept {
  AST::Component::DefValType D;
  D.setPrimValType(ElemT);
  return loadList(Begin, Length, D);
}

Expect<ComponentValVariant>
LiftLowerContext::loadList(uint64_t Begin, uint64_t Length,
                           const ComponentValType &ElemT) const noexcept {
  if (ElemT.isPrimValType()) {
    return loadList(Begin, Length, ElemT.getPrimValType());
  }
  const auto Idx = ElemT.getTypeIndex();
  EXPECTED_TRY(const auto *D, getDefValType(Idx));
  return contextOf(Idx).loadList(Begin, Length, *D);
}

Expect<ComponentValVariant> LiftLowerContext::loadList(
    uint64_t Begin, uint64_t Length,
    const AST::Component::DefValType &ElemT) const noexcept {
  EXPECTED_TRY(auto ElemAlign, alignment(ElemT));
  EXPECTED_TRY(auto ElemSz, elemSize(ElemT));
  const uint64_t ByteLen64 =
      static_cast<uint64_t>(Length) * static_cast<uint64_t>(ElemSz);
  if (ByteLen64 > static_cast<uint64_t>(MaxCanonByteLength)) {
    EXPECTED_TRY(trapDataInvalid("list byte length exceeds MAX"));
  }
  if (Begin != alignTo(Begin, ElemAlign)) {
    EXPECTED_TRY(trapDataInvalid("unaligned pointer for list",
                                 ErrCode::Value::ComponentPtrUnaligned));
  }
  const uint32_t ByteLen = static_cast<uint32_t>(ByteLen64);
  if (Length > 0u && !getMemory()->checkAccessBound(Begin, ByteLen)) {
    EXPECTED_TRY(trapMemoryOOB(Begin, ByteLen));
  }
  ListVal LV;
  LV.Elements.reserve(Length);
  for (uint32_t I = 0; I < Length; ++I) {
    EXPECTED_TRY(auto E, load(Begin + I * ElemSz, ElemT));
    LV.Elements.push_back(std::move(E));
  }
  return makeComponentVal(std::move(LV));
}

Expect<std::pair<uint64_t, uint64_t>>
LiftLowerContext::storeList(const ListVal &Lv,
                            PrimValType ElemT) const noexcept {
  AST::Component::DefValType D;
  D.setPrimValType(ElemT);
  return storeList(Lv, D);
}

Expect<std::pair<uint64_t, uint64_t>>
LiftLowerContext::storeList(const ListVal &Lv,
                            const ComponentValType &ElemT) const noexcept {
  if (ElemT.isPrimValType()) {
    return storeList(Lv, ElemT.getPrimValType());
  }
  const auto Idx = ElemT.getTypeIndex();
  EXPECTED_TRY(const auto *D, getDefValType(Idx));
  return contextOf(Idx).storeList(Lv, *D);
}

Expect<std::pair<uint64_t, uint64_t>> LiftLowerContext::storeList(
    const ListVal &Lv, const AST::Component::DefValType &ElemT) const noexcept {
  EXPECTED_TRY(auto ElemAlign, alignment(ElemT));
  EXPECTED_TRY(auto ElemSz, elemSize(ElemT));
  const uint32_t Length = static_cast<uint32_t>(Lv.Elements.size());
  const uint64_t ByteLen64 =
      static_cast<uint64_t>(Length) * static_cast<uint64_t>(ElemSz);
  assuming(ByteLen64 <= static_cast<uint64_t>(MaxCanonByteLength));
  const uint32_t ByteLen = static_cast<uint32_t>(ByteLen64);
  // Realloc runs even for an empty list, so a bad one traps.
  EXPECTED_TRY(uint64_t Begin, callRealloc(0u, 0u, ElemAlign, ByteLen));
  if (!getMemory()->checkAccessBound(Begin, ByteLen)) {
    EXPECTED_TRY(trapMemoryOOB(Begin, ByteLen));
  }
  for (uint32_t I = 0; I < Length; ++I) {
    EXPECTED_TRY(store(Lv.Elements[I], ElemT, Begin + I * ElemSz));
  }
  return std::make_pair(Begin, static_cast<uint64_t>(Length));
}

} // namespace Component
} // namespace Executor
} // namespace WasmEdge
