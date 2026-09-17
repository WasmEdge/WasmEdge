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
// Scope.
// ---------------------------------------------------------------------------

Expect<void> Scope::consumeValue(uint32_t Idx) noexcept {
  auto &Entry = Values[Idx];
  if (Entry.Consumed) {
    spdlog::error(ErrCode::Value::ComponentValueAlreadyConsumed);
    return Unexpect(ErrCode::Value::ComponentValueAlreadyConsumed);
  }
  Entry.Consumed = true;
  return {};
}

// ---------------------------------------------------------------------------
// Type view resolution.
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
  if (Entry->isResource()) {
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
  if (Entry == nullptr || !Entry->isResource()) {
    return std::nullopt;
  }
  return applyRemap(Remap, *Entry->ResourceId);
}

// The primitive behind a valtype, through prim-alias indirections.
std::optional<AST::Component::PrimValType>
TypeSystem::resolvePrimValType(const QualValType &Q) noexcept {
  if (Q.VT.isPrimValType()) {
    return static_cast<AST::Component::PrimValType>(Q.VT.getCode());
  }
  TypeEntry Storage;
  const auto *Entry = resolveQualType(Q, Storage);
  const auto *Def = Entry != nullptr ? Entry->getDefValType() : nullptr;
  if (Def == nullptr || !Def->isPrimValType()) {
    return std::nullopt;
  }
  return Def->getPrimValType();
}

// ---------------------------------------------------------------------------
// Resource-id walkers over value types and views.
// ---------------------------------------------------------------------------

bool TypeSystem::containsBorrow(const QualValType &Q) noexcept {
  TypeEntry Storage;
  const auto *Entry = resolveQualType(Q, Storage);
  const auto *Def = Entry != nullptr ? Entry->getDefValType() : nullptr;
  if (Def == nullptr || Def->isPrimValType()) {
    return false;
  }
  const auto &D = *Def;
  if (D.isBorrowTy()) {
    return true;
  }
  // Stream and future payloads are borrow-free, so the containers suffice.
  bool Found = false;
  forEachValType(D, [&](const ComponentValType &VT) noexcept {
    Found = Found || containsBorrow({VT, Entry->Home, Entry->Remap});
  });
  return Found;
}

void TypeSystem::collectResources(const QualValType &Q,
                                  std::unordered_set<uint32_t> &Out) noexcept {
  TypeEntry Storage;
  if (const auto *Entry = resolveQualType(Q, Storage)) {
    collectResources(*Entry, Out);
  }
}

void TypeSystem::collectResources(const TypeEntry &E,
                                  std::unordered_set<uint32_t> &Out) noexcept {
  const auto *Def = E.getDefValType();
  if (Def == nullptr || Def->isPrimValType()) {
    return;
  }
  const auto &D = *Def;
  auto Sub = [&](const ComponentValType &VT) noexcept {
    collectResources({VT, E.Home, E.Remap}, Out);
  };
  if (D.isOwnTy() || D.isBorrowTy()) {
    const uint32_t Idx = D.isOwnTy() ? D.getOwn().Idx : D.getBorrow().Idx;
    if (auto Id = resolveResourceId(E.Home, E.Remap, Idx)) {
      Out.insert(*Id);
    }
    return;
  }
  // A stream or future payload can reach a resource too.
  if (D.isStreamTy() || D.isFutureTy()) {
    const auto &Elem =
        D.isStreamTy() ? D.getStream().ValTy : D.getFuture().ValTy;
    if (Elem.has_value()) {
      Sub(*Elem);
    }
    return;
  }
  forEachValType(D, Sub);
}

