// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "executor/component/canonical_abi.h"

#include "common/component_valtype.h"
#include "common/component_variant.h"
#include "common/errinfo.h"
#include "common/spdlog.h"

#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <utility>
#include <vector>

using namespace std::literals;

namespace WasmEdge {
namespace Executor {
namespace Component {

namespace {

// Bit-pattern reinterpret via memcpy; C++17 has no std::bit_cast.
inline uint32_t bitsAsU32(float V) noexcept {
  uint32_t U = 0;
  std::memcpy(&U, &V, sizeof(U));
  return U;
}
inline uint64_t bitsAsU64(double V) noexcept {
  uint64_t U = 0;
  std::memcpy(&U, &V, sizeof(U));
  return U;
}
inline float bitsAsF32(uint32_t V) noexcept {
  float F = 0.f;
  std::memcpy(&F, &V, sizeof(F));
  return F;
}
inline double bitsAsF64(uint64_t V) noexcept {
  double F = 0.;
  std::memcpy(&F, &V, sizeof(F));
  return F;
}
} // namespace

uint64_t LiftLowerContext::slotBits(const ValVariant &V, ValType T) noexcept {
  switch (T.getCode()) {
  case TypeCode::I32:
    return V.get<uint32_t>();
  case TypeCode::I64:
    return V.get<uint64_t>();
  case TypeCode::F32:
    return bitsAsU32(V.get<float>());
  case TypeCode::F64:
    return bitsAsU64(V.get<double>());
  default:
    assumingUnreachable();
  }
}

ValVariant LiftLowerContext::slotFromBits(ValType T, uint64_t Bits) noexcept {
  switch (T.getCode()) {
  case TypeCode::I32:
    return ValVariant(static_cast<uint32_t>(Bits));
  case TypeCode::I64:
    return ValVariant(Bits);
  case TypeCode::F32:
    return ValVariant(bitsAsF32(static_cast<uint32_t>(Bits)));
  case TypeCode::F64:
    return ValVariant(bitsAsF64(Bits));
  default:
    assumingUnreachable();
  }
}

uint64_t LiftLowerContext::liftPtr(const ValVariant &Slot) const noexcept {
  return slotBits(Slot, getPtrType());
}

ValVariant LiftLowerContext::lowerPtr(uint64_t Ptr) const noexcept {
  return slotFromBits(getPtrType(), Ptr);
}

namespace {

// The next flat slot; validation fixes the slot count, so running out is a
// bug of the caller.
const ValVariant &takeSlot(Span<const ValVariant> Flat, size_t &Pos) noexcept {
  assuming(Pos < Flat.size());
  return Flat[Pos++];
}

// The next flat slot as an i32: a discriminant, handle, or packed flags.
uint32_t takeI32(Span<const ValVariant> Flat, size_t &Pos) noexcept {
  return static_cast<uint32_t>(
      LiftLowerContext::slotBits(takeSlot(Flat, Pos), ValType(TypeCode::I32)));
}

// Read a Have-typed flat slot and reinterpret it into the Want slot: a
// variant case takes its bits back out of the joined slot, truncated.
ValVariant coerceLiftSlot(Span<const ValVariant> Flat, size_t &Pos,
                          ValType Have, ValType Want) noexcept {
  return LiftLowerContext::slotFromBits(
      Want, LiftLowerContext::slotBits(takeSlot(Flat, Pos), Have));
}

// The inverse of coerceLiftSlot: widen a lowered slot into Want.
ValVariant coerceLowerSlot(const ValVariant &Raw, ValType Have,
                           ValType Want) noexcept {
  return LiftLowerContext::slotFromBits(Want,
                                        LiftLowerContext::slotBits(Raw, Have));
}

// Tail-pad the lowered payload with zeros typed to the joined slot.
ValVariant zeroSlot(ValType Want) noexcept {
  return LiftLowerContext::slotFromBits(Want, 0u);
}

} // namespace

Expect<ComponentValVariant>
LiftLowerContext::lift(Span<const ValVariant> Flat, size_t &Pos,
                       PrimValType T) const noexcept {
  if (T == PrimValType::String) {
    // Take (ptr, tagged_code_units), then decode per the string encoding.
    const uint64_t Ptr = liftPtr(takeSlot(Flat, Pos));
    const uint64_t Tagged = liftPtr(takeSlot(Flat, Pos));
    EXPECTED_TRY(auto Str, decodeString(Ptr, Tagged));
    return ComponentValVariant{std::move(Str)};
  }
  EXPECTED_TRY(const ValType Slot, primSlotType(T));
  const uint64_t Bits = slotBits(takeSlot(Flat, Pos), Slot);
  switch (T) {
  case PrimValType::Bool:
    // Non-zero -> true.
    return ComponentValVariant{Bits != 0u};
  case PrimValType::U8:
    return ComponentValVariant{static_cast<uint8_t>(Bits)};
  case PrimValType::U16:
    return ComponentValVariant{static_cast<uint16_t>(Bits)};
  case PrimValType::U32:
    return ComponentValVariant{static_cast<uint32_t>(Bits)};
  case PrimValType::U64:
    return ComponentValVariant{Bits};
  case PrimValType::S8:
    return ComponentValVariant{static_cast<int8_t>(Bits)};
  case PrimValType::S16:
    return ComponentValVariant{static_cast<int16_t>(Bits)};
  case PrimValType::S32:
    return ComponentValVariant{static_cast<int32_t>(Bits)};
  case PrimValType::S64:
    return ComponentValVariant{static_cast<int64_t>(Bits)};
  case PrimValType::F32:
    // Canonicalize NaN.
    return ComponentValVariant{
        canonicalizeNaN32(bitsAsF32(static_cast<uint32_t>(Bits)))};
  case PrimValType::F64:
    // Canonicalize NaN.
    return ComponentValVariant{canonicalizeNaN64(bitsAsF64(Bits))};
  case PrimValType::Char: {
    const uint32_t I = static_cast<uint32_t>(Bits);
    EXPECTED_TRY(validateUSV(I));
    return ComponentValVariant{I};
  }
  case PrimValType::ErrorContext:
    return liftErrorContext(static_cast<uint32_t>(Bits));
  default:
    assumingUnreachable();
  }
}

Expect<ComponentValVariant>
LiftLowerContext::lift(Span<const ValVariant> Flat, size_t &Pos,
                       const ComponentValType &T) const noexcept {
  if (T.isPrimValType()) {
    return lift(Flat, Pos, T.getPrimValType());
  }
  const auto Idx = T.getTypeIndex();
  EXPECTED_TRY(const auto *D, getDefValType(Idx));
  return contextOf(Idx).lift(Flat, Pos, *D);
}

Expect<ComponentValVariant>
LiftLowerContext::lift(Span<const ValVariant> Flat, size_t &Pos,
                       const AST::Component::DefValType &T) const noexcept {
  if (T.isPrimValType()) {
    return lift(Flat, Pos, T.getPrimValType());
  }

  if (T.isRecordTy()) {
    RecordVal R;
    for (const auto &F : T.getRecord().LabelTypes) {
      EXPECTED_TRY(auto V, lift(Flat, Pos, F.getValType()));
      R.Fields.emplace_back(std::string(F.getLabel()), std::move(V));
    }
    return makeComponentVal(std::move(R));
  }

  if (T.isTupleTy()) {
    TupleVal Tu;
    for (const auto &V : T.getTuple().Types) {
      EXPECTED_TRY(auto Val, lift(Flat, Pos, V));
      Tu.Values.push_back(std::move(Val));
    }
    return makeComponentVal(std::move(Tu));
  }

  if (T.isListTy()) {
    // with-len lifts len elements straight from the flat slots.
    if (T.getList().Len.has_value()) {
      const auto &L = T.getList();
      const uint32_t Len = *L.Len;
      ListVal LV;
      LV.Elements.reserve(Len);
      for (uint32_t I = 0; I < Len; ++I) {
        EXPECTED_TRY(auto E, lift(Flat, Pos, L.ValTy));
        LV.Elements.push_back(std::move(E));
      }
      return makeComponentVal(std::move(LV));
    }
    const uint64_t Begin = liftPtr(takeSlot(Flat, Pos));
    const uint64_t Length = liftPtr(takeSlot(Flat, Pos));
    return loadList(Begin, Length, T.getList().ValTy);
  }

  if (T.isMapTy()) {
    const uint64_t Begin = liftPtr(takeSlot(Flat, Pos));
    const uint64_t Length = liftPtr(takeSlot(Flat, Pos));
    return loadList(Begin, Length, mapEntryType(T.getMap()));
  }

  if (T.isFlagsTy()) {
    const uint32_t Raw = takeI32(Flat, Pos);
    FlagsVal F;
    const auto &FlagsType = T.getFlags();
    const uint32_t Labels = static_cast<uint32_t>(FlagsType.Labels.size());
    F.Bits.resize(Labels);
    for (uint32_t I = 0; I < Labels; ++I) {
      F.Bits[I] = ((Raw >> I) & 1u) != 0u;
      if (F.Bits[I]) {
        F.SetLabels.push_back(FlagsType.Labels[I]);
      }
    }
    return makeComponentVal(std::move(F));
  }

  if (T.isEnumTy()) {
    const uint32_t Case = takeI32(Flat, Pos);
    const uint32_t NumCases = static_cast<uint32_t>(T.getEnum().Labels.size());
    if (Case >= NumCases) {
      EXPECTED_TRY(
          trapDataInvalid("invalid variant discriminant for enum",
                          ErrCode::Value::ComponentDiscriminantInvalid));
    }
    return makeComponentVal(EnumVal{Case, {}});
  }

  if (T.isOwnTy()) {
    EXPECTED_TRY(uint64_t Rep,
                 liftHandle(T.getOwn().Idx, takeI32(Flat, Pos), true));
    return makeComponentVal(OwnVal{Rep});
  }

  if (T.isBorrowTy()) {
    EXPECTED_TRY(uint64_t Rep,
                 liftHandle(T.getBorrow().Idx, takeI32(Flat, Pos), false));
    return makeComponentVal(BorrowVal{Rep});
  }

  if (T.isStreamTy() || T.isFutureTy()) {
    const bool IsStream = T.isStreamTy();
    EXPECTED_TRY(auto Shared, liftTransmitEnd(IsStream, takeI32(Flat, Pos)));
    return makeComponentVal(StreamFutureVal{std::move(Shared), IsStream});
  }

  if (T.isVariantTy() || T.isOptionTy() || T.isResultTy()) {
    // Read the disc, coerce the prefix, then skip the unused suffix.
    size_t NumCases = 0;
    std::optional<ComponentValType> CasePayloadTy;
    auto PickCase = [&](uint32_t Case) -> Expect<void> {
      if (T.isVariantTy()) {
        const auto &VariantType = T.getVariant();
        NumCases = VariantType.Cases.size();
        if (Case >= NumCases) {
          EXPECTED_TRY(
              trapDataInvalid("invalid variant discriminant",
                              ErrCode::Value::ComponentDiscriminantInvalid));
        }
        CasePayloadTy = VariantType.Cases[Case].second;
      } else if (T.isOptionTy()) {
        NumCases = 2;
        if (Case >= NumCases) {
          EXPECTED_TRY(
              trapDataInvalid("invalid variant discriminant",
                              ErrCode::Value::ComponentDiscriminantInvalid));
        }
        if (Case == 1) {
          CasePayloadTy = T.getOption().ValTy;
        }
      } else {
        NumCases = 2;
        if (Case >= NumCases) {
          EXPECTED_TRY(
              trapDataInvalid("invalid variant discriminant",
                              ErrCode::Value::ComponentDiscriminantInvalid));
        }
        const auto &TaskMgr = T.getResult();
        CasePayloadTy = (Case == 0) ? TaskMgr.ValTy : TaskMgr.ErrTy;
      }
      return {};
    };

    const uint32_t Case = takeI32(Flat, Pos);
    EXPECTED_TRY(PickCase(Case));

    // Joined flat is `[i32] ++ joined`; skip the leading disc.
    EXPECTED_TRY(auto Joined, flattenType(T));
    assuming(!Joined.empty() && Joined.front().getCode() == TypeCode::I32);
    const auto JoinedPayload = Span<const ValType>{Joined}.subspan(1);

    // Native flat for the picked case (empty if no payload).
    std::vector<ValType> CaseFlat;
    if (CasePayloadTy.has_value()) {
      EXPECTED_TRY(CaseFlat, flattenType(*CasePayloadTy));
    }
    assuming(CaseFlat.size() <= JoinedPayload.size());

    // Coerce the case's prefix; skip the join-padding suffix.
    std::vector<ValVariant> Coerced;
    Coerced.reserve(CaseFlat.size());
    for (size_t I = 0; I < CaseFlat.size(); ++I) {
      Coerced.push_back(
          coerceLiftSlot(Flat, Pos, JoinedPayload[I], CaseFlat[I]));
    }
    const size_t Padding = JoinedPayload.size() - CaseFlat.size();
    assuming(Pos + Padding <= Flat.size());
    Pos += Padding;

    std::optional<ComponentValVariant> Payload;
    if (CasePayloadTy.has_value()) {
      size_t PayloadPos = 0;
      EXPECTED_TRY(auto P, lift(Coerced, PayloadPos, *CasePayloadTy));
      Payload = std::move(P);
    }

    if (T.isVariantTy()) {
      VariantVal Vv;
      Vv.Case = Case;
      Vv.Payload = std::move(Payload);
      return makeComponentVal(std::move(Vv));
    }
    if (T.isOptionTy()) {
      OptionVal Ov;
      if (Case == 1) {
        Ov.Value = std::move(Payload);
      }
      return makeComponentVal(std::move(Ov));
    }
    ResultVal Rv;
    Rv.IsOk = (Case == 0);
    Rv.Payload = std::move(Payload);
    return makeComponentVal(std::move(Rv));
  }

  spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
  spdlog::error("    canonical ABI: lift_flat of gated value type"sv);
  return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
}

Expect<void>
LiftLowerContext::lower(const ComponentValVariant &V, PrimValType T,
                        std::vector<ValVariant> &Flat) const noexcept {
  if (T == PrimValType::String) {
    // Encode per the string option, then push (ptr, tagged_code_units).
    EXPECTED_TRY(auto Encoded, encodeString(std::get<std::string>(V)));
    Flat.push_back(lowerPtr(Encoded.first));
    Flat.push_back(lowerPtr(Encoded.second));
    return {};
  }
  EXPECTED_TRY(const ValType Slot, primSlotType(T));
  uint64_t Bits = 0;
  switch (T) {
  case PrimValType::Bool:
    Bits = std::get<bool>(V) ? 1u : 0u;
    break;
  case PrimValType::U8:
    Bits = std::get<uint8_t>(V);
    break;
  case PrimValType::U16:
    Bits = std::get<uint16_t>(V);
    break;
  case PrimValType::U32:
    Bits = std::get<uint32_t>(V);
    break;
  case PrimValType::U64:
    Bits = std::get<uint64_t>(V);
    break;
  case PrimValType::S8:
    // Two's complement in the i32 slot.
    Bits = static_cast<uint32_t>(static_cast<int32_t>(std::get<int8_t>(V)));
    break;
  case PrimValType::S16:
    Bits = static_cast<uint32_t>(static_cast<int32_t>(std::get<int16_t>(V)));
    break;
  case PrimValType::S32:
    Bits = static_cast<uint32_t>(std::get<int32_t>(V));
    break;
  case PrimValType::S64:
    Bits = static_cast<uint64_t>(std::get<int64_t>(V));
    break;
  case PrimValType::F32:
    // Canonicalize NaN.
    Bits = bitsAsU32(canonicalizeNaN32(std::get<float>(V)));
    break;
  case PrimValType::F64:
    // Canonicalize NaN.
    Bits = bitsAsU64(canonicalizeNaN64(std::get<double>(V)));
    break;
  case PrimValType::Char: {
    const uint32_t I = std::get<uint32_t>(V);
    assumeValidUSV(I);
    Bits = I;
    break;
  }
  case PrimValType::ErrorContext: {
    EXPECTED_TRY(const uint32_t Idx, lowerErrorContext(V));
    Bits = Idx;
    break;
  }
  default:
    assumingUnreachable();
  }
  Flat.push_back(slotFromBits(Slot, Bits));
  return {};
}

Expect<void>
LiftLowerContext::lower(const ComponentValVariant &V, const ComponentValType &T,
                        std::vector<ValVariant> &Flat) const noexcept {
  if (T.isPrimValType()) {
    return lower(V, T.getPrimValType(), Flat);
  }
  const auto Idx = T.getTypeIndex();
  EXPECTED_TRY(const auto *D, getDefValType(Idx));
  return contextOf(Idx).lower(V, *D, Flat);
}

Expect<void>
LiftLowerContext::lower(const ComponentValVariant &V,
                        const AST::Component::DefValType &T,
                        std::vector<ValVariant> &Flat) const noexcept {
  if (T.isPrimValType()) {
    return lower(V, T.getPrimValType(), Flat);
  }

  if (T.isRecordTy()) {
    // Concatenate the per-field lowerings.
    const auto &Record = getComponentVal<RecordVal>(V);
    const auto &Fields = T.getRecord().LabelTypes;
    assuming(Record.Fields.size() == Fields.size());
    for (size_t I = 0; I < Fields.size(); ++I) {
      EXPECTED_TRY(
          lower(Record.Fields[I].second, Fields[I].getValType(), Flat));
    }
    return {};
  }

  if (T.isTupleTy()) {
    const auto &Tuple = getComponentVal<TupleVal>(V);
    const auto &Types = T.getTuple().Types;
    assuming(Tuple.Values.size() == Types.size());
    for (size_t I = 0; I < Types.size(); ++I) {
      EXPECTED_TRY(lower(Tuple.Values[I], Types[I], Flat));
    }
    return {};
  }

  if (T.isListTy()) {
    // no-len pushes (ptr, len); with-len concatenates the lowerings.
    if (T.getList().Len.has_value()) {
      const auto &L = T.getList();
      const uint32_t Len = *L.Len;
      const auto &List = getComponentVal<ListVal>(V);
      assuming(List.Elements.size() == Len);
      for (uint32_t I = 0; I < Len; ++I) {
        EXPECTED_TRY(lower(List.Elements[I], L.ValTy, Flat));
      }
      return {};
    }
    EXPECTED_TRY(auto Range,
                 storeList(getComponentVal<ListVal>(V), T.getList().ValTy));
    Flat.push_back(lowerPtr(Range.first));
    Flat.push_back(lowerPtr(Range.second));
    return {};
  }

  if (T.isMapTy()) {
    EXPECTED_TRY(auto Range, storeList(getComponentVal<ListVal>(V),
                                       mapEntryType(T.getMap())));
    Flat.push_back(lowerPtr(Range.first));
    Flat.push_back(lowerPtr(Range.second));
    return {};
  }

  if (T.isFlagsTy()) {
    // Labels are capped at 32, so a single i32 holds them.
    const auto &Flags = getComponentVal<FlagsVal>(V);
    const auto &FlagsType = T.getFlags();
    assuming(Flags.Bits.empty() ||
             Flags.Bits.size() == FlagsType.Labels.size());
    Flat.emplace_back(static_cast<uint32_t>(packFlags(Flags, FlagsType)));
    return {};
  }

  if (T.isEnumTy()) {
    const auto &Enum = getComponentVal<EnumVal>(V);
    const uint32_t Case = resolveEnumCase(Enum, T.getEnum());
    assuming(Case < T.getEnum().Labels.size());
    Flat.emplace_back(Case);
    return {};
  }

  if (T.isOwnTy()) {
    const auto &Own = getComponentVal<OwnVal>(V);
    Flat.emplace_back(lowerHandle(T.getOwn().Idx, Own.Handle, true));
    return {};
  }

  if (T.isBorrowTy()) {
    const auto &Borrow = getComponentVal<BorrowVal>(V);
    Flat.emplace_back(lowerHandle(T.getBorrow().Idx, Borrow.Handle, false));
    return {};
  }

  if (T.isStreamTy() || T.isFutureTy()) {
    const auto &Transmit = getComponentVal<StreamFutureVal>(V);
    EXPECTED_TRY(uint32_t Idx,
                 lowerTransmitEnd(T.isStreamTy(), Transmit.Shared));
    Flat.emplace_back(Idx);
    return {};
  }

  if (T.isVariantTy() || T.isOptionTy() || T.isResultTy()) {
    // Lower to the native flat shape, coerce each slot, then pad zeros.
    size_t NumCases = 0;
    uint32_t Case = 0;
    std::optional<ComponentValType> CasePayloadTy;
    std::optional<ComponentValVariant> CasePayloadVal;
    if (T.isVariantTy()) {
      const auto &Variant = getComponentVal<VariantVal>(V);
      Case = resolveVariantCase(Variant, T.getVariant());
      NumCases = T.getVariant().Cases.size();
      assuming(Case < NumCases);
      CasePayloadTy = T.getVariant().Cases[Case].second;
      if (CasePayloadTy.has_value()) {
        assuming(Variant.Payload.has_value());
        CasePayloadVal = Variant.Payload;
      }
    } else if (T.isOptionTy()) {
      NumCases = 2;
      const auto &Option = getComponentVal<OptionVal>(V);
      if (Option.Value.has_value()) {
        Case = 1u;
        CasePayloadTy = T.getOption().ValTy;
        CasePayloadVal = Option.Value;
      } else {
        Case = 0u;
      }
    } else {
      NumCases = 2;
      const auto &Result = getComponentVal<ResultVal>(V);
      Case = Result.IsOk ? 0u : 1u;
      const auto &TaskMgr = T.getResult();
      CasePayloadTy = Result.IsOk ? TaskMgr.ValTy : TaskMgr.ErrTy;
      if (CasePayloadTy.has_value()) {
        assuming(Result.Payload.has_value());
        CasePayloadVal = Result.Payload;
      }
    }

    // Joined flat is `[i32] ++ joined`; skip the leading disc.
    EXPECTED_TRY(auto Joined, flattenType(T));
    assuming(!Joined.empty() && Joined.front().getCode() == TypeCode::I32);
    const auto JoinedPayload = Span<const ValType>{Joined}.subspan(1);

    Flat.reserve(Flat.size() + 1u + JoinedPayload.size());
    Flat.emplace_back(static_cast<uint32_t>(Case));

    if (CasePayloadTy.has_value()) {
      EXPECTED_TRY(auto CaseFlat, flattenType(*CasePayloadTy));
      assuming(CaseFlat.size() <= JoinedPayload.size());
      std::vector<ValVariant> Native;
      EXPECTED_TRY(lower(*CasePayloadVal, *CasePayloadTy, Native));
      assuming(Native.size() == CaseFlat.size());
      for (size_t I = 0; I < Native.size(); ++I) {
        Flat.push_back(
            coerceLowerSlot(Native[I], CaseFlat[I], JoinedPayload[I]));
      }
      for (size_t I = Native.size(); I < JoinedPayload.size(); ++I) {
        Flat.push_back(zeroSlot(JoinedPayload[I]));
      }
    } else {
      for (const auto &J : JoinedPayload) {
        Flat.push_back(zeroSlot(J));
      }
    }
    return {};
  }

  spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
  spdlog::error("    canonical ABI: lower_flat of gated value type"sv);
  return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
}

namespace {

// Synthesize a TupleTy over a span of types, shared by indirect paths.
AST::Component::DefValType
synthTupleType(Span<const ComponentValType> Types) noexcept {
  AST::Component::TupleTy Tup;
  for (const auto &T : Types) {
    Tup.Types.push_back(T);
  }
  AST::Component::DefValType D;
  D.setTuple(std::move(Tup));
  return D;
}

Expect<void> valueMismatch(std::string_view Detail) noexcept {
  spdlog::error(ErrCode::Value::FuncSigMismatch);
  spdlog::error("    component value does not match the declared type: {}"sv,
                Detail);
  return Unexpect(ErrCode::Value::FuncSigMismatch);
}

} // namespace

// An embedder value must hold the alternative its type lowers from.
Expect<void> LiftLowerContext::checkValue(const ComponentValVariant &V,
                                          PrimValType T) const noexcept {
  bool Ok = false;
  switch (T) {
  case PrimValType::Bool:
    Ok = std::holds_alternative<bool>(V);
    break;
  case PrimValType::S8:
    Ok = std::holds_alternative<int8_t>(V);
    break;
  case PrimValType::U8:
    Ok = std::holds_alternative<uint8_t>(V);
    break;
  case PrimValType::S16:
    Ok = std::holds_alternative<int16_t>(V);
    break;
  case PrimValType::U16:
    Ok = std::holds_alternative<uint16_t>(V);
    break;
  case PrimValType::S32:
    Ok = std::holds_alternative<int32_t>(V);
    break;
  case PrimValType::U32:
  case PrimValType::Char:
    Ok = std::holds_alternative<uint32_t>(V);
    break;
  case PrimValType::S64:
    Ok = std::holds_alternative<int64_t>(V);
    break;
  case PrimValType::U64:
    Ok = std::holds_alternative<uint64_t>(V);
    break;
  case PrimValType::F32:
    Ok = std::holds_alternative<float>(V);
    break;
  case PrimValType::F64:
    Ok = std::holds_alternative<double>(V);
    break;
  case PrimValType::String:
    Ok = std::holds_alternative<std::string>(V);
    break;
  case PrimValType::ErrorContext: {
    Ok = isComponentVal<ErrorContextVal>(V);
    break;
  }
  default:
    return {};
  }
  if (!Ok) {
    return valueMismatch("primitive"sv);
  }
  return {};
}

Expect<void>
LiftLowerContext::checkValue(const ComponentValVariant &V,
                             const ComponentValType &T) const noexcept {
  if (T.isPrimValType()) {
    return checkValue(V, T.getPrimValType());
  }
  const auto Idx = T.getTypeIndex();
  EXPECTED_TRY(const auto *D, getDefValType(Idx));
  return contextOf(Idx).checkValue(V, *D);
}

Expect<void> LiftLowerContext::checkValue(
    const ComponentValVariant &V,
    const AST::Component::DefValType &T) const noexcept {
  if (T.isPrimValType()) {
    return checkValue(V, T.getPrimValType());
  }
  const auto *VCP = std::get_if<std::shared_ptr<ValComp>>(&V);
  if (VCP == nullptr || !*VCP) {
    return valueMismatch("expected an aggregate value"sv);
  }
  const auto &Inner = (*VCP)->V;

  if (T.isRecordTy()) {
    const auto *R = std::get_if<RecordVal>(&Inner);
    const auto &Fields = T.getRecord().LabelTypes;
    if (R == nullptr || R->Fields.size() != Fields.size()) {
      return valueMismatch("record arity"sv);
    }
    for (size_t I = 0; I < Fields.size(); ++I) {
      EXPECTED_TRY(checkValue(R->Fields[I].second, Fields[I].getValType()));
    }
    return {};
  }
  if (T.isTupleTy()) {
    const auto *Tu = std::get_if<TupleVal>(&Inner);
    const auto &Types = T.getTuple().Types;
    if (Tu == nullptr || Tu->Values.size() != Types.size()) {
      return valueMismatch("tuple arity"sv);
    }
    for (size_t I = 0; I < Types.size(); ++I) {
      EXPECTED_TRY(checkValue(Tu->Values[I], Types[I]));
    }
    return {};
  }
  if (T.isVariantTy()) {
    const auto *Vv = std::get_if<VariantVal>(&Inner);
    if (Vv == nullptr) {
      return valueMismatch("expected a variant"sv);
    }
    const auto &VariantType = T.getVariant();
    const uint32_t Case = resolveVariantCase(*Vv, VariantType);
    if (Case >= VariantType.Cases.size()) {
      return valueMismatch("variant case out of range"sv);
    }
    if (VariantType.Cases[Case].second.has_value()) {
      if (!Vv->Payload.has_value()) {
        return valueMismatch("variant case needs a payload"sv);
      }
      return checkValue(*Vv->Payload, *VariantType.Cases[Case].second);
    }
    return {};
  }
  if (T.isOptionTy()) {
    const auto *O = std::get_if<OptionVal>(&Inner);
    if (O == nullptr) {
      return valueMismatch("expected an option"sv);
    }
    if (O->Value.has_value()) {
      return checkValue(*O->Value, T.getOption().ValTy);
    }
    return {};
  }
  if (T.isResultTy()) {
    const auto *R = std::get_if<ResultVal>(&Inner);
    if (R == nullptr) {
      return valueMismatch("expected a result"sv);
    }
    const auto &TaskMgr = T.getResult();
    const std::optional<ComponentValType> &PT =
        R->IsOk ? TaskMgr.ValTy : TaskMgr.ErrTy;
    if (PT.has_value()) {
      if (!R->Payload.has_value()) {
        return valueMismatch("result arm needs a payload"sv);
      }
      return checkValue(*R->Payload, *PT);
    }
    return {};
  }
  if (T.isListTy()) {
    const auto *L = std::get_if<ListVal>(&Inner);
    if (L == nullptr) {
      return valueMismatch("expected a list"sv);
    }
    const auto &Lt = T.getList();
    if (Lt.Len.has_value() && L->Elements.size() != *Lt.Len) {
      return valueMismatch("fixed-length list length"sv);
    }
    for (const auto &E : L->Elements) {
      EXPECTED_TRY(checkValue(E, Lt.ValTy));
    }
    return {};
  }
  if (T.isMapTy()) {
    const auto *L = std::get_if<ListVal>(&Inner);
    if (L == nullptr) {
      return valueMismatch("expected a map"sv);
    }
    const auto Entry = mapEntryType(T.getMap());
    for (const auto &E : L->Elements) {
      EXPECTED_TRY(checkValue(E, Entry));
    }
    return {};
  }
  if (T.isFlagsTy()) {
    const auto *F = std::get_if<FlagsVal>(&Inner);
    if (F == nullptr) {
      return valueMismatch("expected flags"sv);
    }
    // The set-label form resolves against the declared labels at lowering.
    if (F->SetLabels.empty() && !F->Bits.empty() &&
        F->Bits.size() != T.getFlags().Labels.size()) {
      return valueMismatch("flags width"sv);
    }
    return {};
  }
  if (T.isEnumTy()) {
    const auto *E = std::get_if<EnumVal>(&Inner);
    if (E == nullptr) {
      return valueMismatch("expected an enum"sv);
    }
    if (resolveEnumCase(*E, T.getEnum()) >= T.getEnum().Labels.size()) {
      return valueMismatch("enum case out of range"sv);
    }
    return {};
  }
  if (T.isOwnTy()) {
    return std::holds_alternative<OwnVal>(Inner)
               ? Expect<void>{}
               : valueMismatch("expected an own handle"sv);
  }
  if (T.isBorrowTy()) {
    return std::holds_alternative<BorrowVal>(Inner)
               ? Expect<void>{}
               : valueMismatch("expected a borrow handle"sv);
  }
  if (T.isStreamTy() || T.isFutureTy()) {
    return std::holds_alternative<StreamFutureVal>(Inner)
               ? Expect<void>{}
               : valueMismatch("expected a stream or future end"sv);
  }
  return {};
}

Expect<uint32_t> LiftLowerContext::totalFlatCount(
    Span<const ComponentValType> Types) const noexcept {
  uint32_t N = 0;
  for (const auto &T : Types) {
    EXPECTED_TRY(auto Sub, flattenType(T));
    N += static_cast<uint32_t>(Sub.size());
  }
  return N;
}

Expect<std::vector<ComponentValVariant>>
LiftLowerContext::liftValues(Span<const ValVariant> Flat,
                             Span<const ComponentValType> Types,
                             uint32_t MaxFlat) const noexcept {
  EXPECTED_TRY(auto N, totalFlatCount(Types));
  size_t Pos = 0;
  std::vector<ComponentValVariant> Out;
  Out.reserve(Types.size());

  if (N > MaxFlat) {
    // Indirect path: the one slot points at a tuple of the values.
    const uint64_t Ptr = liftPtr(takeSlot(Flat, Pos));
    auto Td = synthTupleType(Types);
    EXPECTED_TRY(auto Align, alignment(Td));
    EXPECTED_TRY(auto Sz, elemSize(Td));
    if (Ptr != alignTo(Ptr, Align)) {
      EXPECTED_TRY(trapDataInvalid("lift_flat_values: unaligned pointer",
                                   ErrCode::Value::ComponentPtrUnaligned));
    }
    if (!Mem->checkAccessBound(Ptr, Sz)) {
      EXPECTED_TRY(trapMemoryOOB(Ptr, Sz));
    }
    EXPECTED_TRY(auto Loaded, load(Ptr, Td));
    auto &Tu =
        std::get<TupleVal>(std::get<std::shared_ptr<ValComp>>(Loaded)->V);
    assuming(Tu.Values.size() == Types.size());
    for (auto &V : Tu.Values) {
      Out.push_back(std::move(V));
    }
  } else {
    // Direct path: the slots of the values, one after the other.
    for (const auto &T : Types) {
      EXPECTED_TRY(auto V, lift(Flat, Pos, T));
      Out.push_back(std::move(V));
    }
  }
  assuming(Pos == Flat.size());
  return Out;
}

Expect<std::vector<ValVariant>> LiftLowerContext::lowerValues(
    Span<const ComponentValVariant> Values, Span<const ComponentValType> Types,
    uint32_t MaxFlat, std::optional<uint64_t> OutParam) const noexcept {
  if (Values.size() != Types.size()) {
    EXPECTED_TRY(valueMismatch("argument count"sv));
  }
  // Everything below reads the values unchecked, so validate them first.
  for (size_t I = 0; I < Types.size(); ++I) {
    EXPECTED_TRY(checkValue(Values[I], Types[I]));
  }
  EXPECTED_TRY(auto N, totalFlatCount(Types));
  std::vector<ValVariant> Out;

  if (N > MaxFlat) {
    // Indirect path: the values go into a tuple in memory.
    auto Td = synthTupleType(Types);
    EXPECTED_TRY(auto Align, alignment(Td));
    EXPECTED_TRY(auto Sz, elemSize(Td));

    uint64_t Ptr = 0;
    bool ReturnPtr = false;
    if (OutParam.has_value()) {
      Ptr = *OutParam;
    } else {
      EXPECTED_TRY(Ptr, callRealloc(0u, 0u, Align, Sz));
      ReturnPtr = true;
    }
    if (Ptr != alignTo(Ptr, Align)) {
      EXPECTED_TRY(trapDataInvalid("lower_flat_values: unaligned pointer",
                                   ErrCode::Value::ComponentPtrUnaligned));
    }
    if (!Mem->checkAccessBound(Ptr, Sz)) {
      EXPECTED_TRY(trapMemoryOOB(Ptr, Sz));
    }

    // Walk the tuple layout in-line, honoring the per-field alignment.
    uint32_t Off = 0u;
    for (size_t I = 0; I < Types.size(); ++I) {
      EXPECTED_TRY(auto FA, alignment(Types[I]));
      Off = alignTo(Off, FA);
      EXPECTED_TRY(store(Values[I], Types[I], Ptr + Off));
      EXPECTED_TRY(auto FS, elemSize(Types[I]));
      Off += FS;
    }
    if (ReturnPtr) {
      Out.push_back(lowerPtr(Ptr));
    }
    return Out;
  }

  // Direct path: the slots of the values, one after the other.
  Out.reserve(N);
  for (size_t I = 0; I < Types.size(); ++I) {
    EXPECTED_TRY(lower(Values[I], Types[I], Out));
  }
  return Out;
}

} // namespace Component
} // namespace Executor
} // namespace WasmEdge
