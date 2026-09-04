// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "executor/component/canonical_abi.h"

#include "canonical_abi_internal.h"
#include "common/spdlog.h"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace WasmEdge {
namespace Executor {
namespace Component {
namespace CanonicalABI {

using namespace std::literals;

namespace {

// Discriminant width: 1..256 cases → 1B, 257..65536 → 2B, else 4B.
constexpr uint32_t kU8Cases = 256;
constexpr uint32_t kU16Cases = 65536;

Expect<uint32_t> alignmentPrim(AST::Component::PrimValType PVT,
                               uint32_t PtrSize) noexcept {
  using P = AST::Component::PrimValType;
  switch (PVT) {
  case P::Bool:
  case P::S8:
  case P::U8:
    return 1u;
  case P::S16:
  case P::U16:
    return 2u;
  case P::S32:
  case P::U32:
  case P::F32:
  case P::Char:
    return 4u;
  case P::S64:
  case P::U64:
  case P::F64:
    return 8u;
  case P::String:
    // (ptr, len) pair — alignment of the pointer.
    return PtrSize;
  case P::ErrorContext:
    // An index into the instance's handles table.
    return 4u;
  default:
    spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
    spdlog::error("    canonical ABI: alignment of unknown prim 0x{:02x}"sv,
                  static_cast<uint8_t>(PVT));
    return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
  }
}

// Next power-of-two ≥ Bytes, used by the flags alignment.
constexpr uint32_t nextPow2(uint32_t Bytes) noexcept {
  uint32_t P = 1u;
  while (P < Bytes) {
    P <<= 1;
  }
  return P;
}
} // namespace

uint32_t discriminantSize(uint32_t NumCases) noexcept {
  assuming(NumCases > 0);
  if (NumCases <= kU8Cases) {
    return 1u;
  }
  if (NumCases <= kU16Cases) {
    return 2u;
  }
  return 4u;
}

// (map k v) is a specialization of (list (tuple k v)).
AST::Component::DefValType
mapEntryType(const AST::Component::MapTy &M) noexcept {
  AST::Component::TupleTy Tup;
  Tup.Types = {M.KeyTy, M.ValTy};
  AST::Component::DefValType Entry;
  Entry.setTuple(std::move(Tup));
  return Entry;
}

Expect<uint32_t> alignment(const Context &Cx,
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
    return alignmentDef(Cx, DT->getDefValType());
  }

