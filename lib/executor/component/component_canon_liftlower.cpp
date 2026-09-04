// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "executor/component/canonical_abi.h"

#include "canonical_abi_internal.h"
#include "common/spdlog.h"

#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace WasmEdge {
namespace Executor {
namespace Component {
namespace CanonicalABI {

using namespace std::literals;

namespace {

// A pointer or length slot takes the selected memory's address type.
uint64_t flatPtr(const Context &Cx, const ValVariant &V) noexcept {
  return Cx.memory64() ? V.get<uint64_t>()
                       : static_cast<uint64_t>(V.get<uint32_t>());
}

ValVariant ptrSlot(const Context &Cx, uint64_t V) noexcept {
  return Cx.memory64() ? ValVariant(V) : ValVariant(static_cast<uint32_t>(V));
}

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
  uint64_t Bits = V;
  double F = 0.;
  std::memcpy(&F, &Bits, sizeof(F));
  return F;
}

inline uint32_t wrapI64ToI32(uint64_t V) noexcept {
  return static_cast<uint32_t>(V);
}

// Read a Have-typed flat slot and reinterpret it into the Want slot.
Expect<ValVariant> coerceLiftSlot(FlatIter &VI, ValType Have,
                                  ValType Want) noexcept {
  auto Raw = VI.next();
  assuming(Raw.has_value());
  const auto Hc = Have.getCode();
  const auto Wc = Want.getCode();
  if (Hc == Wc) {
    return *Raw;
  }
  if (Hc == TypeCode::I32 && Wc == TypeCode::F32) {
    return ValVariant{bitsAsF32(Raw->get<uint32_t>())};
  }
  if (Hc == TypeCode::I64 && Wc == TypeCode::I32) {
    return ValVariant{wrapI64ToI32(Raw->get<uint64_t>())};
  }
  if (Hc == TypeCode::I64 && Wc == TypeCode::F32) {
    return ValVariant{bitsAsF32(wrapI64ToI32(Raw->get<uint64_t>()))};
  }
  if (Hc == TypeCode::I64 && Wc == TypeCode::F64) {
    return ValVariant{bitsAsF64(Raw->get<uint64_t>())};
  }
  assumingUnreachable();
}

// The inverse of coerceLiftSlot: widen a lowered slot into Want.
ValVariant coerceLowerSlot(const ValVariant &Raw, ValType Have,
                           ValType Want) noexcept {
  const auto Hc = Have.getCode();
  const auto Wc = Want.getCode();
  if (Hc == Wc) {
    return Raw;
  }
  if (Hc == TypeCode::F32 && Wc == TypeCode::I32) {
    return ValVariant{bitsAsU32(Raw.get<float>())};
  }
  if (Hc == TypeCode::I32 && Wc == TypeCode::I64) {
    // Same numeric value, widened to the joined i64 shape.
    return ValVariant{static_cast<uint64_t>(Raw.get<uint32_t>())};
  }
  if (Hc == TypeCode::F32 && Wc == TypeCode::I64) {
    return ValVariant{static_cast<uint64_t>(bitsAsU32(Raw.get<float>()))};
  }
  if (Hc == TypeCode::F64 && Wc == TypeCode::I64) {
    return ValVariant{bitsAsU64(Raw.get<double>())};
  }
  assumingUnreachable();
}

// Tail-pad the lowered payload with zeros typed to the joined slot.
ValVariant zeroSlot(ValType Want) noexcept {
  switch (Want.getCode()) {
  case TypeCode::I32:
    return ValVariant{uint32_t{0}};
  case TypeCode::I64:
    return ValVariant{uint64_t{0}};
  case TypeCode::F32:
    return ValVariant{0.f};
  case TypeCode::F64:
    return ValVariant{0.};
  default:
    assumingUnreachable();
  }
}

// Narrow from i32 with zero / sign extension.
ComponentValVariant liftFlatUnsigned(uint32_t Width, uint64_t Raw) noexcept {
  switch (Width) {
  case 8:
    return ComponentValVariant{static_cast<uint8_t>(Raw)};
  case 16:
    return ComponentValVariant{static_cast<uint16_t>(Raw)};
  case 32:
    return ComponentValVariant{static_cast<uint32_t>(Raw)};
  case 64:
    return ComponentValVariant{static_cast<uint64_t>(Raw)};
  default:
    assumingUnreachable();
  }
}

ComponentValVariant liftFlatSigned(uint32_t Width, uint64_t Raw) noexcept {
  switch (Width) {
  case 8:
    return ComponentValVariant{static_cast<int8_t>(Raw)};
  case 16:
    return ComponentValVariant{static_cast<int16_t>(Raw)};
  case 32:
    return ComponentValVariant{static_cast<int32_t>(Raw)};
  case 64:
    return ComponentValVariant{static_cast<int64_t>(Raw)};
  default:
    assumingUnreachable();
  }
}

Expect<ComponentValVariant>
liftFlatPrim(const Context &Cx, FlatIter &VI,
             AST::Component::PrimValType PVT) noexcept {
  using P = AST::Component::PrimValType;
  auto Next = VI.next();
  assuming(Next.has_value() || PVT == P::String);
  switch (PVT) {
  case P::Bool: {
    // Non-zero → true.
    const uint32_t I = Next->get<uint32_t>();
    return ComponentValVariant{I != 0u};
  }
  case P::U8:
    return liftFlatUnsigned(8, Next->get<uint32_t>());
  case P::U16:
    return liftFlatUnsigned(16, Next->get<uint32_t>());
  case P::U32:
    return liftFlatUnsigned(32, Next->get<uint32_t>());
  case P::U64:
    return liftFlatUnsigned(64, Next->get<uint64_t>());
  case P::S8:
    return liftFlatSigned(8, Next->get<uint32_t>());
  case P::S16:
    return liftFlatSigned(16, Next->get<uint32_t>());
  case P::S32:
    return liftFlatSigned(32, Next->get<uint32_t>());
  case P::S64:
    return liftFlatSigned(64, Next->get<uint64_t>());
  case P::F32:
    // Canonicalize NaN.
    return ComponentValVariant{canonicalizeNaN32(Next->get<float>())};
  case P::F64:
    // Canonicalize NaN.
    return ComponentValVariant{canonicalizeNaN64(Next->get<double>())};
  case P::Char: {
    const uint32_t I = Next->get<uint32_t>();
    EXPECTED_TRY(validateUSV(I));
    return ComponentValVariant{I};
  }
  case P::String: {
    // Take (ptr, tagged_code_units), then decode per the string encoding.
    assuming(Next.has_value());
    const uint64_t Ptr = flatPtr(Cx, *Next);
    auto LenV = VI.next();
    assuming(LenV.has_value());
    const uint64_t Tagged = flatPtr(Cx, *LenV);
    EXPECTED_TRY(auto Str, decodeString(Cx, Ptr, Tagged));
    return ComponentValVariant{std::move(Str)};
  }
  case P::ErrorContext:
    return liftErrorContext(Cx, Next->get<uint32_t>());
  default:
    spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
    spdlog::error("    canonical ABI: lift_flat of unknown prim 0x{:02x}"sv,
                  static_cast<uint8_t>(PVT));
    return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
  }
}
} // namespace

Expect<ComponentValVariant> liftFlat(const Context &Cx, FlatIter &VI,
                                     const ComponentValType &T) noexcept {
  using TC = ComponentTypeCode;
  const TC Code = T.getCode();

  if (Code == TC::TypeIndex) {
    const auto *DT = resolveDefType(Cx, T.getTypeIndex());
    if (DT == nullptr || !DT->isDefValType()) {
      spdlog::error(ErrCode::Value::InvalidTypeReference);
      spdlog::error(
          "    canonical ABI: type index {} does not refer to a value type"sv,
          T.getTypeIndex());
      return Unexpect(ErrCode::Value::InvalidTypeReference);
    }
    return liftFlatDef(Cx, VI, DT->getDefValType());
  }
  return liftFlatPrim(
      Cx, VI,
      static_cast<AST::Component::PrimValType>(static_cast<uint8_t>(Code)));
}

Expect<ComponentValVariant>
liftFlatDef(const Context &Cx, FlatIter &VI,
            const AST::Component::DefValType &T) noexcept {
  if (T.isPrimValType()) {
    return liftFlatPrim(Cx, VI, T.getPrimValType());
  }

  if (T.isRecordTy()) {
    RecordVal R;
    for (const auto &F : T.getRecord().LabelTypes) {
      EXPECTED_TRY(auto V, liftFlat(Cx, VI, F.getValType()));
      R.Fields.emplace_back(std::string(F.getLabel()), std::move(V));
    }
    return makeComponentVal(std::move(R));
  }

  if (T.isTupleTy()) {
    TupleVal Tu;
    for (const auto &V : T.getTuple().Types) {
      EXPECTED_TRY(auto Val, liftFlat(Cx, VI, V));
      Tu.Values.push_back(std::move(Val));
    }
    return makeComponentVal(std::move(Tu));
  }

  if (T.isListTy()) {
    // with-len lifts len elements straight from the flat iterator.
    if (T.getList().Len.has_value()) {
      const auto &L = T.getList();
      const uint32_t Len = *L.Len;
      ListVal LV;
      LV.Elements.reserve(Len);
      for (uint32_t I = 0; I < Len; ++I) {
        EXPECTED_TRY(auto E, liftFlat(Cx, VI, L.ValTy));
        LV.Elements.push_back(std::move(E));
      }
      return makeComponentVal(std::move(LV));
    }
    auto PtrV = VI.next();
    auto LenV = VI.next();
    assuming(PtrV.has_value() && LenV.has_value());
    return liftListFromRange(Cx, flatPtr(Cx, *PtrV), flatPtr(Cx, *LenV),
                             T.getList().ValTy);
  }

  if (T.isMapTy()) {
    auto PtrV = VI.next();
    auto LenV = VI.next();
    assuming(PtrV.has_value() && LenV.has_value());
    return liftListFromRangeDef(Cx, flatPtr(Cx, *PtrV), flatPtr(Cx, *LenV),
                                mapEntryType(T.getMap()));
  }

  if (T.isFlagsTy()) {
    auto Next = VI.next();
    assuming(Next.has_value());
    const uint32_t Raw = Next->get<uint32_t>();
    FlagsVal F;
    const auto &Ft = T.getFlags();
    const uint32_t Labels = static_cast<uint32_t>(Ft.Labels.size());
    F.Bits.resize(Labels);
    for (uint32_t I = 0; I < Labels; ++I) {
      F.Bits[I] = ((Raw >> I) & 1u) != 0u;
      if (F.Bits[I]) {
        F.SetLabels.push_back(Ft.Labels[I]);
      }
    }
    return makeComponentVal(std::move(F));
  }

  if (T.isEnumTy()) {
    auto Next = VI.next();
    assuming(Next.has_value());
    const uint32_t Case = Next->get<uint32_t>();
    const uint32_t NumCases = static_cast<uint32_t>(T.getEnum().Labels.size());
    if (Case >= NumCases) {
      EXPECTED_TRY(
          trapDataInvalid("invalid variant discriminant for enum",
                          ErrCode::Value::ComponentDiscriminantInvalid));
    }
    return makeComponentVal(EnumVal{Case, {}});
  }

  if (T.isOwnTy()) {
    auto Next = VI.next();
    assuming(Next.has_value());
    EXPECTED_TRY(uint64_t Rep,
                 liftOwnHandle(Cx, T.getOwn().Idx, Next->get<uint32_t>()));
    return makeComponentVal(OwnVal{Rep});
  }

  if (T.isBorrowTy()) {
    auto Next = VI.next();
    assuming(Next.has_value());
    EXPECTED_TRY(uint64_t Rep, liftBorrowHandle(Cx, T.getBorrow().Idx,
                                                Next->get<uint32_t>()));
    return makeComponentVal(BorrowVal{Rep});
  }

  if (T.isStreamTy() || T.isFutureTy()) {
    const bool IsStream = T.isStreamTy();
    auto Next = VI.next();
    assuming(Next.has_value());
    EXPECTED_TRY(auto Shared, liftCopyEnd(Cx, IsStream, Next->get<uint32_t>()));
    return makeComponentVal(StreamFutureVal{std::move(Shared), IsStream});
  }

  if (T.isVariantTy() || T.isOptionTy() || T.isResultTy()) {
    // Read the disc, coerce the prefix, then drain the unused suffix.
    size_t NumCases = 0;
    std::optional<ComponentValType> CasePayloadTy;
    auto pickCase = [&](uint32_t Case) -> Expect<void> {
      if (T.isVariantTy()) {
        const auto &Vt = T.getVariant();
        NumCases = Vt.Cases.size();
        if (Case >= NumCases) {
          EXPECTED_TRY(
              trapDataInvalid("invalid variant discriminant",
                              ErrCode::Value::ComponentDiscriminantInvalid));
        }
        CasePayloadTy = Vt.Cases[Case].second;
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
        const auto &Rt = T.getResult();
        CasePayloadTy = (Case == 0) ? Rt.ValTy : Rt.ErrTy;
      }
      return {};
    };

    auto DiscRaw = VI.next();
    assuming(DiscRaw.has_value());
    const uint32_t Case = DiscRaw->get<uint32_t>();
    EXPECTED_TRY(pickCase(Case));

    // Joined flat is `[i32] ++ joined`; skip the leading disc.
    EXPECTED_TRY(auto Joined, flattenTypeDef(Cx, T));
    assuming(!Joined.empty() && Joined.front().getCode() == TypeCode::I32);
    const auto JoinedPayload = Span<const ValType>{Joined}.subspan(1);

    // Native flat for the picked case (empty if no payload).
    std::vector<ValType> CaseFlat;
    if (CasePayloadTy.has_value()) {
      EXPECTED_TRY(CaseFlat, flattenType(Cx, *CasePayloadTy));
    }
    assuming(CaseFlat.size() <= JoinedPayload.size());

    // Coerce the case's prefix; drain the join-padding suffix.
    std::vector<ValVariant> Coerced;
    Coerced.reserve(CaseFlat.size());
    for (size_t I = 0; I < CaseFlat.size(); ++I) {
      EXPECTED_TRY(auto V, coerceLiftSlot(VI, JoinedPayload[I], CaseFlat[I]));
      Coerced.push_back(V);
    }
    for (size_t I = CaseFlat.size(); I < JoinedPayload.size(); ++I) {
      auto Skip = VI.next();
      assuming(Skip.has_value());
    }

    std::optional<ComponentValVariant> Payload;
    if (CasePayloadTy.has_value()) {
      Span<const ValVariant> CoercedSpan(Coerced);
      FlatIter PayloadIter(CoercedSpan);
      EXPECTED_TRY(auto P, liftFlat(Cx, PayloadIter, *CasePayloadTy));
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

namespace {

// Two's-complement reinterpretation, bucketed to i32 or i64.
std::vector<ValVariant> lowerSigned32(int32_t V) noexcept {
  return {ValVariant(static_cast<uint32_t>(V))};
}
std::vector<ValVariant> lowerSigned64(int64_t V) noexcept {
  return {ValVariant(static_cast<uint64_t>(V))};
}

Expect<std::vector<ValVariant>>
lowerFlatPrim(const Context &Cx, const ComponentValVariant &V,
              AST::Component::PrimValType PVT) noexcept {
  using P = AST::Component::PrimValType;
  switch (PVT) {
  case P::Bool:
    return std::vector<ValVariant>{ValVariant(std::get<bool>(V) ? 1u : 0u)};
  case P::U8:
    return std::vector<ValVariant>{
        ValVariant(static_cast<uint32_t>(std::get<uint8_t>(V)))};
  case P::U16:
    return std::vector<ValVariant>{
        ValVariant(static_cast<uint32_t>(std::get<uint16_t>(V)))};
  case P::U32:
    return std::vector<ValVariant>{ValVariant(std::get<uint32_t>(V))};
  case P::U64:
    return std::vector<ValVariant>{ValVariant(std::get<uint64_t>(V))};
  case P::S8:
    return lowerSigned32(static_cast<int32_t>(std::get<int8_t>(V)));
  case P::S16:
    return lowerSigned32(static_cast<int32_t>(std::get<int16_t>(V)));
  case P::S32:
    return lowerSigned32(std::get<int32_t>(V));
  case P::S64:
    return lowerSigned64(std::get<int64_t>(V));
  case P::F32:
    // Canonicalize NaN.
    return std::vector<ValVariant>{
        ValVariant(canonicalizeNaN32(std::get<float>(V)))};
  case P::F64:
    // Canonicalize NaN.
    return std::vector<ValVariant>{
        ValVariant(canonicalizeNaN64(std::get<double>(V)))};
  case P::Char: {
    const uint32_t I = std::get<uint32_t>(V);
    assumeValidUSV(I);
    return std::vector<ValVariant>{ValVariant(I)};
  }
  case P::String: {
    // Encode per the string option, then push (ptr, tagged_code_units).
    EXPECTED_TRY(auto Enc, encodeString(Cx, std::get<std::string>(V)));
    return std::vector<ValVariant>{ptrSlot(Cx, Enc.first),
                                   ptrSlot(Cx, Enc.second)};
  }
  case P::ErrorContext: {
    EXPECTED_TRY(auto Idx, lowerErrorContext(Cx, V));
    return std::vector<ValVariant>{ValVariant(Idx)};
  }
  default:
    spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
    spdlog::error("    canonical ABI: lower_flat of unknown prim 0x{:02x}"sv,
                  static_cast<uint8_t>(PVT));
    return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
  }
}
} // namespace

Expect<std::vector<ValVariant>> lowerFlat(const Context &Cx,
                                          const ComponentValVariant &V,
                                          const ComponentValType &T) noexcept {
  using TC = ComponentTypeCode;
  const TC Code = T.getCode();

  if (Code == TC::TypeIndex) {
    const auto *DT = resolveDefType(Cx, T.getTypeIndex());
    if (DT == nullptr || !DT->isDefValType()) {
      spdlog::error(ErrCode::Value::InvalidTypeReference);
      spdlog::error(
          "    canonical ABI: type index {} does not refer to a value type"sv,
          T.getTypeIndex());
      return Unexpect(ErrCode::Value::InvalidTypeReference);
    }
    return lowerFlatDef(Cx, V, DT->getDefValType());
  }
  return lowerFlatPrim(
      Cx, V,
      static_cast<AST::Component::PrimValType>(static_cast<uint8_t>(Code)));
}

Expect<std::vector<ValVariant>>
lowerFlatDef(const Context &Cx, const ComponentValVariant &V,
             const AST::Component::DefValType &T) noexcept {
  if (T.isPrimValType()) {
    return lowerFlatPrim(Cx, V, T.getPrimValType());
  }

  if (T.isRecordTy()) {
    // Concatenate the per-field lowerings.
    const auto &VC = std::get<std::shared_ptr<ValComp>>(V);
    assuming(VC);
    const auto &R = std::get<RecordVal>(VC->V);
    const auto &Fields = T.getRecord().LabelTypes;
    assuming(R.Fields.size() == Fields.size());
    std::vector<ValVariant> Flat;
    for (size_t I = 0; I < Fields.size(); ++I) {
      EXPECTED_TRY(auto Sub,
                   lowerFlat(Cx, R.Fields[I].second, Fields[I].getValType()));
      Flat.insert(Flat.end(), Sub.begin(), Sub.end());
    }
    return Flat;
  }

  if (T.isTupleTy()) {
    const auto &VC = std::get<std::shared_ptr<ValComp>>(V);
    assuming(VC);
    const auto &Tu = std::get<TupleVal>(VC->V);
    const auto &Types = T.getTuple().Types;
    assuming(Tu.Values.size() == Types.size());
    std::vector<ValVariant> Flat;
    for (size_t I = 0; I < Types.size(); ++I) {
      EXPECTED_TRY(auto Sub, lowerFlat(Cx, Tu.Values[I], Types[I]));
      Flat.insert(Flat.end(), Sub.begin(), Sub.end());
    }
    return Flat;
  }

  if (T.isListTy()) {
    // no-len pushes (ptr, len); with-len concatenates the lowerings.
    if (T.getList().Len.has_value()) {
      const auto &L = T.getList();
      const uint32_t Len = *L.Len;
      const auto &VC = std::get<std::shared_ptr<ValComp>>(V);
      assuming(VC);
      const auto &Lv = std::get<ListVal>(VC->V);
      assuming(Lv.Elements.size() == Len);
      std::vector<ValVariant> Flat;
      for (uint32_t I = 0; I < Len; ++I) {
        EXPECTED_TRY(auto Sub, lowerFlat(Cx, Lv.Elements[I], L.ValTy));
        Flat.insert(Flat.end(), Sub.begin(), Sub.end());
      }
      return Flat;
    }
    const auto &VC = std::get<std::shared_ptr<ValComp>>(V);
    assuming(VC);
    const auto &Lv = std::get<ListVal>(VC->V);
    const auto &ElemT = T.getList().ValTy;
    EXPECTED_TRY(auto ElemAlign, alignment(Cx, ElemT));
    EXPECTED_TRY(auto ElemSz, elemSize(Cx, ElemT));
    const uint32_t Length = static_cast<uint32_t>(Lv.Elements.size());
    const uint64_t ByteLen64 =
        static_cast<uint64_t>(Length) * static_cast<uint64_t>(ElemSz);
    assuming(ByteLen64 <= static_cast<uint64_t>(kMaxCanonByteLength));
    const uint32_t ByteLen = static_cast<uint32_t>(ByteLen64);
    // Realloc runs even for an empty list, so a bad one traps.
    EXPECTED_TRY(uint64_t Begin, callRealloc(Cx, 0u, 0u, ElemAlign, ByteLen));
    if (!Cx.Mem->checkAccessBound(Begin, ByteLen)) {
      EXPECTED_TRY(
          trapMemoryOOB("list payload (post-realloc)", Begin, ByteLen));
    }
    for (uint32_t I = 0; I < Length; ++I) {
      EXPECTED_TRY(store(Cx, Lv.Elements[I], ElemT, Begin + I * ElemSz));
    }
    return std::vector<ValVariant>{ptrSlot(Cx, Begin), ptrSlot(Cx, Length)};
  }

  if (T.isMapTy()) {
    const auto &VC = std::get<std::shared_ptr<ValComp>>(V);
    assuming(VC);
    EXPECTED_TRY(auto Range, storeListWithDefElem(Cx, std::get<ListVal>(VC->V),
                                                  mapEntryType(T.getMap())));
    return std::vector<ValVariant>{ptrSlot(Cx, Range.first),
                                   ptrSlot(Cx, Range.second)};
  }

  if (T.isFlagsTy()) {
    // Labels are capped at 32, so a single i32 holds them.
    const auto &VC = std::get<std::shared_ptr<ValComp>>(V);
    assuming(VC);
    const auto &F = std::get<FlagsVal>(VC->V);
    const auto &Ft = T.getFlags();
    assuming(F.Bits.empty() || F.Bits.size() == Ft.Labels.size());
    const uint32_t Packed = static_cast<uint32_t>(packFlags(F, Ft));
    return std::vector<ValVariant>{ValVariant(Packed)};
  }

  if (T.isEnumTy()) {
    const auto &VC = std::get<std::shared_ptr<ValComp>>(V);
    assuming(VC);
    const auto &E = std::get<EnumVal>(VC->V);
    const uint32_t Case = resolveEnumCase(E, T.getEnum());
    assuming(Case < T.getEnum().Labels.size());
    return std::vector<ValVariant>{ValVariant(Case)};
  }

  if (T.isOwnTy()) {
    const auto &VC = std::get<std::shared_ptr<ValComp>>(V);
    assuming(VC);
    const auto &O = std::get<OwnVal>(VC->V);
    return std::vector<ValVariant>{
        ValVariant(lowerHandle(Cx, T.getOwn().Idx, O.Handle, true))};
  }

  if (T.isBorrowTy()) {
    const auto &VC = std::get<std::shared_ptr<ValComp>>(V);
    assuming(VC);
    const auto &B = std::get<BorrowVal>(VC->V);
    return std::vector<ValVariant>{
        ValVariant(lowerHandle(Cx, T.getBorrow().Idx, B.Handle, false))};
  }

  if (T.isStreamTy() || T.isFutureTy()) {
    const auto &VC = std::get<std::shared_ptr<ValComp>>(V);
    assuming(VC);
    const auto &SF = std::get<StreamFutureVal>(VC->V);
    EXPECTED_TRY(uint32_t Idx, lowerCopyEnd(Cx, T.isStreamTy(), SF.Shared));
    return std::vector<ValVariant>{ValVariant(Idx)};
  }

  if (T.isVariantTy() || T.isOptionTy() || T.isResultTy()) {
    // Lower to the native flat shape, coerce each slot, then pad zeros.
    size_t NumCases = 0;
    uint32_t Case = 0;
    std::optional<ComponentValType> CasePayloadTy;
    std::optional<ComponentValVariant> CasePayloadVal;
    if (T.isVariantTy()) {
      const auto &VC = std::get<std::shared_ptr<ValComp>>(V);
      assuming(VC);
      const auto &Vv = std::get<VariantVal>(VC->V);
      Case = resolveVariantCase(Vv, T.getVariant());
      NumCases = T.getVariant().Cases.size();
      assuming(Case < NumCases);
      CasePayloadTy = T.getVariant().Cases[Case].second;
      if (CasePayloadTy.has_value()) {
        assuming(Vv.Payload.has_value());
        CasePayloadVal = Vv.Payload;
      }
    } else if (T.isOptionTy()) {
      NumCases = 2;
      const auto &VC = std::get<std::shared_ptr<ValComp>>(V);
      assuming(VC);
      const auto &Ov = std::get<OptionVal>(VC->V);
      if (Ov.Value.has_value()) {
        Case = 1u;
        CasePayloadTy = T.getOption().ValTy;
        CasePayloadVal = Ov.Value;
      } else {
        Case = 0u;
      }
    } else {
      NumCases = 2;
      const auto &VC = std::get<std::shared_ptr<ValComp>>(V);
      assuming(VC);
      const auto &Rv = std::get<ResultVal>(VC->V);
      Case = Rv.IsOk ? 0u : 1u;
      const auto &Rt = T.getResult();
      CasePayloadTy = Rv.IsOk ? Rt.ValTy : Rt.ErrTy;
      if (CasePayloadTy.has_value()) {
        assuming(Rv.Payload.has_value());
        CasePayloadVal = Rv.Payload;
      }
    }

    // Joined flat is `[i32] ++ joined`; skip the leading disc.
    EXPECTED_TRY(auto Joined, flattenTypeDef(Cx, T));
    assuming(!Joined.empty() && Joined.front().getCode() == TypeCode::I32);
    const auto JoinedPayload = Span<const ValType>{Joined}.subspan(1);

    std::vector<ValVariant> Flat;
    Flat.reserve(1u + JoinedPayload.size());
    Flat.emplace_back(static_cast<uint32_t>(Case));

    if (CasePayloadTy.has_value()) {
      EXPECTED_TRY(auto CaseFlat, flattenType(Cx, *CasePayloadTy));
      assuming(CaseFlat.size() <= JoinedPayload.size());
      EXPECTED_TRY(auto Native, lowerFlat(Cx, *CasePayloadVal, *CasePayloadTy));
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
    return Flat;
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

// Compute total flat count over a span of component types.
Expect<uint32_t> totalFlatCount(const Context &Cx,
                                Span<const ComponentValType> Types) noexcept {
  uint32_t N = 0;
  for (const auto &T : Types) {
    EXPECTED_TRY(auto Sub, flattenType(Cx, T));
    N += static_cast<uint32_t>(Sub.size());
  }
  return N;
}

Expect<void> valueMismatch(std::string_view Detail) noexcept {
  spdlog::error(ErrCode::Value::FuncSigMismatch);
  spdlog::error("    component value does not match the declared type: {}"sv,
                Detail);
  return Unexpect(ErrCode::Value::FuncSigMismatch);
}

// An embedder value must hold the alternative its type lowers from.
Expect<void> checkPrimValue(const ComponentValVariant &V,
                            AST::Component::PrimValType PVT) noexcept {
  using P = AST::Component::PrimValType;
  bool Ok = false;
  switch (PVT) {
  case P::Bool:
    Ok = std::holds_alternative<bool>(V);
    break;
  case P::S8:
    Ok = std::holds_alternative<int8_t>(V);
    break;
  case P::U8:
    Ok = std::holds_alternative<uint8_t>(V);
    break;
  case P::S16:
    Ok = std::holds_alternative<int16_t>(V);
    break;
  case P::U16:
    Ok = std::holds_alternative<uint16_t>(V);
    break;
  case P::S32:
    Ok = std::holds_alternative<int32_t>(V);
    break;
  case P::U32:
  case P::Char:
    Ok = std::holds_alternative<uint32_t>(V);
    break;
  case P::S64:
    Ok = std::holds_alternative<int64_t>(V);
    break;
  case P::U64:
    Ok = std::holds_alternative<uint64_t>(V);
    break;
  case P::F32:
    Ok = std::holds_alternative<float>(V);
    break;
  case P::F64:
    Ok = std::holds_alternative<double>(V);
    break;
  case P::ErrorContext: {
    const auto *VC = std::get_if<std::shared_ptr<ValComp>>(&V);
    Ok = VC != nullptr && *VC &&
         std::holds_alternative<ErrorContextVal>((*VC)->V);
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

Expect<void> checkValueDef(const Context &Cx, const ComponentValVariant &V,
                           const AST::Component::DefValType &T) noexcept;

// Structural check: the lowering path reads the value unchecked.
Expect<void> checkValue(const Context &Cx, const ComponentValVariant &V,
                        const ComponentValType &T) noexcept {
  if (T.getCode() == ComponentTypeCode::TypeIndex) {
    const auto *DT = resolveDefType(Cx, T.getTypeIndex());
    if (DT == nullptr || !DT->isDefValType()) {
      return valueMismatch("unresolved type index"sv);
    }
    return checkValueDef(Cx, V, DT->getDefValType());
  }
  return checkPrimValue(V, static_cast<AST::Component::PrimValType>(
                               static_cast<uint8_t>(T.getCode())));
}

Expect<void> checkValueDef(const Context &Cx, const ComponentValVariant &V,
                           const AST::Component::DefValType &T) noexcept {
  if (T.isPrimValType()) {
    return checkPrimValue(V, T.getPrimValType());
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
      EXPECTED_TRY(checkValue(Cx, R->Fields[I].second, Fields[I].getValType()));
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
      EXPECTED_TRY(checkValue(Cx, Tu->Values[I], Types[I]));
    }
    return {};
  }
  if (T.isVariantTy()) {
    const auto *Vv = std::get_if<VariantVal>(&Inner);
    if (Vv == nullptr) {
      return valueMismatch("expected a variant"sv);
    }
    const auto &Vt = T.getVariant();
    const uint32_t Case = resolveVariantCase(*Vv, Vt);
    if (Case >= Vt.Cases.size()) {
      return valueMismatch("variant case out of range"sv);
    }
    if (Vt.Cases[Case].second.has_value()) {
      if (!Vv->Payload.has_value()) {
        return valueMismatch("variant case needs a payload"sv);
      }
      return checkValue(Cx, *Vv->Payload, *Vt.Cases[Case].second);
    }
    return {};
  }
  if (T.isOptionTy()) {
    const auto *O = std::get_if<OptionVal>(&Inner);
    if (O == nullptr) {
      return valueMismatch("expected an option"sv);
    }
    if (O->Value.has_value()) {
      return checkValue(Cx, *O->Value, T.getOption().ValTy);
    }
    return {};
  }
  if (T.isResultTy()) {
    const auto *R = std::get_if<ResultVal>(&Inner);
    if (R == nullptr) {
      return valueMismatch("expected a result"sv);
    }
    const auto &Rt = T.getResult();
    const std::optional<ComponentValType> &PT = R->IsOk ? Rt.ValTy : Rt.ErrTy;
    if (PT.has_value()) {
      if (!R->Payload.has_value()) {
        return valueMismatch("result arm needs a payload"sv);
      }
      return checkValue(Cx, *R->Payload, *PT);
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
      EXPECTED_TRY(checkValue(Cx, E, Lt.ValTy));
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
      EXPECTED_TRY(checkValueDef(Cx, E, Entry));
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
} // namespace

Expect<std::vector<ComponentValVariant>>
liftFlatValues(const Context &Cx, FlatIter &VI,
               Span<const ComponentValType> Types, uint32_t MaxFlat) noexcept {
  EXPECTED_TRY(auto N, totalFlatCount(Cx, Types));

  if (N > MaxFlat) {
    // Indirect path.
    auto PtrV = VI.next();
    assuming(PtrV.has_value());
    const uint64_t Ptr = flatPtr(Cx, *PtrV);
    auto Td = synthTupleType(Types);
    EXPECTED_TRY(auto Align, alignmentDef(Cx, Td));
    EXPECTED_TRY(auto Sz, elemSizeDef(Cx, Td));
    if (Ptr != alignTo(Ptr, Align)) {
      EXPECTED_TRY(trapDataInvalid("lift_flat_values: unaligned pointer",
                                   ErrCode::Value::ComponentPtrUnaligned));
    }
    if (!Cx.Mem->checkAccessBound(Ptr, Sz)) {
      EXPECTED_TRY(trapMemoryOOB("lift_flat_values area", Ptr, Sz));
    }
    EXPECTED_TRY(auto Loaded, loadDef(Cx, Ptr, Td));
    auto &Tu =
        std::get<TupleVal>(std::get<std::shared_ptr<ValComp>>(Loaded)->V);
    assuming(Tu.Values.size() == Types.size());
    std::vector<ComponentValVariant> Out;
    Out.reserve(Tu.Values.size());
    for (auto &V : Tu.Values) {
      Out.push_back(std::move(V));
    }
    return Out;
  }

  // Direct path: per-type liftFlat.
  std::vector<ComponentValVariant> Out;
  Out.reserve(Types.size());
  for (const auto &T : Types) {
    EXPECTED_TRY(auto V, liftFlat(Cx, VI, T));
    Out.push_back(std::move(V));
  }
  return Out;
}

Expect<std::vector<ValVariant>>
lowerFlatValues(const Context &Cx, Span<const ComponentValVariant> Values,
                Span<const ComponentValType> Types, uint32_t MaxFlat,
                std::optional<uint64_t> OutParam) noexcept {
  if (Values.size() != Types.size()) {
    EXPECTED_TRY(valueMismatch("argument count"sv));
  }
  // Everything below reads the values unchecked, so validate them first.
  for (size_t I = 0; I < Types.size(); ++I) {
    EXPECTED_TRY(checkValue(Cx, Values[I], Types[I]));
  }
  EXPECTED_TRY(auto N, totalFlatCount(Cx, Types));

  if (N > MaxFlat) {
    // Indirect path.
    auto Td = synthTupleType(Types);
    EXPECTED_TRY(auto Align, alignmentDef(Cx, Td));
    EXPECTED_TRY(auto Sz, elemSizeDef(Cx, Td));

    uint64_t Ptr = 0;
    bool ReturnPtr = false;
    if (OutParam.has_value()) {
      Ptr = *OutParam;
    } else {
      EXPECTED_TRY(Ptr, callRealloc(Cx, 0u, 0u, Align, Sz));
      ReturnPtr = true;
    }
    if (Ptr != alignTo(Ptr, Align)) {
      EXPECTED_TRY(trapDataInvalid("lower_flat_values: unaligned pointer",
                                   ErrCode::Value::ComponentPtrUnaligned));
    }
    if (!Cx.Mem->checkAccessBound(Ptr, Sz)) {
      EXPECTED_TRY(trapMemoryOOB("lower_flat_values area", Ptr, Sz));
    }

    // Walk the tuple layout in-line, honoring the per-field alignment.
    uint32_t Off = 0u;
    for (size_t I = 0; I < Types.size(); ++I) {
      EXPECTED_TRY(auto FA, alignment(Cx, Types[I]));
      Off = alignTo(Off, FA);
      EXPECTED_TRY(store(Cx, Values[I], Types[I], Ptr + Off));
      EXPECTED_TRY(auto FS, elemSize(Cx, Types[I]));
      Off += FS;
    }

    if (ReturnPtr) {
      return std::vector<ValVariant>{ptrSlot(Cx, Ptr)};
    }
    return std::vector<ValVariant>{};
  }

  // Direct path: per-value lowerFlat.
  std::vector<ValVariant> Out;
  for (size_t I = 0; I < Types.size(); ++I) {
    EXPECTED_TRY(auto Sub, lowerFlat(Cx, Values[I], Types[I]));
    for (auto &CV : Sub) {
      Out.push_back(std::move(CV));
    }
  }
  return Out;
}

} // namespace CanonicalABI
} // namespace Component
} // namespace Executor
} // namespace WasmEdge
