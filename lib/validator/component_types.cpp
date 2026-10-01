// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- component_types.cpp - Component type views ------------------------===//
//
// The bodies of NameRecord, Scope and Matcher.
//
//===----------------------------------------------------------------------===//
#include "validator/component_types.h"

#include "common/errinfo.h"
#include "common/spdlog.h"
#include "validator/component_context.h"

#include <optional>
#include <string>

namespace WasmEdge {
namespace Validator {
namespace Component {

using namespace std::literals;
// ---------------------------------------------------------------------------
// NameRecord.
// ---------------------------------------------------------------------------

NameRecord::NameRecord(const ExternName &Name) noexcept
    : Original(Name.getOriginalName()) {
  // Remove the hyphens and lowercase the acronyms, then strip the annotation
  // except `[constructor]`, and reduce `[*]l.l` to `l`.
  const bool Annotated = Name.getKind() == ExternName::Kind::Constructor ||
                         Name.getKind() == ExternName::Kind::Method ||
                         Name.getKind() == ExternName::Kind::Static;
  for (const char C : Annotated ? Name.getNoTagName() : Original) {
    if (C != '-') {
      Canonical.push_back(
          static_cast<char>(std::tolower(static_cast<unsigned char>(C))));
    }
  }
  if (Name.getKind() == ExternName::Kind::Constructor) {
    Canonical.insert(0, "[constructor]"sv);
  } else if (Annotated) {
    const auto Dot = Canonical.find('.');
    if (Dot != std::string::npos &&
        std::string_view(Canonical).substr(0, Dot) ==
            std::string_view(Canonical).substr(Dot + 1)) {
      Canonical.resize(Dot);
    }
  }
}

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
// The subtype relation.
// ---------------------------------------------------------------------------

Expect<void> Matcher::matchValType(Context &Ctx, const QualValType &Sub,
                                   const QualValType &Sup,
                                   ResourceMap &Subst) noexcept {
  const auto PSub = Ctx.getPrimValType(Sub);
  const auto PSup = Ctx.getPrimValType(Sup);
  if (PSub.has_value() || PSup.has_value()) {
    if (!PSub.has_value() || !PSup.has_value()) {
      return Unexpect(ErrCode::Value::ArgTypeMismatch);
    }
    return matchPrimValType(*PSub, *PSup);
  }
  TypeEntry SubStorage, SupStorage;
  const auto *SubEntry = Ctx.getTypeEntry(Sub, SubStorage);
  const auto *SupEntry = Ctx.getTypeEntry(Sup, SupStorage);
  if (SubEntry == nullptr || SupEntry == nullptr) {
    return Unexpect(ErrCode::Value::ArgTypeMismatch);
  }
  return matchValType(Ctx, *SubEntry, *SupEntry, Subst);
}

Expect<void> Matcher::matchValType(Context &Ctx, const TypeEntry &Sub,
                                   const TypeEntry &Sup,
                                   ResourceMap &Subst) noexcept {
  const auto *ADef = Sub.getDefValType();
  const auto *BDef = Sup.getDefValType();
  if (ADef == nullptr || BDef == nullptr) {
    return Unexpect(ErrCode::Value::ArgTypeMismatch);
  }
  if (ADef->isPrimValType() || BDef->isPrimValType()) {
    if (!ADef->isPrimValType() || !BDef->isPrimValType()) {
      return Unexpect(ErrCode::Value::ArgTypeMismatch);
    }
    return matchPrimValType(ADef->getPrimValType(), BDef->getPrimValType());
  }
  const auto &A = *ADef;
  const auto &B = *BDef;
  auto MatchSub = [&](const ComponentValType &VA,
                      const ComponentValType &VB) noexcept {
    return matchValType(Ctx, {VA, Sub.Home, Sub.Remap},
                        {VB, Sup.Home, Sup.Remap}, Subst);
  };
  auto MatchOpt =
      [&](const std::optional<ComponentValType> &VA,
          const std::optional<ComponentValType> &VB) noexcept -> Expect<void> {
    if (VA.has_value() != VB.has_value()) {
      return Unexpect(ErrCode::Value::ArgTypeMismatch);
    }
    if (!VA.has_value()) {
      return {};
    }
    return MatchSub(*VA, *VB);
  };
  if (A.isRecordTy() && B.isRecordTy()) {
    const auto &RA = A.getRecord().LabelTypes;
    const auto &RB = B.getRecord().LabelTypes;
    if (RA.size() != RB.size()) {
      return Unexpect(ErrCode::Value::ArgTypeMismatch);
    }
    for (size_t I = 0; I < RA.size(); ++I) {
      if (RA[I].getLabel() != RB[I].getLabel()) {
        return Unexpect(ErrCode::Value::ArgTypeMismatch);
      }
      EXPECTED_TRY(MatchSub(RA[I].getValType(), RB[I].getValType()));
    }
    return {};
  }
  if (A.isVariantTy() && B.isVariantTy()) {
    const auto &VA = A.getVariant().Cases;
    const auto &VB = B.getVariant().Cases;
    if (VA.size() != VB.size()) {
      return Unexpect(ErrCode::Value::ArgTypeMismatch);
    }
    for (size_t I = 0; I < VA.size(); ++I) {
      if (VA[I].first != VB[I].first) {
        return Unexpect(ErrCode::Value::ArgTypeMismatch);
      }
      EXPECTED_TRY(MatchOpt(VA[I].second, VB[I].second));
    }
    return {};
  }
  if (A.isListTy() && B.isListTy()) {
    if (A.getList().Len != B.getList().Len) {
      return Unexpect(ErrCode::Value::ArgTypeMismatch);
    }
    return MatchSub(A.getList().ValTy, B.getList().ValTy);
  }
  if (A.isMapTy() && B.isMapTy()) {
    EXPECTED_TRY(MatchSub(A.getMap().KeyTy, B.getMap().KeyTy));
    return MatchSub(A.getMap().ValTy, B.getMap().ValTy);
  }
  if (A.isTupleTy() && B.isTupleTy()) {
    const auto &TA = A.getTuple().Types;
    const auto &TB = B.getTuple().Types;
    if (TA.size() != TB.size()) {
      return Unexpect(ErrCode::Value::ArgTypeMismatch);
    }
    for (size_t I = 0; I < TA.size(); ++I) {
      EXPECTED_TRY(MatchSub(TA[I], TB[I]));
    }
    return {};
  }
  if (A.isFlagsTy() && B.isFlagsTy()) {
    if (A.getFlags().Labels != B.getFlags().Labels) {
      return Unexpect(ErrCode::Value::ArgTypeMismatch);
    }
    return {};
  }
  if (A.isEnumTy() && B.isEnumTy()) {
    if (A.getEnum().Labels != B.getEnum().Labels) {
      return Unexpect(ErrCode::Value::ComponentEnumMismatch);
    }
    return {};
  }
  if (A.isOptionTy() && B.isOptionTy()) {
    return MatchSub(A.getOption().ValTy, B.getOption().ValTy);
  }
  if (A.isResultTy() && B.isResultTy()) {
    const auto &RA = A.getResult();
    const auto &RB = B.getResult();
    if (RA.ValTy.has_value() && !RB.ValTy.has_value()) {
      return Unexpect(ErrCode::Value::ComponentExpectedNoOkType);
    }
    if (RA.ErrTy.has_value() && !RB.ErrTy.has_value()) {
      return Unexpect(ErrCode::Value::ComponentExpectedNoErrType);
    }
    EXPECTED_TRY(MatchOpt(RA.ValTy, RB.ValTy));
    return MatchOpt(RA.ErrTy, RB.ErrTy);
  }
  if ((A.isStreamTy() && B.isStreamTy()) ||
      (A.isFutureTy() && B.isFutureTy())) {
    const auto &EA = A.isStreamTy() ? A.getStream().ValTy : A.getFuture().ValTy;
    const auto &EB = B.isStreamTy() ? B.getStream().ValTy : B.getFuture().ValTy;
    return MatchOpt(EA, EB);
  }
  if (A.isBorrowTy() && B.isOwnTy()) {
    return Unexpect(ErrCode::Value::ComponentExpectedOwn);
  }
  if (A.isOwnTy() && B.isBorrowTy()) {
    return Unexpect(ErrCode::Value::ComponentExpectedBorrow);
  }
  if ((A.isOwnTy() && B.isOwnTy()) || (A.isBorrowTy() && B.isBorrowTy())) {
    const uint32_t IA = A.isOwnTy() ? A.getOwn().Idx : A.getBorrow().Idx;
    const uint32_t IB = B.isOwnTy() ? B.getOwn().Idx : B.getBorrow().Idx;
    const auto RA = Ctx.getResourceId(Sub.Home, Sub.Remap, IA);
    const auto RB = Ctx.getResourceId(Sup.Home, Sup.Remap, IB);
    if (!RA.has_value() || !RB.has_value()) {
      return Unexpect(ErrCode::Value::ArgTypeMismatch);
    }
    uint32_t SupId = *RB;
    auto It = Subst.Map.find(SupId);
    if (It != Subst.Map.end()) {
      SupId = It->second;
    }
    if (*RA != SupId) {
      return Unexpect(ErrCode::Value::ComponentResourceMismatch);
    }
    return {};
  }
  return Unexpect(ErrCode::Value::ArgTypeMismatch);
}

Expect<void> Matcher::matchExtern(Context &Ctx, const ExternInfo &Sub,
                                  const ExternInfo &Sup,
                                  ResourceMap &Subst) noexcept {
  if (Sub.Kind != Sup.Kind) {
    if (Sup.Kind == ExternKind::FuncType) {
      return Unexpect(ErrCode::Value::ComponentExpectedFunc);
    }
    if (Sup.Kind == ExternKind::ComponentType) {
      return Unexpect(ErrCode::Value::ComponentExpectedComponent);
    }
    return Unexpect(ErrCode::Value::ArgTypeMismatch);
  }
  switch (Sup.Kind) {
  case ExternKind::CoreType: {
    if (Sub.CoreMod == nullptr || Sup.CoreMod == nullptr) {
      return Unexpect(ErrCode::Value::ArgTypeMismatch);
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
        return Unexpect(ErrCode::Value::ComponentMissingExpectedImport);
      }
      if (!matchCoreExtern(*SupExt, SubExt)) {
        return Unexpect(ErrCode::Value::ArgTypeMismatch);
      }
    }
    for (const auto &[Name, SupExt] : Sup.CoreMod->Exports) {
      auto It = Sub.CoreMod->Exports.find(Name);
      if (It == Sub.CoreMod->Exports.end()) {
        return Unexpect(ErrCode::Value::InstanceMissingExpectedExport);
      }
      if (!matchCoreExtern(It->second, SupExt)) {
        return Unexpect(ErrCode::Value::ArgTypeMismatch);
      }
    }
    return {};
  }
  case ExternKind::FuncType:
    // A value-leaf reason stays internal to a function signature.
    return matchFunc(Ctx, Sub.Func, Sup.Func, Subst).map_error([](ErrCode E) {
      switch (E.getEnum()) {
      case ErrCode::Value::ComponentPrimitiveMismatch:
      case ErrCode::Value::ComponentEnumMismatch:
      case ErrCode::Value::ComponentExpectedNoOkType:
      case ErrCode::Value::ComponentExpectedNoErrType:
        return ErrCode(ErrCode::Value::ArgTypeMismatch);
      default:
        return E;
      }
    });
  case ExternKind::ValueBound:
    return matchValType(Ctx, Sub.Value, Sup.Value, Subst);
  case ExternKind::TypeBound:
    return matchTypeEntry(Ctx, Sub.Type, Sup.Type, Subst);
  case ExternKind::InstanceType:
    if (Sub.Shape == nullptr || Sup.Shape == nullptr) {
      return Unexpect(ErrCode::Value::ArgTypeMismatch);
    }
    return matchInstanceShape(Ctx, *Sub.Shape, *Sup.Shape, Subst);
  case ExternKind::ComponentType:
    if (Sub.Shape == nullptr || Sup.Shape == nullptr) {
      return Unexpect(ErrCode::Value::ArgTypeMismatch);
    }
    return matchComponentShape(Ctx, *Sub.Shape, *Sup.Shape, Subst);
  }
  return Unexpect(ErrCode::Value::ArgTypeMismatch);
}

bool Matcher::matchCoreExtern(const CoreExternInfo &Sub,
                              const CoreExternInfo &Sup) noexcept {
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

Expect<void> Matcher::matchFunc(Context &Ctx, const FuncInfo &Sub,
                                const FuncInfo &Sup,
                                ResourceMap &Subst) noexcept {
  if (Sub.FT == nullptr || Sup.FT == nullptr) {
    return Unexpect(ErrCode::Value::ArgTypeMismatch);
  }
  // A sync function never satisfies an async one, nor the reverse.
  if (Sub.FT->isAsync() != Sup.FT->isAsync()) {
    return Unexpect(ErrCode::Value::ArgTypeMismatch);
  }
  const auto PA = Sub.FT->getParamList();
  const auto PB = Sup.FT->getParamList();
  if (PA.size() != PB.size()) {
    return Unexpect(ErrCode::Value::ArgTypeMismatch);
  }
  for (size_t I = 0; I < PA.size(); ++I) {
    if (PA[I].getLabel() != PB[I].getLabel()) {
      return Unexpect(ErrCode::Value::ArgTypeMismatch);
    }
    EXPECTED_TRY(matchValType(Ctx, {PA[I].getValType(), Sub.Home, Sub.Remap},
                              {PB[I].getValType(), Sup.Home, Sup.Remap},
                              Subst));
  }
  const auto &RA = Sub.FT->getResult();
  const auto &RB = Sup.FT->getResult();
  if (RA.has_value() != RB.has_value()) {
    return Unexpect(ErrCode::Value::ComponentExpectedResult);
  }
  if (RA.has_value()) {
    EXPECTED_TRY(matchValType(Ctx, {*RA, Sub.Home, Sub.Remap},
                              {*RB, Sup.Home, Sup.Remap}, Subst));
  }
  return {};
}

Expect<void> Matcher::matchTypeEntry(Context &Ctx, const TypeEntry &Sub,
                                     const TypeEntry &Sup,
                                     ResourceMap &Subst) noexcept {
  // Abstract resource supertype: binds (or re-checks) the substitution.
  if (Sup.isResource() && Sup.DT == nullptr) {
    if (!Sub.isResource()) {
      return Unexpect(ErrCode::Value::ComponentExpectedResource);
    }
    auto [It, New] = Subst.Map.emplace(*Sup.ResourceId, *Sub.ResourceId);
    if (!New && It->second != *Sub.ResourceId) {
      return Unexpect(ErrCode::Value::ComponentResourceMismatch);
    }
    // Abstract-to-abstract bindings work in both directions.
    if (Sub.DT == nullptr) {
      Subst.Map.emplace(*Sub.ResourceId, *Sup.ResourceId);
    }
    return {};
  }
  if (Sup.isResource()) {
    if (!Sub.isResource()) {
      return Unexpect(ErrCode::Value::ComponentExpectedResource);
    }
    uint32_t SupId = *Sup.ResourceId;
    auto It = Subst.Map.find(SupId);
    if (It != Subst.Map.end()) {
      SupId = It->second;
    }
    if (*Sub.ResourceId != SupId) {
      return Unexpect(ErrCode::Value::ArgTypeMismatch);
    }
    return {};
  }
  if (Sub.isResource()) {
    return Unexpect(Sup.getDefValType() != nullptr
                        ? ErrCode::Value::ComponentExpectedDefinedType
                        : ErrCode::Value::ArgTypeMismatch);
  }
  if (Sup.Inst != nullptr) {
    if (Sub.Inst == nullptr) {
      return Unexpect(ErrCode::Value::ArgTypeMismatch);
    }
    return matchInstanceShape(Ctx, *Sub.Inst, *Sup.Inst, Subst);
  }
  if (Sup.Comp != nullptr) {
    if (Sub.Comp == nullptr) {
      return Unexpect(ErrCode::Value::ArgTypeMismatch);
    }
    return matchComponentShape(Ctx, *Sub.Comp, *Sup.Comp, Subst);
  }
  if (Sup.getFuncType() != nullptr) {
    if (Sub.getFuncType() == nullptr) {
      return Unexpect(ErrCode::Value::ArgTypeMismatch);
    }
    return matchFunc(Ctx, {Sub.getFuncType(), Sub.Home, Sub.Remap},
                     {Sup.getFuncType(), Sup.Home, Sup.Remap}, Subst);
  }
  if (Sup.getDefValType() != nullptr) {
    return matchValType(Ctx, Sub, Sup, Subst);
  }
  return Unexpect(ErrCode::Value::ArgTypeMismatch);
}

Expect<void> Matcher::matchInstanceShape(Context &Ctx, const Shape &Sub,
                                         const Shape &Sup,
                                         ResourceMap &Subst) noexcept {
  // Declaration order: abstract resources bind before their functions.
  auto MatchOne = [&](const std::string &Name,
                      const ExternInfo &SupE) noexcept -> Expect<void> {
    auto It = Sub.Exports.find(Name);
    if (It == Sub.Exports.end()) {
      return Unexpect(ErrCode::Value::InstanceMissingExpectedExport);
    }
    return matchExtern(Ctx, It->second, SupE, Subst)
        .map_error(getNestedFailCode);
  };
  if (!Sup.ExportOrder.empty()) {
    for (const auto &Name : Sup.ExportOrder) {
      auto SupIt = Sup.Exports.find(Name);
      if (SupIt != Sup.Exports.end()) {
        EXPECTED_TRY(MatchOne(Name, SupIt->second));
      }
    }
    return {};
  }
  for (const auto &[Name, SupE] : Sup.Exports) {
    EXPECTED_TRY(MatchOne(Name, SupE));
  }
  return {};
}

Expect<void> Matcher::matchComponentShape(Context &Ctx, const Shape &Sub,
                                          const Shape &Sup,
                                          ResourceMap &Subst) noexcept {
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
      return Unexpect(ErrCode::Value::ComponentMissingExpectedImport);
    }
    EXPECTED_TRY(
        matchExtern(Ctx, *SupImp, SubImp, Subst).map_error(getNestedFailCode));
  }
  // Exports covariant.
  for (const auto &[Name, SupE] : Sup.Exports) {
    auto It = Sub.Exports.find(Name);
    if (It == Sub.Exports.end()) {
      return Unexpect(ErrCode::Value::InstanceMissingExpectedExport);
    }
    EXPECTED_TRY(
        matchExtern(Ctx, It->second, SupE, Subst).map_error(getNestedFailCode));
  }
  return {};
}

Expect<void>
Matcher::matchPrimValType(AST::Component::PrimValType Sub,
                          AST::Component::PrimValType Sup) noexcept {
  if (Sub == Sup) {
    return {};
  }
  // Only a class-level difference names the primitive.
  if ((Sub == AST::Component::PrimValType::String) !=
      (Sup == AST::Component::PrimValType::String)) {
    return Unexpect(ErrCode::Value::ComponentPrimitiveMismatch);
  }
  return Unexpect(ErrCode::Value::ArgTypeMismatch);
}

// Positional diagnostics beat leaf reasons nested inside a shape.
ErrCode Matcher::getNestedFailCode(ErrCode Code) noexcept {
  switch (Code.getEnum()) {
  case ErrCode::Value::InstanceMissingExpectedExport:
  case ErrCode::Value::ComponentMissingExpectedImport:
  case ErrCode::Value::ComponentResourceMismatch:
  case ErrCode::Value::ComponentExpectedResource:
  case ErrCode::Value::ComponentExpectedDefinedType:
  case ErrCode::Value::ComponentExpectedOwn:
  case ErrCode::Value::ComponentExpectedBorrow:
    return Code;
  default:
    return ErrCode(ErrCode::Value::ArgTypeMismatch);
  }
}

} // namespace Component
} // namespace Validator
} // namespace WasmEdge