  // PrimValType and ComponentTypeCode share byte values here.
  return alignmentPrim(
      static_cast<AST::Component::PrimValType>(static_cast<uint8_t>(Code)),
      Cx.ptrSize());
}

Expect<uint32_t> alignmentDef(const Context &Cx,
                              const AST::Component::DefValType &T) noexcept {
  if (T.isPrimValType()) {
    return alignmentPrim(T.getPrimValType(), Cx.ptrSize());
  }

  if (T.isRecordTy()) {
    // Max alignment of the fields, default 1.
    uint32_t Max = 1u;
    for (const auto &F : T.getRecord().LabelTypes) {
      EXPECTED_TRY(auto A, alignment(Cx, F.getValType()));
      Max = std::max(Max, A);
    }
    return Max;
  }

  if (T.isTupleTy()) {
    // A tuple flattens like a record.
    uint32_t Max = 1u;
    for (const auto &V : T.getTuple().Types) {
      EXPECTED_TRY(auto A, alignment(Cx, V));
      Max = std::max(Max, A);
    }
    return Max;
  }

  if (T.isVariantTy()) {
    // max(discriminant, the widest case payload).
    const auto &V = T.getVariant();
    const uint32_t NumCases = static_cast<uint32_t>(V.Cases.size());
    uint32_t Max = discriminantSize(NumCases);
    for (const auto &C : V.Cases) {
      if (C.second.has_value()) {
        EXPECTED_TRY(auto A, alignment(Cx, *C.second));
        Max = std::max(Max, A);
      }
    }
    return Max;
  }

  if (T.isOptionTy()) {
    // option<T> is variant{none | some(T)}: 2 cases, disc 1B.
    EXPECTED_TRY(auto A, alignment(Cx, T.getOption().ValTy));
    return std::max(1u, A);
  }

  if (T.isResultTy()) {
    // result<T,E> is variant{ok(T)? | err(E)?}: 2 cases, disc 1B.
    uint32_t Max = 1u;
    const auto &R = T.getResult();
    if (R.ValTy.has_value()) {
      EXPECTED_TRY(auto A, alignment(Cx, *R.ValTy));
      Max = std::max(Max, A);
    }
    if (R.ErrTy.has_value()) {
      EXPECTED_TRY(auto A, alignment(Cx, *R.ErrTy));
      Max = std::max(Max, A);
    }
    return Max;
  }

  if (T.isListTy()) {
    // no-len → the ptr/len pair; with-len → the element alignment.
    const auto &L = T.getList();
    if (L.Len.has_value()) {
      return alignment(Cx, L.ValTy);
    }
    return Cx.ptrSize();
  }

  if (T.isMapTy()) {
    // A map is a list without a length: the ptr/len pair.
    return Cx.ptrSize();
  }

  if (T.isFlagsTy()) {
    // next_pow2(ceil(|labels|/8)), with no labels → 1.
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
    return discriminantSize(NumCases);
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

namespace {

Expect<uint32_t> elemSizePrim(AST::Component::PrimValType PVT,
                              uint32_t PtrSize) noexcept {
  using P = AST::Component::PrimValType;
  switch (PVT) {
  case P::Bool:
  case P::S8:
  case P::U8:
    return 1u;
  case P::S16:
  case P::U16:
    return 2u;
  case P::S32:
  case P::U32:
  case P::F32:
  case P::Char:
    return 4u;
  case P::S64:
  case P::U64:
  case P::F64:
    return 8u;
  case P::String:
    // (ptr, len) pair.
    return 2u * PtrSize;
  case P::ErrorContext:
    return 4u;
  default:
    spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
    spdlog::error("    canonical ABI: elem_size of unknown prim 0x{:02x}"sv,
                  static_cast<uint8_t>(PVT));
    return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
  }
}
} // namespace

// Maximum payload alignment across a variant's cases, 1 if none.
Expect<uint32_t> maxCaseAlignment(
    const Context &Cx,
    const std::vector<std::pair<std::string, std::optional<ComponentValType>>>
        &Cases) noexcept {
  uint32_t M = 1u;
  for (const auto &C : Cases) {
    if (C.second.has_value()) {
      EXPECTED_TRY(auto A, alignment(Cx, *C.second));
      M = std::max(M, A);
    }
  }
  return M;
}

Expect<uint32_t> elemSize(const Context &Cx,
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
    return elemSizeDef(Cx, DT->getDefValType());
  }

  return elemSizePrim(
      static_cast<AST::Component::PrimValType>(static_cast<uint8_t>(Code)),
      Cx.ptrSize());
}

Expect<uint32_t> elemSizeDef(const Context &Cx,
                             const AST::Component::DefValType &T) noexcept {
  if (T.isPrimValType()) {
    return elemSizePrim(T.getPrimValType(), Cx.ptrSize());
  }

  // Track the max field alignment in-loop instead of re-walking.
  if (T.isRecordTy()) {
    uint32_t Off = 0u;
    uint32_t Max = 1u;
    for (const auto &F : T.getRecord().LabelTypes) {
      EXPECTED_TRY(auto A, alignment(Cx, F.getValType()));
      Max = std::max(Max, A);
      Off = alignTo(Off, A);
      EXPECTED_TRY(auto S, elemSize(Cx, F.getValType()));
      Off += S;
    }
    return alignTo(Off, Max);
  }

  if (T.isTupleTy()) {
    uint32_t Off = 0u;
    uint32_t Max = 1u;
    for (const auto &V : T.getTuple().Types) {
      EXPECTED_TRY(auto A, alignment(Cx, V));
      Max = std::max(Max, A);
      Off = alignTo(Off, A);
      EXPECTED_TRY(auto S, elemSize(Cx, V));
      Off += S;
    }
    return alignTo(Off, Max);
  }

  if (T.isVariantTy()) {
    // Variant alignment is max(disc, payloads); one pass gives both.
    const auto &V = T.getVariant();
    const uint32_t NumCases = static_cast<uint32_t>(V.Cases.size());
    const uint32_t Disc = discriminantSize(NumCases);
    uint32_t MaxAlign = 1u;
    uint32_t MaxSize = 0u;
    for (const auto &C : V.Cases) {
      if (C.second.has_value()) {
        EXPECTED_TRY(auto A, alignment(Cx, *C.second));
        MaxAlign = std::max(MaxAlign, A);
        EXPECTED_TRY(auto Sz, elemSize(Cx, *C.second));
        MaxSize = std::max(MaxSize, Sz);
      }
    }
    const uint32_t Aggr = std::max(Disc, MaxAlign);
    return alignTo(alignTo(Disc, MaxAlign) + MaxSize, Aggr);
  }

  if (T.isOptionTy()) {
    // option<T> = variant{none | some(T)}: disc 1B, one payload case.
    EXPECTED_TRY(auto A, alignment(Cx, T.getOption().ValTy));
    EXPECTED_TRY(auto PS, elemSize(Cx, T.getOption().ValTy));
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
        EXPECTED_TRY(auto A, alignment(Cx, *V));
        MaxAlign = std::max(MaxAlign, A);
        EXPECTED_TRY(auto S, elemSize(Cx, *V));
        MaxSize = std::max(MaxSize, S);
      }
      return {};
    };
    EXPECTED_TRY(consider(R.ValTy));
    EXPECTED_TRY(consider(R.ErrTy));
    const uint32_t Aggr = MaxAlign; // disc=1 ≤ MaxAlign
    return alignTo(alignTo(1u, MaxAlign) + MaxSize, Aggr);
  }

