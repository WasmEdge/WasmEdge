// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "executor/component/canonical_abi.h"

#include "common/spdlog.h"

#include <algorithm>
#include <cstdint>

namespace WasmEdge {
namespace Executor {
namespace CanonicalABI {

using namespace std::literals;

namespace {

// CanonicalABI.md L1951-1956 (`def discriminant_type`):
//   match math.ceil(math.log2(n)/8):
//     case 0|1: U8;  case 2: U16;  case 3: U32
// i.e. 1..256 → 1B, 257..65536 → 2B, else → 4B.
constexpr uint32_t kU8Cases = 256;
constexpr uint32_t kU16Cases = 65536;

Expect<uint32_t> alignmentPrim(AST::Component::PrimValType PVT) noexcept {
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
    // (ptr, len) pair — alignment of the i32 ptr (L1930).
    return 4u;
  case P::ErrorContext:
    // error-context — deferred.
    spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
    spdlog::error(
        "    canonical ABI: alignment of error-context not implemented"sv);
    return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
  default:
    spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
    spdlog::error("    canonical ABI: alignment of unknown prim 0x{:02x}"sv,
                  static_cast<uint8_t>(PVT));
    return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
  }
}

// Next power-of-two ≥ Bytes, used by alignment_flags (L1971-1985).
constexpr uint32_t nextPow2(uint32_t Bytes) noexcept {
  uint32_t P = 1u;
  while (P < Bytes) {
    P <<= 1;
  }
  return P;
}

} // namespace

uint32_t discriminantSize(uint32_t NumCases) noexcept {
  // CanonicalABI.md L1951-1956.
  assuming(NumCases > 0);
  if (NumCases <= kU8Cases) {
    return 1u;
  }
  if (NumCases <= kU16Cases) {
    return 2u;
  }
  return 4u;
}

Expect<uint32_t>
alignment(const CanonCtx &Cx,
          const ComponentValType &T) noexcept {
  // CanonicalABI.md L1904.
  using TC = ComponentTypeCode;
  const TC Code = T.getCode();

  if (Code == TC::TypeIndex) {
    assuming(Cx.CompInst != nullptr);
    const auto *DT = Cx.CompInst->getType(T.getTypeIndex());
    if (DT == nullptr || !DT->isDefValType()) {
      spdlog::error(ErrCode::Value::InvalidTypeReference);
      spdlog::error(
          "    canonical ABI: type index {} does not refer to a value type"sv,
          T.getTypeIndex());
      return Unexpect(ErrCode::Value::InvalidTypeReference);
    }
    return alignmentDef(Cx, DT->getDefValType());
  }

  // PrimValType and ComponentTypeCode share byte values for the primitive
  // range (Bool=0x7F .. ErrContext=0x64). Forward to alignmentPrim.
  return alignmentPrim(
      static_cast<AST::Component::PrimValType>(static_cast<uint8_t>(Code)));
}

Expect<uint32_t>
alignmentDef(const CanonCtx &Cx,
             const AST::Component::DefValType &T) noexcept {
  // CanonicalABI.md L1904 (top-level match on type kind).
  if (T.isPrimValType()) {
    return alignmentPrim(T.getPrimValType());
  }

  if (T.isRecordTy()) {
    // alignment_record (L1935-1942): max alignment of fields, default 1.
    uint32_t Max = 1u;
    for (const auto &F : T.getRecord().LabelTypes) {
      EXPECTED_TRY(auto A, alignment(Cx, F.getValType()));
      Max = std::max(Max, A);
    }
    return Max;
  }

  if (T.isTupleTy()) {
    // Tuple flattens to record per spec (also L1935 path via despecialize).
    uint32_t Max = 1u;
    for (const auto &V : T.getTuple().Types) {
      EXPECTED_TRY(auto A, alignment(Cx, V));
      Max = std::max(Max, A);
    }
    return Max;
  }

  if (T.isVariantTy()) {
    // alignment_variant (L1948-1969): max(disc, max_case_alignment).
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
    // option<T> is variant{none | some(T)} — 2 cases, disc=1B.
    EXPECTED_TRY(auto A, alignment(Cx, T.getOption().ValTy));
    return std::max(1u, A);
  }

  if (T.isResultTy()) {
    // result<T,E> is variant{ok(T)? | err(E)?} — 2 cases, disc=1B.
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
    // alignment_list (L1927-1933): no-len → 4 (ptr/len pair);
    //                              with-len → alignment(elem).
    const auto &L = T.getList();
    if (L.Len.has_value()) {
      spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
      spdlog::error(
          "    canonical ABI: alignment of fixed-length list not implemented"sv);
      return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
    }
    return 4u;
  }

  if (T.isFlagsTy()) {
    // alignment_flags (L1971-1985): next_pow2(ceil(|labels|/8)), labels=0 → 1.
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
    const uint32_t NumCases =
        static_cast<uint32_t>(T.getEnum().Labels.size());
    return discriminantSize(NumCases);
  }

  if (T.isOwnTy() || T.isBorrowTy()) {
    // Resource handle is i32.
    return 4u;
  }

  // stream / future are deferred.
  spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
  spdlog::error("    canonical ABI: alignment of gated value type"sv);
  return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
}

namespace {

Expect<uint32_t> elemSizePrim(AST::Component::PrimValType PVT) noexcept {
  // CanonicalABI.md L1994-2008.
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
    // (ptr, len) pair — two i32s (L2009-2013).
    return 8u;
  case P::ErrorContext:
    spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
    spdlog::error(
        "    canonical ABI: elem_size of error-context not implemented"sv);
    return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
  default:
    spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
    spdlog::error("    canonical ABI: elem_size of unknown prim 0x{:02x}"sv,
                  static_cast<uint8_t>(PVT));
    return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
  }
}

// Maximum payload alignment across a variant's cases (`max_case_alignment`,
// CanonicalABI.md L1960-1969). 1 if no case has a payload.
Expect<uint32_t> maxCaseAlignment(
    const CanonCtx &Cx,
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

} // namespace

Expect<uint32_t> elemSize(const CanonCtx &Cx,
                          const ComponentValType &T) noexcept {
  // CanonicalABI.md L1990.
  using TC = ComponentTypeCode;
  const TC Code = T.getCode();

  if (Code == TC::TypeIndex) {
    assuming(Cx.CompInst != nullptr);
    const auto *DT = Cx.CompInst->getType(T.getTypeIndex());
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
      static_cast<AST::Component::PrimValType>(static_cast<uint8_t>(Code)));
}

Expect<uint32_t> elemSizeDef(const CanonCtx &Cx,
                             const AST::Component::DefValType &T) noexcept {
  if (T.isPrimValType()) {
    return elemSizePrim(T.getPrimValType());
  }

  // For records/tuples the aggregate alignment is the max of field alignments,
  // and elem_size_record (L2014-2024) tells us to align the trailing size to
  // it. We track Max in-loop instead of re-walking via alignmentDef.
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
    // elem_size_variant (L2025-2033). Variant alignment = max(disc, payload
    // alignments); compute payload max-align and max-size in a single pass.
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
    // option<T> = variant{none | some(T)} — disc 1B, single payload case.
    EXPECTED_TRY(auto A, alignment(Cx, T.getOption().ValTy));
    EXPECTED_TRY(auto PS, elemSize(Cx, T.getOption().ValTy));
    const uint32_t Aggr = std::max(1u, A);
    return alignTo(alignTo(1u, A) + PS, Aggr);
  }

