// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "executor/component/canonical_abi.h"

#include "common/component_valtype.h"
#include "common/spdlog.h"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

using namespace std::literals;

namespace WasmEdge {
namespace Executor {
namespace Component {

namespace {

// Next power-of-two >= Bytes, used by the flags alignment.
constexpr uint32_t nextPow2(uint32_t Bytes) noexcept {
  uint32_t P = 1u;
  while (P < Bytes) {
    P <<= 1;
  }
  return P;
}
} // namespace

Expect<const AST::Component::DefValType *>
LiftLowerContext::getDefValType(uint32_t Idx) const noexcept {
  const auto *DT = getType(Idx);
  if (DT == nullptr || !DT->isDefValType()) {
    spdlog::error(ErrCode::Value::InvalidTypeReference);
    spdlog::error(
        "    canonical ABI: type index {} does not refer to a value type"sv,
        Idx);
    return Unexpect(ErrCode::Value::InvalidTypeReference);
  }
  return &DT->getDefValType();
}

uint32_t LiftLowerContext::discriminantSize(uint32_t NumCases) noexcept {
  assuming(NumCases > 0);
  if (NumCases <= DiscriminantU8Cases) {
    return 1u;
  }
  if (NumCases <= DiscriminantU16Cases) {
    return 2u;
  }
  return 4u;
}

AST::Component::DefValType
LiftLowerContext::mapEntryType(const AST::Component::MapTy &M) noexcept {
  AST::Component::TupleTy Entry;
  Entry.Types = {M.KeyTy, M.ValTy};
  AST::Component::DefValType D;
  D.setTuple(std::move(Entry));
  return D;
}

Expect<uint32_t> LiftLowerContext::alignment(PrimValType T) const noexcept {
  switch (T) {
  case PrimValType::Bool:
  case PrimValType::S8:
  case PrimValType::U8:
    return 1u;
  case PrimValType::S16:
  case PrimValType::U16:
    return 2u;
  case PrimValType::S32:
  case PrimValType::U32:
  case PrimValType::F32:
  case PrimValType::Char:
    return 4u;
  case PrimValType::S64:
  case PrimValType::U64:
  case PrimValType::F64:
    return 8u;
  case PrimValType::String:
    // (ptr, len) pair - alignment of the pointer.
    return getPtrSize();
  case PrimValType::ErrorContext:
    // An index into the instance's handles table.
    return 4u;
  default:
    spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
    spdlog::error("    canonical ABI: alignment of unknown prim 0x{:02x}"sv,
                  static_cast<uint8_t>(T));
    return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
  }
}

Expect<uint32_t>
LiftLowerContext::alignment(const ComponentValType &T) const noexcept {
  if (T.isPrimValType()) {
    return alignment(T.getPrimValType());
  }
  const auto Idx = T.getTypeIndex();
  EXPECTED_TRY(const auto *D, getDefValType(Idx));
  return contextOf(Idx).alignment(*D);
}

Expect<uint32_t> LiftLowerContext::alignment(
    const AST::Component::DefValType &T) const noexcept {
  if (T.isPrimValType()) {
    return alignment(T.getPrimValType());
  }

  if (T.isRecordTy()) {
    // Max alignment of the fields, default 1.
    uint32_t Max = 1u;
    for (const auto &F : T.getRecord().LabelTypes) {
      EXPECTED_TRY(auto A, alignment(F.getValType()));
      Max = std::max(Max, A);
    }
    return Max;
  }

  if (T.isTupleTy()) {
    // A tuple flattens like a record.
    uint32_t Max = 1u;
    for (const auto &V : T.getTuple().Types) {
      EXPECTED_TRY(auto A, alignment(V));
      Max = std::max(Max, A);
    }
    return Max;
  }

  if (T.isVariantTy()) {
    // max(discriminant, the widest case payload).
    const auto &V = T.getVariant();
    const uint32_t NumCases = static_cast<uint32_t>(V.Cases.size());
    EXPECTED_TRY(auto MaxCase, maxCaseAlignment(V.Cases));
    return std::max(LiftLowerContext::discriminantSize(NumCases), MaxCase);
  }

  if (T.isOptionTy()) {
    // option<T> is variant{none | some(T)}: 2 cases, disc 1B.
    EXPECTED_TRY(auto A, alignment(T.getOption().ValTy));
    return std::max(1u, A);
  }

  if (T.isResultTy()) {
    // result<T,E> is variant{ok(T)? | err(E)?}: 2 cases, disc 1B.
    uint32_t Max = 1u;
    const auto &R = T.getResult();
    if (R.ValTy.has_value()) {
      EXPECTED_TRY(auto A, alignment(*R.ValTy));
      Max = std::max(Max, A);
    }
    if (R.ErrTy.has_value()) {
      EXPECTED_TRY(auto A, alignment(*R.ErrTy));
      Max = std::max(Max, A);
    }
    return Max;
  }

  if (T.isListTy()) {
    // no-len -> the ptr/len pair; with-len -> the element alignment.
    const auto &L = T.getList();
    if (L.Len.has_value()) {
      return alignment(L.ValTy);
    }
    return getPtrSize();
  }

  if (T.isMapTy()) {
    // A map is a list without a length: the ptr/len pair.
    return getPtrSize();
  }

  if (T.isFlagsTy()) {
    // next_pow2(ceil(|labels|/8)), with no labels -> 1.
    const auto &F = T.getFlags();
    const uint32_t Labels = static_cast<uint32_t>(F.Labels.size());
    if (Labels == 0) {
      return 1u;
    }
    const uint32_t Bytes = (Labels + 7u) / 8u;
    return nextPow2(Bytes);
  }

  if (T.isEnumTy()) {
    // Enum aligns to its discriminant.
    const uint32_t NumCases = static_cast<uint32_t>(T.getEnum().Labels.size());
    return LiftLowerContext::discriminantSize(NumCases);
  }

  if (T.isOwnTy() || T.isBorrowTy()) {
    // Resource handle is i32.
    return 4u;
  }

  if (T.isStreamTy() || T.isFutureTy()) {
    // Stream/future ends are i32 handles.
    return 4u;
  }

  spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
  spdlog::error("    canonical ABI: alignment of gated value type"sv);
  return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
}

Expect<uint32_t> LiftLowerContext::elemSize(PrimValType T) const noexcept {
  switch (T) {
  case PrimValType::Bool:
  case PrimValType::S8:
  case PrimValType::U8:
    return 1u;
  case PrimValType::S16:
  case PrimValType::U16:
    return 2u;
  case PrimValType::S32:
  case PrimValType::U32:
  case PrimValType::F32:
  case PrimValType::Char:
    return 4u;
  case PrimValType::S64:
  case PrimValType::U64:
  case PrimValType::F64:
    return 8u;
  case PrimValType::String:
    // (ptr, len) pair.
    return 2u * getPtrSize();
  case PrimValType::ErrorContext:
    return 4u;
  default:
    spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
    spdlog::error("    canonical ABI: elem_size of unknown prim 0x{:02x}"sv,
                  static_cast<uint8_t>(T));
    return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
  }
}

Expect<uint32_t> LiftLowerContext::maxCaseAlignment(
    const std::vector<std::pair<std::string, std::optional<ComponentValType>>>
        &Cases) const noexcept {
  uint32_t M = 1u;
  for (const auto &C : Cases) {
    if (C.second.has_value()) {
      EXPECTED_TRY(auto A, alignment(*C.second));
      M = std::max(M, A);
    }
  }
  return M;
}

Expect<uint32_t>
LiftLowerContext::elemSize(const ComponentValType &T) const noexcept {
  if (T.isPrimValType()) {
    return elemSize(T.getPrimValType());
  }
  const auto Idx = T.getTypeIndex();
  EXPECTED_TRY(const auto *D, getDefValType(Idx));
  return contextOf(Idx).elemSize(*D);
}

Expect<uint32_t>
LiftLowerContext::elemSize(const AST::Component::DefValType &T) const noexcept {
  if (T.isPrimValType()) {
    return elemSize(T.getPrimValType());
  }

  // Track the max field alignment in-loop instead of re-walking.
  if (T.isRecordTy()) {
    uint32_t Off = 0u;
    uint32_t Max = 1u;
    for (const auto &F : T.getRecord().LabelTypes) {
      EXPECTED_TRY(auto A, alignment(F.getValType()));
      Max = std::max(Max, A);
      Off = alignTo(Off, A);
      EXPECTED_TRY(auto S, elemSize(F.getValType()));
      Off += S;
    }
    return alignTo(Off, Max);
  }

  if (T.isTupleTy()) {
    uint32_t Off = 0u;
    uint32_t Max = 1u;
    for (const auto &V : T.getTuple().Types) {
      EXPECTED_TRY(auto A, alignment(V));
      Max = std::max(Max, A);
      Off = alignTo(Off, A);
      EXPECTED_TRY(auto S, elemSize(V));
      Off += S;
    }
    return alignTo(Off, Max);
  }

  if (T.isVariantTy()) {
    // Variant alignment is max(disc, payloads); one pass gives both.
    const auto &V = T.getVariant();
    const uint32_t NumCases = static_cast<uint32_t>(V.Cases.size());
    const uint32_t Disc = LiftLowerContext::discriminantSize(NumCases);
    uint32_t MaxAlign = 1u;
    uint32_t MaxSize = 0u;
    for (const auto &C : V.Cases) {
      if (C.second.has_value()) {
        EXPECTED_TRY(auto A, alignment(*C.second));
        MaxAlign = std::max(MaxAlign, A);
        EXPECTED_TRY(auto Sz, elemSize(*C.second));
        MaxSize = std::max(MaxSize, Sz);
      }
    }
    const uint32_t Aggr = std::max(Disc, MaxAlign);
    return alignTo(alignTo(Disc, MaxAlign) + MaxSize, Aggr);
  }

  if (T.isOptionTy()) {
    // option<T> = variant{none | some(T)}: disc 1B, one payload case.
    EXPECTED_TRY(auto A, alignment(T.getOption().ValTy));
    EXPECTED_TRY(auto PS, elemSize(T.getOption().ValTy));
    const uint32_t Aggr = std::max(1u, A);
    return alignTo(alignTo(1u, A) + PS, Aggr);
  }

  if (T.isResultTy()) {
    // result<T,E> = variant{ok(T)? | err(E)?}: disc 1B.
    const auto &R = T.getResult();
    uint32_t MaxAlign = 1u;
    uint32_t MaxSize = 0u;
    auto consider =
        [&](const std::optional<ComponentValType> &V) -> Expect<void> {
      if (V.has_value()) {
        EXPECTED_TRY(auto A, alignment(*V));
        MaxAlign = std::max(MaxAlign, A);
        EXPECTED_TRY(auto S, elemSize(*V));
        MaxSize = std::max(MaxSize, S);
      }
      return {};
    };
    EXPECTED_TRY(consider(R.ValTy));
    EXPECTED_TRY(consider(R.ErrTy));
    const uint32_t Aggr = MaxAlign; // disc=1 <= MaxAlign
    return alignTo(alignTo(1u, MaxAlign) + MaxSize, Aggr);
  }

  if (T.isListTy()) {
    // no-len -> ptr + len; with-len -> len * the element size.
    const auto &L = T.getList();
    if (L.Len.has_value()) {
      EXPECTED_TRY(auto ElemSz, elemSize(L.ValTy));
      return static_cast<uint32_t>(static_cast<uint64_t>(*L.Len) *
                                   static_cast<uint64_t>(ElemSz));
    }
    return 2u * getPtrSize();
  }

  if (T.isMapTy()) {
    return 2u * getPtrSize();
  }

  if (T.isFlagsTy()) {
    // ceil(|labels|/8), aligned to the flags alignment.
    const auto &F = T.getFlags();
    const uint32_t Labels = static_cast<uint32_t>(F.Labels.size());
    const uint32_t Bytes = (Labels + 7u) / 8u;
    EXPECTED_TRY(auto A, alignment(T));
    return alignTo(Bytes, A);
  }

  if (T.isEnumTy()) {
    const uint32_t NumCases = static_cast<uint32_t>(T.getEnum().Labels.size());
    return LiftLowerContext::discriminantSize(NumCases);
  }

  if (T.isOwnTy() || T.isBorrowTy()) {
    return 4u;
  }

  if (T.isStreamTy() || T.isFutureTy()) {
    return 4u;
  }

  spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
  spdlog::error("    canonical ABI: elem_size of gated value type"sv);
  return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
}

namespace {

const ValType I32T{TypeCode::I32};
const ValType I64T{TypeCode::I64};
const ValType F32T{TypeCode::F32};
const ValType F64T{TypeCode::F64};
} // namespace

Expect<ValType> LiftLowerContext::primSlotType(PrimValType T) const noexcept {
  switch (T) {
  case PrimValType::Bool:
  case PrimValType::S8:
  case PrimValType::U8:
  case PrimValType::S16:
  case PrimValType::U16:
  case PrimValType::S32:
  case PrimValType::U32:
  case PrimValType::Char:
  case PrimValType::ErrorContext:
    return I32T;
  case PrimValType::S64:
  case PrimValType::U64:
    return I64T;
  case PrimValType::F32:
    return F32T;
  case PrimValType::F64:
    return F64T;
  case PrimValType::String:
    return getPtrType();
  default:
    spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
    spdlog::error("    canonical ABI: flatten of unknown prim 0x{:02x}"sv,
                  static_cast<uint8_t>(T));
    return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
  }
}

namespace {
// Join two flat slot types into the shape a variant case must fit.
ValType joinFlat(ValType A, ValType B) noexcept {
  if (A == B) {
    return A;
  }
  const auto Ac = A.getCode();
  const auto Bc = B.getCode();
  if ((Ac == TypeCode::I32 && Bc == TypeCode::F32) ||
      (Ac == TypeCode::F32 && Bc == TypeCode::I32)) {
    return I32T;
  }
  return I64T;
}
} // namespace

Expect<std::vector<ValType>>
LiftLowerContext::flattenType(PrimValType T) const noexcept {
  EXPECTED_TRY(const ValType Slot, primSlotType(T));
  if (T == PrimValType::String) {
    // ptr + len; both the memory's address type.
    return std::vector<ValType>{Slot, Slot};
  }
  return std::vector<ValType>{Slot};
}

Expect<std::vector<ValType>>
LiftLowerContext::flattenType(const ComponentValType &T) const noexcept {
  if (T.isPrimValType()) {
    return flattenType(T.getPrimValType());
  }
  const auto Idx = T.getTypeIndex();
  EXPECTED_TRY(const auto *D, getDefValType(Idx));
  return contextOf(Idx).flattenType(*D);
}

Expect<std::vector<ValType>> LiftLowerContext::flattenType(
    const AST::Component::DefValType &T) const noexcept {
  if (T.isPrimValType()) {
    return flattenType(T.getPrimValType());
  }

  if (T.isRecordTy()) {
    std::vector<ValType> Flat;
    for (const auto &F : T.getRecord().LabelTypes) {
      EXPECTED_TRY(auto Sub, flattenType(F.getValType()));
      Flat.insert(Flat.end(), Sub.begin(), Sub.end());
    }
    return Flat;
  }

  if (T.isTupleTy()) {
    std::vector<ValType> Flat;
    for (const auto &V : T.getTuple().Types) {
      EXPECTED_TRY(auto Sub, flattenType(V));
      Flat.insert(Flat.end(), Sub.begin(), Sub.end());
    }
    return Flat;
  }

  if (T.isVariantTy()) {
    // Payloads join element-wise; the result is [disc] ++ joined.
    const auto &V = T.getVariant();
    std::vector<ValType> Flat;
    for (const auto &C : V.Cases) {
      if (!C.second.has_value()) {
        continue;
      }
      EXPECTED_TRY(auto Sub, flattenType(*C.second));
      for (size_t I = 0; I < Sub.size(); ++I) {
        if (I < Flat.size()) {
          Flat[I] = joinFlat(Flat[I], Sub[I]);
        } else {
          Flat.push_back(Sub[I]);
        }
      }
    }
    // The discriminant flattens to [i32] at every width.
    std::vector<ValType> Result{I32T};
    Result.insert(Result.end(), Flat.begin(), Flat.end());
    return Result;
  }

  if (T.isOptionTy()) {
    // option<T> = variant{none | some(T)}.
    EXPECTED_TRY(auto Sub, flattenType(T.getOption().ValTy));
    std::vector<ValType> Result{I32T};
    Result.insert(Result.end(), Sub.begin(), Sub.end());
    return Result;
  }

  if (T.isResultTy()) {
    // result<T,E> = variant{ok(T)? | err(E)?}.
    const auto &R = T.getResult();
    std::vector<ValType> Flat;
    auto Fold = [&](const std::optional<ComponentValType> &V) -> Expect<void> {
      if (!V.has_value()) {
        return {};
      }
      EXPECTED_TRY(auto Sub, flattenType(*V));
      for (size_t I = 0; I < Sub.size(); ++I) {
        if (I < Flat.size()) {
          Flat[I] = joinFlat(Flat[I], Sub[I]);
        } else {
          Flat.push_back(Sub[I]);
        }
      }
      return {};
    };
    EXPECTED_TRY(Fold(R.ValTy));
    EXPECTED_TRY(Fold(R.ErrTy));
    std::vector<ValType> Result{I32T};
    Result.insert(Result.end(), Flat.begin(), Flat.end());
    return Result;
  }

  if (T.isMapTy()) {
    return std::vector<ValType>{getPtrType(), getPtrType()};
  }

  if (T.isListTy()) {
    // no-len gives [ptr, len]; with-len repeats the element flattening.
    const auto &L = T.getList();
    if (L.Len.has_value()) {
      EXPECTED_TRY(auto Sub, flattenType(L.ValTy));
      std::vector<ValType> Result;
      Result.reserve(static_cast<size_t>(Sub.size()) * *L.Len);
      for (uint32_t I = 0; I < *L.Len; ++I) {
        Result.insert(Result.end(), Sub.begin(), Sub.end());
      }
      return Result;
    }
    return std::vector<ValType>{getPtrType(), getPtrType()};
  }

  if (T.isFlagsTy()) {
    // A flags type is capped at 32 labels, so a single i32 is enough.
    return std::vector<ValType>{I32T};
  }

  if (T.isEnumTy()) {
    return std::vector<ValType>{I32T};
  }

  if (T.isOwnTy() || T.isBorrowTy()) {
    return std::vector<ValType>{I32T};
  }

  if (T.isStreamTy() || T.isFutureTy()) {
    // Stream/future ends travel as single handle indices.
    return std::vector<ValType>{I32T};
  }

  spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
  spdlog::error("    canonical ABI: flatten of gated value type"sv);
  return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
}

Expect<AST::FunctionType>
LiftLowerContext::flattenFuncType(const AST::Component::FuncType &FnType,
                                  bool IsLift, bool Async,
                                  bool Callback) const noexcept {
  // The shape follows the canon options, not the asyncness of the type.
  AST::FunctionType F;
  auto &Params = F.getParamTypes();
  auto &Results = F.getReturnTypes();

  // Flatten params.
  for (const auto &P : FnType.getParamList()) {
    EXPECTED_TRY(auto Sub, flattenType(P.getValType()));
    Params.insert(Params.end(), Sub.begin(), Sub.end());
  }

  // Flatten results.
  for (const auto &R : FnType.getResultList()) {
    EXPECTED_TRY(auto Sub, flattenType(R.getValType()));
    Results.insert(Results.end(), Sub.begin(), Sub.end());
  }

  // An indirect param or result is one pointer in the selected memory.
  const ValType Ptr = getPtrType();

  if (Async) {
    if (IsLift) {
      // Async lift: params spill past the cap; results carry the callback code.
      if (Params.size() > MaxFlatParams) {
        Params.assign(1, Ptr);
      }
      Results.clear();
      if (Callback) {
        Results.push_back(I32T);
      }
    } else {
      // Async lower: params spill, results become an out-pointer plus state.
      if (Params.size() > MaxFlatAsyncParams) {
        Params.assign(1, Ptr);
      }
      if (!Results.empty()) {
        Params.push_back(Ptr);
      }
      Results.assign(1, I32T);
    }
    return F;
  }

  // Params over the cap collapse to a single pointer in both directions.
  if (Params.size() > MaxFlatParams) {
    Params.assign(1, Ptr);
  }

  // Results over the cap.
  if (Results.size() > MaxFlatResults) {
    if (IsLift) {
      // The core function returns one pointer to the return area.
      Results.assign(1, Ptr);
    } else {
      // The trailing pointer is the caller's out-pointer for the lowered tuple.
      Params.push_back(Ptr);
      Results.clear();
    }
  }

  return F;
}

bool LiftLowerContext::valTypeEq(
    const Runtime::Instance::ComponentInstance *AInst,
    const AST::Component::DefValType &A,
    const Runtime::Instance::ComponentInstance *BInst,
    const AST::Component::DefValType &B, uint32_t Depth) noexcept {
  if (A.isPrimValType() && B.isPrimValType()) {
    return A.getPrimValType() == B.getPrimValType();
  }
  if (A.isRecordTy() && B.isRecordTy()) {
    const auto &RA = A.getRecord().LabelTypes;
    const auto &RB = B.getRecord().LabelTypes;
    if (RA.size() != RB.size()) {
      return false;
    }
    for (size_t I = 0; I < RA.size(); ++I) {
      if (RA[I].getLabel() != RB[I].getLabel() ||
          !LiftLowerContext::valTypeEq(AInst, RA[I].getValType(), BInst,
                                       RB[I].getValType(), Depth + 1)) {
        return false;
      }
    }
    return true;
  }
  if (A.isVariantTy() && B.isVariantTy()) {
    const auto &VA = A.getVariant().Cases;
    const auto &VB = B.getVariant().Cases;
    if (VA.size() != VB.size()) {
      return false;
    }
    for (size_t I = 0; I < VA.size(); ++I) {
      if (VA[I].first != VB[I].first ||
          VA[I].second.has_value() != VB[I].second.has_value()) {
        return false;
      }
      if (VA[I].second.has_value() &&
          !LiftLowerContext::valTypeEq(AInst, *VA[I].second, BInst,
                                       *VB[I].second, Depth + 1)) {
        return false;
      }
    }
    return true;
  }
  if (A.isListTy() && B.isListTy()) {
    return A.getList().Len == B.getList().Len &&
           LiftLowerContext::valTypeEq(AInst, A.getList().ValTy, BInst,
                                       B.getList().ValTy, Depth + 1);
  }
  if (A.isMapTy() && B.isMapTy()) {
    return LiftLowerContext::valTypeEq(AInst, A.getMap().KeyTy, BInst,
                                       B.getMap().KeyTy, Depth + 1) &&
           LiftLowerContext::valTypeEq(AInst, A.getMap().ValTy, BInst,
                                       B.getMap().ValTy, Depth + 1);
  }
  if (A.isTupleTy() && B.isTupleTy()) {
    const auto &TA = A.getTuple().Types;
    const auto &TB = B.getTuple().Types;
    if (TA.size() != TB.size()) {
      return false;
    }
    for (size_t I = 0; I < TA.size(); ++I) {
      if (!LiftLowerContext::valTypeEq(AInst, TA[I], BInst, TB[I], Depth + 1)) {
        return false;
      }
    }
    return true;
  }
  if (A.isFlagsTy() && B.isFlagsTy()) {
    return A.getFlags().Labels == B.getFlags().Labels;
  }
  if (A.isEnumTy() && B.isEnumTy()) {
    return A.getEnum().Labels == B.getEnum().Labels;
  }
  if (A.isOptionTy() && B.isOptionTy()) {
    return LiftLowerContext::valTypeEq(AInst, A.getOption().ValTy, BInst,
                                       B.getOption().ValTy, Depth + 1);
  }
  if (A.isResultTy() && B.isResultTy()) {
    const auto &RA = A.getResult();
    const auto &RB = B.getResult();
    if (RA.ValTy.has_value() != RB.ValTy.has_value() ||
        RA.ErrTy.has_value() != RB.ErrTy.has_value()) {
      return false;
    }
    if (RA.ValTy.has_value() &&
        !LiftLowerContext::valTypeEq(AInst, *RA.ValTy, BInst, *RB.ValTy,
                                     Depth + 1)) {
      return false;
    }
    if (RA.ErrTy.has_value() &&
        !LiftLowerContext::valTypeEq(AInst, *RA.ErrTy, BInst, *RB.ErrTy,
                                     Depth + 1)) {
      return false;
    }
    return true;
  }
  if ((A.isOwnTy() && B.isOwnTy()) || (A.isBorrowTy() && B.isBorrowTy())) {
    const uint32_t IA = A.isOwnTy() ? A.getOwn().Idx : A.getBorrow().Idx;
    const uint32_t IB = B.isOwnTy() ? B.getOwn().Idx : B.getBorrow().Idx;
    return AInst != nullptr && BInst != nullptr &&
           AInst->getTypeResource(IA) == BInst->getTypeResource(IB);
  }
  if (A.isStreamTy() && B.isStreamTy()) {
    const auto &SA = A.getStream().ValTy;
    const auto &SB = B.getStream().ValTy;
    if (SA.has_value() != SB.has_value()) {
      return false;
    }
    return !SA.has_value() ||
           LiftLowerContext::valTypeEq(AInst, *SA, BInst, *SB, Depth + 1);
  }
  if (A.isFutureTy() && B.isFutureTy()) {
    const auto &FA = A.getFuture().ValTy;
    const auto &FB = B.getFuture().ValTy;
    if (FA.has_value() != FB.has_value()) {
      return false;
    }
    return !FA.has_value() ||
           LiftLowerContext::valTypeEq(AInst, *FA, BInst, *FB, Depth + 1);
  }
  return false;
}

bool LiftLowerContext::valTypeEq(
    const Runtime::Instance::ComponentInstance *AInst,
    const ComponentValType &A,
    const Runtime::Instance::ComponentInstance *BInst,
    const ComponentValType &B, uint32_t Depth) noexcept {
  if (Depth > 100) {
    return false;
  }
  const bool AIdx = !A.isPrimValType();
  const bool BIdx = !B.isPrimValType();
  if (!AIdx && !BIdx) {
    return A.getCode() == B.getCode();
  }
  auto DefOf =
      [](const Runtime::Instance::ComponentInstance *Home, bool IsIdx,
         const ComponentValType &Ty) -> const AST::Component::DefType * {
    if (!IsIdx || Home == nullptr) {
      return nullptr;
    }
    const auto Found = Home->getType(Ty.getTypeIndex());
    return Found ? *Found : nullptr;
  };
  const auto *DA = DefOf(AInst, AIdx, A);
  const auto *DB = DefOf(BInst, BIdx, B);
  if (AIdx != BIdx) {
    // One side is a primitive: the other must resolve to the same prim.
    const auto *D = AIdx ? DA : DB;
    if (D == nullptr || !D->isDefValType() ||
        !D->getDefValType().isPrimValType()) {
      return false;
    }
    return D->getDefValType().getPrimValType() ==
           (AIdx ? B : A).getPrimValType();
  }
  if (AInst == BInst && A.getTypeIndex() == B.getTypeIndex()) {
    return true;
  }
  if (DA != nullptr && DA == DB) {
    return true;
  }
  if (DA == nullptr || DB == nullptr || !DA->isDefValType() ||
      !DB->isDefValType()) {
    return false;
  }
  // The nested indices of an aliased definition are its owner's.
  const auto OwnerA = AInst->getTypeOwner(A.getTypeIndex());
  const auto OwnerB = BInst->getTypeOwner(B.getTypeIndex());
  return LiftLowerContext::valTypeEq(
      OwnerA && *OwnerA != nullptr ? *OwnerA : AInst, DA->getDefValType(),
      OwnerB && *OwnerB != nullptr ? *OwnerB : BInst, DB->getDefValType(),
      Depth);
}

bool LiftLowerContext::valTypeEq(
    const Runtime::Instance::ComponentInstance *AInst,
    const std::optional<ComponentValType> &A,
    const Runtime::Instance::ComponentInstance *BInst,
    const std::optional<ComponentValType> &B) noexcept {
  if (A.has_value() != B.has_value()) {
    return false;
  }
  return !A.has_value() || valTypeEq(AInst, *A, BInst, *B);
}

} // namespace Component
} // namespace Executor
} // namespace WasmEdge
