// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "executor/component/canonical_abi.h"

#include "common/spdlog.h"
#include "executor/component/async_runtime.h"
#include "executor/executor.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <unordered_set>
#include <vector>

namespace WasmEdge {
namespace Executor {
namespace CanonicalABI {

namespace {
using namespace std::literals;
// Resolve the runtime resource identity for an own/borrow handle type.
const Runtime::Instance::ComponentInstance::ResourceTypeRT *
handleResource(const CanonCtx &Cx, uint32_t TypeIdx) noexcept {
  if (Cx.ResourceResolver) {
    return Cx.ResourceResolver(TypeIdx);
  }
  return Cx.CompInst != nullptr ? Cx.CompInst->getTypeResource(TypeIdx)
                                : nullptr;
}

// lift_own: transferring ownership out of the instance removes the handle.
Expect<uint32_t> liftOwnHandle(const CanonCtx &Cx, uint32_t TypeIdx,
                               uint32_t Idx) noexcept {
  const auto *RT = handleResource(Cx, TypeIdx);
  if (Cx.CompInst == nullptr || RT == nullptr) {
    // No table context (unit ABI tests): pass the raw value through.
    return Idx;
  }
  auto *Slot = Cx.CompInst->handleGet(Idx);
  if (Slot == nullptr) {
    spdlog::error(ErrCode::Value::ComponentHandleUnknown);
    spdlog::error("    canonical ABI: unknown handle index {}"sv, Idx);
    return Unexpect(ErrCode::Value::ComponentHandleUnknown);
  }
  if (Slot->RT != RT) {
    spdlog::error(ErrCode::Value::ComponentHandleWrongType);
    spdlog::error(
        "    canonical ABI: handle index {} used with the wrong type"sv, Idx);
    return Unexpect(ErrCode::Value::ComponentHandleWrongType);
  }
  if (!Slot->Own || Slot->Lends != 0) {
    spdlog::error(ErrCode::Value::ComponentResourceBorrowed);
    spdlog::error("    canonical ABI: own handle {} is lent or not owned"sv,
                  Idx);
    return Unexpect(ErrCode::Value::ComponentResourceBorrowed);
  }
  return Cx.CompInst->handleRemove(Idx)->Rep;
}

// lift_borrow: the handle stays; the rep travels for the call's duration.
Expect<uint32_t> liftBorrowHandle(const CanonCtx &Cx, uint32_t TypeIdx,
                                  uint32_t Idx) noexcept {
  const auto *RT = handleResource(Cx, TypeIdx);
  if (Cx.CompInst == nullptr || RT == nullptr) {
    return Idx;
  }
  // The owning instance passes representations directly for borrows
  // (CanonicalABI lift_borrow, owner case).
  if (RT->Impl == Cx.CompInst) {
    return Idx;
  }
  auto *Slot = Cx.CompInst->handleGet(Idx);
  if (Slot == nullptr) {
    spdlog::error(ErrCode::Value::ComponentHandleUnknown);
    spdlog::error("    canonical ABI: unknown handle index {}"sv, Idx);
    return Unexpect(ErrCode::Value::ComponentHandleUnknown);
  }
  if (Slot->RT != RT) {
    spdlog::error(ErrCode::Value::ComponentHandleWrongType);
    spdlog::error(
        "    canonical ABI: handle index {} used with the wrong type"sv, Idx);
    return Unexpect(ErrCode::Value::ComponentHandleWrongType);
  }
  // The borrow lends the handle for the duration of the call.
  Slot->Lends += 1;
  if (Cx.LiftedBorrows != nullptr) {
    Cx.LiftedBorrows->emplace_back(Cx.CompInst, Idx);
  }
  return Slot->Rep;
}

// lower_own / lower_borrow: entering an instance inserts a table entry.
uint32_t lowerHandle(const CanonCtx &Cx, uint32_t TypeIdx, uint32_t Rep,
                     bool Own) noexcept {
  const auto *RT = handleResource(Cx, TypeIdx);
  if (Cx.CompInst == nullptr || RT == nullptr) {
    return Rep;
  }
  // Borrows lowered into the owning instance receive the representation
  // directly (CanonicalABI lower_borrow, owner case).
  if (!Own && RT->Impl == Cx.CompInst) {
    return Rep;
  }
  if (!Own && Cx.BorrowTask != nullptr) {
    // The receiving task scopes the borrow (spec lower_borrow).
    Cx.BorrowTask->NumBorrows += 1;
    return Cx.CompInst->handleAdd(RT, Rep, Own, Cx.BorrowTask);
  }
  return Cx.CompInst->handleAdd(RT, Rep, Own);
}

namespace AC = Runtime::Instance::Component;

// lift_stream / lift_future: transferring the readable end removes it from
// the sender's table (CanonicalABI.md `lift_async_value`).
Expect<std::shared_ptr<void>> liftCopyEnd(const CanonCtx &Cx, bool IsStream,
                                          uint32_t Idx) noexcept {
  const auto WantKind = IsStream ? AC::WaitableObj::Kind::StreamRead
                                 : AC::WaitableObj::Kind::FutureRead;
  auto *W = Cx.CompInst != nullptr ? Cx.CompInst->waitableGet(Idx) : nullptr;
  if (W == nullptr || W->getKind() != WantKind) {
    spdlog::error(ErrCode::Value::ComponentHandleUnknown);
    spdlog::error("    unknown handle index {}"sv, Idx);
    return Unexpect(ErrCode::Value::ComponentHandleUnknown);
  }
  auto *E = static_cast<AC::CopyEndObj *>(W);
  if (E->St == AC::CopyState::Done && E->DoneByDrop) {
    const auto Code = IsStream
                          ? ErrCode::Value::ComponentStreamLiftAfterDrop
                          : ErrCode::Value::ComponentFutureLiftAfterSuccess;
    spdlog::error(Code);
    spdlog::error("    cannot lift {} after being notified that the writable "
                  "end dropped"sv,
                  IsStream ? "stream" : "future");
    return Unexpect(Code);
  }
  if (E->inWaitableSet()) {
    const auto Code = IsStream ? ErrCode::Value::ComponentStreamLiftInSet
                               : ErrCode::Value::ComponentFutureLiftInSet;
    spdlog::error(Code);
    spdlog::error("    cannot lift {} while it's in a waitable set"sv,
                  IsStream ? "stream" : "future");
    return Unexpect(Code);
  }
  if (E->St == AC::CopyState::Done) {
    const auto Code = IsStream
                          ? ErrCode::Value::ComponentStreamLiftAfterDrop
                          : ErrCode::Value::ComponentFutureLiftAfterSuccess;
    spdlog::error(Code);
    if (IsStream) {
      spdlog::error("    cannot lift stream after being notified that the "
                    "writable end dropped"sv);
    } else {
      spdlog::error("    cannot lift future after previous read succeeded"sv);
    }
    return Unexpect(Code);
  }
  if (E->copying()) {
    spdlog::error(ErrCode::Value::ComponentStreamRemoveBusy);
    spdlog::error("    cannot remove busy stream"sv);
    return Unexpect(ErrCode::Value::ComponentStreamRemoveBusy);
  }
  auto Shared = E->Shared;
  Cx.CompInst->waitableRemove(Idx);
  return std::static_pointer_cast<void>(Shared);
}

// lower_stream / lower_future: entering an instance mints a fresh readable
// end over the shared object.
Expect<uint32_t> lowerCopyEnd(const CanonCtx &Cx, bool IsStream,
                              const std::shared_ptr<void> &SharedV) noexcept {
  if (Cx.CompInst == nullptr || !SharedV) {
    spdlog::error(ErrCode::Value::ComponentTrap);
    spdlog::error("    cannot lower a stream or future without a table"sv);
    return Unexpect(ErrCode::Value::ComponentTrap);
  }
  auto Shared = std::static_pointer_cast<AC::SharedCopyObj>(SharedV);
  auto End = std::make_shared<AC::CopyEndObj>(
      IsStream ? AC::WaitableObj::Kind::StreamRead
               : AC::WaitableObj::Kind::FutureRead,
      Shared);
  auto *EndP = End.get();
  const uint32_t Idx = Cx.CompInst->waitableAdd(std::move(End));
  EndP->TableIdx = Idx;
  return Idx;
}
} // namespace

using namespace std::literals;

namespace {

// Invoke the guest's `realloc` core function. Returns the freshly-allocated
// address. Traps on invoke failure since a realloc shortage at this layer is
// unrecoverable.
Expect<uint32_t> callRealloc(const CanonCtx &Cx, uint32_t OldPtr,
                             uint32_t OldSize, uint32_t Align,
                             uint32_t NewSize) noexcept {
  if (Cx.Exec == nullptr || Cx.Realloc == nullptr) {
    spdlog::error(ErrCode::Value::ComponentTrap);
    spdlog::error("    canonical ABI: realloc required but not provided"sv);
    return Unexpect(ErrCode::Value::ComponentTrap);
  }
  std::array<ValVariant, 4> Args{ValVariant(OldPtr), ValVariant(OldSize),
                                 ValVariant(Align), ValVariant(NewSize)};
  auto ParamTypes = Cx.Realloc->getFuncType().getParamTypes();
  EXPECTED_TRY(auto Res, Cx.Exec->invoke(Cx.Realloc, Args, ParamTypes));
  if (Res.empty()) {
    spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
    spdlog::error("    canonical ABI: realloc returned no value"sv);
    return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
  }
  const uint32_t Ptr = Res[0].first.get<uint32_t>();
  // The spec only bounds-checks the returned pointer; returning 0 is a valid
  // allocation (a component may place a single live allocation at address 0).
  // Spec `trap_if(ptr + new_size > len(cx.opts.memory))`.
  if (Cx.Mem != nullptr) {
    const uint64_t End = uint64_t(Ptr) + uint64_t(NewSize);
    if (End > uint64_t(Cx.Mem->getPageSize()) * 65536ULL) {
      spdlog::error(ErrCode::Value::ComponentReallocOOB);
      spdlog::error(
          "    canonical ABI: realloc return: beyond end of memory"sv);
      return Unexpect(ErrCode::Value::ComponentReallocOOB);
    }
  }
  return Ptr;
}

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

// Host-supplied aggregate values may carry case/flag labels instead of
// resolved indices; map them against the declared type here at the
// lowering boundary.
uint32_t resolveVariantCase(const VariantVal &V,
                            const AST::Component::VariantTy &T) noexcept {
  if (!V.Label.empty()) {
    for (size_t I = 0; I < T.Cases.size(); ++I) {
      if (T.Cases[I].first == V.Label) {
        return static_cast<uint32_t>(I);
      }
    }
  }
  return V.Case;
}

uint32_t resolveEnumCase(const EnumVal &E,
                         const AST::Component::EnumTy &T) noexcept {
  if (!E.Label.empty()) {
    for (size_t I = 0; I < T.Labels.size(); ++I) {
      if (T.Labels[I] == E.Label) {
        return static_cast<uint32_t>(I);
      }
    }
  }
  return E.Case;
}

uint64_t packFlags(const FlagsVal &F,
                   const AST::Component::FlagsTy &T) noexcept {
  uint64_t Packed = 0;
  if (F.Bits.empty() && !F.SetLabels.empty()) {
    for (const auto &Label : F.SetLabels) {
      for (size_t I = 0; I < T.Labels.size(); ++I) {
        if (T.Labels[I] == Label) {
          Packed |= (1ull << I);
        }
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

Expect<uint32_t> alignment(const CanonCtx &Cx,
                           const ComponentValType &T) noexcept {
  // CanonicalABI.md L1904.
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

  // PrimValType and ComponentTypeCode share byte values for the primitive
  // range (Bool=0x7F .. ErrContext=0x64). Forward to alignmentPrim.
  return alignmentPrim(
      static_cast<AST::Component::PrimValType>(static_cast<uint8_t>(Code)));
}

Expect<uint32_t> alignmentDef(const CanonCtx &Cx,
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
      return alignment(Cx, L.ValTy);
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
    // elem_size_list (L2009-2013): no-len → 8 (ptr + len);
    //                              with-len → len * elem_size(elem).
    const auto &L = T.getList();
    if (L.Len.has_value()) {
      EXPECTED_TRY(auto ElemSz, elemSize(Cx, L.ValTy));
      return static_cast<uint32_t>(static_cast<uint64_t>(*L.Len) *
                                   static_cast<uint64_t>(ElemSz));
    }
    return 8u;
  }

  if (T.isFlagsTy()) {
    // elem_size_flags (L2035-2040): ceil(|labels|/8) aligned to
    // alignment_flags.
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

Expect<std::vector<ValType>> flattenType(const CanonCtx &Cx,
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

  if (T.isListTy()) {
    // flatten_list (L2882-2885): no-len → [i32, i32] (ptr, len);
    //                            with-len → flatten(elem) repeated len times.
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

  if (T.isStreamTy() || T.isFutureTy()) {
    // Stream/future ends travel as single handle indices.
    return std::vector<ValType>{I32T};
  }

  spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
  spdlog::error("    canonical ABI: flatten of gated value type"sv);
  return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
}

Expect<FlatFuncType> flattenFuncType(const CanonCtx &Cx,
                                     const AST::Component::FuncType &FT,
                                     bool IsLift, FlattenOpts Opts) noexcept {
  // CanonicalABI.md `flatten_functype`. The shape depends only on the canon
  // options, not on whether the function TYPE is async: a sync lift/lower
  // of an async-typed function uses the sync shape.
  FlatFuncType F;

  // Flatten params.
  for (const auto &P : FT.getParamList()) {
    EXPECTED_TRY(auto Sub, flattenType(Cx, P.getValType()));
    F.Params.insert(F.Params.end(), Sub.begin(), Sub.end());
  }

  // Flatten results.
  for (const auto &R : FT.getResultList()) {
    EXPECTED_TRY(auto Sub, flattenType(Cx, R.getValType()));
    F.Results.insert(F.Results.end(), Sub.begin(), Sub.end());
  }

  if (Opts.Async) {
    if (IsLift) {
      // Async lift: params spill beyond MAX_FLAT_PARAMS; the core function
      // returns the packed callback code (callback) or nothing (stackful).
      if (F.Params.size() > MaxFlatParams) {
        F.Params.clear();
        F.Params.push_back(I32T);
      }
      F.Results.clear();
      if (Opts.Callback) {
        F.Results.push_back(I32T);
      }
    } else {
      // Async lower: params spill beyond MAX_FLAT_ASYNC_PARAMS; any result
      // adds a trailing return-buffer pointer; the core function returns
      // the packed subtask state.
      if (F.Params.size() > MaxFlatAsyncParams) {
        F.Params.clear();
        F.Params.push_back(I32T);
      }
      if (!F.Results.empty()) {
        F.Params.push_back(I32T);
      }
      F.Results.clear();
      F.Results.push_back(I32T);
    }
    return F;
  }

  // Params over the cap → indirect-param path (spec L2823-2824 for lift;
  // L2829-2830 for lower). Both directions collapse to a single ptr_type.
  if (F.Params.size() > MaxFlatParams) {
    F.Params.clear();
    F.Params.push_back(I32T);
  }

  // Results over the cap.
  if (F.Results.size() > MaxFlatResults) {
    if (IsLift) {
      // Spec L2826-2828: results = [ptr_type()]; core function returns a
      // single i32 pointer to the return area.
      F.Results.clear();
      F.Results.push_back(I32T);
    } else {
      // Spec L2829-2831: params += [ptr_type()]; results = []. The trailing
      // i32 is the caller-provided out-pointer where the lowered tuple is
      // written; the thunk reads it from the end of the argument list.
      F.Params.push_back(I32T);
      F.Results.clear();
    }
  }

  return F;
}

namespace {

// Inner recursion for `containsListOrString`. The `Seen` set guards against
// cycles in mutually recursive type-index references.
bool containsListOrStringDef(const CanonCtx &Cx,
                             const AST::Component::DefValType &T,
                             std::unordered_set<uint32_t> &Seen) noexcept;
bool containsListOrStringImpl(const CanonCtx &Cx, const ComponentValType &T,
                              std::unordered_set<uint32_t> &Seen) noexcept {
  using TC = ComponentTypeCode;
  if (T.getCode() == TC::String) {
    return true;
  }
  if (T.getCode() != TC::TypeIndex) {
    return false;
  }
  const uint32_t Idx = T.getTypeIndex();
  if (!Seen.insert(Idx).second) {
    return false;
  }
  const auto *DT = resolveDefType(Cx, Idx);
  if (DT == nullptr || !DT->isDefValType()) {
    return false;
  }
  return containsListOrStringDef(Cx, DT->getDefValType(), Seen);
}
bool containsListOrStringDef(const CanonCtx &Cx,
                             const AST::Component::DefValType &T,
                             std::unordered_set<uint32_t> &Seen) noexcept {
  if (T.isPrimValType()) {
    return T.getPrimValType() == AST::Component::PrimValType::String;
  }
  if (T.isListTy()) {
    return true;
  }
  if (T.isRecordTy()) {
    for (const auto &F : T.getRecord().LabelTypes) {
      if (containsListOrStringImpl(Cx, F.getValType(), Seen)) {
        return true;
      }
    }
    return false;
  }
  if (T.isTupleTy()) {
    for (const auto &Ty : T.getTuple().Types) {
      if (containsListOrStringImpl(Cx, Ty, Seen)) {
        return true;
      }
    }
    return false;
  }
  if (T.isVariantTy()) {
    for (const auto &C : T.getVariant().Cases) {
      if (C.second.has_value() &&
          containsListOrStringImpl(Cx, *C.second, Seen)) {
        return true;
      }
    }
    return false;
  }
  if (T.isOptionTy()) {
    return containsListOrStringImpl(Cx, T.getOption().ValTy, Seen);
  }
  if (T.isResultTy()) {
    const auto &R = T.getResult();
    if (R.ValTy.has_value() && containsListOrStringImpl(Cx, *R.ValTy, Seen)) {
      return true;
    }
    if (R.ErrTy.has_value() && containsListOrStringImpl(Cx, *R.ErrTy, Seen)) {
      return true;
    }
    return false;
  }
  // flags / enum / own / borrow / stream / future contain neither list nor
  // string by construction.
  return false;
}

} // namespace

bool containsListOrString(const CanonCtx &Cx,
                          const ComponentValType &T) noexcept {
  std::unordered_set<uint32_t> Seen;
  return containsListOrStringImpl(Cx, T, Seen);
}

namespace {

// CanonicalABI.md L2172, L2223 (MAX_STRING_BYTE_LENGTH ==
// MAX_LIST_BYTE_LENGTH).
constexpr uint32_t kMaxCanonByteLength = (1u << 28) - 1u;

// CanonicalABI.md (definitions.py L1348-1367) NaN canonicalization. WasmEdge
// follows the DETERMINISTIC_PROFILE: every NaN crossing the canonical ABI — in
// either direction, via flat values or linear memory — collapses to the
// canonical quiet-NaN bit pattern instead of the non-deterministic profile's
// random scrambling, keeping float values reproducible across the boundary.
// The four primitive conversions (load / store / lift_flat / lower_flat) funnel
// through these, so floats nested in records / tuples / variants / lists are
// covered transitively.
constexpr uint32_t kCanonicalF32NaNBits = 0x7fc00000u;
constexpr uint64_t kCanonicalF64NaNBits = 0x7ff8000000000000ull;

float canonicalizeNaN32(float F) noexcept {
  if (std::isnan(F)) {
    float C = 0.f;
    std::memcpy(&C, &kCanonicalF32NaNBits, sizeof(C));
    return C;
  }
  return F;
}

double canonicalizeNaN64(double F) noexcept {
  if (std::isnan(F)) {
    double C = 0.;
    std::memcpy(&C, &kCanonicalF64NaNBits, sizeof(C));
    return C;
  }
  return F;
}

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

[[nodiscard]] Expect<void>
trapDataInvalid(const std::string_view Msg,
                ErrCode::Value Code = ErrCode::Value::ComponentTrap) noexcept {
  spdlog::error(Code);
  spdlog::error("    canonical ABI: {}"sv, Msg);
  return Unexpect(Code);
}

// convert_i32_to_char (CanonicalABI.md L2135-2139): trap on >0x10FFFF or
// UTF-16 surrogate. Used on load/lift paths where the value originates from
// guest memory or guest-supplied flat values.
[[nodiscard]] Expect<void> validateUSV(uint32_t I) noexcept {
  if (I >= 0x110000u) {
    return trapDataInvalid(
        "invalid `char` bit pattern: code point out of range",
        ErrCode::Value::ComponentCharInvalid);
  }
  if (I >= 0xD800u && I <= 0xDFFFu) {
    return trapDataInvalid("invalid `char` bit pattern: surrogate",
                           ErrCode::Value::ComponentCharInvalid);
  }
  return {};
}

// Used on store/lower paths where the char originates from a host-constructed
// ComponentValVariant — a malformed value here is a host-side bug, mirroring
// the spec's implicit assert on the producer side.
void assumeValidUSV(uint32_t I) noexcept {
  assuming(I < 0x110000u && (I < 0xD800u || I > 0xDFFFu));
}

// ---- String transcoding (CanonicalABI.md `string-encoding` option) ----------
// The host represents component strings as a UTF-8 std::string. decodeString
// converts the guest's on-the-wire bytes (in Cx.Enc) into that UTF-8 form (lift
// / load); encodeString does the reverse, allocating the guest buffer via
// realloc (lower / store). Only the host<->guest direction is transcoded; the
// host side is canonically UTF-8, which collapses the spec's full src/dst
// encoding matrix (store_string_into_range, L1592-1709) to the src='utf8' rows.

// utf16_tag for an i32 ptr_type (CanonicalABI.md L1388): high bit of the
// length.
constexpr uint32_t kUtf16Tag = 0x80000000u;

// Append a Unicode scalar value as UTF-8. Caller guarantees a valid USV.
void appendUtf8(std::string &Out, uint32_t CP) noexcept {
  if (CP < 0x80u) {
    Out.push_back(static_cast<char>(CP));
  } else if (CP < 0x800u) {
    Out.push_back(static_cast<char>(0xC0u | (CP >> 6)));
    Out.push_back(static_cast<char>(0x80u | (CP & 0x3Fu)));
  } else if (CP < 0x10000u) {
    Out.push_back(static_cast<char>(0xE0u | (CP >> 12)));
    Out.push_back(static_cast<char>(0x80u | ((CP >> 6) & 0x3Fu)));
    Out.push_back(static_cast<char>(0x80u | (CP & 0x3Fu)));
  } else {
    Out.push_back(static_cast<char>(0xF0u | (CP >> 18)));
    Out.push_back(static_cast<char>(0x80u | ((CP >> 12) & 0x3Fu)));
    Out.push_back(static_cast<char>(0x80u | ((CP >> 6) & 0x3Fu)));
    Out.push_back(static_cast<char>(0x80u | (CP & 0x3Fu)));
  }
}

// Decode one UTF-8 scalar starting at S[I], advancing I past it. Validates the
// sequence (bad lead/continuation byte, truncation, overlong form, surrogate,
// out-of-range) and traps on any malformed input. The single source of truth
// for UTF-8 decoding shared by validateUtf8 and utf8ToCodePoints.
Expect<uint32_t> decodeUtf8Scalar(std::string_view S, size_t &I) noexcept {
  const size_t N = S.size();
  const uint8_t B0 = static_cast<uint8_t>(S[I]);
  uint32_t CP = 0;
  size_t Len = 0;
  uint32_t Min = 0;
  if (B0 < 0x80u) {
    CP = B0;
    Len = 1;
    Min = 0;
  } else if ((B0 & 0xE0u) == 0xC0u) {
    CP = B0 & 0x1Fu;
    Len = 2;
    Min = 0x80u;
  } else if ((B0 & 0xF0u) == 0xE0u) {
    CP = B0 & 0x0Fu;
    Len = 3;
    Min = 0x800u;
  } else if ((B0 & 0xF8u) == 0xF0u) {
    CP = B0 & 0x07u;
    Len = 4;
    Min = 0x10000u;
  } else {
    EXPECTED_TRY(trapDataInvalid("invalid UTF-8 lead byte",
                                 ErrCode::Value::ComponentUTF8Invalid));
  }
  if (I + Len > N) {
    EXPECTED_TRY(trapDataInvalid("truncated UTF-8 sequence",
                                 ErrCode::Value::ComponentUTF8Incomplete));
  }
  for (size_t K = 1; K < Len; ++K) {
    const uint8_t B = static_cast<uint8_t>(S[I + K]);
    if ((B & 0xC0u) != 0x80u) {
      EXPECTED_TRY(trapDataInvalid("invalid UTF-8 continuation byte",
                                   ErrCode::Value::ComponentUTF8Invalid));
    }
    CP = (CP << 6) | (B & 0x3Fu);
  }
  if (CP < Min || CP > 0x10FFFFu || (CP >= 0xD800u && CP <= 0xDFFFu)) {
    EXPECTED_TRY(trapDataInvalid("invalid UTF-8 scalar value",
                                 ErrCode::Value::ComponentUTF8Invalid));
  }
  I += Len;
  return CP;
}

// Validate that S is well-formed UTF-8, trapping otherwise. Allocates nothing —
// used by the lift/load path, which only needs to reject malformed guest bytes
// before handing the raw view to the host.
Expect<void> validateUtf8(std::string_view S) noexcept {
  for (size_t I = 0; I < S.size();) {
    EXPECTED_TRY(auto CP, decodeUtf8Scalar(S, I));
    static_cast<void>(CP);
  }
  return {};
}

// Decode a UTF-8 host string into Unicode scalar values, validating as we go.
// Used by the encode side where the source is the host's UTF-8 std::string.
Expect<std::vector<uint32_t>> utf8ToCodePoints(std::string_view S) noexcept {
  std::vector<uint32_t> CPs;
  for (size_t I = 0; I < S.size();) {
    EXPECTED_TRY(auto CP, decodeUtf8Scalar(S, I));
    CPs.push_back(CP);
  }
  return CPs;
}

// Decode UTF-16-LE bytes into a UTF-8 string, pairing surrogates (L1395-1423).
Expect<std::string> utf16leToUtf8(Span<const Byte> Bytes) noexcept {
  std::string Out;
  const size_t N = Bytes.size();
  // Each UTF-16 code unit expands to at most 3 UTF-8 bytes (a surrogate pair is
  // two units → one 4-byte scalar, still ≤ 3 bytes/unit), so this never
  // under-reserves and avoids reallocation while appending.
  Out.reserve(N / 2 * 3);
  for (size_t I = 0; I + 1 < N; I += 2) {
    uint32_t U =
        static_cast<uint32_t>(static_cast<uint8_t>(Bytes[I])) |
        (static_cast<uint32_t>(static_cast<uint8_t>(Bytes[I + 1])) << 8);
    if (U >= 0xD800u && U <= 0xDBFFu) {
      if (I + 3 >= N) {
        EXPECTED_TRY(trapDataInvalid("unpaired UTF-16 high surrogate"));
      }
      const uint32_t L =
          static_cast<uint32_t>(static_cast<uint8_t>(Bytes[I + 2])) |
          (static_cast<uint32_t>(static_cast<uint8_t>(Bytes[I + 3])) << 8);
      if (L < 0xDC00u || L > 0xDFFFu) {
        EXPECTED_TRY(trapDataInvalid("invalid UTF-16 low surrogate"));
      }
      U = 0x10000u + ((U - 0xD800u) << 10) + (L - 0xDC00u);
      I += 2;
    } else if (U >= 0xDC00u && U <= 0xDFFFu) {
      EXPECTED_TRY(trapDataInvalid("unpaired UTF-16 low surrogate"));
    }
    appendUtf8(Out, U);
  }
  return Out;
}

// Encode Unicode scalar values to UTF-16-LE bytes (surrogate pair for >U+FFFF).
std::vector<Byte>
codePointsToUtf16le(const std::vector<uint32_t> &CPs) noexcept {
  std::vector<Byte> Out;
  Out.reserve(CPs.size() * 2);
  auto push16 = [&](uint32_t U) {
    Out.push_back(static_cast<Byte>(U & 0xFFu));
    Out.push_back(static_cast<Byte>((U >> 8) & 0xFFu));
  };
  for (uint32_t CP : CPs) {
    if (CP < 0x10000u) {
      push16(CP);
    } else {
      const uint32_t V = CP - 0x10000u;
      push16(0xD800u + (V >> 10));
      push16(0xDC00u + (V & 0x3FFu));
    }
  }
  return Out;
}

// load_string_from_range (CanonicalABI.md L1395-1423): decode tagged_code_units
// of guest bytes at Begin into the host UTF-8 string.
Expect<std::string> decodeString(const CanonCtx &Cx, uint32_t Begin,
                                 uint32_t TaggedCodeUnits) noexcept {
  enum class WireEnc { Utf8, Utf16, Latin1 } W = WireEnc::Utf8;
  uint32_t Alignment = 1;
  uint64_t ByteLen64 = 0;
  switch (Cx.Enc) {
  case StringEncoding::UTF8:
    W = WireEnc::Utf8;
    Alignment = 1;
    ByteLen64 = TaggedCodeUnits;
    break;
  case StringEncoding::UTF16:
    W = WireEnc::Utf16;
    Alignment = 2;
    ByteLen64 = 2ull * TaggedCodeUnits;
    break;
  case StringEncoding::Latin1UTF16:
    Alignment = 2;
    if ((TaggedCodeUnits & kUtf16Tag) != 0u) {
      W = WireEnc::Utf16;
      ByteLen64 = 2ull * (TaggedCodeUnits ^ kUtf16Tag);
    } else {
      W = WireEnc::Latin1;
      ByteLen64 = TaggedCodeUnits;
    }
    break;
  }
  if (ByteLen64 > static_cast<uint64_t>(kMaxCanonByteLength)) {
    EXPECTED_TRY(trapDataInvalid("string byte length exceeds MAX"));
  }
  const uint32_t ByteLen = static_cast<uint32_t>(ByteLen64);
  if (Begin != alignTo(Begin, Alignment)) {
    EXPECTED_TRY(trapDataInvalid("unaligned pointer for string",
                                 ErrCode::Value::ComponentPtrUnaligned));
  }
  if (!Cx.Mem->checkAccessBound(Begin, ByteLen)) {
    // The adapter between two components reports the fused-adapter
    // vocabulary; the host boundary reports the reference-interpreter one.
    const auto Code = Cx.CrossComponent ? ErrCode::Value::ComponentStrOOB
                                        : ErrCode::Value::ComponentStrPtrLenOOB;
    spdlog::error(Code);
    spdlog::error("    canonical ABI: string at 0x{:x} len={} out of bounds"sv,
                  Begin, ByteLen);
    return Unexpect(Code);
  }
  auto SV = Cx.Mem->getStringView(Begin, ByteLen);
  switch (W) {
  case WireEnc::Utf8: {
    // load_string utf8 (L1418-1421): the spec decodes as UTF-8 and traps on
    // malformed input, so validate before handing the bytes to the host (which
    // treats component strings as well-formed UTF-8).
    EXPECTED_TRY(validateUtf8(SV));
    return std::string(SV);
  }
  case WireEnc::Latin1: {
    std::string Out;
    Out.reserve(ByteLen);
    for (size_t I = 0; I < SV.size(); ++I) {
      appendUtf8(Out, static_cast<uint8_t>(SV[I]));
    }
    return Out;
  }
  case WireEnc::Utf16:
    return utf16leToUtf8(
        Span<const Byte>{reinterpret_cast<const Byte *>(SV.data()), SV.size()});
  }
  assumingUnreachable();
}

// store_string_into_range (CanonicalABI.md L1592-1709) with the host source
// always UTF-8. Allocates the guest buffer and returns (begin,
// tagged_code_units) ready to write into the (ptr, len) header.
Expect<std::pair<uint32_t, uint32_t>>
encodeString(const CanonCtx &Cx, const std::string &S) noexcept {
  // Helper: realloc a buffer, bounds-check it, copy `Bytes` in.
  auto writeBuf = [&](Span<const Byte> Bytes,
                      uint32_t Align) -> Expect<uint32_t> {
    const uint32_t Len = static_cast<uint32_t>(Bytes.size());
    // The spec calls realloc even for empty payloads and bounds-checks the
    // result, so a misbehaving realloc traps deterministically.
    EXPECTED_TRY(auto Begin, callRealloc(Cx, 0u, 0u, Align, Len));
    if (Begin != alignTo(Begin, Align)) {
      EXPECTED_TRY(trapDataInvalid("unaligned pointer for string buffer",
                                   ErrCode::Value::ComponentPtrUnaligned));
    }
    if (!Cx.Mem->checkAccessBound(Begin, Len)) {
      EXPECTED_TRY(trapMemoryOOB("string payload (post-realloc)", Begin, Len));
    }
    EXPECTED_TRY(Cx.Mem->setBytes(Bytes, Begin, 0u, Len));
    return Begin;
  };

  // Encode the code points as UTF-16-LE into a fresh 2-aligned guest buffer and
  // return (begin, tagged_code_units); `Tag` sets the high-bit utf16 marker for
  // the latin1+utf16 fallback (CanonicalABI.md L1705).
  auto writeUtf16 = [&](const std::vector<uint32_t> &CPs,
                        bool Tag) -> Expect<std::pair<uint32_t, uint32_t>> {
    auto Bytes = codePointsToUtf16le(CPs);
    const uint32_t Units = static_cast<uint32_t>(Bytes.size() / 2);
    EXPECTED_TRY(auto Begin, writeBuf(Bytes, 2u));
    return std::make_pair(Begin, Tag ? (Units | kUtf16Tag) : Units);
  };

  switch (Cx.Enc) {
  case StringEncoding::UTF8: {
    // store_string_copy utf8->utf8 (L1633): plain byte copy.
    Span<const Byte> Bytes{reinterpret_cast<const Byte *>(S.data()), S.size()};
    EXPECTED_TRY(auto Begin, writeBuf(Bytes, 1u));
    return std::make_pair(Begin, static_cast<uint32_t>(S.size()));
  }
  case StringEncoding::UTF16: {
    // store_utf8_to_utf16 (L1665): code units = bytes / 2.
    EXPECTED_TRY(auto CPs, utf8ToCodePoints(S));
    return writeUtf16(CPs, /*Tag=*/false);
  }
  case StringEncoding::Latin1UTF16: {
    // store_string_to_latin1_or_utf16 (L1689): latin1 when every scalar fits in
    // a byte, else utf16 with the high-bit tag. The buffer is 2-aligned either
    // way (the latin1 case may have been a utf16 fallback per spec). Build the
    // latin1 bytes in the same pass that checks the < 0x100 bound, falling back
    // to a full utf16 encode the moment an astral scalar appears.
    EXPECTED_TRY(auto CPs, utf8ToCodePoints(S));
    std::vector<Byte> Latin1;
    Latin1.reserve(CPs.size());
    bool AllLatin1 = true;
    for (uint32_t CP : CPs) {
      if (CP >= 0x100u) {
        AllLatin1 = false;
        break;
      }
      Latin1.push_back(static_cast<Byte>(CP));
    }
    if (AllLatin1) {
      EXPECTED_TRY(auto Begin, writeBuf(Latin1, 2u));
      return std::make_pair(Begin, static_cast<uint32_t>(CPs.size()));
    }
    return writeUtf16(CPs, /*Tag=*/true);
  }
  }
  assumingUnreachable();
}

// Load a primitive at Ptr. CanonicalABI.md L2054-2065.
Expect<ComponentValVariant> loadPrim(const CanonCtx &Cx, uint32_t Ptr,
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
    // decode_i32_as_float (L1326, L1363): canonicalize NaN on load.
    float V = 0.f;
    EXPECTED_TRY(Cx.Mem->loadValue<float>(V, Ptr));
    return ComponentValVariant{canonicalizeNaN32(V)};
  }
  case P::F64: {
    // decode_i64_as_float (L1327, L1366): canonicalize NaN on load.
    double V = 0.;
    EXPECTED_TRY(Cx.Mem->loadValue<double>(V, Ptr));
    return ComponentValVariant{canonicalizeNaN64(V)};
  }
  case P::Char: {
    uint32_t V = 0;
    EXPECTED_TRY(Cx.Mem->loadValue<uint32_t>(V, Ptr));
    EXPECTED_TRY(validateUSV(V));
    return ComponentValVariant{V};
  }
  case P::String: {
    // load_string (L1383-1423): read (begin, tagged_code_units), then decode
    // per the canon string-encoding option into the host's UTF-8 string.
    uint32_t Begin = 0;
    uint32_t Tagged = 0;
    EXPECTED_TRY(Cx.Mem->loadValue<uint32_t>(Begin, Ptr));
    EXPECTED_TRY(Cx.Mem->loadValue<uint32_t>(Tagged, Ptr + 4u));
    EXPECTED_TRY(auto Str, decodeString(Cx, Begin, Tagged));
    return ComponentValVariant{std::move(Str)};
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

namespace {
// Forward declaration: fixed-length list load reuses the same in-place element
// loader as variable-length lists, but liftListFromRange (internal linkage) is
// defined further below. Declared here so load() can call it.
Expect<ComponentValVariant>
liftListFromRange(const CanonCtx &Cx, uint32_t Begin, uint32_t Length,
                  const ComponentValType &ElemT) noexcept;
} // namespace

Expect<ComponentValVariant> load(const CanonCtx &Cx, uint32_t Ptr,
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
    return loadDef(Cx, Ptr, DT->getDefValType());
  }

  return loadPrim(
      Cx, Ptr,
      static_cast<AST::Component::PrimValType>(static_cast<uint8_t>(Code)));
}

Expect<ComponentValVariant>
loadDef(const CanonCtx &Cx, uint32_t Ptr,
        const AST::Component::DefValType &T) noexcept {
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
      EXPECTED_TRY(
          trapDataInvalid("invalid variant discriminant",
                          ErrCode::Value::ComponentDiscriminantInvalid));
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
      EXPECTED_TRY(
          trapDataInvalid("invalid variant discriminant for option",
                          ErrCode::Value::ComponentDiscriminantInvalid));
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
    // load_list (L2226-2243). with-len → load len elems in place at Ptr
    // (load_list_from_valid_range); no separate begin/length header.
    if (T.getList().Len.has_value()) {
      return liftListFromRange(Cx, Ptr, *T.getList().Len, T.getList().ValTy);
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
      EXPECTED_TRY(trapDataInvalid("unaligned pointer for list",
                                   ErrCode::Value::ComponentPtrUnaligned));
    }
    const uint32_t ByteLen = static_cast<uint32_t>(ByteLen64);
    if (Length > 0u && !Cx.Mem->checkAccessBound(Begin, ByteLen)) {
      EXPECTED_TRY(trapMemoryOOB("list payload", Begin, ByteLen));
    }
    ListVal LV;
    LV.Elements.reserve(Length);
    for (uint32_t I = 0; I < Length; ++I) {
      EXPECTED_TRY(auto E, load(Cx, Begin + I * ElemSz, T.getList().ValTy));
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
    const uint32_t NumCases = static_cast<uint32_t>(T.getEnum().Labels.size());
    const uint32_t DiscSize = discriminantSize(NumCases);
    uint32_t Case = 0;
    EXPECTED_TRY(loadN<uint32_t>(*Cx.Mem, DiscSize, Case, Ptr));
    if (Case >= NumCases) {
      EXPECTED_TRY(
          trapDataInvalid("invalid variant discriminant for enum",
                          ErrCode::Value::ComponentDiscriminantInvalid));
    }
    return makeComponentVal(EnumVal{Case, {}});
  }

  // lift_own / lift_borrow (L2297-2303 / L2316-2322): the transported value
  // carries the representation; the handle leaves (own) or stays (borrow).
  if (T.isOwnTy()) {
    uint32_t H = 0;
    EXPECTED_TRY(Cx.Mem->loadValue<uint32_t>(H, Ptr));
    EXPECTED_TRY(uint32_t Rep, liftOwnHandle(Cx, T.getOwn().Idx, H));
    return makeComponentVal(OwnVal{Rep});
  }

  if (T.isBorrowTy()) {
    uint32_t H = 0;
    EXPECTED_TRY(Cx.Mem->loadValue<uint32_t>(H, Ptr));
    EXPECTED_TRY(uint32_t Rep, liftBorrowHandle(Cx, T.getBorrow().Idx, H));
    return makeComponentVal(BorrowVal{Rep});
  }

  if (T.isStreamTy() || T.isFutureTy()) {
    const bool IsStream = T.isStreamTy();
    uint32_t H = 0;
    EXPECTED_TRY(Cx.Mem->loadValue<uint32_t>(H, Ptr));
    EXPECTED_TRY(auto Shared, liftCopyEnd(Cx, IsStream, H));
    return makeComponentVal(StreamFutureVal{std::move(Shared), IsStream});
  }

  spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
  spdlog::error("    canonical ABI: load of gated value type"sv);
  return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
}

namespace {

Expect<void> storePrim(const CanonCtx &Cx, const ComponentValVariant &V,
                       AST::Component::PrimValType PVT, uint32_t Ptr) noexcept {
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
    // encode_float_as_i32 (L1528, L1570): canonicalize NaN on store
    // (DETERMINISTIC_PROFILE).
    return Cx.Mem->storeValue<float>(canonicalizeNaN32(std::get<float>(V)),
                                     Ptr);
  case P::F64:
    // encode_float_as_i64 (L1529, L1573): canonicalize NaN on store.
    return Cx.Mem->storeValue<double>(canonicalizeNaN64(std::get<double>(V)),
                                      Ptr);
  case P::Char: {
    const uint32_t I = std::get<uint32_t>(V);
    assumeValidUSV(I);
    return Cx.Mem->storeValue<uint32_t>(I, Ptr);
  }
  case P::String: {
    // store_string (CanonicalABI.md L1587-1709): encode the host UTF-8 string
    // per the canon string-encoding option, then store the (begin,
    // tagged_code_units) pair at Ptr.
    EXPECTED_TRY(auto Enc, encodeString(Cx, std::get<std::string>(V)));
    EXPECTED_TRY(Cx.Mem->storeValue<uint32_t>(Enc.first, Ptr));
    EXPECTED_TRY(Cx.Mem->storeValue<uint32_t>(Enc.second, Ptr + 4u));
    return {};
  }
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
    const auto *DT = resolveDefType(Cx, T.getTypeIndex());
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
    const auto &VC = std::get<std::shared_ptr<ValComp>>(V);
    assuming(VC);
    const auto &R = std::get<RecordVal>(VC->V);
    const auto &Fields = T.getRecord().LabelTypes;
    assuming(R.Fields.size() == Fields.size());
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
    const auto &VC = std::get<std::shared_ptr<ValComp>>(V);
    assuming(VC);
    const auto &Tu = std::get<TupleVal>(VC->V);
    const auto &Types = T.getTuple().Types;
    assuming(Tu.Values.size() == Types.size());
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
    const auto &VC = std::get<std::shared_ptr<ValComp>>(V);
    assuming(VC);
    const auto &Vv = std::get<VariantVal>(VC->V);
    const auto &Vt = T.getVariant();
    const uint32_t Case = resolveVariantCase(Vv, Vt);
    assuming(Case < Vt.Cases.size());
    const uint32_t DiscSize =
        discriminantSize(static_cast<uint32_t>(Vt.Cases.size()));
    EXPECTED_TRY(storeN<uint32_t>(*Cx.Mem, DiscSize, Case, Ptr));
    if (Vt.Cases[Case].second.has_value()) {
      assuming(Vv.Payload.has_value());
      EXPECTED_TRY(auto MaxAlign, maxCaseAlignment(Cx, Vt.Cases));
      const uint32_t PayloadOff = alignTo(Ptr + DiscSize, MaxAlign);
      EXPECTED_TRY(store(Cx, *Vv.Payload, *Vt.Cases[Case].second, PayloadOff));
    }
    return {};
  }

  if (T.isOptionTy()) {
    const auto &VC = std::get<std::shared_ptr<ValComp>>(V);
    assuming(VC);
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
    const auto &VC = std::get<std::shared_ptr<ValComp>>(V);
    assuming(VC);
    const auto &R = std::get<ResultVal>(VC->V);
    const auto &Rt = T.getResult();
    const uint32_t Disc = R.IsOk ? 0u : 1u;
    EXPECTED_TRY(storeN<uint32_t>(*Cx.Mem, 1, Disc, Ptr));
    const std::optional<ComponentValType> &PT = R.IsOk ? Rt.ValTy : Rt.ErrTy;
    if (PT.has_value()) {
      assuming(R.Payload.has_value());
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
    // store_list (CanonicalABI.md L2674-2694). with-len →
    // store_list_into_valid_range: each element stored in place at Ptr, no
    // realloc and no (begin, length) header.
    if (T.getList().Len.has_value()) {
      const auto &L = T.getList();
      const uint32_t Len = *L.Len;
      const auto &VC = std::get<std::shared_ptr<ValComp>>(V);
      assuming(VC);
      const auto &Lv = std::get<ListVal>(VC->V);
      assuming(Lv.Elements.size() == Len);
      const auto &ElemT = L.ValTy;
      EXPECTED_TRY(auto ElemSz, elemSize(Cx, ElemT));
      for (uint32_t I = 0; I < Len; ++I) {
        EXPECTED_TRY(store(Cx, Lv.Elements[I], ElemT, Ptr + I * ElemSz));
      }
      return {};
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
    // The spec calls realloc even for empty lists and bounds-checks the
    // result, so a misbehaving realloc traps deterministically.
    EXPECTED_TRY(uint32_t Begin, callRealloc(Cx, 0u, 0u, ElemAlign, ByteLen));
    if (!Cx.Mem->checkAccessBound(Begin, ByteLen)) {
      EXPECTED_TRY(
          trapMemoryOOB("list payload (post-realloc)", Begin, ByteLen));
    }
    for (uint32_t I = 0; I < Length; ++I) {
      EXPECTED_TRY(store(Cx, Lv.Elements[I], ElemT, Begin + I * ElemSz));
    }
    EXPECTED_TRY(Cx.Mem->storeValue<uint32_t>(Begin, Ptr));
    EXPECTED_TRY(Cx.Mem->storeValue<uint32_t>(Length, Ptr + 4u));
    return {};
  }

  if (T.isFlagsTy()) {
    // store_flags (L2735-2740).
    const auto &VC = std::get<std::shared_ptr<ValComp>>(V);
    assuming(VC);
    const auto &F = std::get<FlagsVal>(VC->V);
    const auto &Ft = T.getFlags();
    assuming(F.Bits.empty() || F.Bits.size() == Ft.Labels.size());
    const uint32_t Bytes = static_cast<uint32_t>((Ft.Labels.size() + 7) / 8);
    const uint64_t Packed = packFlags(F, Ft);
    if (Bytes > 0u) {
      assuming(Bytes <= 4u);
      EXPECTED_TRY(
          storeN<uint32_t>(*Cx.Mem, Bytes, static_cast<uint32_t>(Packed), Ptr));
    }
    return {};
  }

  if (T.isEnumTy()) {
    const auto &VC = std::get<std::shared_ptr<ValComp>>(V);
    assuming(VC);
    const auto &E = std::get<EnumVal>(VC->V);
    const auto &Et = T.getEnum();
    const uint32_t Case = resolveEnumCase(E, Et);
    assuming(Case < Et.Labels.size());
    const uint32_t DiscSize =
        discriminantSize(static_cast<uint32_t>(Et.Labels.size()));
    return storeN<uint32_t>(*Cx.Mem, DiscSize, Case, Ptr);
  }

  if (T.isOwnTy()) {
    const auto &VC = std::get<std::shared_ptr<ValComp>>(V);
    assuming(VC);
    const auto &O = std::get<OwnVal>(VC->V);
    return Cx.Mem->storeValue<uint32_t>(
        lowerHandle(Cx, T.getOwn().Idx, O.Handle, true), Ptr);
  }

  if (T.isBorrowTy()) {
    const auto &VC = std::get<std::shared_ptr<ValComp>>(V);
    assuming(VC);
    const auto &B = std::get<BorrowVal>(VC->V);
    return Cx.Mem->storeValue<uint32_t>(
        lowerHandle(Cx, T.getBorrow().Idx, B.Handle, false), Ptr);
  }

  if (T.isStreamTy() || T.isFutureTy()) {
    const auto &VC = std::get<std::shared_ptr<ValComp>>(V);
    assuming(VC);
    const auto &SF = std::get<StreamFutureVal>(VC->V);
    EXPECTED_TRY(uint32_t Idx, lowerCopyEnd(Cx, T.isStreamTy(), SF.Shared));
    return Cx.Mem->storeValue<uint32_t>(Idx, Ptr);
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
    EXPECTED_TRY(trapDataInvalid("unaligned pointer for list",
                                 ErrCode::Value::ComponentPtrUnaligned));
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

// Bit-pattern reinterpret helpers for variant payload coerce
// (CanonicalABI.md L3047-3055 / L3168-3173). C++17 has no std::bit_cast,
// so the conversion goes through std::memcpy on a same-sized scratch slot.
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

// Spec L3066-3068 (`def wrap_i64_to_i32`).
inline uint32_t wrapI64ToI32(uint64_t V) noexcept {
  return static_cast<uint32_t>(V);
}

// CoerceValueIter.next() (spec L3047-3055): read a `Have`-typed flat slot
// from VI and reinterpret it into the `Want`-typed slot that the
// variant's selected case expects. The (have, want) pairs are limited
// to those produced by `joinFlat` (L2917-2920); other pairs are
// unreachable.
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

// Symmetric inverse of coerceLiftSlot (spec L3168-3173): widen / reinterpret
// a lowered slot from its native flat type `Have` into the variant's joined
// slot `Want`.
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
    // L3171: same numeric value, slot widened to occupy the joined i64 shape
    // expected by the downstream core caller.
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

// L3175-3176: tail-pad the lowered payload with a zero value typed to the
// joined slot, so downstream consumers see slots of the expected shape.
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
  assuming(Next.has_value() || PVT == P::String || PVT == P::ErrorContext);
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
    // lift_flat_float (L1932): canonicalize NaN.
    return ComponentValVariant{canonicalizeNaN32(Next->get<float>())};
  case P::F64:
    // lift_flat_float (L1933): canonicalize NaN.
    return ComponentValVariant{canonicalizeNaN64(Next->get<double>())};
  case P::Char: {
    const uint32_t I = Next->get<uint32_t>();
    EXPECTED_TRY(validateUSV(I));
    return ComponentValVariant{I};
  }
  case P::String: {
    // lift_flat string (L3010-3013): (ptr, tagged_code_units) from flat values,
    // then decode per the canon string-encoding option.
    assuming(Next.has_value());
    const uint32_t Ptr = Next->get<uint32_t>();
    auto LenV = VI.next();
    assuming(LenV.has_value());
    const uint32_t Tagged = LenV->get<uint32_t>();
    EXPECTED_TRY(auto Str, decodeString(Cx, Ptr, Tagged));
    return ComponentValVariant{std::move(Str)};
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
    // lift_flat_list (L3015-3026). with-len → lift len elements straight from
    // the flat iterator (no ptr/len pair, no memory load).
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
    const uint32_t Begin = PtrV->get<uint32_t>();
    const uint32_t Length = LenV->get<uint32_t>();
    return liftListFromRange(Cx, Begin, Length, T.getList().ValTy);
  }

  if (T.isFlagsTy()) {
    // lift_flat_flags (L3074-3084).
    auto Next = VI.next();
    assuming(Next.has_value());
    const uint32_t Raw = Next->get<uint32_t>();
    FlagsVal F;
    const uint32_t Labels = static_cast<uint32_t>(T.getFlags().Labels.size());
    F.Bits.resize(Labels);
    for (uint32_t I = 0; I < Labels; ++I) {
      F.Bits[I] = ((Raw >> I) & 1u) != 0u;
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
    EXPECTED_TRY(uint32_t Rep,
                 liftOwnHandle(Cx, T.getOwn().Idx, Next->get<uint32_t>()));
    return makeComponentVal(OwnVal{Rep});
  }

  if (T.isBorrowTy()) {
    auto Next = VI.next();
    assuming(Next.has_value());
    EXPECTED_TRY(uint32_t Rep, liftBorrowHandle(Cx, T.getBorrow().Idx,
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
    // lift_flat_variant (L3042-3072): read disc, then read the joined flat
    // slots, coercing the prefix that belongs to the selected case into the
    // case's native flat shape and draining the unused suffix.
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

// CanonicalABI.md L3118-3128 (`lower_flat_signed`): two's-complement reinterp
// into the unsigned width, then bucketed to i32 or i64.
std::vector<ValVariant> lowerSigned32(int32_t V) noexcept {
  return {ValVariant(static_cast<uint32_t>(V))};
}
std::vector<ValVariant> lowerSigned64(int64_t V) noexcept {
  return {ValVariant(static_cast<uint64_t>(V))};
}

Expect<std::vector<ValVariant>>
lowerFlatPrim(const CanonCtx &Cx, const ComponentValVariant &V,
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
    // lower_flat_float (L2026): canonicalize NaN (DETERMINISTIC_PROFILE).
    return std::vector<ValVariant>{
        ValVariant(canonicalizeNaN32(std::get<float>(V)))};
  case P::F64:
    // lower_flat_float (L2027): canonicalize NaN (DETERMINISTIC_PROFILE).
    return std::vector<ValVariant>{
        ValVariant(canonicalizeNaN64(std::get<double>(V)))};
  case P::Char: {
    const uint32_t I = std::get<uint32_t>(V);
    assumeValidUSV(I);
    return std::vector<ValVariant>{ValVariant(I)};
  }
  case P::String: {
    // lower_flat string (L3130-3132): encode per the canon string-encoding
    // option, push (ptr, tagged_code_units).
    EXPECTED_TRY(auto Enc, encodeString(Cx, std::get<std::string>(V)));
    return std::vector<ValVariant>{ValVariant(Enc.first),
                                   ValVariant(Enc.second)};
  }
  case P::ErrorContext:
    spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
    spdlog::error(
        "    canonical ABI: lower_flat of error-context not implemented"sv);
    return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
  default:
    spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
    spdlog::error("    canonical ABI: lower_flat of unknown prim 0x{:02x}"sv,
                  static_cast<uint8_t>(PVT));
    return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
  }
}

} // namespace

Expect<std::vector<ValVariant>> lowerFlat(const CanonCtx &Cx,
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
lowerFlatDef(const CanonCtx &Cx, const ComponentValVariant &V,
             const AST::Component::DefValType &T) noexcept {
  if (T.isPrimValType()) {
    return lowerFlatPrim(Cx, V, T.getPrimValType());
  }

  if (T.isRecordTy()) {
    // lower_flat_record (L3147-3156): concatenate per-field lowerings.
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
    // lower_flat_list (L3134-3145): no-len → realloc + store + push (ptr, len);
    // with-len → concatenate per-element lowerings straight into flat values.
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
    // The spec calls realloc even for empty lists and bounds-checks the
    // result, so a misbehaving realloc traps deterministically.
    EXPECTED_TRY(uint32_t Begin, callRealloc(Cx, 0u, 0u, ElemAlign, ByteLen));
    if (!Cx.Mem->checkAccessBound(Begin, ByteLen)) {
      EXPECTED_TRY(
          trapMemoryOOB("list payload (post-realloc)", Begin, ByteLen));
    }
    for (uint32_t I = 0; I < Length; ++I) {
      EXPECTED_TRY(store(Cx, Lv.Elements[I], ElemT, Begin + I * ElemSz));
    }
    return std::vector<ValVariant>{ValVariant(Begin), ValVariant(Length)};
  }

  if (T.isFlagsTy()) {
    // lower_flat_flags (L3182-3192). Preview 2: labels ≤ 32 → single i32.
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
    // lower_flat_variant (L3158-3180): lower the selected case's payload to
    // its native flat shape, then coerce each slot into the variant's joined
    // flat shape and tail-pad zeros up to the joined length.
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

// Synthesize a TupleTy over a span of component types. Shared by the
// indirect-path of lift_flat_values / lower_flat_values.
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
Expect<uint32_t> totalFlatCount(const CanonCtx &Cx,
                                Span<const ComponentValType> Types) noexcept {
  uint32_t N = 0;
  for (const auto &T : Types) {
    EXPECTED_TRY(auto Sub, flattenType(Cx, T));
    N += static_cast<uint32_t>(Sub.size());
  }
  return N;
}

} // namespace

Expect<std::vector<ComponentValVariant>>
liftFlatValues(const CanonCtx &Cx, FlatIter &VI,
               Span<const ComponentValType> Types, uint32_t MaxFlat) noexcept {
  EXPECTED_TRY(auto N, totalFlatCount(Cx, Types));

  if (N > MaxFlat) {
    // Indirect path (CanonicalABI.md L3195-3200).
    auto PtrV = VI.next();
    assuming(PtrV.has_value());
    const uint32_t Ptr = PtrV->get<uint32_t>();
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
lowerFlatValues(const CanonCtx &Cx, Span<const ComponentValVariant> Values,
                Span<const ComponentValType> Types, uint32_t MaxFlat,
                std::optional<uint32_t> OutParam) noexcept {
  assuming(Values.size() == Types.size());
  EXPECTED_TRY(auto N, totalFlatCount(Cx, Types));

  if (N > MaxFlat) {
    // Indirect path (CanonicalABI.md L3215-3226).
    auto Td = synthTupleType(Types);
    EXPECTED_TRY(auto Align, alignmentDef(Cx, Td));
    EXPECTED_TRY(auto Sz, elemSizeDef(Cx, Td));

    uint32_t Ptr = 0;
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

    // Walk the tuple layout in-line — matches the loop used by indirect-params
    // in convValsToCoreWASM. Per-field alignment is honored.
    uint32_t Off = 0u;
    for (size_t I = 0; I < Types.size(); ++I) {
      EXPECTED_TRY(auto FA, alignment(Cx, Types[I]));
      Off = alignTo(Off, FA);
      EXPECTED_TRY(store(Cx, Values[I], Types[I], Ptr + Off));
      EXPECTED_TRY(auto FS, elemSize(Cx, Types[I]));
      Off += FS;
    }

    if (ReturnPtr) {
      return std::vector<ValVariant>{ValVariant(Ptr)};
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
} // namespace Executor
} // namespace WasmEdge