  if (T.isListTy()) {
    // no-len → ptr + len; with-len → len * the element size.
    const auto &L = T.getList();
    if (L.Len.has_value()) {
      EXPECTED_TRY(auto ElemSz, elemSize(Cx, L.ValTy));
      return static_cast<uint32_t>(static_cast<uint64_t>(*L.Len) *
                                   static_cast<uint64_t>(ElemSz));
    }
    return 2u * Cx.ptrSize();
  }

  if (T.isMapTy()) {
    return 2u * Cx.ptrSize();
  }

  if (T.isFlagsTy()) {
    // ceil(|labels|/8), aligned to the flags alignment.
    const auto &F = T.getFlags();
    const uint32_t Labels = static_cast<uint32_t>(F.Labels.size());
    const uint32_t Bytes = (Labels + 7u) / 8u;
    EXPECTED_TRY(auto A, alignmentDef(Cx, T));
    return alignTo(Bytes, A);
  }

  if (T.isEnumTy()) {
    const uint32_t NumCases = static_cast<uint32_t>(T.getEnum().Labels.size());
    return discriminantSize(NumCases);
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

Expect<std::vector<ValType>> flattenTypePrim(AST::Component::PrimValType PVT,
                                             ValType Ptr) noexcept {
  using P = AST::Component::PrimValType;
  switch (PVT) {
  case P::Bool:
  case P::S8:
  case P::U8:
  case P::S16:
  case P::U16:
  case P::S32:
  case P::U32:
  case P::Char:
    return std::vector<ValType>{I32T};
  case P::S64:
  case P::U64:
    return std::vector<ValType>{I64T};
  case P::F32:
    return std::vector<ValType>{F32T};
  case P::F64:
    return std::vector<ValType>{F64T};
  case P::String:
    // ptr + len; both the memory's address type.
    return std::vector<ValType>{Ptr, Ptr};
  case P::ErrorContext:
    return std::vector<ValType>{I32T};
  default:
    spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
    spdlog::error("    canonical ABI: flatten of unknown prim 0x{:02x}"sv,
                  static_cast<uint8_t>(PVT));
    return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
  }
}

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

Expect<std::vector<ValType>> flattenType(const Context &Cx,
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
    return flattenTypeDef(Cx, DT->getDefValType());
  }

  return flattenTypePrim(
      static_cast<AST::Component::PrimValType>(static_cast<uint8_t>(Code)),
      Cx.ptrType());
}

Expect<std::vector<ValType>>
flattenTypeDef(const Context &Cx,
               const AST::Component::DefValType &T) noexcept {
  if (T.isPrimValType()) {
    return flattenTypePrim(T.getPrimValType(), Cx.ptrType());
  }

  if (T.isRecordTy()) {
    std::vector<ValType> Flat;
    for (const auto &F : T.getRecord().LabelTypes) {
      EXPECTED_TRY(auto Sub, flattenType(Cx, F.getValType()));
      Flat.insert(Flat.end(), Sub.begin(), Sub.end());
    }
    return Flat;
  }

  if (T.isTupleTy()) {
    std::vector<ValType> Flat;
    for (const auto &V : T.getTuple().Types) {
      EXPECTED_TRY(auto Sub, flattenType(Cx, V));
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
      EXPECTED_TRY(auto Sub, flattenType(Cx, *C.second));
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
    EXPECTED_TRY(auto Sub, flattenType(Cx, T.getOption().ValTy));
    std::vector<ValType> Result{I32T};
    Result.insert(Result.end(), Sub.begin(), Sub.end());
    return Result;
  }

  if (T.isResultTy()) {
    // result<T,E> = variant{ok(T)? | err(E)?}.
    const auto &R = T.getResult();
    std::vector<ValType> Flat;
    auto fold = [&](const std::optional<ComponentValType> &V) -> Expect<void> {
      if (!V.has_value()) {
        return {};
      }
      EXPECTED_TRY(auto Sub, flattenType(Cx, *V));
      for (size_t I = 0; I < Sub.size(); ++I) {
        if (I < Flat.size()) {
          Flat[I] = joinFlat(Flat[I], Sub[I]);
        } else {
          Flat.push_back(Sub[I]);
        }
      }
      return {};
    };
    EXPECTED_TRY(fold(R.ValTy));
    EXPECTED_TRY(fold(R.ErrTy));
    std::vector<ValType> Result{I32T};
    Result.insert(Result.end(), Flat.begin(), Flat.end());
    return Result;
  }

  if (T.isMapTy()) {
    return std::vector<ValType>{Cx.ptrType(), Cx.ptrType()};
  }

  if (T.isListTy()) {
    // no-len gives [ptr, len]; with-len repeats the element flattening.
    const auto &L = T.getList();
    if (L.Len.has_value()) {
      EXPECTED_TRY(auto Sub, flattenType(Cx, L.ValTy));
      std::vector<ValType> Result;
      Result.reserve(static_cast<size_t>(Sub.size()) * *L.Len);
      for (uint32_t I = 0; I < *L.Len; ++I) {
        Result.insert(Result.end(), Sub.begin(), Sub.end());
      }
      return Result;
    }
    return std::vector<ValType>{Cx.ptrType(), Cx.ptrType()};
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

Expect<AST::FunctionType> flattenFuncType(const Context &Cx,
                                          const AST::Component::FuncType &FT,
                                          bool IsLift, bool Async,
                                          bool Callback) noexcept {
  // The shape follows the canon options, not the asyncness of the type.
  AST::FunctionType F;
  auto &Params = F.getParamTypes();
  auto &Results = F.getReturnTypes();

  // Flatten params.
  for (const auto &P : FT.getParamList()) {
    EXPECTED_TRY(auto Sub, flattenType(Cx, P.getValType()));
    Params.insert(Params.end(), Sub.begin(), Sub.end());
  }

  // Flatten results.
  for (const auto &R : FT.getResultList()) {
    EXPECTED_TRY(auto Sub, flattenType(Cx, R.getValType()));
    Results.insert(Results.end(), Sub.begin(), Sub.end());
  }

  // An indirect param or result is one pointer in the selected memory.
  const ValType Ptr = Cx.ptrType();

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

} // namespace CanonicalABI
} // namespace Component
} // namespace Executor
} // namespace WasmEdge