void TypeSystem::collectResources(const ExternInfo &Info,
                                  std::unordered_set<uint32_t> &Out) noexcept {
  switch (Info.Kind) {
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
    if (E.isResource()) {
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
    if (E.getDefValType() != nullptr) {
      collectResources(E, Out);
      return;
    }
    if (const auto *FT = E.getFuncType()) {
      for (const auto &P : FT->getParamList()) {
        collectResources({P.getValType(), E.Home, E.Remap}, Out);
      }
      for (const auto &R : FT->getResultList()) {
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

// Effective size of a value type (the spec limit metric), memoized per node.
uint64_t TypeSystem::getTypeSize(const QualValType &Q) noexcept {
  TypeEntry Storage;
  const auto *Entry = resolveQualType(Q, Storage);
  return Entry != nullptr ? getTypeSize(*Entry) : 1;
}

uint64_t TypeSystem::getTypeSize(const TypeEntry &E) noexcept {
  const auto *Def = E.getDefValType();
  if (Def == nullptr || Def->isPrimValType()) {
    return 1;
  }
  auto It = TypeSizeMemo.find(Def);
  if (It != TypeSizeMemo.end()) {
    return It->second;
  }
  TypeSizeMemo.emplace(Def, 1); // Break cycles defensively.
  const auto &D = *Def;
  uint64_t Size = 1;
  // A payload-less variant case and every flags or enum label count one.
  if (D.isVariantTy()) {
    for (const auto &[Label, Ty] : D.getVariant().Cases) {
      if (!Ty.has_value()) {
        Size += 1;
      }
    }
  } else if (D.isFlagsTy()) {
    Size += D.getFlags().Labels.size();
  } else if (D.isEnumTy()) {
    Size += D.getEnum().Labels.size();
  }
  forEachValType(D, [&](const ComponentValType &VT) noexcept {
    Size += getTypeSize({VT, E.Home, E.Remap});
  });
  TypeSizeMemo[Def] = Size;
  return Size;
}

uint64_t TypeSystem::getTypeSize(const ExternInfo &Info) noexcept {
  switch (Info.Kind) {
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
      Size += getTypeSize({P.getValType(), Info.Func.Home, Info.Func.Remap});
    }
    for (const auto &R : Info.Func.FT->getResultList()) {
      Size += getTypeSize({R.getValType(), Info.Func.Home, Info.Func.Remap});
    }
    TypeSizeMemo.emplace(Info.Func.FT, Size);
    return Size;
  }
  case ExternKind::ValueBound:
    return 1 + getTypeSize(Info.Value);
  case ExternKind::TypeBound: {
    const auto &E = Info.Type;
    if (E.isResource()) {
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
          Size += 1 + getTypeSize(Sub);
        }
      } else {
        for (const auto &[Name, Sub] : E.Comp->Imports) {
          Size += 1 + getTypeSize(Sub);
        }
        for (const auto &[Name, Sub] : E.Comp->Exports) {
          Size += 1 + getTypeSize(Sub);
        }
      }
      TypeSizeMemo[Key] = Size;
      return Size;
    }
    if (E.getDefValType() != nullptr) {
      return getTypeSize(E);
    }
    if (E.getFuncType() != nullptr) {
      ExternInfo FI;
      FI.Kind = ExternKind::FuncType;
      FI.Func = {E.getFuncType(), E.Home, E.Remap};
      return getTypeSize(FI);
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
      Size += 1 + getTypeSize(Sub);
    }
    for (const auto &[Name, Sub] : Info.Shape->Exports) {
      Size += 1 + getTypeSize(Sub);
    }
    TypeSizeMemo[Info.Shape] = Size;
    return Size;
  }
  }
  return 1;
}

// Depth of a value type: leaves count 1, wrappers add 1.
uint64_t TypeSystem::getTypeDepth(const QualValType &Q) noexcept {
  TypeEntry Storage;
  const auto *Entry = resolveQualType(Q, Storage);
  return Entry != nullptr ? getTypeDepth(*Entry) : 1;
}

uint64_t TypeSystem::getTypeDepth(const TypeEntry &E) noexcept {
  const auto *Def = E.getDefValType();
  if (Def == nullptr || Def->isPrimValType()) {
    return 1;
  }
  auto It = TypeDepthMemo.find(Def);
  if (It != TypeDepthMemo.end()) {
    return It->second;
  }
  TypeDepthMemo.emplace(Def, 1); // Break cycles defensively.
  const auto &D = *Def;
  uint64_t Max = 0;
  forEachValType(D, [&](const ComponentValType &VT) noexcept {
    Max = std::max(Max, getTypeDepth({VT, E.Home, E.Remap}));
  });
  const uint64_t Depth = Max + 1;
  TypeDepthMemo[Def] = Depth;
  return Depth;
}

uint64_t TypeSystem::getTypeDepth(const ExternInfo &Info) noexcept {
  uint64_t Max = 0;
  switch (Info.Kind) {
  case ExternKind::FuncType:
    if (Info.Func.FT != nullptr) {
      for (const auto &P : Info.Func.FT->getParamList()) {
        Max = std::max(Max, getTypeDepth({P.getValType(), Info.Func.Home,
                                          Info.Func.Remap}));
      }
      for (const auto &R : Info.Func.FT->getResultList()) {
        Max = std::max(Max, getTypeDepth({R.getValType(), Info.Func.Home,
                                          Info.Func.Remap}));
      }
    }
    break;
  case ExternKind::TypeBound: {
    const auto &E = Info.Type;
    if (E.Inst != nullptr || E.Comp != nullptr) {
      ExternInfo Probe;
      Probe.Kind = E.Inst != nullptr ? ExternKind::InstanceType
                                     : ExternKind::ComponentType;
      Probe.Shape = E.Inst != nullptr ? E.Inst : E.Comp;
      Max = getTypeDepth(Probe);
    } else if (E.getDefValType() != nullptr) {
      Max = getTypeDepth(E);
    } else if (E.getFuncType() != nullptr) {
      ExternInfo Probe;
      Probe.Kind = ExternKind::FuncType;
      Probe.Func = {E.getFuncType(), E.Home, E.Remap};
      Max = getTypeDepth(Probe);
    }
    break;
  }
  case ExternKind::ValueBound:
    Max = getTypeDepth(Info.Value);
    break;
  case ExternKind::InstanceType:
  case ExternKind::ComponentType: {
    // A shape is one nesting level; one walk covers both shapes.
    if (Info.Shape == nullptr) {
      return 1;
    }
    auto It = TypeDepthMemo.find(Info.Shape);
    if (It != TypeDepthMemo.end()) {
      return It->second;
    }
    TypeDepthMemo.emplace(Info.Shape, 1); // Break cycles defensively.
    for (const auto &[Name, E] : Info.Shape->Imports) {
      Max = std::max(Max, getTypeDepth(E));
    }
    for (const auto &[Name, E] : Info.Shape->Exports) {
      Max = std::max(Max, getTypeDepth(E));
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

Expect<void> TypeSystem::checkTypeLimits(const ExternInfo &Info) noexcept {
  const uint64_t Size = getTypeSize(Info);
  if (Size >= MaxTypeSize) {
    spdlog::error(ErrCode::Value::ComponentTypeSizeLimit);
    spdlog::error("    Effective type size {} exceeds the limit of {}."sv, Size,
                  MaxTypeSize);
    return Unexpect(ErrCode::Value::ComponentTypeSizeLimit);
  }
  const uint64_t Depth = getTypeDepth(Info);
  if (Depth > MaxTypeDepth) {
    spdlog::error(ErrCode::Value::ComponentTypeNestingDepth);
    spdlog::error("    Value type nesting depth {} exceeds the limit."sv,
                  Depth);
    return Unexpect(ErrCode::Value::ComponentTypeNestingDepth);
  }
  return {};
}

// ---------------------------------------------------------------------------
// Canonical ABI flattening (spec `flatten_functype`).
// ---------------------------------------------------------------------------

// Spec flatten_type: appends the flat core types of Q; false if unflattenable.
bool TypeSystem::flattenValType(const QualValType &Q, std::vector<ValType> &Out,
                                const ValType &Ptr) noexcept {
  if (Out.size() > MaxFlatExpand) {
    return true;
  }
  if (const auto Prim = resolvePrimValType(Q)) {
    switch (*Prim) {
    case AST::Component::PrimValType::Bool:
    case AST::Component::PrimValType::S8:
    case AST::Component::PrimValType::U8:
    case AST::Component::PrimValType::S16:
    case AST::Component::PrimValType::U16:
    case AST::Component::PrimValType::S32:
    case AST::Component::PrimValType::U32:
    case AST::Component::PrimValType::Char:
      Out.push_back(ValType(TypeCode::I32));
      return true;
    case AST::Component::PrimValType::S64:
    case AST::Component::PrimValType::U64:
      Out.push_back(ValType(TypeCode::I64));
      return true;
    case AST::Component::PrimValType::F32:
      Out.push_back(ValType(TypeCode::F32));
      return true;
    case AST::Component::PrimValType::F64:
      Out.push_back(ValType(TypeCode::F64));
      return true;
    case AST::Component::PrimValType::String:
      Out.push_back(Ptr);
      Out.push_back(Ptr);
      return true;
    case AST::Component::PrimValType::ErrorContext:
      Out.push_back(ValType(TypeCode::I32));
      return true;
    default:
      return false;
    }
  }
  TypeEntry Storage;
  const auto *Entry = resolveQualType(Q, Storage);
  const auto *Def = Entry != nullptr ? Entry->getDefValType() : nullptr;
  if (Def == nullptr) {
    return false;
  }
  const auto &D = *Def;
  auto Sub = [&](const ComponentValType &VT) noexcept {
    return flattenValType({VT, Entry->Home, Entry->Remap}, Out, Ptr);
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
      // Fixed-length lists flatten to Len copies of the element.
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
        if (!flattenValType({Ty, Entry->Home, Entry->Remap}, Flat, Ptr)) {
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

// True iff the type transitively contains a list, map, or string.
bool TypeSystem::needsMemory(const QualValType &Q) noexcept {
  if (const auto Prim = resolvePrimValType(Q)) {
    return *Prim == AST::Component::PrimValType::String;
  }
  TypeEntry Storage;
  const auto *Entry = resolveQualType(Q, Storage);
  const auto *Def = Entry != nullptr ? Entry->getDefValType() : nullptr;
  if (Def == nullptr) {
    return false;
  }
  const auto &D = *Def;
  if (D.isListTy() || D.isMapTy()) {
    return true;
  }
  bool Found = false;
  forEachValType(D, [&](const ComponentValType &VT) noexcept {
    Found = Found || needsMemory({VT, Entry->Home, Entry->Remap});
  });
  return Found;
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

// Rebuild a view across an instantiation boundary under the Node remap.
ExternInfo TypeSystem::rebuildExtern(const ExternInfo &E,
                                     const ResourceMap *Node,
                                     ShapeMemo &Memo) noexcept {
  ExternInfo R = E;
  switch (E.Kind) {
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
  if (E.isResource()) {
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

// One walk for both shapes; each loop is a no-op for the other kind.
const Shape *TypeSystem::rebuildShape(const Shape *S, const ResourceMap *Node,
                                      ShapeMemo &Memo) noexcept {
  if (S == nullptr) {
    return nullptr;
  }
  auto It = Memo.find(S);
  if (It != Memo.end()) {
    return It->second;
  }
  auto *R = addShape();
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
  // Only declaration-built shapes carry their own declared resources.
  if (Inst == nullptr || Inst->DeclScope == nullptr ||
      Inst->DeclScope->Kind != ScopeKind::InstanceType) {
    return Inst;
  }
  std::unordered_set<uint32_t> Ids;
  ExternInfo Probe;
  Probe.Kind = ExternKind::InstanceType;
  Probe.Shape = Inst;
  collectResources(Probe, Ids);
  auto *Node = addResourceMap();
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
  auto *Result = addShape();
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
  const auto PSub = Types.resolvePrimValType(Sub);
  const auto PSup = Types.resolvePrimValType(Sup);
  if (PSub.has_value() || PSup.has_value()) {
    return PSub.has_value() && PSup.has_value() &&
           matchPrimValType(*PSub, *PSup);
  }
  TypeEntry SubStorage, SupStorage;
  const auto *SubEntry = Types.resolveQualType(Sub, SubStorage);
  const auto *SupEntry = Types.resolveQualType(Sup, SupStorage);
  return SubEntry != nullptr && SupEntry != nullptr &&
         matchValType(*SubEntry, *SupEntry);
}

bool Matcher::matchPrimValType(AST::Component::PrimValType Sub,
                               AST::Component::PrimValType Sup) noexcept {
  if (Sub != Sup) {
    // Only a class-level difference names the primitive.
    if ((Sub == AST::Component::PrimValType::String) !=
        (Sup == AST::Component::PrimValType::String)) {
      FailCode = ErrCode::Value::ComponentPrimitiveMismatch;
    }
    return false;
  }
  return true;
}

bool Matcher::matchValType(const TypeEntry &Sub,
                           const TypeEntry &Sup) noexcept {
  const auto *ADef = Sub.getDefValType();
  const auto *BDef = Sup.getDefValType();
  if (ADef == nullptr || BDef == nullptr) {
    return false;
  }
  if (ADef->isPrimValType() || BDef->isPrimValType()) {
    return ADef->isPrimValType() && BDef->isPrimValType() &&
           matchPrimValType(ADef->getPrimValType(), BDef->getPrimValType());
  }
  const auto &A = *ADef;
  const auto &B = *BDef;
  auto MatchSub = [&](const ComponentValType &VA,
                      const ComponentValType &VB) noexcept {
    return matchValType({VA, Sub.Home, Sub.Remap}, {VB, Sup.Home, Sup.Remap});
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
    const auto RA = Types.resolveResourceId(Sub.Home, Sub.Remap, IA);
    const auto RB = Types.resolveResourceId(Sup.Home, Sup.Remap, IB);
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
  if (Sup.isResource() && Sup.DT == nullptr) {
    if (!Sub.isResource()) {
      FailCode = ErrCode::Value::ComponentExpectedResource;
      return false;
    }
    auto [It, New] = Subst.Map.emplace(*Sup.ResourceId, *Sub.ResourceId);
    if (!New && It->second != *Sub.ResourceId) {
      FailCode = ErrCode::Value::ComponentResourceMismatch;
      return false;
    }
    // Abstract-to-abstract bindings work in both directions.
    if (Sub.DT == nullptr) {
      Subst.Map.emplace(*Sub.ResourceId, *Sup.ResourceId);
    }
    return true;
  }
  if (Sup.isResource()) {
    if (!Sub.isResource()) {
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
  if (Sub.isResource()) {
    if (Sup.getDefValType() != nullptr) {
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
  if (Sup.getFuncType() != nullptr) {
    if (Sub.getFuncType() == nullptr) {
      return false;
    }
    return matchFunc({Sub.getFuncType(), Sub.Home, Sub.Remap},
                     {Sup.getFuncType(), Sup.Home, Sup.Remap});
  }
  if (Sup.getDefValType() != nullptr) {
    return matchValType(Sub, Sup);
  }
  return false;
}

// Positional diagnostics beat leaf reasons nested inside a shape.
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
  // Declaration order: abstract resources bind before their functions.
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
  // Imports are contravariant: whatever Sub requires, Sup must require.
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
  if (Sub.Kind != Sup.Kind) {
    if (Sup.Kind == ExternKind::FuncType) {
      FailCode = ErrCode::Value::ComponentExpectedFunc;
    } else if (Sup.Kind == ExternKind::ComponentType) {
      FailCode = ErrCode::Value::ComponentExpectedComponent;
    }
    return false;
  }
  switch (Sup.Kind) {
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
    // A value-leaf reason stays internal to a function signature.
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

bool Matcher::matchCoreExtern(const CoreExternInfo &Sub,
                              const CoreExternInfo &Sup) const noexcept {
  if (Sub.Kind != Sup.Kind) {
    return false;
  }
  switch (Sup.Kind) {
  case ExternalType::Function:
  case ExternalType::Tag: {
    if (Sub.Func == nullptr || Sup.Func == nullptr) {
      return false;
    }
    const auto &CA = Sub.Func->getCompositeType();
    const auto &CB = Sup.Func->getCompositeType();
    return CA.isFunc() && CB.isFunc() &&
           CA.getFuncType().getParamTypes() ==
               CB.getFuncType().getParamTypes() &&
           CA.getFuncType().getReturnTypes() ==
               CB.getFuncType().getReturnTypes();
  }
  case ExternalType::Table:
    return Sub.Table != nullptr && Sup.Table != nullptr &&
           Sub.Table->getRefType() == Sup.Table->getRefType() &&
           AST::TypeMatcher::matchLimit(Sup.Table->getLimit(),
                                        Sub.Table->getLimit());
  case ExternalType::Memory:
    return Sub.Memory != nullptr && Sup.Memory != nullptr &&
           AST::TypeMatcher::matchLimit(Sup.Memory->getLimit(),
                                        Sub.Memory->getLimit());
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
