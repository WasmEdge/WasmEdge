// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- component_types.cpp - Component type system -----------------------===//
//
// The bodies of TypeSystem and Matcher.
//
//===----------------------------------------------------------------------===//
#include "validator/component_types.h"

#include "common/errinfo.h"
#include "common/spdlog.h"

#include <algorithm>
#include <string>
#include <unordered_set>

namespace WasmEdge {
namespace Validator {
namespace Component {

using namespace std::literals;

// ---------------------------------------------------------------------------
// Type view normalization.
// ---------------------------------------------------------------------------

const TypeEntry *TypeSystem::resolveQualType(const QualValType &Q,
                                             TypeEntry &Storage) noexcept {
  if (Q.VT.isPrimValType() || Q.Home == nullptr) {
    return nullptr;
  }
  const auto *Entry = Q.Home->getType(Q.VT.getTypeIndex());
  if (Entry == nullptr) {
    return nullptr;
  }
  Storage = *Entry;
  if (Entry->ResourceId.has_value()) {
    Storage.ResourceId = applyRemap(Q.Remap, *Entry->ResourceId);
  }
  Storage.Remap = composeRemap(Q.Remap, Entry->Remap);
  return &Storage;
}

std::optional<uint32_t>
TypeSystem::resolveResourceId(const Scope *Home, const ResourceMap *Remap,
                              uint32_t Idx) const noexcept {
  if (Home == nullptr) {
    return std::nullopt;
  }
  const auto *Entry = Home->getType(Idx);
  if (Entry == nullptr || !Entry->ResourceId.has_value()) {
    return std::nullopt;
  }
  return applyRemap(Remap, *Entry->ResourceId);
}

// Resolves through type-index and prim-alias indirections.
NormalVal TypeSystem::normalizeValType(const QualValType &Q) noexcept {
  if (Q.VT.isPrimValType()) {
    return {Q.VT.getCode(), nullptr, nullptr, nullptr, true};
  }
  TypeEntry Storage;
  const auto *Entry = resolveQualType(Q, Storage);
  if (Entry == nullptr) {
    return {};
  }
  return normalizeEntry(*Entry);
}

NormalVal TypeSystem::normalizeEntry(const TypeEntry &E) const noexcept {
  if (E.DT == nullptr || !E.DT->isDefValType()) {
    return {};
  }
  const auto &DVT = E.DT->getDefValType();
  if (DVT.isPrimValType()) {
    return {static_cast<ComponentTypeCode>(DVT.getPrimValType()), nullptr,
            nullptr, nullptr, true};
  }
  return {ComponentTypeCode::TypeIndex, &DVT, E.Home, E.Remap, true};
}

// ---------------------------------------------------------------------------
// Resource-id walkers over value types and views.
// ---------------------------------------------------------------------------

bool TypeSystem::containsBorrow(const QualValType &Q) noexcept {
  const auto N = normalizeValType(Q);
  if (!N.Valid || N.DVT == nullptr) {
    return false;
  }
  const auto &D = *N.DVT;
  auto Sub = [&](const ComponentValType &VT) noexcept {
    return containsBorrow({VT, N.Home, N.Remap});
  };
  if (D.isBorrowTy()) {
    return true;
  }
  if (D.isRecordTy()) {
    for (const auto &LT : D.getRecord().LabelTypes) {
      if (Sub(LT.getValType())) {
        return true;
      }
    }
    return false;
  }
  if (D.isVariantTy()) {
    for (const auto &[Label, Ty] : D.getVariant().Cases) {
      if (Ty.has_value() && Sub(*Ty)) {
        return true;
      }
    }
    return false;
  }
  if (D.isListTy()) {
    return Sub(D.getList().ValTy);
  }
  if (D.isTupleTy()) {
    for (const auto &Ty : D.getTuple().Types) {
      if (Sub(Ty)) {
        return true;
      }
    }
    return false;
  }
  if (D.isMapTy()) {
    return Sub(D.getMap().KeyTy) || Sub(D.getMap().ValTy);
  }
  if (D.isOptionTy()) {
    return Sub(D.getOption().ValTy);
  }
  if (D.isResultTy()) {
    const auto &R = D.getResult();
    return (R.ValTy.has_value() && Sub(*R.ValTy)) ||
           (R.ErrTy.has_value() && Sub(*R.ErrTy));
  }
  // stream and future do not recurse: their element type is borrow-free by
  // construction, since defining one rejects a borrow in its payload.
  return false;
}

void TypeSystem::collectResources(const QualValType &Q,
                                  std::unordered_set<uint32_t> &Out) noexcept {
  collectNormalValResources(normalizeValType(Q), Out);
}

void TypeSystem::collectNormalValResources(
    const NormalVal &N, std::unordered_set<uint32_t> &Out) noexcept {
  if (!N.Valid || N.DVT == nullptr) {
    return;
  }
  const auto &D = *N.DVT;
  auto Sub = [&](const ComponentValType &VT) noexcept {
    collectResources({VT, N.Home, N.Remap}, Out);
  };
  if (D.isOwnTy() || D.isBorrowTy()) {
    const uint32_t Idx = D.isOwnTy() ? D.getOwn().Idx : D.getBorrow().Idx;
    if (auto Id = resolveResourceId(N.Home, N.Remap, Idx)) {
      Out.insert(*Id);
    }
    return;
  }
  if (D.isRecordTy()) {
    for (const auto &LT : D.getRecord().LabelTypes) {
      Sub(LT.getValType());
    }
  } else if (D.isVariantTy()) {
    for (const auto &[Label, Ty] : D.getVariant().Cases) {
      if (Ty.has_value()) {
        Sub(*Ty);
      }
    }
  } else if (D.isListTy()) {
    Sub(D.getList().ValTy);
  } else if (D.isTupleTy()) {
    for (const auto &Ty : D.getTuple().Types) {
      Sub(Ty);
    }
  } else if (D.isMapTy()) {
    Sub(D.getMap().KeyTy);
    Sub(D.getMap().ValTy);
  } else if (D.isOptionTy()) {
    Sub(D.getOption().ValTy);
  } else if (D.isResultTy()) {
    if (D.getResult().ValTy.has_value()) {
      Sub(*D.getResult().ValTy);
    }
    if (D.getResult().ErrTy.has_value()) {
      Sub(*D.getResult().ErrTy);
    }
  } else if (D.isStreamTy()) {
    if (D.getStream().ValTy.has_value()) {
      Sub(*D.getStream().ValTy);
    }
  } else if (D.isFutureTy()) {
    if (D.getFuture().ValTy.has_value()) {
      Sub(*D.getFuture().ValTy);
    }
  }
}

void TypeSystem::collectResources(const ExternInfo &Info,
                                  std::unordered_set<uint32_t> &Out) noexcept {
  switch (Info.K) {
  case ExternKind::CoreType:
    return;
  case ExternKind::FuncType: {
    if (Info.Func.FT == nullptr) {
      return;
    }
    for (const auto &P : Info.Func.FT->getParamList()) {
      collectResources({P.getValType(), Info.Func.Home, Info.Func.Remap}, Out);
    }
    for (const auto &R : Info.Func.FT->getResultList()) {
      collectResources({R.getValType(), Info.Func.Home, Info.Func.Remap}, Out);
    }
    return;
  }
  case ExternKind::ValueBound:
    collectResources(Info.Value, Out);
    return;
  case ExternKind::TypeBound: {
    const auto &E = Info.Type;
    if (E.ResourceId.has_value()) {
      Out.insert(*E.ResourceId);
      return;
    }
    if (E.Inst != nullptr) {
      for (const auto &[Name, Sub] : E.Inst->Exports) {
        collectResources(Sub, Out);
      }
      return;
    }
    if (E.Comp != nullptr) {
      for (const auto &[Name, Sub] : E.Comp->Imports) {
        collectResources(Sub, Out);
      }
      for (const auto &[Name, Sub] : E.Comp->Exports) {
        collectResources(Sub, Out);
      }
      return;
    }
    if (E.DT != nullptr && E.DT->isDefValType()) {
      collectNormalValResources(normalizeEntry(E), Out);
      return;
    }
    if (E.DT != nullptr && E.DT->isFuncType()) {
      for (const auto &P : E.DT->getFuncType().getParamList()) {
        collectResources({P.getValType(), E.Home, E.Remap}, Out);
      }
      for (const auto &R : E.DT->getFuncType().getResultList()) {
        collectResources({R.getValType(), E.Home, E.Remap}, Out);
      }
    }
    return;
  }
  case ExternKind::InstanceType:
  case ExternKind::ComponentType:
    // Instances carry no imports, so one walk covers both shapes.
    if (Info.Shape != nullptr) {
      for (const auto &[Name, Sub] : Info.Shape->Imports) {
        collectResources(Sub, Out);
      }
      for (const auto &[Name, Sub] : Info.Shape->Exports) {
        collectResources(Sub, Out);
      }
    }
    return;
  }
}

bool TypeSystem::originatesIn(uint32_t Id, const Scope &S) const noexcept {
  const auto &Entry = getResource(Id);
  for (const auto *Cur = Entry.Origin; Cur != nullptr; Cur = Cur->Parent) {
    if (Cur == &S) {
      return true;
    }
  }
  return false;
}

// ---------------------------------------------------------------------------
// Effective type-size and nesting-depth limits.
// ---------------------------------------------------------------------------

// Effective size of a value type. This is the metric of the spec limit, and
// it is memoized per node.
uint64_t TypeSystem::sizeOfValType(const QualValType &Q) noexcept {
  return sizeOfNormalVal(normalizeValType(Q));
}

uint64_t TypeSystem::sizeOfNormalVal(const NormalVal &N) noexcept {
  if (!N.Valid || N.DVT == nullptr) {
    return 1;
  }
  auto It = TypeSizeMemo.find(N.DVT);
  if (It != TypeSizeMemo.end()) {
    return It->second;
  }
  TypeSizeMemo.emplace(N.DVT, 1); // Break cycles defensively.
  const auto &D = *N.DVT;
  uint64_t Size = 1;
  auto Add = [&](const ComponentValType &VT) noexcept {
    Size += sizeOfValType({VT, N.Home, N.Remap});
  };
  if (D.isRecordTy()) {
    for (const auto &LT : D.getRecord().LabelTypes) {
      Add(LT.getValType());
    }
  } else if (D.isVariantTy()) {
    for (const auto &[Label, Ty] : D.getVariant().Cases) {
      if (Ty.has_value()) {
        Add(*Ty);
      } else {
        Size += 1;
      }
    }
  } else if (D.isListTy()) {
    Add(D.getList().ValTy);
  } else if (D.isTupleTy()) {
    for (const auto &Ty : D.getTuple().Types) {
      Add(Ty);
    }
  } else if (D.isMapTy()) {
    Add(D.getMap().KeyTy);
    Add(D.getMap().ValTy);
  } else if (D.isOptionTy()) {
    Add(D.getOption().ValTy);
  } else if (D.isResultTy()) {
    if (D.getResult().ValTy.has_value()) {
      Add(*D.getResult().ValTy);
    }
    if (D.getResult().ErrTy.has_value()) {
      Add(*D.getResult().ErrTy);
    }
  } else if (D.isFlagsTy()) {
    Size += D.getFlags().Labels.size();
  } else if (D.isEnumTy()) {
    Size += D.getEnum().Labels.size();
  }
  TypeSizeMemo[N.DVT] = Size;
  return Size;
}

uint64_t TypeSystem::sizeOfExtern(const ExternInfo &Info) noexcept {
  switch (Info.K) {
  case ExternKind::CoreType:
    return Info.CoreMod != nullptr
               ? 1 + Info.CoreMod->Imports.size() + Info.CoreMod->Exports.size()
               : 1;
  case ExternKind::FuncType: {
    if (Info.Func.FT == nullptr) {
      return 1;
    }
    auto It = TypeSizeMemo.find(Info.Func.FT);
    if (It != TypeSizeMemo.end()) {
      return It->second;
    }
    uint64_t Size = 1;
    for (const auto &P : Info.Func.FT->getParamList()) {
      Size += sizeOfValType({P.getValType(), Info.Func.Home, Info.Func.Remap});
    }
    for (const auto &R : Info.Func.FT->getResultList()) {
      Size += sizeOfValType({R.getValType(), Info.Func.Home, Info.Func.Remap});
    }
    TypeSizeMemo.emplace(Info.Func.FT, Size);
    return Size;
  }
  case ExternKind::ValueBound:
    return 1 + sizeOfValType(Info.Value);
  case ExternKind::TypeBound: {
    const auto &E = Info.Type;
    if (E.ResourceId.has_value()) {
      return 1;
    }
    if (E.Inst != nullptr || E.Comp != nullptr) {
      const void *Key = E.Inst != nullptr ? static_cast<const void *>(E.Inst)
                                          : static_cast<const void *>(E.Comp);
      auto It = TypeSizeMemo.find(Key);
      if (It != TypeSizeMemo.end()) {
        return It->second;
      }
      TypeSizeMemo.emplace(Key, 1);
      uint64_t Size = 1;
      if (E.Inst != nullptr) {
        for (const auto &[Name, Sub] : E.Inst->Exports) {
          Size += 1 + sizeOfExtern(Sub);
        }
      } else {
        for (const auto &[Name, Sub] : E.Comp->Imports) {
          Size += 1 + sizeOfExtern(Sub);
        }
        for (const auto &[Name, Sub] : E.Comp->Exports) {
          Size += 1 + sizeOfExtern(Sub);
        }
      }
      TypeSizeMemo[Key] = Size;
      return Size;
    }
    if (E.DT != nullptr && E.DT->isDefValType()) {
      return sizeOfNormalVal(normalizeEntry(E));
    }
    if (E.DT != nullptr && E.DT->isFuncType()) {
      ExternInfo FI;
      FI.K = ExternKind::FuncType;
      FI.Func = {&E.DT->getFuncType(), E.Home, E.Remap};
      return sizeOfExtern(FI);
    }
    return 1;
  }
  case ExternKind::InstanceType:
  case ExternKind::ComponentType: {
    // Instances carry no imports, so one walk covers both shapes.
    if (Info.Shape == nullptr) {
      return 1;
    }
    auto It = TypeSizeMemo.find(Info.Shape);
    if (It != TypeSizeMemo.end()) {
      return It->second;
    }
    TypeSizeMemo.emplace(Info.Shape, 1);
    uint64_t Size = 1;
    for (const auto &[Name, Sub] : Info.Shape->Imports) {
      Size += 1 + sizeOfExtern(Sub);
    }
    for (const auto &[Name, Sub] : Info.Shape->Exports) {
      Size += 1 + sizeOfExtern(Sub);
    }
    TypeSizeMemo[Info.Shape] = Size;
    return Size;
  }
  }
  return 1;
}

// Depth of a value type: leaves count 1, wrappers add 1. Guards the recursive
// canonical-ABI walkers.
uint64_t TypeSystem::depthOfValType(const QualValType &Q) noexcept {
  return depthOfNormalVal(normalizeValType(Q));
}

uint64_t TypeSystem::depthOfNormalVal(const NormalVal &N) noexcept {
  if (!N.Valid || N.DVT == nullptr) {
    return 1;
  }
  auto It = TypeDepthMemo.find(N.DVT);
  if (It != TypeDepthMemo.end()) {
    return It->second;
  }
  TypeDepthMemo.emplace(N.DVT, 1); // Break cycles defensively.
  const auto &D = *N.DVT;
  uint64_t Max = 0;
  auto Sub = [&](const ComponentValType &VT) noexcept {
    Max = std::max(Max, depthOfValType({VT, N.Home, N.Remap}));
  };
  if (D.isRecordTy()) {
    for (const auto &LT : D.getRecord().LabelTypes) {
      Sub(LT.getValType());
    }
  } else if (D.isVariantTy()) {
    for (const auto &[Name, VT] : D.getVariant().Cases) {
      if (VT.has_value()) {
        Sub(*VT);
      }
    }
  } else if (D.isListTy()) {
    Sub(D.getList().ValTy);
  } else if (D.isTupleTy()) {
    for (const auto &VT : D.getTuple().Types) {
      Sub(VT);
    }
  } else if (D.isMapTy()) {
    Sub(D.getMap().KeyTy);
    Sub(D.getMap().ValTy);
  } else if (D.isOptionTy()) {
    Sub(D.getOption().ValTy);
  } else if (D.isResultTy()) {
    if (D.getResult().ValTy.has_value()) {
      Sub(*D.getResult().ValTy);
    }
    if (D.getResult().ErrTy.has_value()) {
      Sub(*D.getResult().ErrTy);
    }
  }
  const uint64_t Depth = Max + 1;
  TypeDepthMemo[N.DVT] = Depth;
  return Depth;
}

uint64_t TypeSystem::depthOfExtern(const ExternInfo &Info) noexcept {
  uint64_t Max = 0;
  switch (Info.K) {
  case ExternKind::FuncType:
    if (Info.Func.FT != nullptr) {
      for (const auto &P : Info.Func.FT->getParamList()) {
        Max = std::max(Max, depthOfValType({P.getValType(), Info.Func.Home,
                                            Info.Func.Remap}));
      }
      for (const auto &R : Info.Func.FT->getResultList()) {
        Max = std::max(Max, depthOfValType({R.getValType(), Info.Func.Home,
                                            Info.Func.Remap}));
      }
    }
    break;
  case ExternKind::TypeBound: {
    const auto &E = Info.Type;
    if (E.Inst != nullptr || E.Comp != nullptr) {
      ExternInfo Probe;
      Probe.K = E.Inst != nullptr ? ExternKind::InstanceType
                                  : ExternKind::ComponentType;
      Probe.Shape = E.Inst != nullptr ? E.Inst : E.Comp;
      Max = depthOfExtern(Probe);
    } else if (E.DT != nullptr && E.DT->isDefValType()) {
      Max = depthOfNormalVal(normalizeEntry(E));
    } else if (E.DT != nullptr && E.DT->isFuncType()) {
      ExternInfo Probe;
      Probe.K = ExternKind::FuncType;
      Probe.Func = {&E.DT->getFuncType(), E.Home, E.Remap};
      Max = depthOfExtern(Probe);
    }
    break;
  }
  case ExternKind::ValueBound:
    Max = depthOfValType(Info.Value);
    break;
  case ExternKind::InstanceType:
  case ExternKind::ComponentType: {
    // A shape is one nesting level of its own. Instances carry no imports, so
    // one walk covers both shapes.
    if (Info.Shape == nullptr) {
      return 1;
    }
    auto It = TypeDepthMemo.find(Info.Shape);
    if (It != TypeDepthMemo.end()) {
      return It->second;
    }
    TypeDepthMemo.emplace(Info.Shape, 1); // Break cycles defensively.
    for (const auto &[Name, E] : Info.Shape->Imports) {
      Max = std::max(Max, depthOfExtern(E));
    }
    for (const auto &[Name, E] : Info.Shape->Exports) {
      Max = std::max(Max, depthOfExtern(E));
    }
    TypeDepthMemo[Info.Shape] = Max + 1;
    return Max + 1;
  }
  case ExternKind::CoreType:
    // Core types carry no component value types.
    break;
  }
  return Max;
}

Expect<void> TypeSystem::checkTypeSize(uint64_t Size) const noexcept {
  if (Size >= MaxTypeSize) {
    spdlog::error(ErrCode::Value::ComponentTypeSizeLimit);
    spdlog::error("    Effective type size {} exceeds the limit of {}."sv, Size,
                  MaxTypeSize);
    return Unexpect(ErrCode::Value::ComponentTypeSizeLimit);
  }
  return {};
}

Expect<void> TypeSystem::checkTypeDepth(uint64_t Depth) const noexcept {
  // The reference engine bounds value-type nesting at 100.
  if (Depth > 100) {
    spdlog::error(ErrCode::Value::ComponentTypeNestingDepth);
    spdlog::error("    Value type nesting depth {} exceeds the limit."sv,
                  Depth);
    return Unexpect(ErrCode::Value::ComponentTypeNestingDepth);
  }
  return {};
}

Expect<void> TypeSystem::checkTypeLimits(const ExternInfo &Info) noexcept {
  EXPECTED_TRY(checkTypeSize(sizeOfExtern(Info)));
  EXPECTED_TRY(checkTypeDepth(depthOfExtern(Info)));
  return {};
}

// ---------------------------------------------------------------------------
// Canonical ABI flattening (spec `flatten_functype`).
// ---------------------------------------------------------------------------

// Spec flatten_type: appends the flat core types of Q. Returns false for
// types that cannot be flattened (async-gated ones).
bool TypeSystem::flattenValType(const QualValType &Q, std::vector<ValType> &Out,
                                const ValType &Ptr) noexcept {
  if (Out.size() > MaxFlatExpand) {
    return true;
  }
  const auto N = normalizeValType(Q);
  if (!N.Valid) {
    return false;
  }
  if (N.DVT == nullptr) {
    switch (N.Prim) {
    case ComponentTypeCode::Bool:
    case ComponentTypeCode::S8:
    case ComponentTypeCode::U8:
    case ComponentTypeCode::S16:
    case ComponentTypeCode::U16:
    case ComponentTypeCode::S32:
    case ComponentTypeCode::U32:
    case ComponentTypeCode::Char:
      Out.push_back(ValType(TypeCode::I32));
      return true;
    case ComponentTypeCode::S64:
    case ComponentTypeCode::U64:
      Out.push_back(ValType(TypeCode::I64));
      return true;
    case ComponentTypeCode::F32:
      Out.push_back(ValType(TypeCode::F32));
      return true;
    case ComponentTypeCode::F64:
      Out.push_back(ValType(TypeCode::F64));
      return true;
    case ComponentTypeCode::String:
      Out.push_back(Ptr);
      Out.push_back(Ptr);
      return true;
    case ComponentTypeCode::ErrContext:
      Out.push_back(ValType(TypeCode::I32));
      return true;
    default:
      return false;
    }
  }
  const auto &D = *N.DVT;
  auto Sub = [&](const ComponentValType &VT) noexcept {
    return flattenValType({VT, N.Home, N.Remap}, Out, Ptr);
  };
  if (D.isRecordTy()) {
    for (const auto &LT : D.getRecord().LabelTypes) {
      if (!Sub(LT.getValType())) {
        return false;
      }
    }
    return true;
  }
  if (D.isTupleTy()) {
    for (const auto &Ty : D.getTuple().Types) {
      if (!Sub(Ty)) {
        return false;
      }
    }
    return true;
  }
  if (D.isListTy()) {
    const auto &L = D.getList();
    if (L.Len.has_value()) {
      // Fixed-length lists flatten to Len copies of the element. Every copy
      // widens Out, so the ceiling also bounds the iteration count.
      for (uint32_t I = 0; I < *L.Len && Out.size() <= MaxFlatExpand; ++I) {
        if (!Sub(L.ValTy)) {
          return false;
        }
      }
      return true;
    }
    Out.push_back(Ptr);
    Out.push_back(Ptr);
    return true;
  }
  if (D.isMapTy()) {
    // (map k v) flattens like (list (tuple k v)).
    Out.push_back(Ptr);
    Out.push_back(Ptr);
    return true;
  }
  if (D.isFlagsTy() || D.isEnumTy() || D.isOwnTy() || D.isBorrowTy() ||
      D.isStreamTy() || D.isFutureTy()) {
    Out.push_back(ValType(TypeCode::I32));
    return true;
  }
  if (D.isVariantTy() || D.isOptionTy() || D.isResultTy()) {
    // flatten_variant: discriminant + element-wise join of the payloads.
    std::vector<std::vector<ComponentValType>> Payloads;
    if (D.isVariantTy()) {
      for (const auto &[Label, Ty] : D.getVariant().Cases) {
        if (Ty.has_value()) {
          Payloads.push_back({*Ty});
        } else {
          Payloads.push_back({});
        }
      }
    } else if (D.isOptionTy()) {
      Payloads.push_back({});
      Payloads.push_back({D.getOption().ValTy});
    } else {
      const auto &R = D.getResult();
      Payloads.push_back(R.ValTy.has_value()
                             ? std::vector<ComponentValType>{*R.ValTy}
                             : std::vector<ComponentValType>{});
      Payloads.push_back(R.ErrTy.has_value()
                             ? std::vector<ComponentValType>{*R.ErrTy}
                             : std::vector<ComponentValType>{});
    }
    auto Join = [](ValType A, ValType B) noexcept {
      if (A == B) {
        return A;
      }
      if ((A.getCode() == TypeCode::I32 && B.getCode() == TypeCode::F32) ||
          (A.getCode() == TypeCode::F32 && B.getCode() == TypeCode::I32)) {
        return ValType(TypeCode::I32);
      }
      return ValType(TypeCode::I64);
    };
    std::vector<ValType> Joined;
    for (const auto &Payload : Payloads) {
      std::vector<ValType> Flat;
      for (const auto &Ty : Payload) {
        if (!flattenValType({Ty, N.Home, N.Remap}, Flat, Ptr)) {
          return false;
        }
      }
      for (size_t I = 0; I < Flat.size(); ++I) {
        if (I < Joined.size()) {
          Joined[I] = Join(Joined[I], Flat[I]);
        } else {
          Joined.push_back(Flat[I]);
        }
      }
    }
    Out.push_back(ValType(TypeCode::I32));
    Out.insert(Out.end(), Joined.begin(), Joined.end());
    return true;
  }
  return false;
}

// True iff the type transitively contains a list or string (drives the memory
// / realloc option requirements).
bool TypeSystem::needsMemory(const QualValType &Q) noexcept {
  const auto N = normalizeValType(Q);
  if (!N.Valid) {
    return false;
  }
  if (N.DVT == nullptr) {
    return N.Prim == ComponentTypeCode::String;
  }
  const auto &D = *N.DVT;
  auto Sub = [&](const ComponentValType &VT) noexcept {
    return needsMemory({VT, N.Home, N.Remap});
  };
  if (D.isListTy()) {
    return true;
  }
  if (D.isRecordTy()) {
    for (const auto &LT : D.getRecord().LabelTypes) {
      if (Sub(LT.getValType())) {
        return true;
      }
    }
    return false;
  }
  if (D.isVariantTy()) {
    for (const auto &[Label, Ty] : D.getVariant().Cases) {
      if (Ty.has_value() && Sub(*Ty)) {
        return true;
      }
    }
    return false;
  }
  if (D.isTupleTy()) {
    for (const auto &Ty : D.getTuple().Types) {
      if (Sub(Ty)) {
        return true;
      }
    }
    return false;
  }
  if (D.isMapTy()) {
    return true;
  }
  if (D.isOptionTy()) {
    return Sub(D.getOption().ValTy);
  }
  if (D.isResultTy()) {
    const auto &R = D.getResult();
    return (R.ValTy.has_value() && Sub(*R.ValTy)) ||
           (R.ErrTy.has_value() && Sub(*R.ErrTy));
  }
  return false;
}

Expect<AST::FunctionType>
TypeSystem::flattenFuncType(const FuncInfo &FI, const ValType &Ptr,
                            bool &ParamsNeedMemory,
                            bool &ResultsNeedMemory) noexcept {
  AST::FunctionType Sig;
  ParamsNeedMemory = false;
  ResultsNeedMemory = false;
  if (FI.FT == nullptr) {
    spdlog::error(ErrCode::Value::InvalidTypeReference);
    return Unexpect(ErrCode::Value::InvalidTypeReference);
  }
  for (const auto &P : FI.FT->getParamList()) {
    const QualValType Q{P.getValType(), FI.Home, FI.Remap};
    if (!flattenValType(Q, Sig.getParamTypes(), Ptr)) {
      spdlog::error(ErrCode::Value::InvalidTypeReference);
      return Unexpect(ErrCode::Value::InvalidTypeReference);
    }
    ParamsNeedMemory = ParamsNeedMemory || needsMemory(Q);
  }
  for (const auto &R : FI.FT->getResultList()) {
    const QualValType Q{R.getValType(), FI.Home, FI.Remap};
    if (!flattenValType(Q, Sig.getReturnTypes(), Ptr)) {
      spdlog::error(ErrCode::Value::InvalidTypeReference);
      return Unexpect(ErrCode::Value::InvalidTypeReference);
    }
    ResultsNeedMemory = ResultsNeedMemory || needsMemory(Q);
  }
  return Sig;
}

// ---------------------------------------------------------------------------
// Instantiation-time substitution.
// ---------------------------------------------------------------------------

// Rebuild a view across an instantiation boundary. Node is the combined
// substitution and freshening, and Memo keeps shape identity.
ExternInfo TypeSystem::rebuildExtern(const ExternInfo &E,
                                     const ResourceMap *Node,
                                     ShapeMemo &Memo) noexcept {
  ExternInfo R = E;
  switch (E.K) {
  case ExternKind::CoreType:
    break;
  case ExternKind::FuncType:
    R.Func.Remap = composeRemap(Node, E.Func.Remap);
    break;
  case ExternKind::ValueBound:
    R.Value.Remap = composeRemap(Node, E.Value.Remap);
    break;
  case ExternKind::TypeBound:
    R.Type = rebuildTypeEntry(E.Type, Node, Memo);
    break;
  case ExternKind::InstanceType:
  case ExternKind::ComponentType:
    R.Shape = rebuildShape(E.Shape, Node, Memo);
    break;
  }
  return R;
}

TypeEntry TypeSystem::rebuildTypeEntry(const TypeEntry &E,
                                       const ResourceMap *Node,
                                       ShapeMemo &Memo) noexcept {
  TypeEntry R = E;
  if (E.ResourceId.has_value()) {
    R.ResourceId = Node->apply(*E.ResourceId);
    if (*R.ResourceId != *E.ResourceId) {
      R.NameId = getResource(*R.ResourceId).NameId;
    }
  }
  R.Remap = composeRemap(Node, E.Remap);
  if (E.Inst != nullptr) {
    R.Inst = rebuildShape(E.Inst, Node, Memo);
  }
  if (E.Comp != nullptr) {
    R.Comp = rebuildShape(E.Comp, Node, Memo);
  }
  return R;
}

// One walk for both shapes: instances have no imports, components no
// declaration order, so each loop is a no-op for the other kind.
const Shape *TypeSystem::rebuildShape(const Shape *S, const ResourceMap *Node,
                                      ShapeMemo &Memo) noexcept {
  if (S == nullptr) {
    return nullptr;
  }
  auto It = Memo.find(S);
  if (It != Memo.end()) {
    return It->second;
  }
  auto *R = newShape();
  Memo.emplace(S, R);
  R->DeclScope = S->DeclScope;
  R->ExportOrder = S->ExportOrder;
  for (const auto &[Name, E] : S->Imports) {
    R->Imports.emplace_back(Name, rebuildExtern(E, Node, Memo));
  }
  for (const auto &[Name, E] : S->Exports) {
    R->Exports.emplace(Name, rebuildExtern(E, Node, Memo));
  }
  return R;
}

const Shape *TypeSystem::freshenDeclaredResources(const Shape *Inst,
                                                  const Scope &Origin,
                                                  bool FromImport) noexcept {
  // Only declaration-built shapes carry their own declared resources. The
  // resources of a concrete instance belong to the enclosing component.
  if (Inst == nullptr || Inst->DeclScope == nullptr ||
      Inst->DeclScope->K != Scope::Kind::InstanceType) {
    return Inst;
  }
  std::unordered_set<uint32_t> Ids;
  ExternInfo Probe;
  Probe.K = ExternKind::InstanceType;
  Probe.Shape = Inst;
  collectResources(Probe, Ids);
  auto *Node = newResourceMap();
  for (const uint32_t Id : Ids) {
    if (originatesIn(Id, *Inst->DeclScope)) {
      Node->Map.emplace(Id, addResource(nullptr, &Origin, FromImport));
    }
  }
  if (Node->Map.empty()) {
    return Inst;
  }
  ShapeMemo Memo;
  return rebuildShape(Inst, Node, Memo);
}

const Shape *
TypeSystem::rebuildInstanceExports(const Shape &CI,
                                   const ResourceMap *Node) noexcept {
  ShapeMemo Memo;
  auto *Result = newShape();
  Result->DeclScope = CI.DeclScope;
  for (const auto &[Name, E] : CI.Exports) {
    Result->Exports.emplace(Name, rebuildExtern(E, Node, Memo));
    Result->ExportOrder.emplace_back(Name);
  }
  return Result;
}

// ---------------------------------------------------------------------------
// The subtype relation.
// ---------------------------------------------------------------------------

bool Matcher::matchValType(const QualValType &Sub,
                           const QualValType &Sup) noexcept {
  return matchNormalVal(Types.normalizeValType(Sub),
                        Types.normalizeValType(Sup));
}

bool Matcher::matchNormalVal(const NormalVal &NSub,
                             const NormalVal &NSup) noexcept {
  if (!NSub.Valid || !NSup.Valid) {
    return false;
  }
  if (NSub.DVT == nullptr && NSup.DVT == nullptr) {
    if (NSub.Prim != NSup.Prim) {
      // Only a class-level difference names the primitive. A
      // scalar-against-scalar mismatch keeps the positional diagnostic.
      if ((NSub.Prim == ComponentTypeCode::String) !=
          (NSup.Prim == ComponentTypeCode::String)) {
        FailCode = ErrCode::Value::ComponentPrimitiveMismatch;
      }
      return false;
    }
    return true;
  }
  if (NSub.DVT == nullptr || NSup.DVT == nullptr) {
    return false;
  }
  const auto &A = *NSub.DVT;
  const auto &B = *NSup.DVT;
  auto MatchSub = [&](const ComponentValType &VA,
                      const ComponentValType &VB) noexcept {
    return matchValType({VA, NSub.Home, NSub.Remap},
                        {VB, NSup.Home, NSup.Remap});
  };
  auto MatchOpt = [&](const std::optional<ComponentValType> &VA,
                      const std::optional<ComponentValType> &VB) noexcept {
    if (VA.has_value() != VB.has_value()) {
      return false;
    }
    return !VA.has_value() || MatchSub(*VA, *VB);
  };
  if (A.isRecordTy() && B.isRecordTy()) {
    const auto &RA = A.getRecord().LabelTypes;
    const auto &RB = B.getRecord().LabelTypes;
    if (RA.size() != RB.size()) {
      return false;
    }
    for (size_t I = 0; I < RA.size(); ++I) {
      if (RA[I].getLabel() != RB[I].getLabel() ||
          !MatchSub(RA[I].getValType(), RB[I].getValType())) {
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
      if (VA[I].first != VB[I].first || !MatchOpt(VA[I].second, VB[I].second)) {
        return false;
      }
    }
    return true;
  }
  if (A.isListTy() && B.isListTy()) {
    if (A.getList().Len != B.getList().Len) {
      return false;
    }
    return MatchSub(A.getList().ValTy, B.getList().ValTy);
  }
  if (A.isMapTy() && B.isMapTy()) {
    return MatchSub(A.getMap().KeyTy, B.getMap().KeyTy) &&
           MatchSub(A.getMap().ValTy, B.getMap().ValTy);
  }
  if (A.isTupleTy() && B.isTupleTy()) {
    const auto &TA = A.getTuple().Types;
    const auto &TB = B.getTuple().Types;
    if (TA.size() != TB.size()) {
      return false;
    }
    for (size_t I = 0; I < TA.size(); ++I) {
      if (!MatchSub(TA[I], TB[I])) {
        return false;
      }
    }
    return true;
  }
  if (A.isFlagsTy() && B.isFlagsTy()) {
    return A.getFlags().Labels == B.getFlags().Labels;
  }
  if (A.isEnumTy() && B.isEnumTy()) {
    if (A.getEnum().Labels != B.getEnum().Labels) {
      FailCode = ErrCode::Value::ComponentEnumMismatch;
      return false;
    }
    return true;
  }
  if (A.isOptionTy() && B.isOptionTy()) {
    return MatchSub(A.getOption().ValTy, B.getOption().ValTy);
  }
  if (A.isResultTy() && B.isResultTy()) {
    const auto &RA = A.getResult();
    const auto &RB = B.getResult();
    if (RA.ValTy.has_value() && !RB.ValTy.has_value()) {
      FailCode = ErrCode::Value::ComponentExpectedNoOkType;
      return false;
    }
    if (RA.ErrTy.has_value() && !RB.ErrTy.has_value()) {
      FailCode = ErrCode::Value::ComponentExpectedNoErrType;
      return false;
    }
    return MatchOpt(RA.ValTy, RB.ValTy) && MatchOpt(RA.ErrTy, RB.ErrTy);
  }
  if ((A.isStreamTy() && B.isStreamTy()) ||
      (A.isFutureTy() && B.isFutureTy())) {
    const auto &EA = A.isStreamTy() ? A.getStream().ValTy : A.getFuture().ValTy;
    const auto &EB = B.isStreamTy() ? B.getStream().ValTy : B.getFuture().ValTy;
    return MatchOpt(EA, EB);
  }
  if (A.isBorrowTy() && B.isOwnTy()) {
    FailCode = ErrCode::Value::ComponentExpectedOwn;
    return false;
  }
  if (A.isOwnTy() && B.isBorrowTy()) {
    FailCode = ErrCode::Value::ComponentExpectedBorrow;
    return false;
  }
  if ((A.isOwnTy() && B.isOwnTy()) || (A.isBorrowTy() && B.isBorrowTy())) {
    const uint32_t IA = A.isOwnTy() ? A.getOwn().Idx : A.getBorrow().Idx;
    const uint32_t IB = B.isOwnTy() ? B.getOwn().Idx : B.getBorrow().Idx;
    const auto RA = Types.resolveResourceId(NSub.Home, NSub.Remap, IA);
    const auto RB = Types.resolveResourceId(NSup.Home, NSup.Remap, IB);
    if (!RA.has_value() || !RB.has_value()) {
      return false;
    }
    uint32_t SupId = *RB;
    auto It = Subst.Map.find(SupId);
    if (It != Subst.Map.end()) {
      SupId = It->second;
    }
    if (*RA != SupId) {
      FailCode = ErrCode::Value::ComponentResourceMismatch;
      return false;
    }
    return true;
  }
  return false;
}

bool Matcher::matchFunc(const FuncInfo &Sub, const FuncInfo &Sup) noexcept {
  if (Sub.FT == nullptr || Sup.FT == nullptr) {
    return false;
  }
  // A sync function never satisfies an async one, nor the reverse.
  if (Sub.FT->isAsync() != Sup.FT->isAsync()) {
    return false;
  }
  const auto PA = Sub.FT->getParamList();
  const auto PB = Sup.FT->getParamList();
  if (PA.size() != PB.size()) {
    return false;
  }
  for (size_t I = 0; I < PA.size(); ++I) {
    if (PA[I].getLabel() != PB[I].getLabel() ||
        !matchValType({PA[I].getValType(), Sub.Home, Sub.Remap},
                      {PB[I].getValType(), Sup.Home, Sup.Remap})) {
      return false;
    }
  }
  const auto RA = Sub.FT->getResultList();
  const auto RB = Sup.FT->getResultList();
  if (RA.size() != RB.size()) {
    FailCode = ErrCode::Value::ComponentExpectedResult;
    return false;
  }
  for (size_t I = 0; I < RA.size(); ++I) {
    if (!matchValType({RA[I].getValType(), Sub.Home, Sub.Remap},
                      {RB[I].getValType(), Sup.Home, Sup.Remap})) {
      return false;
    }
  }
  return true;
}

bool Matcher::matchTypeEntry(const TypeEntry &Sub,
                             const TypeEntry &Sup) noexcept {
  // Abstract resource supertype: binds (or re-checks) the substitution.
  if (Sup.ResourceId.has_value() && Sup.DT == nullptr) {
    if (!Sub.ResourceId.has_value()) {
      FailCode = ErrCode::Value::ComponentExpectedResource;
      return false;
    }
    auto [It, New] = Subst.Map.emplace(*Sup.ResourceId, *Sub.ResourceId);
    if (!New && It->second != *Sub.ResourceId) {
      FailCode = ErrCode::Value::ComponentResourceMismatch;
      return false;
    }
    // Abstract-to-abstract bindings work in both directions (component
    // shapes are matched contravariantly on imports).
    if (Sub.DT == nullptr) {
      Subst.Map.emplace(*Sub.ResourceId, *Sup.ResourceId);
    }
    return true;
  }
  if (Sup.ResourceId.has_value()) {
    if (!Sub.ResourceId.has_value()) {
      FailCode = ErrCode::Value::ComponentExpectedResource;
      return false;
    }
    uint32_t SupId = *Sup.ResourceId;
    auto It = Subst.Map.find(SupId);
    if (It != Subst.Map.end()) {
      SupId = It->second;
    }
    return *Sub.ResourceId == SupId;
  }
  if (Sub.ResourceId.has_value()) {
    if (Sup.DT != nullptr && Sup.DT->isDefValType()) {
      FailCode = ErrCode::Value::ComponentExpectedDefinedType;
    }
    return false;
  }
  if (Sup.Inst != nullptr) {
    return Sub.Inst != nullptr && matchInstanceShape(*Sub.Inst, *Sup.Inst);
  }
  if (Sup.Comp != nullptr) {
    return Sub.Comp != nullptr && matchComponentShape(*Sub.Comp, *Sup.Comp);
  }
  if (Sup.DT == nullptr || Sub.DT == nullptr) {
    return false;
  }
  if (Sup.DT->isFuncType()) {
    if (!Sub.DT->isFuncType()) {
      return false;
    }
    return matchFunc({&Sub.DT->getFuncType(), Sub.Home, Sub.Remap},
                     {&Sup.DT->getFuncType(), Sup.Home, Sup.Remap});
  }
  if (Sup.DT->isDefValType()) {
    if (!Sub.DT->isDefValType()) {
      return false;
    }
    return matchNormalVal(Types.normalizeEntry(Sub), Types.normalizeEntry(Sup));
  }
  return false;
}

// Positional diagnostics ("type mismatch in instance export") beat leaf
// reasons when the failure is nested inside an instance/component shape.
void Matcher::clearLeafFailCode() noexcept {
  switch (FailCode) {
  case ErrCode::Value::InstanceMissingExpectedExport:
  case ErrCode::Value::ComponentMissingExpectedImport:
  case ErrCode::Value::ComponentResourceMismatch:
  case ErrCode::Value::ComponentExpectedResource:
  case ErrCode::Value::ComponentExpectedDefinedType:
  case ErrCode::Value::ComponentExpectedOwn:
  case ErrCode::Value::ComponentExpectedBorrow:
    break;
  default:
    FailCode = ErrCode::Value::Success;
    break;
  }
}

bool Matcher::matchInstanceShape(const Shape &Sub, const Shape &Sup) noexcept {
  // Walk expected exports in declaration order: abstract resources must
  // bind before the functions that reference them.
  auto MatchOne = [&](const std::string &Name,
                      const ExternInfo &SupE) noexcept {
    auto It = Sub.Exports.find(Name);
    if (It == Sub.Exports.end()) {
      FailCode = ErrCode::Value::InstanceMissingExpectedExport;
      return false;
    }
    if (!matchExtern(It->second, SupE)) {
      clearLeafFailCode();
      return false;
    }
    return true;
  };
  if (!Sup.ExportOrder.empty()) {
    for (const auto &Name : Sup.ExportOrder) {
      auto SupIt = Sup.Exports.find(Name);
      if (SupIt != Sup.Exports.end() && !MatchOne(Name, SupIt->second)) {
        return false;
      }
    }
    return true;
  }
  for (const auto &[Name, SupE] : Sup.Exports) {
    if (!MatchOne(Name, SupE)) {
      return false;
    }
  }
  return true;
}

bool Matcher::matchComponentShape(const Shape &Sub, const Shape &Sup) noexcept {
  // Imports contravariant: everything Sub requires, Sup must require
  // compatibly (so args satisfying Sup satisfy Sub).
  for (const auto &[Name, SubImp] : Sub.Imports) {
    const ExternInfo *SupImp = nullptr;
    for (const auto &[SupName, E] : Sup.Imports) {
      if (SupName == Name) {
        SupImp = &E;
        break;
      }
    }
    if (SupImp == nullptr) {
      FailCode = ErrCode::Value::ComponentMissingExpectedImport;
      return false;
    }
    if (!matchExtern(*SupImp, SubImp)) {
      clearLeafFailCode();
      return false;
    }
  }
  // Exports covariant.
  for (const auto &[Name, SupE] : Sup.Exports) {
    auto It = Sub.Exports.find(Name);
    if (It == Sub.Exports.end()) {
      FailCode = ErrCode::Value::InstanceMissingExpectedExport;
      return false;
    }
    if (!matchExtern(It->second, SupE)) {
      clearLeafFailCode();
      return false;
    }
  }
  return true;
}

bool Matcher::matchExtern(const ExternInfo &Sub,
                          const ExternInfo &Sup) noexcept {
  if (Sub.K != Sup.K) {
    if (Sup.K == ExternKind::FuncType) {
      FailCode = ErrCode::Value::ComponentExpectedFunc;
    } else if (Sup.K == ExternKind::ComponentType) {
      FailCode = ErrCode::Value::ComponentExpectedComponent;
    }
    return false;
  }
  switch (Sup.K) {
  case ExternKind::CoreType: {
    if (Sub.CoreMod == nullptr || Sup.CoreMod == nullptr) {
      return false;
    }
    // Imports are contravariant by (module, name). Exports are covariant.
    for (const auto &[Mod, Name, SubExt] : Sub.CoreMod->Imports) {
      const CoreExternInfo *SupExt = nullptr;
      for (const auto &[SMod, SName, E] : Sup.CoreMod->Imports) {
        if (SMod == Mod && SName == Name) {
          SupExt = &E;
          break;
        }
      }
      if (SupExt == nullptr) {
        FailCode = ErrCode::Value::ComponentMissingExpectedImport;
        return false;
      }
      if (!matchCoreExtern(*SupExt, SubExt)) {
        return false;
      }
    }
    for (const auto &[Name, SupExt] : Sup.CoreMod->Exports) {
      auto It = Sub.CoreMod->Exports.find(Name);
      if (It == Sub.CoreMod->Exports.end()) {
        FailCode = ErrCode::Value::InstanceMissingExpectedExport;
        return false;
      }
      if (!matchCoreExtern(It->second, SupExt)) {
        return false;
      }
    }
    return true;
  }
  case ExternKind::FuncType: {
    if (matchFunc(Sub.Func, Sup.Func)) {
      return true;
    }
    // A value-leaf reason stays internal to a function signature. The
    // diagnostic names the parameter or result position instead.
    if (FailCode == ErrCode::Value::ComponentPrimitiveMismatch ||
        FailCode == ErrCode::Value::ComponentEnumMismatch ||
        FailCode == ErrCode::Value::ComponentExpectedNoOkType ||
        FailCode == ErrCode::Value::ComponentExpectedNoErrType) {
      FailCode = ErrCode::Value::Success;
    }
    return false;
  }
  case ExternKind::ValueBound:
    return matchValType(Sub.Value, Sup.Value);
  case ExternKind::TypeBound:
    return matchTypeEntry(Sub.Type, Sup.Type);
  case ExternKind::InstanceType:
    return Sub.Shape != nullptr && Sup.Shape != nullptr &&
           matchInstanceShape(*Sub.Shape, *Sup.Shape);
  case ExternKind::ComponentType:
    return Sub.Shape != nullptr && Sup.Shape != nullptr &&
           matchComponentShape(*Sub.Shape, *Sup.Shape);
  }
  return false;
}

bool Matcher::matchCoreFuncType(const AST::SubType *Sub,
                                const AST::SubType *Sup) const noexcept {
  if (Sub == nullptr || Sup == nullptr) {
    return false;
  }
  const auto &CA = Sub->getCompositeType();
  const auto &CB = Sup->getCompositeType();
  if (!CA.isFunc() || !CB.isFunc()) {
    return false;
  }
  return CA.getFuncType().getParamTypes() == CB.getFuncType().getParamTypes() &&
         CA.getFuncType().getReturnTypes() == CB.getFuncType().getReturnTypes();
}

bool Matcher::matchCoreExtern(const CoreExternInfo &Sub,
                              const CoreExternInfo &Sup) const noexcept {
  if (Sub.Kind != Sup.Kind) {
    return false;
  }
  auto MatchLimits = [](const AST::Limit &LA, const AST::Limit &LB) noexcept {
    if (LA.is64() != LB.is64() || LA.isShared() != LB.isShared()) {
      return false;
    }
    if (LA.getMin() < LB.getMin()) {
      return false;
    }
    if (LB.hasMax()) {
      return LA.hasMax() && LA.getMax() <= LB.getMax();
    }
    return true;
  };
  switch (Sup.Kind) {
  case ExternalType::Function:
  case ExternalType::Tag:
    return matchCoreFuncType(Sub.Func, Sup.Func);
  case ExternalType::Table:
    return Sub.Table != nullptr && Sup.Table != nullptr &&
           Sub.Table->getRefType() == Sup.Table->getRefType() &&
           MatchLimits(Sub.Table->getLimit(), Sup.Table->getLimit());
  case ExternalType::Memory:
    return Sub.Memory != nullptr && Sup.Memory != nullptr &&
           MatchLimits(Sub.Memory->getLimit(), Sup.Memory->getLimit());
  case ExternalType::Global:
    return Sub.Global != nullptr && Sup.Global != nullptr &&
           Sub.Global->getValType() == Sup.Global->getValType() &&
           Sub.Global->getValMut() == Sup.Global->getValMut();
  default:
    return false;
  }
}

// ---------------------------------------------------------------------------
// Instantiation.
// ---------------------------------------------------------------------------

} // namespace Component
} // namespace Validator
} // namespace WasmEdge