  if (T.isResultTy()) {
    // result<T,E> = variant{ok(T)? | err(E)?} — disc 1B.
    const auto &R = T.getResult();
    uint32_t MaxAlign = 1u;
    uint32_t MaxSize = 0u;
    auto consider = [&](const std::optional<ComponentValType> &V)
        -> Expect<void> {
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
    // elem_size_list (L2009-2013): no-len → 8 (ptr + len).
    if (T.getList().Len.has_value()) {
      spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
      spdlog::error(
          "    canonical ABI: elem_size of fixed-length list not implemented"sv);
      return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
    }
    return 8u;
  }

  if (T.isFlagsTy()) {
    // elem_size_flags (L2035-2040): ceil(|labels|/8) aligned to alignment_flags.
    const auto &F = T.getFlags();
    const uint32_t Labels = static_cast<uint32_t>(F.Labels.size());
    const uint32_t Bytes = (Labels + 7u) / 8u;
    EXPECTED_TRY(auto A, alignmentDef(Cx, T));
    return alignTo(Bytes, A);
  }

  if (T.isEnumTy()) {
    const uint32_t NumCases =
        static_cast<uint32_t>(T.getEnum().Labels.size());
    return discriminantSize(NumCases);
  }

  if (T.isOwnTy() || T.isBorrowTy()) {
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

Expect<std::vector<ValType>>
flattenTypePrim(AST::Component::PrimValType PVT) noexcept {
  // CanonicalABI.md L2862-2870 / L2874 / L2875 (excluding gated rows).
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
    // ptr + len; both i32.
    return std::vector<ValType>{I32T, I32T};
  case P::ErrorContext:
    spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
    spdlog::error(
        "    canonical ABI: flatten of error-context not implemented"sv);
    return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
  default:
    spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
    spdlog::error("    canonical ABI: flatten of unknown prim 0x{:02x}"sv,
                  static_cast<uint8_t>(PVT));
    return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
  }
}

// CanonicalABI.md L2917-2920 `def join`.
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
flattenType(const CanonCtx &Cx, const ComponentValType &T) noexcept {
  using TC = ComponentTypeCode;
  const TC Code = T.getCode();

  if (Code == TC::TypeIndex) {
    assuming(Cx.CompInst != nullptr);
    const auto *DT = Cx.CompInst->getType(T.getTypeIndex());
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
      static_cast<AST::Component::PrimValType>(static_cast<uint8_t>(Code)));
}

Expect<std::vector<ValType>>
flattenTypeDef(const CanonCtx &Cx,
               const AST::Component::DefValType &T) noexcept {
  if (T.isPrimValType()) {
    return flattenTypePrim(T.getPrimValType());
  }

  if (T.isRecordTy()) {
    // flatten_record (L2890-2894).
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
    // flatten_variant (L2906-2915):
    //   payloads are joined element-wise; result is [disc] ++ joined.
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
    // Discriminant is u8/u16/u32 — flattens to [i32] in all cases (L2862-2867).
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
    auto fold = [&](const std::optional<ComponentValType> &V)
        -> Expect<void> {
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

  if (T.isListTy()) {
    // flatten_list (L2882-2885): no-len → [i32, i32] (ptr, len).
    if (T.getList().Len.has_value()) {
      spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
      spdlog::error(
          "    canonical ABI: flatten of fixed-length list not implemented"sv);
      return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
    }
    return std::vector<ValType>{I32T, I32T};
  }

  if (T.isFlagsTy()) {
    // L2874. Preview 2 caps |labels| ≤ 32, so a single i32 is always enough.
    return std::vector<ValType>{I32T};
  }

  if (T.isEnumTy()) {
    return std::vector<ValType>{I32T};
  }

  if (T.isOwnTy() || T.isBorrowTy()) {
    return std::vector<ValType>{I32T};
  }

  spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
  spdlog::error("    canonical ABI: flatten of gated value type"sv);
  return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
}

Expect<FlatFuncType>
flattenFuncType(const CanonCtx &Cx, const AST::Component::FuncType &FT,
                bool IsLift) noexcept {
  // CanonicalABI.md L2819-2832 (sync branch only).
  if (FT.isAsync()) {
    spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
    spdlog::error("    canonical ABI: async functype not implemented"sv);
    return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
  }

  FlatFuncType F;

  // Flatten params (L2820).
  for (const auto &P : FT.getParamList()) {
    EXPECTED_TRY(auto Sub, flattenType(Cx, P.getValType()));
    F.Params.insert(F.Params.end(), Sub.begin(), Sub.end());
  }

  // Flatten results.
  for (const auto &R : FT.getResultList()) {
    EXPECTED_TRY(auto Sub, flattenType(Cx, R.getValType()));
    F.Results.insert(F.Results.end(), Sub.begin(), Sub.end());
  }

  // Params over the cap → indirect-param path; deferred.
  if (F.Params.size() > MaxFlatParams) {
    spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
    spdlog::error(
        "    canonical ABI: param-side indirect (>{} flat params) not implemented"sv,
        MaxFlatParams);
    return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
  }

  // Results over the cap.
  if (F.Results.size() > MaxFlatResults) {
    if (IsLift) {
      // Spec L2826-2828: results = [ptr_type()]; core function returns a
      // single i32 pointer to the return area.
      F.Results.clear();
      F.Results.push_back(I32T);
    } else {
      // Spec L2829-2831: params += [ptr_type()]; results = [].
      // Lower-side indirect-return is not yet implemented.
      spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
      spdlog::error(
          "    canonical ABI: lower-side indirect-return (>{} flat results) not implemented"sv,
          MaxFlatResults);
      return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
    }
  }

  return F;
}

namespace {

// CanonicalABI.md L2172, L2223 (MAX_STRING_BYTE_LENGTH == MAX_LIST_BYTE_LENGTH).
constexpr uint32_t kMaxCanonByteLength = (1u << 28) - 1u;

// Wraps MemoryInstance::loadValue<T,N> with a runtime byte width. The
// template-arg comma must stay out of EXPECTED_TRY's macro expansion, which is
// why this dispatcher exists instead of inlining loadValue at each call site.
// CanonicalABI.md L2081-2083 (`load_int`).
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

// Helper: trap with a diagnostic that includes the spec-relevant facts.
[[nodiscard]] Expect<void> trapMemoryOOB(const std::string_view What,
                                         uint32_t Ptr, uint32_t Len) noexcept {
  spdlog::error(ErrCode::Value::MemoryOutOfBounds);
  spdlog::error("    canonical ABI: {} at ptr=0x{:x} len={} out of bounds"sv,
                What, Ptr, Len);
  return Unexpect(ErrCode::Value::MemoryOutOfBounds);
}

[[nodiscard]] Expect<void> trapDataInvalid(const std::string_view Msg) noexcept {
  spdlog::error(ErrCode::Value::MemoryOutOfBounds);
  spdlog::error("    canonical ABI: {}"sv, Msg);
  return Unexpect(ErrCode::Value::MemoryOutOfBounds);
}

// convert_i32_to_char (CanonicalABI.md L2135-2139): trap on >0x10FFFF or
// UTF-16 surrogate.
[[nodiscard]] Expect<void> validateUSV(uint32_t I) noexcept {
  if (I >= 0x110000u) {
    return trapDataInvalid("char code point out of range");
  }
  if (I >= 0xD800u && I <= 0xDFFFu) {
    return trapDataInvalid("char is a UTF-16 surrogate");
  }
  return {};
}

// Load a primitive at Ptr. CanonicalABI.md L2054-2065.
Expect<ComponentValVariant>
loadPrim(const CanonCtx &Cx, uint32_t Ptr,
         AST::Component::PrimValType PVT) noexcept {
  assuming(Cx.Mem != nullptr);
  using P = AST::Component::PrimValType;
  switch (PVT) {
  case P::Bool: {
    // convert_int_to_bool (L2088-2090): 0 → false, else true.
    uint32_t I = 0;
    EXPECTED_TRY(loadN<uint32_t>(*Cx.Mem, 1, I, Ptr));
    return ComponentValVariant{I != 0};
  }
  case P::S8: {
    int32_t V = 0;
    EXPECTED_TRY(loadN<int32_t>(*Cx.Mem, 1, V, Ptr));
    return ComponentValVariant{static_cast<int8_t>(V)};
  }
  case P::U8: {
    uint32_t V = 0;
    EXPECTED_TRY(loadN<uint32_t>(*Cx.Mem, 1, V, Ptr));
    return ComponentValVariant{static_cast<uint8_t>(V)};
  }
  case P::S16: {
    int32_t V = 0;
    EXPECTED_TRY(loadN<int32_t>(*Cx.Mem, 2, V, Ptr));
    return ComponentValVariant{static_cast<int16_t>(V)};
  }
  case P::U16: {
    uint32_t V = 0;
    EXPECTED_TRY(loadN<uint32_t>(*Cx.Mem, 2, V, Ptr));
    return ComponentValVariant{static_cast<uint16_t>(V)};
  }
  case P::S32: {
    int32_t V = 0;
    EXPECTED_TRY(Cx.Mem->loadValue<int32_t>(V, Ptr));
    return ComponentValVariant{V};
  }
  case P::U32: {
    uint32_t V = 0;
    EXPECTED_TRY(Cx.Mem->loadValue<uint32_t>(V, Ptr));
    return ComponentValVariant{V};
  }
  case P::S64: {
    int64_t V = 0;
    EXPECTED_TRY(Cx.Mem->loadValue<int64_t>(V, Ptr));
    return ComponentValVariant{V};
  }
  case P::U64: {
    uint64_t V = 0;
    EXPECTED_TRY(Cx.Mem->loadValue<uint64_t>(V, Ptr));
    return ComponentValVariant{V};
  }
  case P::F32: {
    float V = 0.f;
    EXPECTED_TRY(Cx.Mem->loadValue<float>(V, Ptr));
    // decode_i32_as_float canonicalises NaN payloads (L2106-2119). C++ NaN
    // handling already keeps the bit pattern, but downstream comparison may
    // need normalisation — return the value as-is for now; the visible
    // semantics are unchanged for non-NaN values.
    return ComponentValVariant{V};
  }
  case P::F64: {
    double V = 0.;
    EXPECTED_TRY(Cx.Mem->loadValue<double>(V, Ptr));
    return ComponentValVariant{V};
  }
  case P::Char: {
    uint32_t V = 0;
    EXPECTED_TRY(Cx.Mem->loadValue<uint32_t>(V, Ptr));
    EXPECTED_TRY(validateUSV(V));
    return ComponentValVariant{V};
  }
  case P::String: {
    // load_string (L2163-2225): UTF-8 only (other encodings deferred at the
    // canon-options level; see component_canon.cpp L164-169).
    uint32_t Begin = 0;
    uint32_t Tagged = 0;
    EXPECTED_TRY(Cx.Mem->loadValue<uint32_t>(Begin, Ptr));
    EXPECTED_TRY(Cx.Mem->loadValue<uint32_t>(Tagged, Ptr + 4u));
    const uint32_t ByteLen = Tagged;
    if (ByteLen > kMaxCanonByteLength) {
      EXPECTED_TRY(trapDataInvalid("string byte length exceeds MAX"));
    }
    if (!Cx.Mem->checkAccessBound(Begin, ByteLen)) {
      EXPECTED_TRY(trapMemoryOOB("string payload", Begin, ByteLen));
    }
    auto SV = Cx.Mem->getStringView(Begin, ByteLen);
    return ComponentValVariant{std::string(SV)};
  }
  case P::ErrorContext:
    spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
    spdlog::error("    canonical ABI: load of error-context not implemented"sv);
    return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
  default:
    spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
    spdlog::error("    canonical ABI: load of unknown prim 0x{:02x}"sv,
                  static_cast<uint8_t>(PVT));
    return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
  }
}

} // namespace

Expect<ComponentValVariant> load(const CanonCtx &Cx, uint32_t Ptr,
                                 const ComponentValType &T) noexcept {
  using TC = ComponentTypeCode;
  const TC Code = T.getCode();

  if (Code == TC::TypeIndex) {
    assuming(Cx.CompInst != nullptr);
    const auto *DT = Cx.CompInst->getType(T.getTypeIndex());
    if (DT == nullptr || !DT->isDefValType()) {
      spdlog::error(ErrCode::Value::InvalidTypeReference);
      spdlog::error(
          "    canonical ABI: type index {} does not refer to a value type"sv,
          T.getTypeIndex());
      return Unexpect(ErrCode::Value::InvalidTypeReference);
    }
    return loadDef(Cx, Ptr, DT->getDefValType());
  }

  return loadPrim(
      Cx, Ptr,
      static_cast<AST::Component::PrimValType>(static_cast<uint8_t>(Code)));
}

Expect<ComponentValVariant> loadDef(const CanonCtx &Cx, uint32_t Ptr,
                                    const AST::Component::DefValType
                                        &T) noexcept {
  if (T.isPrimValType()) {
    return loadPrim(Cx, Ptr, T.getPrimValType());
  }

  if (T.isRecordTy()) {
    // load_record (L2245-2251).
    RecordVal R;
    uint32_t Off = Ptr;
    for (const auto &F : T.getRecord().LabelTypes) {
      EXPECTED_TRY(auto A, alignment(Cx, F.getValType()));
      Off = alignTo(Off, A);
      EXPECTED_TRY(auto V, load(Cx, Off, F.getValType()));
      R.Fields.emplace_back(std::string(F.getLabel()), std::move(V));
      EXPECTED_TRY(auto S, elemSize(Cx, F.getValType()));
      Off += S;
    }
    return makeComponentVal(std::move(R));
  }

  if (T.isTupleTy()) {
    TupleVal Tu;
    uint32_t Off = Ptr;
    for (const auto &V : T.getTuple().Types) {
      EXPECTED_TRY(auto A, alignment(Cx, V));
      Off = alignTo(Off, A);
      EXPECTED_TRY(auto Val, load(Cx, Off, V));
      Tu.Values.push_back(std::move(Val));
      EXPECTED_TRY(auto S, elemSize(Cx, V));
      Off += S;
    }
    return makeComponentVal(std::move(Tu));
  }

  if (T.isVariantTy()) {
    // load_variant (L2263-2272).
    const auto &Vt = T.getVariant();
    const uint32_t NumCases = static_cast<uint32_t>(Vt.Cases.size());
    const uint32_t DiscSize = discriminantSize(NumCases);
    uint32_t Case = 0;
    EXPECTED_TRY(loadN<uint32_t>(*Cx.Mem, DiscSize, Case, Ptr));
    if (Case >= NumCases) {
      EXPECTED_TRY(trapDataInvalid("variant case index out of range"));
    }
    VariantVal VV;
    VV.Case = Case;
    if (Vt.Cases[Case].second.has_value()) {
      EXPECTED_TRY(auto MaxAlign, maxCaseAlignment(Cx, Vt.Cases));
      const uint32_t PayloadOff = alignTo(Ptr + DiscSize, MaxAlign);
      EXPECTED_TRY(auto PV, load(Cx, PayloadOff, *Vt.Cases[Case].second));
      VV.Payload = std::move(PV);
    }
    return makeComponentVal(std::move(VV));
  }

  if (T.isOptionTy()) {
    // option<T> = variant{none(0) | some(T)(1)}, disc 1B.
    uint32_t Disc = 0;
    EXPECTED_TRY(loadN<uint32_t>(*Cx.Mem, 1, Disc, Ptr));
    if (Disc >= 2u) {
      EXPECTED_TRY(trapDataInvalid("option discriminant out of range"));
    }
    OptionVal OV;
    if (Disc == 1u) {
      EXPECTED_TRY(auto A, alignment(Cx, T.getOption().ValTy));
      const uint32_t PayloadOff = alignTo(Ptr + 1u, A);
      EXPECTED_TRY(auto PV, load(Cx, PayloadOff, T.getOption().ValTy));
      OV.Value = std::move(PV);
    }
    return makeComponentVal(std::move(OV));
  }

  if (T.isResultTy()) {
    // result<T,E> = variant{ok(0)(T?) | err(1)(E?)}, disc 1B.
    const auto &R = T.getResult();
    uint32_t Disc = 0;
    EXPECTED_TRY(loadN<uint32_t>(*Cx.Mem, 1, Disc, Ptr));
    if (Disc >= 2u) {
      EXPECTED_TRY(trapDataInvalid("result discriminant out of range"));
    }
    ResultVal RV;
    RV.IsOk = (Disc == 0u);
    const std::optional<ComponentValType> &PT = RV.IsOk ? R.ValTy : R.ErrTy;
    if (PT.has_value()) {
      uint32_t MaxAlign = 1u;
      if (R.ValTy.has_value()) {
        EXPECTED_TRY(auto A, alignment(Cx, *R.ValTy));
        MaxAlign = std::max(MaxAlign, A);
      }
      if (R.ErrTy.has_value()) {
        EXPECTED_TRY(auto A, alignment(Cx, *R.ErrTy));
        MaxAlign = std::max(MaxAlign, A);
      }
      const uint32_t PayloadOff = alignTo(Ptr + 1u, MaxAlign);
      EXPECTED_TRY(auto PV, load(Cx, PayloadOff, *PT));
      RV.Payload = std::move(PV);
    }
    return makeComponentVal(std::move(RV));
  }

  if (T.isListTy()) {
    // load_list (L2226-2243).
    if (T.getList().Len.has_value()) {
      spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
      spdlog::error(
          "    canonical ABI: load of fixed-length list not implemented"sv);
      return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
    }
    uint32_t Begin = 0;
    uint32_t Length = 0;
    EXPECTED_TRY(Cx.Mem->loadValue<uint32_t>(Begin, Ptr));
    EXPECTED_TRY(Cx.Mem->loadValue<uint32_t>(Length, Ptr + 4u));
    EXPECTED_TRY(auto ElemAlign, alignment(Cx, T.getList().ValTy));
    EXPECTED_TRY(auto ElemSz, elemSize(Cx, T.getList().ValTy));
    // L2234: byte_length = length * elem_size > MAX → trap.
    uint64_t ByteLen64 =
        static_cast<uint64_t>(Length) * static_cast<uint64_t>(ElemSz);
    if (ByteLen64 > static_cast<uint64_t>(kMaxCanonByteLength)) {
      EXPECTED_TRY(trapDataInvalid("list byte length exceeds MAX"));
    }
    if (Begin != alignTo(Begin, ElemAlign)) {
      EXPECTED_TRY(trapDataInvalid("list pointer misaligned"));
    }
    const uint32_t ByteLen = static_cast<uint32_t>(ByteLen64);
    if (Length > 0u && !Cx.Mem->checkAccessBound(Begin, ByteLen)) {
      EXPECTED_TRY(trapMemoryOOB("list payload", Begin, ByteLen));
    }
    ListVal LV;
    LV.Elements.reserve(Length);
    for (uint32_t I = 0; I < Length; ++I) {
      EXPECTED_TRY(auto E,
                   load(Cx, Begin + I * ElemSz, T.getList().ValTy));
      LV.Elements.push_back(std::move(E));
    }
    return makeComponentVal(std::move(LV));
  }

  if (T.isFlagsTy()) {
    // load_flags (L2279-2288).
    const auto &F = T.getFlags();
    const uint32_t Labels = static_cast<uint32_t>(F.Labels.size());
    const uint32_t Bytes = (Labels + 7u) / 8u;
    uint64_t Raw = 0;
    if (Bytes > 0u) {
      // Preview 2: Labels ≤ 32 → at most 4 bytes.
      assuming(Bytes <= 4u);
      uint32_t V = 0;
      EXPECTED_TRY(loadN<uint32_t>(*Cx.Mem, Bytes, V, Ptr));
      Raw = V;
    }
    FlagsVal FV;
    FV.Bits.resize(Labels);
    for (uint32_t I = 0; I < Labels; ++I) {
      FV.Bits[I] = ((Raw >> I) & 1ull) != 0ull;
    }
    return makeComponentVal(std::move(FV));
  }

  if (T.isEnumTy()) {
    const uint32_t NumCases =
        static_cast<uint32_t>(T.getEnum().Labels.size());
    const uint32_t DiscSize = discriminantSize(NumCases);
    uint32_t Case = 0;
    EXPECTED_TRY(loadN<uint32_t>(*Cx.Mem, DiscSize, Case, Ptr));
    if (Case >= NumCases) {
      EXPECTED_TRY(trapDataInvalid("enum case index out of range"));
    }
    return makeComponentVal(EnumVal{Case});
  }

  // lift_own / lift_borrow (L2297-2303 / L2316-2322): resource-table
  // interaction is not yet implemented; the raw handle is preserved verbatim
  // so future support can pick it up.
  if (T.isOwnTy()) {
    uint32_t H = 0;
    EXPECTED_TRY(Cx.Mem->loadValue<uint32_t>(H, Ptr));
    return makeComponentVal(OwnVal{H});
  }

  if (T.isBorrowTy()) {
    uint32_t H = 0;
    EXPECTED_TRY(Cx.Mem->loadValue<uint32_t>(H, Ptr));
    return makeComponentVal(BorrowVal{H});
  }

  // stream / future are deferred.
  spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
  spdlog::error("    canonical ABI: load of gated value type"sv);
  return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
}

namespace {

Expect<void> storePrim(const CanonCtx &Cx, const ComponentValVariant &V,
                       AST::Component::PrimValType PVT,
                       uint32_t Ptr) noexcept {
  assuming(Cx.Mem != nullptr);
  using P = AST::Component::PrimValType;
  switch (PVT) {
  case P::Bool: {
    const uint32_t B = std::get<bool>(V) ? 1u : 0u;
    return storeN<uint32_t>(*Cx.Mem, 1, B, Ptr);
  }
  case P::S8: {
    const uint32_t Bits =
        static_cast<uint32_t>(static_cast<uint8_t>(std::get<int8_t>(V)));
    return storeN<uint32_t>(*Cx.Mem, 1, Bits, Ptr);
  }
  case P::U8:
    return storeN<uint32_t>(*Cx.Mem, 1, std::get<uint8_t>(V), Ptr);
  case P::S16: {
    const uint32_t Bits =
        static_cast<uint32_t>(static_cast<uint16_t>(std::get<int16_t>(V)));
    return storeN<uint32_t>(*Cx.Mem, 2, Bits, Ptr);
  }
  case P::U16:
    return storeN<uint32_t>(*Cx.Mem, 2, std::get<uint16_t>(V), Ptr);
  case P::S32:
    return Cx.Mem->storeValue<uint32_t>(
        static_cast<uint32_t>(std::get<int32_t>(V)), Ptr);
  case P::U32:
    return Cx.Mem->storeValue<uint32_t>(std::get<uint32_t>(V), Ptr);
  case P::S64:
    return Cx.Mem->storeValue<uint64_t>(
        static_cast<uint64_t>(std::get<int64_t>(V)), Ptr);
  case P::U64:
    return Cx.Mem->storeValue<uint64_t>(std::get<uint64_t>(V), Ptr);
  case P::F32:
    return Cx.Mem->storeValue<float>(std::get<float>(V), Ptr);
  case P::F64:
    return Cx.Mem->storeValue<double>(std::get<double>(V), Ptr);
  case P::Char: {
    const uint32_t I = std::get<uint32_t>(V);
    EXPECTED_TRY(validateUSV(I));
    return Cx.Mem->storeValue<uint32_t>(I, Ptr);
  }
  case P::String:
    // store_string (L2477+) requires realloc to allocate the payload buffer;
    // realloc plumbing not yet wired through CanonCtx.
    spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
    spdlog::error(
        "    canonical ABI: store of string requires realloc"sv);
    return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
  case P::ErrorContext:
    spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
    spdlog::error(
        "    canonical ABI: store of error-context not implemented"sv);
    return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
  default:
    spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
    spdlog::error("    canonical ABI: store of unknown prim 0x{:02x}"sv,
                  static_cast<uint8_t>(PVT));
    return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
  }
}

} // namespace

Expect<void> store(const CanonCtx &Cx, const ComponentValVariant &V,
                   const ComponentValType &T, uint32_t Ptr) noexcept {
  using TC = ComponentTypeCode;
  const TC Code = T.getCode();

  if (Code == TC::TypeIndex) {
    assuming(Cx.CompInst != nullptr);
    const auto *DT = Cx.CompInst->getType(T.getTypeIndex());
    if (DT == nullptr || !DT->isDefValType()) {
      spdlog::error(ErrCode::Value::InvalidTypeReference);
      spdlog::error(
          "    canonical ABI: type index {} does not refer to a value type"sv,
          T.getTypeIndex());
      return Unexpect(ErrCode::Value::InvalidTypeReference);
    }
    return storeDef(Cx, V, DT->getDefValType(), Ptr);
  }

  return storePrim(
      Cx, V,
      static_cast<AST::Component::PrimValType>(static_cast<uint8_t>(Code)),
      Ptr);
}

Expect<void> storeDef(const CanonCtx &Cx, const ComponentValVariant &V,
                      const AST::Component::DefValType &T,
                      uint32_t Ptr) noexcept {
  if (T.isPrimValType()) {
    return storePrim(Cx, V, T.getPrimValType(), Ptr);
  }

  if (T.isRecordTy()) {
    // store_record (L2696-2710).
    const auto VC = std::get<std::shared_ptr<ValComp>>(V);
    if (!VC) {
      EXPECTED_TRY(trapDataInvalid("store record: empty value"));
    }
    const auto &R = std::get<RecordVal>(VC->V);
    const auto &Fields = T.getRecord().LabelTypes;
    if (R.Fields.size() != Fields.size()) {
      EXPECTED_TRY(trapDataInvalid("store record: field count mismatch"));
    }
    uint32_t Off = Ptr;
    for (size_t I = 0; I < Fields.size(); ++I) {
      EXPECTED_TRY(auto A, alignment(Cx, Fields[I].getValType()));
      Off = alignTo(Off, A);
      EXPECTED_TRY(store(Cx, R.Fields[I].second, Fields[I].getValType(), Off));
      EXPECTED_TRY(auto S, elemSize(Cx, Fields[I].getValType()));
      Off += S;
    }
    return {};
  }

  if (T.isTupleTy()) {
    const auto VC = std::get<std::shared_ptr<ValComp>>(V);
    if (!VC) {
      EXPECTED_TRY(trapDataInvalid("store tuple: empty value"));
    }
    const auto &Tu = std::get<TupleVal>(VC->V);
    const auto &Types = T.getTuple().Types;
    if (Tu.Values.size() != Types.size()) {
      EXPECTED_TRY(trapDataInvalid("store tuple: value count mismatch"));
    }
    uint32_t Off = Ptr;
    for (size_t I = 0; I < Types.size(); ++I) {
      EXPECTED_TRY(auto A, alignment(Cx, Types[I]));
      Off = alignTo(Off, A);
      EXPECTED_TRY(store(Cx, Tu.Values[I], Types[I], Off));
      EXPECTED_TRY(auto S, elemSize(Cx, Types[I]));
      Off += S;
    }
    return {};
  }

  if (T.isVariantTy()) {
    // store_variant (L2711-2734).
    const auto VC = std::get<std::shared_ptr<ValComp>>(V);
    if (!VC) {
      EXPECTED_TRY(trapDataInvalid("store variant: empty value"));
    }
    const auto &Vv = std::get<VariantVal>(VC->V);
    const auto &Vt = T.getVariant();
    if (Vv.Case >= Vt.Cases.size()) {
      EXPECTED_TRY(trapDataInvalid("store variant: case out of range"));
    }
    const uint32_t DiscSize =
        discriminantSize(static_cast<uint32_t>(Vt.Cases.size()));
    EXPECTED_TRY(storeN<uint32_t>(*Cx.Mem, DiscSize, Vv.Case, Ptr));
    if (Vt.Cases[Vv.Case].second.has_value()) {
      if (!Vv.Payload.has_value()) {
        EXPECTED_TRY(trapDataInvalid("store variant: payload missing"));
      }
      EXPECTED_TRY(auto MaxAlign, maxCaseAlignment(Cx, Vt.Cases));
      const uint32_t PayloadOff = alignTo(Ptr + DiscSize, MaxAlign);
      EXPECTED_TRY(
          store(Cx, *Vv.Payload, *Vt.Cases[Vv.Case].second, PayloadOff));
    }
    return {};
  }

  if (T.isOptionTy()) {
    const auto VC = std::get<std::shared_ptr<ValComp>>(V);
    if (!VC) {
      EXPECTED_TRY(trapDataInvalid("store option: empty value"));
    }
    const auto &O = std::get<OptionVal>(VC->V);
    const uint32_t Disc = O.Value.has_value() ? 1u : 0u;
    EXPECTED_TRY(storeN<uint32_t>(*Cx.Mem, 1, Disc, Ptr));
    if (O.Value.has_value()) {
      EXPECTED_TRY(auto A, alignment(Cx, T.getOption().ValTy));
      const uint32_t PayloadOff = alignTo(Ptr + 1u, A);
      EXPECTED_TRY(store(Cx, *O.Value, T.getOption().ValTy, PayloadOff));
    }
    return {};
  }

  if (T.isResultTy()) {
    const auto VC = std::get<std::shared_ptr<ValComp>>(V);
    if (!VC) {
      EXPECTED_TRY(trapDataInvalid("store result: empty value"));
    }
    const auto &R = std::get<ResultVal>(VC->V);
    const auto &Rt = T.getResult();
    const uint32_t Disc = R.IsOk ? 0u : 1u;
    EXPECTED_TRY(storeN<uint32_t>(*Cx.Mem, 1, Disc, Ptr));
    const std::optional<ComponentValType> &PT = R.IsOk ? Rt.ValTy : Rt.ErrTy;
    if (PT.has_value()) {
      if (!R.Payload.has_value()) {
        EXPECTED_TRY(trapDataInvalid("store result: payload missing"));
      }
      uint32_t MaxAlign = 1u;
      if (Rt.ValTy.has_value()) {
        EXPECTED_TRY(auto A, alignment(Cx, *Rt.ValTy));
        MaxAlign = std::max(MaxAlign, A);
      }
      if (Rt.ErrTy.has_value()) {
        EXPECTED_TRY(auto A, alignment(Cx, *Rt.ErrTy));
        MaxAlign = std::max(MaxAlign, A);
      }
      const uint32_t PayloadOff = alignTo(Ptr + 1u, MaxAlign);
      EXPECTED_TRY(store(Cx, *R.Payload, *PT, PayloadOff));
    }
    return {};
  }

  if (T.isListTy()) {
    // store_list (L2674+) requires realloc; not yet wired.
    spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
    spdlog::error(
        "    canonical ABI: store of list requires realloc"sv);
    return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
  }

  if (T.isFlagsTy()) {
    // store_flags (L2735-2740).
    const auto VC = std::get<std::shared_ptr<ValComp>>(V);
    if (!VC) {
      EXPECTED_TRY(trapDataInvalid("store flags: empty value"));
    }
    const auto &F = std::get<FlagsVal>(VC->V);
    const auto &Ft = T.getFlags();
    if (F.Bits.size() != Ft.Labels.size()) {
      EXPECTED_TRY(trapDataInvalid("store flags: bit count mismatch"));
    }
    const uint32_t Bytes =
        static_cast<uint32_t>((Ft.Labels.size() + 7) / 8);
    uint64_t Packed = 0;
    for (size_t I = 0; I < F.Bits.size(); ++I) {
      if (F.Bits[I]) {
        Packed |= (1ull << I);
      }
    }
    if (Bytes > 0u) {
      assuming(Bytes <= 4u);
      EXPECTED_TRY(
          storeN<uint32_t>(*Cx.Mem, Bytes, static_cast<uint32_t>(Packed), Ptr));
    }
    return {};
  }

  if (T.isEnumTy()) {
    const auto VC = std::get<std::shared_ptr<ValComp>>(V);
    if (!VC) {
      EXPECTED_TRY(trapDataInvalid("store enum: empty value"));
    }
    const auto &E = std::get<EnumVal>(VC->V);
    const auto &Et = T.getEnum();
    if (E.Case >= Et.Labels.size()) {
      EXPECTED_TRY(trapDataInvalid("store enum: case out of range"));
    }
    const uint32_t DiscSize =
        discriminantSize(static_cast<uint32_t>(Et.Labels.size()));
    return storeN<uint32_t>(*Cx.Mem, DiscSize, E.Case, Ptr);
  }

  if (T.isOwnTy()) {
    const auto VC = std::get<std::shared_ptr<ValComp>>(V);
    if (!VC) {
      EXPECTED_TRY(trapDataInvalid("store own: empty value"));
    }
    const auto &O = std::get<OwnVal>(VC->V);
    return Cx.Mem->storeValue<uint32_t>(O.Handle, Ptr);
  }

  if (T.isBorrowTy()) {
    const auto VC = std::get<std::shared_ptr<ValComp>>(V);
    if (!VC) {
      EXPECTED_TRY(trapDataInvalid("store borrow: empty value"));
    }
    const auto &B = std::get<BorrowVal>(VC->V);
    return Cx.Mem->storeValue<uint32_t>(B.Handle, Ptr);
  }

  spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
  spdlog::error("    canonical ABI: store of gated value type"sv);
  return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
}

namespace {

// Load a list payload at (Begin, Length) for element type ElemT.
// Mirrors `load_list_from_range` (CanonicalABI.md L2233-2243) — used by
// liftFlat for `list<T>` after reading the (ptr, len) pair from the
// flat core values. Performs the spec trap_if checks.
Expect<ComponentValVariant>
liftListFromRange(const CanonCtx &Cx, uint32_t Begin, uint32_t Length,
                  const ComponentValType &ElemT) noexcept {
  EXPECTED_TRY(auto ElemAlign, alignment(Cx, ElemT));
  EXPECTED_TRY(auto ElemSz, elemSize(Cx, ElemT));
  uint64_t ByteLen64 =
      static_cast<uint64_t>(Length) * static_cast<uint64_t>(ElemSz);
  if (ByteLen64 > static_cast<uint64_t>(kMaxCanonByteLength)) {
    EXPECTED_TRY(trapDataInvalid("list byte length exceeds MAX"));
  }
  if (Begin != alignTo(Begin, ElemAlign)) {
    EXPECTED_TRY(trapDataInvalid("list pointer misaligned"));
  }
  const uint32_t ByteLen = static_cast<uint32_t>(ByteLen64);
  if (Length > 0u && !Cx.Mem->checkAccessBound(Begin, ByteLen)) {
    EXPECTED_TRY(trapMemoryOOB("list payload", Begin, ByteLen));
  }
  ListVal LV;
  LV.Elements.reserve(Length);
  for (uint32_t I = 0; I < Length; ++I) {
    EXPECTED_TRY(auto E, load(Cx, Begin + I * ElemSz, ElemT));
    LV.Elements.push_back(std::move(E));
  }
  return makeComponentVal(std::move(LV));
}

// CanonicalABI.md L2990-3001 — narrow-from-i32 with zero/sign extension.
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
liftFlatPrim(const CanonCtx &Cx, FlatIter &VI,
             AST::Component::PrimValType PVT) noexcept {
  using P = AST::Component::PrimValType;
  auto Next = VI.next();
  if (!Next.has_value() && PVT != P::String && PVT != P::ErrorContext) {
    EXPECTED_TRY(trapDataInvalid("lift_flat: flat iterator exhausted"));
  }
  switch (PVT) {
  case P::Bool: {
    // convert_int_to_bool (L2959): non-zero → true.
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
    return ComponentValVariant{Next->get<float>()};
  case P::F64:
    return ComponentValVariant{Next->get<double>()};
  case P::Char: {
    const uint32_t I = Next->get<uint32_t>();
    EXPECTED_TRY(validateUSV(I));
    return ComponentValVariant{I};
  }
  case P::String: {
    // lift_flat_string (L3010-3013): ptr + len pair from flat values.
    if (!Next.has_value()) {
      EXPECTED_TRY(trapDataInvalid("lift_flat: flat iterator exhausted"));
    }
    const uint32_t Ptr = Next->get<uint32_t>();
    auto LenV = VI.next();
    if (!LenV.has_value()) {
      EXPECTED_TRY(trapDataInvalid("lift_flat: flat iterator exhausted"));
    }
    const uint32_t Len = LenV->get<uint32_t>();
    if (Len > kMaxCanonByteLength) {
      EXPECTED_TRY(trapDataInvalid("string byte length exceeds MAX"));
    }
    if (!Cx.Mem->checkAccessBound(Ptr, Len)) {
      EXPECTED_TRY(trapMemoryOOB("string payload", Ptr, Len));
    }
    auto SV = Cx.Mem->getStringView(Ptr, Len);
    return ComponentValVariant{std::string(SV)};
  }
  case P::ErrorContext:
    spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
    spdlog::error(
        "    canonical ABI: lift_flat of error-context not implemented"sv);
    return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
  default:
    spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
    spdlog::error("    canonical ABI: lift_flat of unknown prim 0x{:02x}"sv,
                  static_cast<uint8_t>(PVT));
    return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
  }
}

} // namespace

Expect<ComponentValVariant> liftFlat(const CanonCtx &Cx, FlatIter &VI,
                                     const ComponentValType &T) noexcept {
  using TC = ComponentTypeCode;
  const TC Code = T.getCode();

  if (Code == TC::TypeIndex) {
    assuming(Cx.CompInst != nullptr);
    const auto *DT = Cx.CompInst->getType(T.getTypeIndex());
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
liftFlatDef(const CanonCtx &Cx, FlatIter &VI,
            const AST::Component::DefValType &T) noexcept {
  if (T.isPrimValType()) {
    return liftFlatPrim(Cx, VI, T.getPrimValType());
  }

  if (T.isRecordTy()) {
    // lift_flat_record (L3028-3040).
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
    // lift_flat_list (L3015-3026).
    if (T.getList().Len.has_value()) {
      spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
      spdlog::error(
          "    canonical ABI: lift_flat of fixed-length list not implemented"sv);
      return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
    }
    auto PtrV = VI.next();
    auto LenV = VI.next();
    if (!PtrV.has_value() || !LenV.has_value()) {
      EXPECTED_TRY(trapDataInvalid("lift_flat list: iterator exhausted"));
    }
    const uint32_t Begin = PtrV->get<uint32_t>();
    const uint32_t Length = LenV->get<uint32_t>();
    return liftListFromRange(Cx, Begin, Length, T.getList().ValTy);
  }

  if (T.isFlagsTy()) {
    // lift_flat_flags (L3074-3084).
    auto Next = VI.next();
    if (!Next.has_value()) {
      EXPECTED_TRY(trapDataInvalid("lift_flat flags: iterator exhausted"));
    }
    const uint32_t Raw = Next->get<uint32_t>();
    FlagsVal F;
    const uint32_t Labels =
        static_cast<uint32_t>(T.getFlags().Labels.size());
    F.Bits.resize(Labels);
    for (uint32_t I = 0; I < Labels; ++I) {
      F.Bits[I] = ((Raw >> I) & 1u) != 0u;
    }
    return makeComponentVal(std::move(F));
  }

  if (T.isEnumTy()) {
    auto Next = VI.next();
    if (!Next.has_value()) {
      EXPECTED_TRY(trapDataInvalid("lift_flat enum: iterator exhausted"));
    }
    const uint32_t Case = Next->get<uint32_t>();
    const uint32_t NumCases =
        static_cast<uint32_t>(T.getEnum().Labels.size());
    if (Case >= NumCases) {
      EXPECTED_TRY(trapDataInvalid("enum case index out of range"));
    }
    return makeComponentVal(EnumVal{Case});
  }

  if (T.isOwnTy()) {
    auto Next = VI.next();
    if (!Next.has_value()) {
      EXPECTED_TRY(trapDataInvalid("lift_flat own: iterator exhausted"));
    }
    return makeComponentVal(OwnVal{Next->get<uint32_t>()});
  }

  if (T.isBorrowTy()) {
    auto Next = VI.next();
    if (!Next.has_value()) {
      EXPECTED_TRY(trapDataInvalid("lift_flat borrow: iterator exhausted"));
    }
    return makeComponentVal(BorrowVal{Next->get<uint32_t>()});
  }

  if (T.isVariantTy() || T.isOptionTy() || T.isResultTy()) {
    // lift_flat_variant (L3042-3072): disc + per-case coerced payload reads.
    // Only the payload-free shape is implemented — payload-bearing variants
    // require CoerceValueIter and only become reachable from param-side
    // direct-path lift, which isn't on yet.
    bool AnyPayload = false;
    size_t NumCases = 0;
    if (T.isVariantTy()) {
      for (const auto &C : T.getVariant().Cases) {
        if (C.second.has_value()) {
          AnyPayload = true;
          break;
        }
      }
      NumCases = T.getVariant().Cases.size();
    } else if (T.isOptionTy()) {
      AnyPayload = true; // some(T) always carries a payload by construction
      NumCases = 2;
    } else {
      AnyPayload = T.getResult().ValTy.has_value() ||
                   T.getResult().ErrTy.has_value();
      NumCases = 2;
    }
    if (AnyPayload) {
      spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
      spdlog::error(
          "    canonical ABI: lift_flat of variant/option/result with payload not implemented"sv);
      return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
    }
    auto Next = VI.next();
    if (!Next.has_value()) {
      EXPECTED_TRY(trapDataInvalid("lift_flat variant: iterator exhausted"));
    }
    const uint32_t Case = Next->get<uint32_t>();
    if (Case >= NumCases) {
      EXPECTED_TRY(trapDataInvalid("variant case index out of range"));
    }
    if (T.isVariantTy()) {
      VariantVal Vv;
      Vv.Case = Case;
      return makeComponentVal(std::move(Vv));
    }
    if (T.isOptionTy()) {
      // Unreachable in practice: option<T> always has a payload, so AnyPayload
      // would have short-circuited above.
      return makeComponentVal(OptionVal{});
    }
    ResultVal Rv;
    Rv.IsOk = (Case == 0);
    return makeComponentVal(std::move(Rv));
  }

  spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
  spdlog::error("    canonical ABI: lift_flat of gated value type"sv);
  return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
}

} // namespace CanonicalABI
} // namespace Executor
} // namespace WasmEdge
