// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "common/errinfo.h"
#include "common/spdlog.h"
#include "executor/component/canonical_abi.h"
#include "validator/component_name.h"
#include "validator/validator.h"

#include <algorithm>
#include <memory>
#include <unordered_set>
#include <variant>

namespace WasmEdge {
namespace Validator {

using namespace std::literals;

namespace {
std::string toLowerStr(std::string_view SV) {
  std::string Result(SV);
  std::transform(
      Result.begin(), Result.end(), Result.begin(),
      [](unsigned char C) { return static_cast<char>(std::tolower(C)); });
  return Result;
}

// Maps a component-side ExternDesc::DescType to its Sort::SortType.
// Returns nullopt for `CoreType` (= `(core module (type i))`), which has no
// representation in Sort::SortType; callers must handle that sort separately
// via Sort::CoreSortType::Module.
std::optional<AST::Component::Sort::SortType>
descTypeToSortType(AST::Component::ExternDesc::DescType DT) noexcept {
  switch (DT) {
  case AST::Component::ExternDesc::DescType::CoreType:
    return std::nullopt;
  case AST::Component::ExternDesc::DescType::FuncType:
    return AST::Component::Sort::SortType::Func;
  case AST::Component::ExternDesc::DescType::ValueBound:
    return AST::Component::Sort::SortType::Value;
  case AST::Component::ExternDesc::DescType::TypeBound:
    return AST::Component::Sort::SortType::Type;
  case AST::Component::ExternDesc::DescType::ComponentType:
    return AST::Component::Sort::SortType::Component;
  case AST::Component::ExternDesc::DescType::InstanceType:
    return AST::Component::Sort::SortType::Instance;
  default:
    assumingUnreachable();
  }
}

// Shallow sort-kind match between a Sort and an ExternDesc. The spec's
// instantiation / export-ascription rules require the supplied sortidx to be
// a subtype of the externdesc. Here we only enforce that the kind agrees;
// deep structural subtyping (record fields, func signatures, etc.) is not
// yet implemented and is the main remaining correctness gap at these sites.
bool sortMatchesDescType(const AST::Component::Sort &S,
                         AST::Component::ExternDesc::DescType DT) noexcept {
  auto Mapped = descTypeToSortType(DT);
  if (S.isCore()) {
    return !Mapped.has_value() &&
           S.getCoreSortType() == AST::Component::Sort::CoreSortType::Module;
  }
  return Mapped.has_value() && S.getSortType() == *Mapped;
}

// Fallback type-index lookup against an InstanceType's own local
// type-decl space (used when the outer ComponentContext scope doesn't
// own the InstanceType).
const AST::Component::InstanceType *
resolveNestedInstanceType(const AST::Component::InstanceType &Parent,
                          uint32_t TypeIdx) noexcept {
  uint32_t LocalIdx = 0;
  for (const auto &LocalDecl : Parent.getDecl()) {
    if (!LocalDecl.isType()) {
      continue;
    }
    if (LocalIdx == TypeIdx) {
      const auto *LocalDT = LocalDecl.getType();
      if (LocalDT != nullptr && LocalDT->isInstanceType()) {
        return &LocalDT->getInstanceType();
      }
      return nullptr;
    }
    LocalIdx++;
  }
  return nullptr;
}

// Resolve a type index in `Comp`'s own type index space to an InstanceType.
// Returns nullptr when the index does not refer to an inline InstanceType
// definition — callers treat nullptr as "no required shape" and fall back
// to inferred exports. TypeBound imports and outer-alias type imports
// currently fall through to nullptr; a more complete resolver would walk
// the alias chain to recover the underlying InstanceType.
const AST::Component::InstanceType *
resolveChildInstanceType(const AST::Component::Component &Comp,
                         uint32_t TypeIdx) {
  uint32_t CurrentIdx = 0;
  for (const auto &Sec : Comp.getSections()) {
    if (std::holds_alternative<AST::Component::TypeSection>(Sec)) {
      const auto &TSec = std::get<AST::Component::TypeSection>(Sec);
      for (const auto &DT : TSec.getContent()) {
        if (CurrentIdx == TypeIdx) {
          if (DT.isInstanceType()) {
            return &DT.getInstanceType();
          }
          return nullptr;
        }
        CurrentIdx++;
      }
    } else if (std::holds_alternative<AST::Component::ImportSection>(Sec)) {
      const auto &ISec = std::get<AST::Component::ImportSection>(Sec);
      for (const auto &Import : ISec.getContent()) {
        if (Import.getDesc().getDescType() ==
            AST::Component::ExternDesc::DescType::TypeBound) {
          if (CurrentIdx == TypeIdx) {
            return nullptr;
          }
          CurrentIdx++;
        }
      }
    } else if (std::holds_alternative<AST::Component::AliasSection>(Sec)) {
      const auto &ASec = std::get<AST::Component::AliasSection>(Sec);
      for (const auto &Alias : ASec.getContent()) {
        if (!Alias.getSort().isCore() &&
            Alias.getSort().getSortType() ==
                AST::Component::Sort::SortType::Type) {
          if (CurrentIdx == TypeIdx) {
            return nullptr;
          }
          CurrentIdx++;
        }
      }
    }
  }
  return nullptr;
}

// Validate that a name may appear at an export position: reject the
// `relative-url=` prefix (not part of the extern-name grammar) and any
// plainname/interfacename kind that isn't allowed on an export.
Expect<ComponentName> validateExportName(std::string_view Name) noexcept {
  if (Name.rfind("relative-url="sv, 0) == 0) {
    spdlog::error(ErrCode::Value::InvalidExternName);
    spdlog::error("    Export name '{}' is not a valid extern name"sv, Name);
    return Unexpect(ErrCode::Value::InvalidExternName);
  }
  EXPECTED_TRY(ComponentName CName, ComponentName::parse(Name));
  switch (CName.getKind()) {
  case ComponentNameKind::Label:
  case ComponentNameKind::Constructor:
  case ComponentNameKind::Method:
  case ComponentNameKind::Static:
  case ComponentNameKind::InterfaceType:
    return CName;
  default:
    spdlog::error(ErrCode::Value::InvalidExportName);
    spdlog::error("    Export name '{}' kind is not valid for exports"sv, Name);
    return Unexpect(ErrCode::Value::InvalidExportName);
  }
}

} // namespace

void Validator::populateInstanceFromType(
    uint32_t InstIdx, const AST::Component::InstanceType &IT) noexcept {
  for (const auto &Decl : IT.getDecl()) {
    if (!Decl.isExportDecl()) {
      continue;
    }
    const auto &Exp = Decl.getExport();
    const auto &ED = Exp.getExternDesc();
    auto ST = descTypeToSortType(ED.getDescType());
    if (!ST.has_value()) {
      // A `(core module)` export has no component sort; record it so an
      // alias export can still resolve it (GAP-ED-2).
      CompCtx.addCoreModuleInstanceExport(InstIdx, Exp.getName());
      continue;
    }
    const AST::Component::InstanceType *NestedIT = nullptr;
    if (ED.getDescType() ==
        AST::Component::ExternDesc::DescType::InstanceType) {
      NestedIT = CompCtx.getInstanceType(ED.getTypeIndex());
      if (NestedIT == nullptr) {
        NestedIT = resolveNestedInstanceType(IT, ED.getTypeIndex());
      }
    }
    // Resource-typed exports carry a canonical id so an alias-export of
    // the type slot can preserve identity. `(sub resource)` introduces a
    // fresh id; `(eq i)` should inherit, but cross-scope resource lookup
    // inside an InstanceType body is not yet implemented (TODO).
    std::optional<uint64_t> ResourceId;
    if (ED.getDescType() == AST::Component::ExternDesc::DescType::TypeBound &&
        !ED.isEqType()) {
      ResourceId = CompCtx.allocateFreshResourceId();
    }
    CompCtx.addInstanceExport(InstIdx, Exp.getName(), *ST, NestedIT,
                              /*NestedInstIdx=*/std::nullopt, ResourceId);
  }
}

bool Validator::exportSatisfies(
    const AST::Component::InstanceType &RequiredCtx,
    const ComponentContext::InstanceExport &Provided,
    const AST::Component::ExternDesc &Required) const noexcept {
  auto RequiredST = descTypeToSortType(Required.getDescType());
  if (!RequiredST.has_value()) {
    // `(core module)` has no InstanceExport::ST variant — no constraint.
    return true;
  }
  if (Provided.ST != *RequiredST) {
    return false;
  }
  // Instance-on-instance: recurse if both sides resolve to an
  // InstanceType. Otherwise sort-kind match (pre-Phase-3 behaviour).
  if (Required.getDescType() ==
          AST::Component::ExternDesc::DescType::InstanceType &&
      Provided.IT != nullptr) {
    const auto *RequiredIT = CompCtx.getInstanceType(Required.getTypeIndex());
    if (RequiredIT == nullptr) {
      // Required's idx lives in RequiredCtx's local type-decls.
      RequiredIT =
          resolveNestedInstanceType(RequiredCtx, Required.getTypeIndex());
    }
    if (RequiredIT != nullptr) {
      return isInstanceSubtype(*Provided.IT, *RequiredIT);
    }
  }
  return true;
}

std::optional<std::string> Validator::findMissingRequiredExport(
    uint32_t ProvidedInstIdx,
    const AST::Component::InstanceType &RequiredIT) const noexcept {
  const auto &Exports = CompCtx.getInstance(ProvidedInstIdx).Exports;
  for (const auto &Decl : RequiredIT.getDecl()) {
    if (!Decl.isExportDecl()) {
      continue;
    }
    const auto &Exp = Decl.getExport();
    auto It = Exports.find(std::string(Exp.getName()));
    if (It == Exports.end()) {
      return std::string(Exp.getName());
    }
    if (!exportSatisfies(RequiredIT, It->second, Exp.getExternDesc())) {
      return std::string(Exp.getName());
    }
  }
  return std::nullopt;
}

bool Validator::isInstanceSubtype(
    const AST::Component::InstanceType &S,
    const AST::Component::InstanceType &T) const noexcept {
  // S subtype T iff every export declared by T is present in S with a
  // satisfying type. Build a quick (name → externdesc) lookup of S's
  // exports for the lookup.
  std::unordered_map<std::string, const AST::Component::ExternDesc *> SExports;
  for (const auto &Decl : S.getDecl()) {
    if (Decl.isExportDecl()) {
      const auto &E = Decl.getExport();
      SExports.emplace(std::string(E.getName()), &E.getExternDesc());
    }
  }
  for (const auto &Decl : T.getDecl()) {
    if (!Decl.isExportDecl()) {
      continue;
    }
    const auto &E = Decl.getExport();
    auto It = SExports.find(std::string(E.getName()));
    if (It == SExports.end()) {
      return false;
    }
    auto SKind = descTypeToSortType(It->second->getDescType());
    auto TKind = descTypeToSortType(E.getExternDesc().getDescType());
    if (SKind != TKind) {
      return false;
    }
    // Instance-on-instance: nested type indices on each side belong to
    // that side's own decls, so fall back via resolveNestedInstanceType.
    if (E.getExternDesc().getDescType() ==
        AST::Component::ExternDesc::DescType::InstanceType) {
      const auto *SubIT = CompCtx.getInstanceType(It->second->getTypeIndex());
      if (SubIT == nullptr) {
        SubIT = resolveNestedInstanceType(S, It->second->getTypeIndex());
      }
      const auto *ReqIT =
          CompCtx.getInstanceType(E.getExternDesc().getTypeIndex());
      if (ReqIT == nullptr) {
        ReqIT = resolveNestedInstanceType(T, E.getExternDesc().getTypeIndex());
      }
      if (SubIT != nullptr && ReqIT != nullptr &&
          !isInstanceSubtype(*SubIT, *ReqIT)) {
        return false;
      }
    }
  }
  return true;
}

Expect<void>
Validator::validate(const AST::Component::Component &Comp) noexcept {
  spdlog::warn("Component Model Validation is in active development."sv);
  CompCtx.reset();
  return validateComponent(Comp).and_then([&]() {
    const_cast<AST::Component::Component &>(Comp).setIsValidated();
    return Expect<void>{};
  });
}

Expect<void>
Validator::validateComponent(const AST::Component::Component &Comp) noexcept {
  // Validation enters a fresh component scope and walks sections in their
  // binary order. The per-sort index spaces are built incrementally as
  // definitions are validated, so sortidx references in later sections
  // only resolve against entries already introduced. Custom sections are
  // ignored. Nested components recurse through this same function.
  auto ReportError = [](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Component));
    return E;
  };

  CompCtx.enterComponent(&Comp);
  for (const auto &Sec : Comp.getSections()) {
    auto Func = [&](auto &&S) -> Expect<void> {
      using T = std::decay_t<decltype(S)>;
      if constexpr (std::is_same_v<T, AST::CustomSection>) {
        // Always pass validation.
      } else {
        EXPECTED_TRY(validate(S).map_error(ReportError));
      }
      return {};
    };
    EXPECTED_TRY(std::visit(Func, Sec));
  }
  // Value linearity: every value in the value index space must have been
  // consumed exactly once before the component scope closes.
  if (auto Idx = CompCtx.firstUnconsumedValue()) {
    spdlog::error(ErrCode::Value::ComponentValueNotConsumed);
    spdlog::error(
        "    Component: value index {} was not consumed before end of component"sv,
        *Idx);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Component));
    CompCtx.exitComponent();
    return Unexpect(ErrCode::Value::ComponentValueNotConsumed);
  }
  CompCtx.exitComponent();
  return {};
}

Expect<void>
Validator::validate(const AST::Component::CoreModuleSection &ModSec) noexcept {
  EXPECTED_TRY(validate(ModSec.getContent()).map_error([](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Sec_CoreMod));
    return E;
  }));
  const_cast<AST::Module &>(ModSec.getContent()).setIsValidated();
  CompCtx.addCoreModule(ModSec.getContent());
  return {};
}

Expect<void> Validator::validate(
    const AST::Component::CoreInstanceSection &InstSec) noexcept {
  for (const auto &Inst : InstSec.getContent()) {
    EXPECTED_TRY(validate(Inst).map_error([](auto E) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Sec_CoreInstance));
      return E;
    }));
  }
  return {};
}

Expect<void>
Validator::validate(const AST::Component::CoreTypeSection &TypeSec) noexcept {
  for (const auto &Type : TypeSec.getContent()) {
    EXPECTED_TRY(validate(Type).map_error([](auto E) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Sec_CoreType));
      return E;
    }));
  }
  return {};
}

Expect<void>
Validator::validate(const AST::Component::ComponentSection &CompSec) noexcept {
  EXPECTED_TRY(validateComponent(CompSec.getContent()).map_error([](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Sec_Component));
    return E;
  }));
  CompCtx.addComponent(CompSec.getContent());
  return {};
}

Expect<void>
Validator::validate(const AST::Component::InstanceSection &InstSec) noexcept {
  for (const auto &Inst : InstSec.getContent()) {
    EXPECTED_TRY(validate(Inst).map_error([](auto E) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Sec_Instance));
      return E;
    }));
  }
  return {};
}

Expect<void>
Validator::validate(const AST::Component::AliasSection &AliasSec) noexcept {
  for (const auto &Alias : AliasSec.getContent()) {
    EXPECTED_TRY(validate(Alias).map_error([](auto E) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Sec_Alias));
      return E;
    }));
    const auto &Sort = Alias.getSort();
    const bool IsOuter =
        Alias.getTargetType() == AST::Component::Alias::TargetType::Outer;
    if (Sort.isCore()) {
      uint32_t NewCoreIdx;
      // For an `alias core export` of a memory, carry the source memory's type
      // into the core-memory sort space so canonical options can subtype-check
      // its index type later (GAP-CI-1).
      if (Alias.getTargetType() ==
              AST::Component::Alias::TargetType::CoreExport &&
          Sort.getCoreSortType() ==
              AST::Component::Sort::CoreSortType::Memory) {
        std::optional<AST::MemoryType> SrcMem;
        const auto SrcIdx = Alias.getExport().first;
        if (SrcIdx < CompCtx.getCoreSortIndexSize(
                         AST::Component::Sort::CoreSortType::Instance)) {
          const auto &Exports = CompCtx.getCoreInstance(SrcIdx);
          const auto It = Exports.find(std::string(Alias.getExport().second));
          if (It != Exports.end() && It->second.Mem.has_value()) {
            SrcMem = It->second.Mem;
          }
        }
        NewCoreIdx = CompCtx.addCoreMemory(std::move(SrcMem));
      } else {
        NewCoreIdx = CompCtx.incCoreSortIndexSize(Sort.getCoreSortType());
      }
      // Carry the outer-aliased module's slot so the alias stays enumerable
      // when instantiated.
      if (IsOuter && Sort.getCoreSortType() ==
                         AST::Component::Sort::CoreSortType::Module) {
        CompCtx.carryOuterCoreModule(NewCoreIdx, Alias.getOuter().first,
                                     Alias.getOuter().second);
      }
    } else {
      uint32_t NewIdx = CompCtx.incSortIndexSize(Sort.getSortType());
      // Component analogue of the outer core-module carry above.
      if (IsOuter &&
          Sort.getSortType() == AST::Component::Sort::SortType::Component) {
        CompCtx.carryOuterComponent(NewIdx, Alias.getOuter().first,
                                    Alias.getOuter().second);
      }
      // Outer-aliasing a resource type keeps the resource's identity in this
      // scope so later own/borrow and (eq i) checks treat the slot correctly.
      if (IsOuter &&
          Sort.getSortType() == AST::Component::Sort::SortType::Type) {
        CompCtx.carryOuterResource(NewIdx, Alias.getOuter().first,
                                   Alias.getOuter().second);
      }
      // If the alias creates a new instance entry out of an `alias export`
      // on another instance, propagate the source instance's export table
      // into the new slot so a subsequent `alias export` on this slot can
      // resolve nested exports.
      if (Alias.getTargetType() == AST::Component::Alias::TargetType::Export) {
        const auto SrcInstIdx = Alias.getExport().first;
        const auto &SrcName = Alias.getExport().second;
        const auto &SrcExports = CompCtx.getInstance(SrcInstIdx).Exports;
        auto It = SrcExports.find(std::string(SrcName));
        if (It != SrcExports.end()) {
          if (Sort.getSortType() == AST::Component::Sort::SortType::Instance) {
            if (It->second.IT != nullptr) {
              populateInstanceFromType(NewIdx, *It->second.IT);
            } else if (It->second.NestedInstIdx.has_value()) {
              const auto &NestedExports =
                  CompCtx.getInstance(*It->second.NestedInstIdx).Exports;
              for (const auto &[Name, IE] : NestedExports) {
                CompCtx.addInstanceExport(NewIdx, Name, IE.ST, IE.IT,
                                          IE.NestedInstIdx, IE.ResourceId);
              }
            }
          } else if (Sort.getSortType() ==
                         AST::Component::Sort::SortType::Type &&
                     It->second.ResourceId.has_value()) {
            // Alias-export of a resource type — same identity.
            CompCtx.addResource(NewIdx, {*It->second.ResourceId,
                                         /*LocallyDefined=*/false});
          }
        }
      }
    }
  }
  return {};
}

Expect<void>
Validator::validate(const AST::Component::TypeSection &TypeSec) noexcept {
  for (const auto &Type : TypeSec.getContent()) {
    EXPECTED_TRY(validate(Type).map_error([](auto E) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Sec_Type));
      return E;
    }));
  }
  return {};
}

Expect<void>
Validator::validate(const AST::Component::CanonSection &CanonSec) noexcept {
  for (const auto &C : CanonSec.getContent()) {
    EXPECTED_TRY(validate(C).map_error([](auto E) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Sec_Canon));
      return E;
    }));
  }
  return {};
}

Expect<void>
Validator::validate(const AST::Component::StartSection &StartSec) noexcept {
  // Validation steps:
  //   1. `f` is in bounds of the component func index space.
  //   2. `f`'s functype param arity equals |arg*| and result arity equals
  //      `r`. Per-argument value-type subtype is checked once subtype
  //      machinery exists (Phase 3).
  //   3. Each argument index is in bounds of the value index space and
  //      has not already been consumed (value linearity).
  //   4. The function's result types are appended to the value index space
  //      as fresh values, so later definitions can reference them.
  const auto &Start = StartSec.getContent();

  // 1. Function index bounds.
  const uint32_t FuncIdx = Start.getFunctionIndex();
  const uint32_t FuncSpaceSize =
      CompCtx.getSortIndexSize(AST::Component::Sort::SortType::Func);
  if (FuncIdx >= FuncSpaceSize) {
    spdlog::error(ErrCode::Value::InvalidIndex);
    spdlog::error(
        "    Start: function index {} exceeds func index space size {}"sv,
        FuncIdx, FuncSpaceSize);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Sec_Start));
    return Unexpect(ErrCode::Value::InvalidIndex);
  }

  // 2. Look up the func type. If null (e.g. imported component func without
  // populated FuncType yet), skip arity check — the bound check above is
  // enough for the moment.
  const AST::Component::FuncType *FT = CompCtx.getFunc(FuncIdx);
  if (FT != nullptr) {
    const auto Args = Start.getArguments();
    const auto &ParamList = FT->getParamList();
    if (Args.size() != ParamList.size()) {
      spdlog::error(ErrCode::Value::InvalidIndex);
      spdlog::error(
          "    Start: argument count {} does not match func {} param arity {}"sv,
          Args.size(), FuncIdx, ParamList.size());
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Sec_Start));
      return Unexpect(ErrCode::Value::InvalidIndex);
    }
    const uint32_t ResultArity =
        static_cast<uint32_t>(FT->getResultList().size());
    if (Start.getResult() != ResultArity) {
      spdlog::error(ErrCode::Value::InvalidIndex);
      spdlog::error(
          "    Start: declared result count {} does not match func {} result arity {}"sv,
          Start.getResult(), FuncIdx, ResultArity);
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Sec_Start));
      return Unexpect(ErrCode::Value::InvalidIndex);
    }
  }

  // 3. Each argument index must be in value index space bounds and must not
  // have been consumed already (value linearity).
  for (const uint32_t ArgIdx : Start.getArguments()) {
    if (auto Res = CompCtx.consumeValue(ArgIdx); !Res) {
      spdlog::error(Res.error());
      if (Res.error() == ErrCode::Value::InvalidIndex) {
        spdlog::error(
            "    Start: argument value index {} is out of value index space bounds"sv,
            ArgIdx);
      } else {
        spdlog::error("    Start: value index {} has already been consumed"sv,
                      ArgIdx);
      }
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Sec_Start));
      return Unexpect(Res);
    }
  }

  // 4. Append result values to the value index space.
  for (uint32_t I = 0; I < Start.getResult(); ++I) {
    CompCtx.addValue();
  }
  return {};
}

Expect<void>
Validator::validate(const AST::Component::ImportSection &ImpSec) noexcept {
  for (const auto &Imp : ImpSec.getContent()) {
    EXPECTED_TRY(validate(Imp).map_error([](auto E) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Sec_Import));
      return E;
    }));
  }
  return {};
}

Expect<void>
Validator::validate(const AST::Component::ExportSection &ExpSec) noexcept {
  for (const auto &Exp : ExpSec.getContent()) {
    EXPECTED_TRY(validate(Exp).map_error([](auto E) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Sec_Export));
      return E;
    }));
  }
  return {};
}

Expect<void>
Validator::validate(const AST::Component::ValueSection &ValSec) noexcept {
  // Each defined value appends one entry to the value index space. Validate
  // the declared type first, then register it so that downstream value
  // references (e.g. start arguments) resolve against the right bounds.
  for (const auto &Val : ValSec.getContent()) {
    EXPECTED_TRY(validate(Val.getType()).map_error([](auto E) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Sec_Value));
      return E;
    }));
    CompCtx.addValue();
  }
  return {};
}

Expect<void>
Validator::validate(const AST::Component::CoreInstance &Inst) noexcept {
  if (Inst.isInstantiateModule()) {
    // Instantiate module case.

    // Check the module index bound first.
    const uint32_t ModIdx = Inst.getModuleIndex();
    if (ModIdx >= CompCtx.getCoreSortIndexSize(
                      AST::Component::Sort::CoreSortType::Module)) {
      spdlog::error(ErrCode::Value::InvalidIndex);
      spdlog::error(
          "    CoreInstance: Module index {} exceeds available core modules {}"sv,
          ModIdx,
          CompCtx.getCoreSortIndexSize(
              AST::Component::Sort::CoreSortType::Module));
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_CoreInstance));
      return Unexpect(ErrCode::Value::InvalidIndex);
    }
    // Reject duplicate argument names on an instantiate expression. The
    // spec requires argument names to be strongly-unique per instantiation.
    {
      std::unordered_set<std::string_view> SeenArgs;
      for (const auto &Arg : Inst.getInstantiateArgs()) {
        if (!SeenArgs.insert(Arg.getName()).second) {
          spdlog::error(ErrCode::Value::ComponentDuplicateName);
          spdlog::error("    CoreInstance: Duplicate argument name '{}'"sv,
                        Arg.getName());
          spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_CoreInstance));
          return Unexpect(ErrCode::Value::ComponentDuplicateName);
        }
      }
    }

    // Imports + exports come from the raw Module (inline) or the
    // CoreModuleType (imported / aliased) — GAP-CI-1.
    const auto &CoreModSlot = CompCtx.getCoreModule(ModIdx);
    const auto *Mod = CoreModSlot.Body;
    const auto *ModTy = CoreModSlot.Type;

    // Required arg module-names (one per distinct CoreImportDecl module).
    std::vector<std::string_view> RequiredArgNames;
    if (Mod != nullptr) {
      for (const auto &Import : Mod->getImportSection().getContent()) {
        RequiredArgNames.push_back(Import.getModuleName());
      }
    } else if (ModTy != nullptr && ModTy->isModuleType()) {
      for (const auto &Decl : ModTy->getModuleType()) {
        if (Decl.isImport()) {
          RequiredArgNames.push_back(Decl.getImport().getModuleName());
        }
      }
    }

    auto Args = Inst.getInstantiateArgs();
    for (const auto ImportName : RequiredArgNames) {
      const auto ArgIt =
          std::find_if(Args.begin(), Args.end(), [&](const auto &Arg) {
            return Arg.getName() == ImportName;
          });
      if (ArgIt == Args.end()) {
        spdlog::error(ErrCode::Value::MissingArgument);
        spdlog::error(
            "    CoreInstance: Module index {} missing argument for import '{}'"sv,
            Inst.getModuleIndex(), ImportName);
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_CoreInstance));
        return Unexpect(ErrCode::Value::MissingArgument);
      }
    }

    // GAP-CI-1: the index type (i32/i64) of each provided memory must match
    // the corresponding memory import of the instantiated module.
    if (Mod != nullptr) {
      const uint32_t InstCount = CompCtx.getCoreSortIndexSize(
          AST::Component::Sort::CoreSortType::Instance);
      for (const auto &Import : Mod->getImportSection().getContent()) {
        if (Import.getExternalType() != ExternalType::Memory) {
          continue;
        }
        const auto ArgIt =
            std::find_if(Args.begin(), Args.end(), [&](const auto &Arg) {
              return Arg.getName() == Import.getModuleName();
            });
        if (ArgIt == Args.end() || ArgIt->getIndex() >= InstCount) {
          continue;
        }
        const auto &ProvExports = CompCtx.getCoreInstance(ArgIt->getIndex());
        const auto ExpIt =
            ProvExports.find(std::string(Import.getExternalName()));
        if (ExpIt == ProvExports.end() ||
            ExpIt->second.Kind != ExternalType::Memory ||
            !ExpIt->second.Mem.has_value()) {
          continue;
        }
        if (Import.getExternalMemoryType().getLimit().is64() !=
            ExpIt->second.Mem->getLimit().is64()) {
          spdlog::error(ErrCode::Value::ComponentMemoryIndexTypeMismatch);
          spdlog::error(
              "    CoreInstance: memory import '{}'.'{}' index type does not "
              "match the provided memory"sv,
              Import.getModuleName(), Import.getExternalName());
          spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_CoreInstance));
          return Unexpect(ErrCode::Value::ComponentMemoryIndexTypeMismatch);
        }
      }
    }

    // GAP-CI-1 (cont.): for an imported core module type, subtype-check each
    // provided core extern (memory / table / global) against the import.
    if (ModTy != nullptr && ModTy->isModuleType()) {
      const uint32_t InstCount = CompCtx.getCoreSortIndexSize(
          AST::Component::Sort::CoreSortType::Instance);
      for (const auto &Decl : ModTy->getModuleType()) {
        if (!Decl.isImport()) {
          continue;
        }
        const auto &Imp = Decl.getImport();
        const auto Desc = Imp.getImportDesc();
        if (!Desc.isMemory() && !Desc.isTable() && !Desc.isGlobal()) {
          continue;
        }
        const auto ArgIt =
            std::find_if(Args.begin(), Args.end(), [&](const auto &Arg) {
              return Arg.getName() == Imp.getModuleName();
            });
        if (ArgIt == Args.end() || ArgIt->getIndex() >= InstCount) {
          continue;
        }
        const auto &ProvExports = CompCtx.getCoreInstance(ArgIt->getIndex());
        const auto ExpIt = ProvExports.find(std::string(Imp.getName()));
        if (ExpIt == ProvExports.end()) {
          continue;
        }
        const auto &Got = ExpIt->second;
        auto reportMismatch =
            [&](ErrCode::Value Code,
                std::string_view What) -> Unexpected<ErrCode> {
          spdlog::error(Code);
          spdlog::error("    CoreInstance: import '{}'.'{}' {}"sv,
                        Imp.getModuleName(), Imp.getName(), What);
          spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_CoreInstance));
          return Unexpect(Code);
        };
        if (Desc.isMemory() && Got.Kind == ExternalType::Memory &&
            Got.Mem.has_value()) {
          const auto &W = Desc.getMemoryType().getLimit();
          const auto &G = Got.Mem->getLimit();
          if (W.is64() != G.is64()) {
            return reportMismatch(
                ErrCode::Value::ComponentMemoryIndexTypeMismatch,
                "memory index type mismatch");
          }
        } else if (Desc.isTable() && Got.Kind == ExternalType::Table &&
                   Got.Tab.has_value()) {
          const auto &W = Desc.getTableType();
          const auto &G = *Got.Tab;
          if (!(W.getRefType() == G.getRefType())) {
            return reportMismatch(
                ErrCode::Value::ComponentTableElemTypeMismatch,
                "table element type mismatch");
          }
          const auto &WL = W.getLimit();
          const auto &GL = G.getLimit();
          const bool LimitsOk =
              GL.is64() == WL.is64() && GL.getMin() >= WL.getMin() &&
              (!WL.hasMax() || (GL.hasMax() && GL.getMax() <= WL.getMax()));
          if (!LimitsOk) {
            return reportMismatch(ErrCode::Value::ComponentTableLimitsMismatch,
                                  "table limits mismatch");
          }
        } else if (Desc.isGlobal() && Got.Kind == ExternalType::Global &&
                   Got.Glob.has_value()) {
          const auto &W = Desc.getGlobalType();
          const auto &G = *Got.Glob;
          if (!(W.getValType() == G.getValType()) ||
              W.getValMut() != G.getValMut()) {
            return reportMismatch(ErrCode::Value::ComponentGlobalTypeMismatch,
                                  "global type mismatch");
          }
        }
      }
    }

    // Allocate the core:instance and bind exports to it.
    uint32_t InstanceIdx = CompCtx.addCoreInstance();
    if (Mod != nullptr) {
      for (const auto &ExportDesc : Mod->getExportSection().getContent()) {
        // Resolve the memory type so re-exports carry their index type for
        // later instantiation subtype checks (GAP-CI-1).
        const AST::MemoryType *MemTy = nullptr;
        if (ExportDesc.getExternalType() == ExternalType::Memory) {
          const uint32_t Target = ExportDesc.getExternalIndex();
          uint32_t MemImpCount = 0;
          for (const auto &Imp : Mod->getImportSection().getContent()) {
            if (Imp.getExternalType() != ExternalType::Memory) {
              continue;
            }
            if (MemImpCount == Target) {
              MemTy = &Imp.getExternalMemoryType();
              break;
            }
            ++MemImpCount;
          }
          if (MemTy == nullptr) {
            const auto Mems = Mod->getMemorySection().getContent();
            const uint32_t Local = Target - MemImpCount;
            if (Local < Mems.size()) {
              MemTy = &Mems[Local];
            }
          }
        }
        CompCtx.addCoreInstanceExport(InstanceIdx, ExportDesc.getExternalName(),
                                      ExportDesc.getExternalType(), MemTy);
      }
    } else if (ModTy != nullptr && ModTy->isModuleType()) {
      for (const auto &Decl : ModTy->getModuleType()) {
        if (!Decl.isExport()) {
          continue;
        }
        const auto &Exp = Decl.getExport();
        const auto &ImpDesc = Exp.getImportDesc();
        ExternalType ET;
        if (ImpDesc.isFunc()) {
          ET = ExternalType::Function;
        } else if (ImpDesc.isTable()) {
          ET = ExternalType::Table;
        } else if (ImpDesc.isMemory()) {
          ET = ExternalType::Memory;
        } else if (ImpDesc.isGlobal()) {
          ET = ExternalType::Global;
        } else if (ImpDesc.isTag()) {
          ET = ExternalType::Tag;
        } else {
          continue;
        }
        // Carry the core extern type so a later instantiation can subtype-check
        // it (GAP-CI-1). Pointers are copied into the export entry immediately,
        // so pointing into the temporary descriptor is safe here.
        const AST::MemoryType *MemTy =
            ImpDesc.isMemory() ? &ImpDesc.getMemoryType() : nullptr;
        const AST::TableType *TabTy =
            ImpDesc.isTable() ? &ImpDesc.getTableType() : nullptr;
        const AST::GlobalType *GlobTy =
            ImpDesc.isGlobal() ? &ImpDesc.getGlobalType() : nullptr;
        CompCtx.addCoreInstanceExport(InstanceIdx, Exp.getName(), ET, MemTy,
                                      TabTy, GlobTy);
      }
    }
  } else if (Inst.isInlineExport()) {
    // Inline export case.
    // Allocate the core instance first, then register each inline export.
    uint32_t InstanceIdx = CompCtx.addCoreInstance();

    // Check the core:sort index bound and register the inline exports.
    // Inline-export names on a core instance must be strongly-unique.
    std::unordered_set<std::string_view> SeenExports;
    for (const auto &Export : Inst.getInlineExports()) {
      if (!SeenExports.insert(Export.getName()).second) {
        spdlog::error(ErrCode::Value::ComponentDuplicateName);
        spdlog::error("    CoreInstance: Duplicate inline-export name '{}'"sv,
                      Export.getName());
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_CoreInstance));
        return Unexpect(ErrCode::Value::ComponentDuplicateName);
      }
      const auto &Sort = Export.getSortIdx().getSort();
      uint32_t Idx = Export.getSortIdx().getIdx();
      assuming(Sort.isCore());
      if (Idx >= CompCtx.getCoreSortIndexSize(Sort.getCoreSortType())) {
        // The error message differs of the tag core sort.
        ErrCode::Value ErrValue = ErrCode::Value::InvalidIndex;
        if (Sort.getCoreSortType() == AST::Component::Sort::CoreSortType::Tag) {
          ErrValue = ErrCode::Value::UnknownCoreTag;
        }
        spdlog::error(ErrValue);
        spdlog::error(
            "    CoreInstance: Inline export '{}' refers to invalid index {}"sv,
            Export.getName(), Idx);
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_CoreInstance));
        return Unexpect(ErrValue);
      }
      // Map CoreSortType to ExternalType for the instance export map.
      ExternalType ET;
      switch (Sort.getCoreSortType()) {
      case AST::Component::Sort::CoreSortType::Func:
        ET = ExternalType::Function;
        break;
      case AST::Component::Sort::CoreSortType::Table:
        ET = ExternalType::Table;
        break;
      case AST::Component::Sort::CoreSortType::Memory:
        ET = ExternalType::Memory;
        break;
      case AST::Component::Sort::CoreSortType::Global:
        ET = ExternalType::Global;
        break;
      case AST::Component::Sort::CoreSortType::Tag:
        ET = ExternalType::Tag;
        break;
      default:
        spdlog::error(ErrCode::Value::InvalidIndex);
        spdlog::error(
            "    CoreInstance: Inline export '{}' has unsupported core sort"sv,
            Export.getName());
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_CoreInstance));
        return Unexpect(ErrCode::Value::InvalidIndex);
      }
      CompCtx.addCoreInstanceExport(InstanceIdx, Export.getName(), ET);
    }
  } else {
    assumingUnreachable();
  }
  return {};
}

Expect<void>
Validator::validate(const AST::Component::Instance &Inst) noexcept {
  if (Inst.isInstantiateModule()) {
    // Instantiate module case.

    // Check the component index bound first.
    const uint32_t CompIdx = Inst.getComponentIndex();
    if (CompIdx >=
        CompCtx.getSortIndexSize(AST::Component::Sort::SortType::Component)) {
      spdlog::error(ErrCode::Value::InvalidIndex);
      spdlog::error(
          "    Instance: Component index {} exceeds available components {}"sv,
          CompIdx,
          CompCtx.getSortIndexSize(AST::Component::Sort::SortType::Component));
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Instance));
      return Unexpect(ErrCode::Value::InvalidIndex);
    }
    // Reject duplicate argument names on an instantiate expression. The
    // spec requires argument names to be strongly-unique per instantiation.
    {
      std::unordered_set<std::string_view> SeenArgs;
      for (const auto &Arg : Inst.getInstantiateArgs()) {
        if (!SeenArgs.insert(Arg.getName()).second) {
          spdlog::error(ErrCode::Value::ComponentDuplicateName);
          spdlog::error("    Instance: Duplicate argument name '{}'"sv,
                        Arg.getName());
          spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Instance));
          return Unexpect(ErrCode::Value::ComponentDuplicateName);
        }
      }
    }

    // Source: raw Component (inline) or ComponentType (imported / aliased).
    const auto &CompSlot = CompCtx.getComponent(CompIdx);
    const auto *Comp = CompSlot.Body;
    const auto *CompTy = CompSlot.Type;

    // Verify each component import is satisfied by some instantiate arg.
    auto Args = Inst.getInstantiateArgs();
    auto checkImport =
        [&](std::string_view ImportName,
            const AST::Component::ExternDesc &ImportDesc) -> Expect<void> {
      const auto ArgIt =
          std::find_if(Args.begin(), Args.end(), [&](const auto &Arg) {
            return Arg.getName() == ImportName;
          });
      if (ArgIt == Args.end()) {
        spdlog::error(ErrCode::Value::MissingArgument);
        spdlog::error(
            "    Instance: Component index {} missing argument for import '{}'"sv,
            Inst.getComponentIndex(), ImportName);
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Instance));
        return Unexpect(ErrCode::Value::MissingArgument);
      }
      const auto &Sort = ArgIt->getIndex().getSort();
      const uint32_t Idx = ArgIt->getIndex().getIdx();
      // Only `core module` is admissible as a core-side import externdesc.
      if (Sort.isCore() && Sort.getCoreSortType() !=
                               AST::Component::Sort::CoreSortType::Module) {
        spdlog::error(ErrCode::Value::ArgTypeMismatch);
        spdlog::error("    Instance: Argument '{}' uses a core sort other than "
                      "`core module`, which no import externdesc can accept"sv,
                      ImportName);
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Instance));
        return Unexpect(ErrCode::Value::ArgTypeMismatch);
      }
      if (!sortMatchesDescType(Sort, ImportDesc.getDescType())) {
        spdlog::error(ErrCode::Value::ArgTypeMismatch);
        spdlog::error("    Instance: Argument '{}' sort mismatch for import"sv,
                      ImportName);
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Instance));
        return Unexpect(ErrCode::Value::ArgTypeMismatch);
      }
      if (Sort.isCore()) {
        if (Idx >= CompCtx.getCoreSortIndexSize(Sort.getCoreSortType())) {
          spdlog::error(ErrCode::Value::InvalidIndex);
          spdlog::error(
              "    Instance: Argument '{}' refers to invalid index {}"sv,
              ImportName, Idx);
          spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Instance));
          return Unexpect(ErrCode::Value::InvalidIndex);
        }
        return {};
      }
      if (Idx >= CompCtx.getSortIndexSize(Sort.getSortType())) {
        spdlog::error(ErrCode::Value::InvalidIndex);
        spdlog::error(
            "    Instance: Argument '{}' refers to invalid index {}"sv,
            ImportName, Idx);
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Instance));
        return Unexpect(ErrCode::Value::InvalidIndex);
      }
      // Partial subtype: for instance-typed imports, verify the provided
      // instance has every required export (raw-Component path only;
      // ComponentType path needs GAP-DECL-ED).
      if (Comp != nullptr &&
          ImportDesc.getDescType() ==
              AST::Component::ExternDesc::DescType::InstanceType &&
          Sort.getSortType() == AST::Component::Sort::SortType::Instance) {
        const auto *RequiredIT =
            resolveChildInstanceType(*Comp, ImportDesc.getTypeIndex());
        if (RequiredIT != nullptr) {
          if (auto Missing = findMissingRequiredExport(Idx, *RequiredIT)) {
            spdlog::error(ErrCode::Value::InstanceMissingExpectedExport);
            spdlog::error("    Instance: Argument '{}' missing required export "
                          "'{}' for import"sv,
                          ImportName, *Missing);
            spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Instance));
            return Unexpect(ErrCode::Value::InstanceMissingExpectedExport);
          }
        }
      }
      return {};
    };
    if (Comp != nullptr) {
      for (const auto &Sec : Comp->getSections()) {
        if (const auto *IS = std::get_if<AST::Component::ImportSection>(&Sec)) {
          for (const auto &Imp : IS->getContent()) {
            EXPECTED_TRY(checkImport(Imp.getName(), Imp.getDesc()));
          }
        }
      }
    } else if (CompTy != nullptr) {
      for (const auto &CD : CompTy->getDecl()) {
        if (CD.isImportDecl()) {
          const auto &ID = CD.getImport();
          EXPECTED_TRY(checkImport(ID.getName(), ID.getExternDesc()));
        }
      }
    }

    // Allocate the slot + populate exports so alias-export can resolve.
    // Raw-Component path resolves the IT for instance-typed exports;
    // ComponentType path registers sort-kind only (GAP-DECL-ED).
    // TODO (GAP-I-3): fresh ResourceIds + per-export resource remapping.
    uint32_t InstanceIdx = CompCtx.addInstance();
    if (Comp != nullptr) {
      for (const auto &Sec : Comp->getSections()) {
        const auto *ES = std::get_if<AST::Component::ExportSection>(&Sec);
        if (ES == nullptr) {
          continue;
        }
        for (const auto &Exp : ES->getContent()) {
          const auto &ExpSort = Exp.getSortIndex().getSort();
          if (ExpSort.isCore()) {
            continue;
          }
          const AST::Component::InstanceType *IT = nullptr;
          if (ExpSort.getSortType() ==
                  AST::Component::Sort::SortType::Instance &&
              Exp.getDesc().has_value() &&
              Exp.getDesc()->getDescType() ==
                  AST::Component::ExternDesc::DescType::InstanceType) {
            IT = resolveChildInstanceType(*Comp, Exp.getDesc()->getTypeIndex());
          }
          CompCtx.addInstanceExport(InstanceIdx, Exp.getName(),
                                    ExpSort.getSortType(), IT);
        }
      }
    } else if (CompTy != nullptr) {
      for (const auto &CD : CompTy->getDecl()) {
        if (!CD.isInstanceDecl()) {
          continue;
        }
        const auto &ID = CD.getInstance();
        if (!ID.isExportDecl()) {
          continue;
        }
        const auto &ED = ID.getExport();
        const auto OptST = descTypeToSortType(ED.getExternDesc().getDescType());
        if (!OptST.has_value()) {
          continue; // `(core module)` export — not a component-side entry.
        }
        CompCtx.addInstanceExport(InstanceIdx, ED.getName(), *OptST);
      }
    }
  } else if (Inst.isInlineExport()) {
    // Allocate the instance first so exports can be registered on it.
    uint32_t InstanceIdx = CompCtx.addInstance();

    // Check the sort index bound of the inline exports, then record each
    // export on the new instance with a NestedInstIdx fallback so alias
    // chains through inline-export instances can follow the source.
    // Inline-export names on a component instance must be strongly-unique.
    std::unordered_set<std::string_view> SeenExports;
    for (const auto &Export : Inst.getInlineExports()) {
      // Inline-export names must be valid component export names.
      if (auto Res = validateExportName(Export.getName()); !Res) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Instance));
        return Unexpect(Res.error());
      }
      if (!SeenExports.insert(Export.getName()).second) {
        spdlog::error(ErrCode::Value::ComponentDuplicateName);
        spdlog::error("    Instance: Duplicate inline-export name '{}'"sv,
                      Export.getName());
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Instance));
        return Unexpect(ErrCode::Value::ComponentDuplicateName);
      }
      const auto &Sort = Export.getSortIdx().getSort();
      uint32_t Idx = Export.getSortIdx().getIdx();
      if (Sort.isCore()) {
        if (Idx >= CompCtx.getCoreSortIndexSize(Sort.getCoreSortType())) {
          spdlog::error(ErrCode::Value::InvalidIndex);
          spdlog::error(
              "    Instance: Inline export '{}' refers to invalid index {}"sv,
              Export.getName(), Idx);
          spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Instance));
          return Unexpect(ErrCode::Value::InvalidIndex);
        }
        continue;
      }
      if (Idx >= CompCtx.getSortIndexSize(Sort.getSortType())) {
        spdlog::error(ErrCode::Value::InvalidIndex);
        spdlog::error(
            "    Instance: Inline export '{}' refers to invalid index {}"sv,
            Export.getName(), Idx);
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Instance));
        return Unexpect(ErrCode::Value::InvalidIndex);
      }
      if (Sort.getSortType() == AST::Component::Sort::SortType::Type) {
        auto SubstitutedIdx =
            CompCtx.getSubstitutedType(std::string(Export.getName()));
        if (SubstitutedIdx.has_value() && Idx != SubstitutedIdx.value()) {
          spdlog::error(ErrCode::Value::InvalidTypeReference);
          spdlog::error(
              "    Instance: Inline export '{}' type index {} does not match substituted type index {}"sv,
              Export.getName(), Idx, *SubstitutedIdx);
          return Unexpect(ErrCode::Value::InvalidTypeReference);
        }
      }
      std::optional<uint32_t> NestedIdx;
      const AST::Component::InstanceType *PropagatedIT = nullptr;
      if (Sort.getSortType() == AST::Component::Sort::SortType::Instance) {
        NestedIdx = Idx;
        // GAP-I-5b: forward source's InstanceType so later ascription /
        // subtype checks have a concrete type. Populated only when the
        // source slot was bound via ExternDesc::InstanceType; inline-
        // export / instantiate sources read nullptr (Phase-3 follow-up).
        PropagatedIT = CompCtx.getInstance(Idx).Type;
      }
      CompCtx.addInstanceExport(InstanceIdx, Export.getName(),
                                Sort.getSortType(), PropagatedIT, NestedIdx);
    }
  } else {
    assumingUnreachable();
  }
  return {};
}

Expect<void> Validator::validate(const AST::Component::CoreAlias &A) noexcept {
  // CoreAlias is always an outer alias.
  uint32_t Ct = A.getComponentJump();
  uint32_t Idx = A.getIndex();

  uint32_t OutLinkCompCnt = 0;
  const auto *TargetCtx = &CompCtx.getCurrentContext();
  while (Ct > OutLinkCompCnt && TargetCtx != nullptr) {
    TargetCtx = TargetCtx->Parent;
    OutLinkCompCnt++;
  }
  if (TargetCtx == nullptr) {
    spdlog::error(ErrCode::Value::InvalidIndex);
    // The final hop is the one that ran off the top of the scope chain, so the
    // number of actually-enclosing components is one less than the hop count.
    spdlog::error(
        "    CoreAlias: outer count {} exceeds enclosing component count {}"sv,
        Ct, OutLinkCompCnt - 1);
    return Unexpect(ErrCode::Value::InvalidIndex);
  }

  const auto &Sort = A.getSort();
  if (Sort.isCore()) {
    if (Idx >= TargetCtx->getCoreSortIndexSize(Sort.getCoreSortType())) {
      spdlog::error(ErrCode::Value::InvalidIndex);
      spdlog::error("    CoreAlias: outer index {} out of bounds"sv, Idx);
      return Unexpect(ErrCode::Value::InvalidIndex);
    }
    CompCtx.incCoreSortIndexSize(Sort.getCoreSortType());
  }
  return {};
}

Expect<void> Validator::validate(const AST::Component::Alias &Alias) noexcept {
  const auto &Sort = Alias.getSort();
  switch (Alias.getTargetType()) {
  case AST::Component::Alias::TargetType::Export: {
    const auto Idx = Alias.getExport().first;
    const auto &Name = Alias.getExport().second;

    if (Idx >=
        CompCtx.getSortIndexSize(AST::Component::Sort::SortType::Instance)) {
      spdlog::error(ErrCode::Value::InvalidIndex);
      spdlog::error(
          "    Alias export: Export index {} exceeds available component instance index {}"sv,
          Idx,
          CompCtx.getSortIndexSize(AST::Component::Sort::SortType::Instance));
      return Unexpect(ErrCode::Value::InvalidIndex);
    }

    const auto &InstExports = CompCtx.getInstance(Idx).Exports;
    auto It = InstExports.find(std::string(Name));
    if (It == InstExports.cend()) {
      spdlog::error(ErrCode::Value::ComponentUnknownExport);
      spdlog::error(
          "    Alias export: No matching export '{}' found in component instance index {}"sv,
          Name, Idx);
      return Unexpect(ErrCode::Value::ComponentUnknownExport);
    }

    // A component instance can only export a core module as a core sort.
    const bool Matches = Sort.isCore()
                             ? (It->second.IsCoreModule &&
                                Sort.getCoreSortType() ==
                                    AST::Component::Sort::CoreSortType::Module)
                             : (It->second.ST == Sort.getSortType());
    if (!Matches) {
      spdlog::error(ErrCode::Value::InvalidTypeReference);
      spdlog::error("    Alias export: Type mapping mismatch for export '{}'"sv,
                    Name);
      return Unexpect(ErrCode::Value::InvalidTypeReference);
    }
    return {};
  }
  case AST::Component::Alias::TargetType::CoreExport: {
    const auto Idx = Alias.getExport().first;
    const auto &Name = Alias.getExport().second;

    if (!Sort.isCore()) {
      spdlog::error(ErrCode::Value::InvalidTypeReference);
      spdlog::error("    Alias core:export: Mapping a export '{}' to sort"sv,
                    Name);
      return Unexpect(ErrCode::Value::InvalidTypeReference);
    }

    if (Idx >= CompCtx.getCoreSortIndexSize(
                   AST::Component::Sort::CoreSortType::Instance)) {
      spdlog::error(ErrCode::Value::InvalidIndex);
      spdlog::error(
          "    Alias core:export: Export index {} exceeds available core instance index {}"sv,
          Idx,
          CompCtx.getCoreSortIndexSize(
              AST::Component::Sort::CoreSortType::Instance) -
              1);
      return Unexpect(ErrCode::Value::InvalidIndex);
    }

    const auto &CoreExports = CompCtx.getCoreInstance(Idx);
    auto It = CoreExports.find(std::string(Name));
    if (It == CoreExports.end()) {
      spdlog::error(ErrCode::Value::ExportNotFound);
      spdlog::error(
          "    Alias core:export: No matching export '{}' found in core instance index {}"sv,
          Name, Idx);
      return Unexpect(ErrCode::Value::ExportNotFound);
    }

    const auto ExternTy = It->second.Kind;
    AST::Component::Sort::CoreSortType ST;
    switch (ExternTy) {
    case ExternalType::Function:
      ST = AST::Component::Sort::CoreSortType::Func;
      break;
    case ExternalType::Table:
      ST = AST::Component::Sort::CoreSortType::Table;
      break;
    case ExternalType::Memory:
      ST = AST::Component::Sort::CoreSortType::Memory;
      break;
    case ExternalType::Global:
      ST = AST::Component::Sort::CoreSortType::Global;
      break;
    case ExternalType::Tag:
      ST = AST::Component::Sort::CoreSortType::Tag;
      break;
    default:
      spdlog::error(ErrCode::Value::InvalidTypeReference);
      spdlog::error(
          "    Alias core:export: Type mapping mismatch for export '{}'"sv,
          Name);
      return Unexpect(ErrCode::Value::InvalidTypeReference);
    }
    if (ST != Sort.getCoreSortType()) {
      // The error message differs of the tag core sort.
      ErrCode::Value ErrValue = ErrCode::Value::InvalidIndex;
      if (Sort.getCoreSortType() == AST::Component::Sort::CoreSortType::Tag) {
        ErrValue = ErrCode::Value::UnknownCoreTag;
      }
      spdlog::error(ErrValue);
      spdlog::error(
          "    Alias core:export: Type mapping mismatch for export '{}'"sv,
          Name);
      return Unexpect(ErrValue);
    }
    return {};
  }
  case AST::Component::Alias::TargetType::Outer: {
    const auto Ct = Alias.getOuter().first;
    const auto Idx = Alias.getOuter().second;

    uint32_t OutLinkCompCnt = 0;
    const auto *TargetCtx = &CompCtx.getCurrentContext();
    while (Ct > OutLinkCompCnt && TargetCtx != nullptr) {
      TargetCtx = TargetCtx->Parent;
      OutLinkCompCnt++;
    }
    if (TargetCtx == nullptr) {
      spdlog::error(ErrCode::Value::InvalidIndex);
      // The final hop is the one that ran off the top of the scope chain, so
      // the number of actually-enclosing components is one less than the hop
      // count.
      spdlog::error(
          "    Alias outer: Component out-link count {} is exceeding the enclosing component count {}"sv,
          Ct, OutLinkCompCnt - 1);
      return Unexpect(ErrCode::Value::InvalidIndex);
    }

    if (Sort.isCore()) {
      if (Sort.getCoreSortType() !=
              AST::Component::Sort::CoreSortType::Module &&
          Sort.getCoreSortType() != AST::Component::Sort::CoreSortType::Type) {
        spdlog::error(ErrCode::Value::InvalidTypeReference);
        spdlog::error(
            "    Alias outer: Invalid core:sort for outer alias. Only type, module, or component are allowed."sv);
        return Unexpect(ErrCode::Value::InvalidTypeReference);
      }
      if (Idx >= TargetCtx->getCoreSortIndexSize(Sort.getCoreSortType())) {
        spdlog::error(ErrCode::Value::InvalidIndex);
        spdlog::error(
            "    Alias outer: core:sort index {} invalid in component context"sv,
            Idx);
        return Unexpect(ErrCode::Value::InvalidIndex);
      }
    } else {
      if (Sort.getSortType() != AST::Component::Sort::SortType::Type &&
          Sort.getSortType() != AST::Component::Sort::SortType::Component) {
        spdlog::error(ErrCode::Value::InvalidTypeReference);
        spdlog::error(
            "    Alias outer: Invalid sort for outer alias. Only type, module, or component are allowed."sv);
        return Unexpect(ErrCode::Value::InvalidTypeReference);
      }
      if (Idx >= TargetCtx->getSortIndexSize(Sort.getSortType())) {
        spdlog::error(ErrCode::Value::InvalidIndex);
        spdlog::error(
            "    Alias outer: sort index {} invalid in component context"sv,
            Idx);
        return Unexpect(ErrCode::Value::InvalidIndex);
      }
      // Outer-aliasing a resource type is permitted: the alias preserves the
      // resource's type identity rather than introducing a fresh generative
      // resource, so a nested type/component can legitimately refer to a
      // resource in an enclosing scope (e.g. an instance type referring to a
      // resource imported by its enclosing component type).
      //
      // TODO: implement wasm-tools' full free-variable rule — reject an outer
      // alias to a type whose transitive free variables include resources that
      // would escape a crossed component boundary. That needs a free-variable
      // walker over type bodies, tracked alongside the Phase 3/4 type-handle
      // work (GAP-TH-1, GAP-EX-5).
    }
    return {};
  }
  default:
    assumingUnreachable();
  }
}

Expect<void>
Validator::validate(const AST::Component::CoreDefType &DType) noexcept {
  if (DType.isRecType()) {
    // Each sub-type in the rec group gets its own entry in core:type.
    for (const auto &ST : DType.getSubTypes()) {
      CompCtx.addCoreType(&ST);
    }
  } else if (DType.isModuleType()) {
    // Module types are validated with an initially-empty type index space.
    CompCtx.enterTypeDefinition();
    for (const auto &Decl : DType.getModuleType()) {
      EXPECTED_TRY(validate(Decl).map_error([](auto E) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_CoreDefType));
        return E;
      }));
    }
    CompCtx.exitComponent();
    // Module type gets a core:type slot (Body=nullptr) bound to the
    // CoreDefType so (core module (type i)) can recover the body — GAP-CI-1.
    uint32_t NewTypeIdx = CompCtx.addCoreType();
    CompCtx.setCoreModuleType(NewTypeIdx, &DType);
  } else {
    assumingUnreachable();
  }
  return {};
}

Expect<void>
Validator::validate(const AST::Component::DefType &DType) noexcept {
  auto ReportError = [](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_DefType));
    return E;
  };

  if (DType.isDefValType()) {
    EXPECTED_TRY(validate(DType.getDefValType()).map_error(ReportError));
  } else if (DType.isFuncType()) {
    EXPECTED_TRY(validate(DType.getFuncType()).map_error(ReportError));
  } else if (DType.isComponentType()) {
    EXPECTED_TRY(validate(DType.getComponentType()).map_error(ReportError));
  } else if (DType.isInstanceType()) {
    EXPECTED_TRY(validate(DType.getInstanceType()).map_error(ReportError));
  } else if (DType.isResourceType()) {
    EXPECTED_TRY(validate(DType.getResourceType()).map_error(ReportError));
  } else {
    assumingUnreachable();
  }
  // addType records body/id/locality for resource DefTypes in one step.
  CompCtx.addType(&DType);
  return {};
}

Expect<void>
Validator::validate(const AST::Component::Canonical &Canon) noexcept {
  switch (Canon.getOpCode()) {
  case ComponentCanonOpCode::Lift:
    return validateCanonLift(Canon);
  case ComponentCanonOpCode::Lower:
    return validateCanonLower(Canon);
  case ComponentCanonOpCode::Resource__new:
    return validateCanonResourceNew(Canon);
  case ComponentCanonOpCode::Resource__rep:
    return validateCanonResourceRep(Canon);
  case ComponentCanonOpCode::Resource__drop:
  case ComponentCanonOpCode::Resource__drop_async:
    return validateCanonResourceDrop(Canon);
  default:
    spdlog::error(ErrCode::Value::ComponentNotImplValidator);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Canonical));
    return Unexpect(ErrCode::Value::ComponentNotImplValidator);
  }
}

Expect<void> Validator::validateCanonOptions(
    ComponentCanonOpCode Code,
    Span<const AST::Component::CanonOpt> Opts) noexcept {
  using OptCode = ComponentCanonOptCode;
  using CanonOp = ComponentCanonOpCode;

  // Only canon lift/lower accept canonical options. Any other built-in
  // (resource.new/rep/drop, drop_async, ...) must be invoked without options.
  if (Code != CanonOp::Lift && Code != CanonOp::Lower && !Opts.empty()) {
    spdlog::error(ErrCode::Value::InvalidCanonOption);
    spdlog::error(
        "    canonical options are not allowed for this canon built-in"sv);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Canonical));
    return Unexpect(ErrCode::Value::InvalidCanonOption);
  }

  bool HasEncoding = false;
  bool HasMemory = false;
  bool HasRealloc = false;
  bool HasPostReturn = false;
  bool HasAsync = false;
  bool HasCallback = false;
  bool HasAlwaysTaskReturn = false;
  uint32_t ReallocIdx = 0;
  uint32_t CallbackIdx = 0;
  uint32_t PostReturnIdx = 0;
  uint32_t MemoryIdx = 0;

  auto RejectDup = [&](const char *Name) -> Expect<void> {
    spdlog::error(ErrCode::Value::InvalidCanonOption);
    spdlog::error("    canonical option '{}' appears more than once"sv, Name);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Canonical));
    return Unexpect(ErrCode::Value::InvalidCanonOption);
  };
  auto RejectSite = [&](const char *Name) -> Expect<void> {
    spdlog::error(ErrCode::Value::InvalidCanonOption);
    spdlog::error(
        "    canonical option '{}' is not allowed in this canon built-in"sv,
        Name);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Canonical));
    return Unexpect(ErrCode::Value::InvalidCanonOption);
  };

  for (const auto &Opt : Opts) {
    switch (Opt.getCode()) {
    case OptCode::Encode_UTF8:
    case OptCode::Encode_UTF16:
    case OptCode::Encode_Latin1:
      if (HasEncoding) {
        return RejectDup("string-encoding");
      }
      HasEncoding = true;
      break;
    case OptCode::Memory:
      if (HasMemory) {
        return RejectDup("memory");
      }
      HasMemory = true;
      MemoryIdx = Opt.getIndex();
      break;
    case OptCode::Realloc:
      if (HasRealloc) {
        return RejectDup("realloc");
      }
      HasRealloc = true;
      ReallocIdx = Opt.getIndex();
      break;
    case OptCode::PostReturn:
      if (Code != CanonOp::Lift) {
        return RejectSite("post-return");
      }
      if (HasPostReturn) {
        return RejectDup("post-return");
      }
      HasPostReturn = true;
      PostReturnIdx = Opt.getIndex();
      break;
    case OptCode::Async:
      if (HasAsync) {
        return RejectDup("async");
      }
      HasAsync = true;
      break;
    case OptCode::Callback:
      if (Code != CanonOp::Lift) {
        return RejectSite("callback");
      }
      if (HasCallback) {
        return RejectDup("callback");
      }
      HasCallback = true;
      CallbackIdx = Opt.getIndex();
      break;
    case OptCode::AlwaysTaskReturn:
      if (Code != CanonOp::Lift) {
        return RejectSite("always-task-return");
      }
      if (HasAlwaysTaskReturn) {
        return RejectDup("always-task-return");
      }
      HasAlwaysTaskReturn = true;
      break;
    default:
      spdlog::error(ErrCode::Value::UnknownCanonicalOption);
      spdlog::error("    unknown canonical option code 0x{:02x}"sv,
                    static_cast<unsigned>(Opt.getCode()));
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Canonical));
      return Unexpect(ErrCode::Value::UnknownCanonicalOption);
    }
  }

  // Structural rules.
  if (HasPostReturn && HasAsync) {
    spdlog::error(ErrCode::Value::InvalidCanonOption);
    spdlog::error(
        "    canonical options 'post-return' and 'async' are mutually exclusive"sv);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Canonical));
    return Unexpect(ErrCode::Value::InvalidCanonOption);
  }
  if (HasCallback && !HasAsync) {
    spdlog::error(ErrCode::Value::InvalidCanonOption);
    spdlog::error("    canonical option 'callback' requires 'async'"sv);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Canonical));
    return Unexpect(ErrCode::Value::InvalidCanonOption);
  }
  if (HasAlwaysTaskReturn && !HasAsync) {
    spdlog::error(ErrCode::Value::InvalidCanonOption);
    spdlog::error(
        "    canonical option 'always-task-return' requires 'async'"sv);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Canonical));
    return Unexpect(ErrCode::Value::InvalidCanonOption);
  }
  if (HasRealloc && !HasMemory) {
    spdlog::error(ErrCode::Value::InvalidCanonOption);
    spdlog::error(
        "    canonical option 'realloc' requires 'memory' to also be specified"sv);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Canonical));
    return Unexpect(ErrCode::Value::InvalidCanonOption);
  }

  // Index bounds checks. Core func signature body checks (realloc/callback)
  // are deferred until getCoreFunc signatures are populated.
  if (HasMemory &&
      MemoryIdx >= CompCtx.getCoreSortIndexSize(
                       AST::Component::Sort::CoreSortType::Memory)) {
    spdlog::error(ErrCode::Value::InvalidIndex);
    spdlog::error(
        "    canonical option 'memory': core memory index {} out of bounds"sv,
        MemoryIdx);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Canonical));
    return Unexpect(ErrCode::Value::InvalidIndex);
  }
  // The canonical ABI requires a 32-bit linear memory (GAP-CI-1).
  if (HasMemory) {
    const auto *Mem = CompCtx.getCoreMemory(MemoryIdx);
    if (Mem != nullptr && Mem->getLimit().is64()) {
      spdlog::error(ErrCode::Value::ComponentCanonMemoryNot32Bit);
      spdlog::error(
          "    canonical option 'memory': core memory {} is not a 32-bit "
          "linear memory"sv,
          MemoryIdx);
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Canonical));
      return Unexpect(ErrCode::Value::ComponentCanonMemoryNot32Bit);
    }
  }
  const uint32_t CoreFuncSpaceSize =
      CompCtx.getCoreSortIndexSize(AST::Component::Sort::CoreSortType::Func);
  if (HasRealloc && ReallocIdx >= CoreFuncSpaceSize) {
    spdlog::error(ErrCode::Value::InvalidIndex);
    spdlog::error(
        "    canonical option 'realloc': core func index {} out of bounds"sv,
        ReallocIdx);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Canonical));
    return Unexpect(ErrCode::Value::InvalidIndex);
  }
  if (HasCallback && CallbackIdx >= CoreFuncSpaceSize) {
    spdlog::error(ErrCode::Value::InvalidIndex);
    spdlog::error(
        "    canonical option 'callback': core func index {} out of bounds"sv,
        CallbackIdx);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Canonical));
    return Unexpect(ErrCode::Value::InvalidIndex);
  }
  if (HasPostReturn && PostReturnIdx >= CoreFuncSpaceSize) {
    spdlog::error(ErrCode::Value::InvalidIndex);
    spdlog::error(
        "    canonical option 'post-return': core func index {} out of bounds"sv,
        PostReturnIdx);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Canonical));
    return Unexpect(ErrCode::Value::InvalidIndex);
  }
  return {};
}

namespace {

// Subset of canon options that the flatten-derived checks below need to
// consult (spec L3290-3296 for lift, L3519-3524 for lower). Re-parsed from
// the option list rather than threaded out of validateCanonOptions to keep
// that function's contract narrow.
struct ParsedCanonOpts {
  bool HasMemory = false;
  bool HasRealloc = false;
  bool HasPostReturn = false;
  bool HasAsync = false;
  uint32_t PostReturnIdx = 0;
};

ParsedCanonOpts
parseCanonOpts(Span<const AST::Component::CanonOpt> Opts) noexcept {
  using OptCode = ComponentCanonOptCode;
  ParsedCanonOpts S;
  for (const auto &Opt : Opts) {
    switch (Opt.getCode()) {
    case OptCode::Memory:
      S.HasMemory = true;
      break;
    case OptCode::Realloc:
      S.HasRealloc = true;
      break;
    case OptCode::PostReturn:
      S.HasPostReturn = true;
      S.PostReturnIdx = Opt.getIndex();
      break;
    case OptCode::Async:
      S.HasAsync = true;
      break;
    default:
      break;
    }
  }
  return S;
}

// Build a CanonCtx whose TypeResolver routes through `Ctx.getDefType`, so
// the executor-side flatten / contains-list-string helpers can run without a
// ComponentInstance.
Executor::CanonicalABI::CanonCtx
makeValidatorCanonCtx(const ComponentContext &Ctx) noexcept {
  Executor::CanonicalABI::CanonCtx Cx{};
  Cx.TypeResolver = [&Ctx](uint32_t Idx) { return Ctx.getDefType(Idx); };
  return Cx;
}

// Spec L3290 / L3519: the core function reached by canon lift / produced by
// canon lower must have the signature `flatten_functype($opts, $ft, dir)`.
// Compare two FunctionTypes by TypeCode of their flat lists.
bool sameFlatSignature(const AST::FunctionType &A,
                       const Executor::CanonicalABI::FlatFuncType &B) noexcept {
  const auto &AP = A.getParamTypes();
  const auto &AR = A.getReturnTypes();
  if (AP.size() != B.Params.size() || AR.size() != B.Results.size()) {
    return false;
  }
  for (size_t I = 0; I < AP.size(); ++I) {
    if (AP[I].getCode() != B.Params[I].getCode()) {
      return false;
    }
  }
  for (size_t I = 0; I < AR.size(); ++I) {
    if (AR[I].getCode() != B.Results[I].getCode()) {
      return false;
    }
  }
  return true;
}

// Wrap a FlatFuncType into a SubType wrapping a CompositeType wrapping a
// FunctionType, suitable for tracking in ComponentContext::CoreFuncs.
// Mirrors HostFunction<T>::initializeFuncType (include/runtime/hostfunc.h)
// and the dynamic build in CanonLowerHostFunc (component_lower_thunk.cpp).
std::unique_ptr<AST::SubType>
flatSigToSubType(const Executor::CanonicalABI::FlatFuncType &F) {
  AST::FunctionType FT;
  FT.getParamTypes().assign(F.Params.begin(), F.Params.end());
  FT.getReturnTypes().assign(F.Results.begin(), F.Results.end());
  auto ST = std::make_unique<AST::SubType>(FT);
  return ST;
}

// Lift / lower share a few common option-requirement rules driven by the
// flat ABI of the component func type. CanonicalABI.md L3270-3277, L3290-3296,
// L3519-3524. `IsLift` decides which side of the spec table to consult.
Expect<void> checkCanonFlatRules(const ComponentContext &CompCtx,
                                 const AST::Component::FuncType &FT,
                                 bool IsLift,
                                 const ParsedCanonOpts &O) noexcept {
  Executor::CanonicalABI::CanonCtx Cx = makeValidatorCanonCtx(CompCtx);

  const char *Site = IsLift ? "canon lift" : "canon lower";
  const bool RequireReallocForList = [&]() {
    // lift(T) requires realloc if T contains list/string (params);
    // lower(T) requires realloc if T contains list/string (result).
    if (IsLift) {
      for (const auto &P : FT.getParamList()) {
        if (Executor::CanonicalABI::containsListOrString(Cx, P.getValType())) {
          return true;
        }
      }
    } else {
      for (const auto &R : FT.getResultList()) {
        if (Executor::CanonicalABI::containsListOrString(Cx, R.getValType())) {
          return true;
        }
      }
    }
    return false;
  }();
  const bool RequireMemoryForList = [&]() {
    if (IsLift) {
      for (const auto &R : FT.getResultList()) {
        if (Executor::CanonicalABI::containsListOrString(Cx, R.getValType())) {
          return true;
        }
      }
    } else {
      for (const auto &P : FT.getParamList()) {
        if (Executor::CanonicalABI::containsListOrString(Cx, P.getValType())) {
          return true;
        }
      }
    }
    return false;
  }();

  // Pre-flatten counts (before the over-cap collapse) drive the spec's
  // threshold rules. Total over the spans of each list-component flatten.
  size_t PreFlatParams = 0;
  for (const auto &P : FT.getParamList()) {
    EXPECTED_TRY(auto Sub,
                 Executor::CanonicalABI::flattenType(Cx, P.getValType()));
    PreFlatParams += Sub.size();
  }
  size_t PreFlatResults = 0;
  for (const auto &R : FT.getResultList()) {
    EXPECTED_TRY(auto Sub,
                 Executor::CanonicalABI::flattenType(Cx, R.getValType()));
    PreFlatResults += Sub.size();
  }

  // Spec L3293-3294 (lift) / L3520-3521 (lower).
  if (RequireReallocForList && !O.HasRealloc) {
    spdlog::error(ErrCode::Value::InvalidCanonOption);
    spdlog::error(
        "    {}: 'realloc' is required because {} contains a list / string"sv,
        Site, IsLift ? "param" : "result");
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Canonical));
    return Unexpect(ErrCode::Value::InvalidCanonOption);
  }
  if (RequireMemoryForList && !O.HasMemory) {
    spdlog::error(ErrCode::Value::InvalidCanonOption);
    spdlog::error(
        "    {}: 'memory' is required because {} contains a list / string"sv,
        Site, IsLift ? "result" : "param");
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Canonical));
    return Unexpect(ErrCode::Value::InvalidCanonOption);
  }

  // Threshold rules (L3295-3296 for lift, L3522-3523 for lower). Sync only —
  // async would set different caps but is already rejected by flattenFuncType.
  // For lift: params need realloc to spill, results need memory. For lower
  // the two swap.
  const char *ParamsSpillOpt = IsLift ? "realloc" : "memory";
  const bool ParamsSpillOptPresent = IsLift ? O.HasRealloc : O.HasMemory;
  const char *ResultsSpillOpt = IsLift ? "memory" : "realloc";
  const bool ResultsSpillOptPresent = IsLift ? O.HasMemory : O.HasRealloc;
  if (PreFlatParams > Executor::CanonicalABI::MaxFlatParams &&
      !ParamsSpillOptPresent) {
    spdlog::error(ErrCode::Value::InvalidCanonOption);
    spdlog::error("    {}: param flat count {} exceeds MAX_FLAT_PARAMS, "
                  "'{}' is required"sv,
                  Site, PreFlatParams, ParamsSpillOpt);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Canonical));
    return Unexpect(ErrCode::Value::InvalidCanonOption);
  }
  if (PreFlatResults > Executor::CanonicalABI::MaxFlatResults &&
      !ResultsSpillOptPresent) {
    spdlog::error(ErrCode::Value::InvalidCanonOption);
    spdlog::error("    {}: result flat count {} exceeds MAX_FLAT_RESULTS, "
                  "'{}' is required"sv,
                  Site, PreFlatResults, ResultsSpillOpt);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Canonical));
    return Unexpect(ErrCode::Value::InvalidCanonOption);
  }
  return {};
}

} // namespace

Expect<void>
Validator::validateCanonLift(const AST::Component::Canonical &Canon) noexcept {
  const uint32_t CoreFuncIdx = Canon.getIndex();
  const uint32_t CoreFuncSpaceSize =
      CompCtx.getCoreSortIndexSize(AST::Component::Sort::CoreSortType::Func);
  // 1. Core func index bounds.
  if (CoreFuncIdx >= CoreFuncSpaceSize) {
    spdlog::error(ErrCode::Value::InvalidIndex);
    spdlog::error(
        "    canon lift: core func index {} exceeds core func index space size {}"sv,
        CoreFuncIdx, CoreFuncSpaceSize);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Canonical));
    return Unexpect(ErrCode::Value::InvalidIndex);
  }
  const uint32_t TypeIdx = Canon.getTargetIndex();
  const uint32_t TypeSpaceSize =
      CompCtx.getSortIndexSize(AST::Component::Sort::SortType::Type);
  // 2. Target type index bounds.
  if (TypeIdx >= TypeSpaceSize) {
    spdlog::error(ErrCode::Value::InvalidIndex);
    spdlog::error(
        "    canon lift: type index {} exceeds type index space size {}"sv,
        TypeIdx, TypeSpaceSize);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Canonical));
    return Unexpect(ErrCode::Value::InvalidIndex);
  }
  // 3. Target type must be a component FuncType.
  const auto *DT = CompCtx.getDefType(TypeIdx);
  if (DT == nullptr) {
    // Unresolved slot: the index was registered by an import or outer alias
    // but the concrete definition has not been filled in yet.
    spdlog::error(ErrCode::Value::InvalidTypeReference);
    spdlog::error(
        "    canon lift: target type index {} is an unresolved type slot"sv,
        TypeIdx);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Canonical));
    return Unexpect(ErrCode::Value::InvalidTypeReference);
  }
  if (!DT->isFuncType()) {
    spdlog::error(ErrCode::Value::InvalidTypeReference);
    spdlog::error(
        "    canon lift: target type index {} does not reference a component func type"sv,
        TypeIdx);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Canonical));
    return Unexpect(ErrCode::Value::InvalidTypeReference);
  }
  // 4. Validate canonical options (Lift site allows all).
  EXPECTED_TRY(validateCanonOptions(Canon.getOpCode(), Canon.getOptions()));

  // 5. Flatten-derived checks (spec L3290-3296). Catches async / gated value
  // types and enforces realloc/memory presence rules at validator time so
  // instantiation no longer has to.
  const auto Opts = parseCanonOpts(Canon.getOptions());
  auto Cx = makeValidatorCanonCtx(CompCtx);
  EXPECTED_TRY(auto FlatSig,
               Executor::CanonicalABI::flattenFuncType(Cx, DT->getFuncType(),
                                                       /*IsLift=*/true));
  EXPECTED_TRY(checkCanonFlatRules(CompCtx, DT->getFuncType(),
                                   /*IsLift=*/true, Opts));

  // 6. Spec L3292: post-return signature must be
  // `(func (param flatten_functype({}, $ft, 'lift').results))`. Only checked
  // when the core func type was threaded through (e.g., CoreImportDesc).
  if (Opts.HasPostReturn) {
    if (Opts.PostReturnIdx >= CompCtx.getCoreSortIndexSize(
                                  AST::Component::Sort::CoreSortType::Func)) {
      // Already caught by validateCanonOptions, but re-guarded for safety.
      return {};
    }
    if (const auto *PRType = CompCtx.getCoreFunc(Opts.PostReturnIdx);
        PRType != nullptr && PRType->getCompositeType().isFunc()) {
      const auto &PRFunc = PRType->getCompositeType().getFuncType();
      if (!PRFunc.getReturnTypes().empty() ||
          PRFunc.getParamTypes().size() != FlatSig.Results.size()) {
        spdlog::error(ErrCode::Value::InvalidCanonOption);
        spdlog::error("    canon lift: post-return must have signature "
                      "(func (param ...flatten_lift_results))"sv);
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Canonical));
        return Unexpect(ErrCode::Value::InvalidCanonOption);
      }
      for (size_t I = 0; I < FlatSig.Results.size(); ++I) {
        if (PRFunc.getParamTypes()[I].getCode() !=
            FlatSig.Results[I].getCode()) {
          spdlog::error(ErrCode::Value::InvalidCanonOption);
          spdlog::error("    canon lift: post-return param[{}] type mismatch"sv,
                        I);
          spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Canonical));
          return Unexpect(ErrCode::Value::InvalidCanonOption);
        }
      }
    }
    // If PRType is nullptr the core func came from a path that doesn't yet
    // track core SubTypes (aliases, canon-synthesized cores). The
    // instantiate-time pre-flight will still catch a signature mismatch.
  }

  // 7. Spec L3290: `$callee` must have type
  // `flatten_functype($opts, $ft, 'lift')`. Same caveat as above on SubType
  // availability.
  if (const auto *CalleeSub = CompCtx.getCoreFunc(CoreFuncIdx);
      CalleeSub != nullptr && CalleeSub->getCompositeType().isFunc()) {
    if (!sameFlatSignature(CalleeSub->getCompositeType().getFuncType(),
                           FlatSig)) {
      spdlog::error(ErrCode::Value::InvalidCanonOption);
      spdlog::error("    canon lift: $callee signature does not match "
                    "flatten_functype($opts, $ft, 'lift')"sv);
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Canonical));
      return Unexpect(ErrCode::Value::InvalidCanonOption);
    }
  }

  // 8. Allocate component func, binding the resolved FuncType.
  CompCtx.addFunc(&DT->getFuncType());
  return {};
}

Expect<void>
Validator::validateCanonLower(const AST::Component::Canonical &Canon) noexcept {
  const uint32_t FuncIdx = Canon.getIndex();
  const uint32_t FuncSpaceSize =
      CompCtx.getSortIndexSize(AST::Component::Sort::SortType::Func);
  // 1. Component func index bounds.
  if (FuncIdx >= FuncSpaceSize) {
    spdlog::error(ErrCode::Value::InvalidIndex);
    spdlog::error(
        "    canon lower: component func index {} exceeds func index space size {}"sv,
        FuncIdx, FuncSpaceSize);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Canonical));
    return Unexpect(ErrCode::Value::InvalidIndex);
  }
  // 2. Validate canonical options (per-site rules for Lower).
  EXPECTED_TRY(validateCanonOptions(Canon.getOpCode(), Canon.getOptions()));

  // 3. Flatten-derived checks (spec L3519-3524). Catches async / gated value
  // types and enforces realloc/memory presence rules early.
  const auto *Callee = CompCtx.getFunc(FuncIdx);
  if (Callee == nullptr) {
    // No FuncType bound to this slot (typically an unresolved import). The
    // signature synthesis below cannot proceed; defer to instantiate-time.
    CompCtx.addCoreFunc();
    return {};
  }
  const auto Opts = parseCanonOpts(Canon.getOptions());
  auto Cx = makeValidatorCanonCtx(CompCtx);
  EXPECTED_TRY(auto FlatSig,
               Executor::CanonicalABI::flattenFuncType(Cx, *Callee,
                                                       /*IsLift=*/false));
  EXPECTED_TRY(checkCanonFlatRules(CompCtx, *Callee, /*IsLift=*/false, Opts));

  // 4. Allocate the resulting core func, binding the synthesized flat ABI
  // (spec L3519) so downstream callers see the correct signature.
  CompCtx.addCoreFuncOwned(flatSigToSubType(FlatSig));
  return {};
}

Expect<void> Validator::validateCanonResourceNew(
    const AST::Component::Canonical &Canon) noexcept {
  const uint32_t Idx = Canon.getIndex();
  const uint32_t TypeSpaceSize =
      CompCtx.getSortIndexSize(AST::Component::Sort::SortType::Type);
  // 1. Type index bounds.
  if (Idx >= TypeSpaceSize) {
    spdlog::error(ErrCode::Value::InvalidIndex);
    spdlog::error(
        "    canon resource.new: type index {} exceeds type index space size {}"sv,
        Idx, TypeSpaceSize);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Canonical));
    return Unexpect(ErrCode::Value::InvalidIndex);
  }
  // 2. Type must be a locally-defined resource.
  const auto *RInfo = CompCtx.getResource(Idx);
  if (RInfo == nullptr) {
    spdlog::error(ErrCode::Value::InvalidTypeReference);
    spdlog::error(
        "    canon resource.new: type index {} does not reference a resource"sv,
        Idx);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Canonical));
    return Unexpect(ErrCode::Value::InvalidTypeReference);
  }
  if (!RInfo->LocallyDefined) {
    spdlog::error(ErrCode::Value::InvalidTypeReference);
    spdlog::error(
        "    canon resource.new: type index {} is not locally defined (imported or outer-aliased resources are not allowed)"sv,
        Idx);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Canonical));
    return Unexpect(ErrCode::Value::InvalidTypeReference);
  }
  // 4. Validate canonical options.
  EXPECTED_TRY(validateCanonOptions(Canon.getOpCode(), Canon.getOptions()));
  // 5. Allocate the resulting core func with synthesized signature
  // [i32] -> [i32] (rep i32 in, new handle out).
  CompCtx.addCoreFunc(&CoreFuncType_I32_I32);
  return {};
}

Expect<void> Validator::validateCanonResourceRep(
    const AST::Component::Canonical &Canon) noexcept {
  const uint32_t Idx = Canon.getIndex();
  const uint32_t TypeSpaceSize =
      CompCtx.getSortIndexSize(AST::Component::Sort::SortType::Type);
  // 1. Type index bounds.
  if (Idx >= TypeSpaceSize) {
    spdlog::error(ErrCode::Value::InvalidIndex);
    spdlog::error(
        "    canon resource.rep: type index {} exceeds type index space size {}"sv,
        Idx, TypeSpaceSize);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Canonical));
    return Unexpect(ErrCode::Value::InvalidIndex);
  }
  // 2. Type must be a locally-defined resource.
  const auto *RInfo = CompCtx.getResource(Idx);
  if (RInfo == nullptr) {
    spdlog::error(ErrCode::Value::InvalidTypeReference);
    spdlog::error(
        "    canon resource.rep: type index {} does not reference a resource"sv,
        Idx);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Canonical));
    return Unexpect(ErrCode::Value::InvalidTypeReference);
  }
  if (!RInfo->LocallyDefined) {
    spdlog::error(ErrCode::Value::InvalidTypeReference);
    spdlog::error(
        "    canon resource.rep: type index {} is not locally defined (imported or outer-aliased resources are not allowed)"sv,
        Idx);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Canonical));
    return Unexpect(ErrCode::Value::InvalidTypeReference);
  }
  // 4. Validate canonical options.
  EXPECTED_TRY(validateCanonOptions(Canon.getOpCode(), Canon.getOptions()));
  // 5. Allocate the resulting core func with synthesized signature
  // [i32] -> [i32] (handle in, rep out).
  CompCtx.addCoreFunc(&CoreFuncType_I32_I32);
  return {};
}

Expect<void> Validator::validateCanonResourceDrop(
    const AST::Component::Canonical &Canon) noexcept {
  const uint32_t Idx = Canon.getIndex();
  const uint32_t TypeSpaceSize =
      CompCtx.getSortIndexSize(AST::Component::Sort::SortType::Type);
  // 1. Type index bounds.
  if (Idx >= TypeSpaceSize) {
    spdlog::error(ErrCode::Value::InvalidIndex);
    spdlog::error(
        "    canon resource.drop: type index {} exceeds type index space size {}"sv,
        Idx, TypeSpaceSize);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Canonical));
    return Unexpect(ErrCode::Value::InvalidIndex);
  }
  // 2. Type must be a resource type.
  if (CompCtx.getResource(Idx) == nullptr) {
    spdlog::error(ErrCode::Value::InvalidTypeReference);
    spdlog::error(
        "    canon resource.drop: type index {} does not reference a resource"sv,
        Idx);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Canonical));
    return Unexpect(ErrCode::Value::InvalidTypeReference);
  }
  // 3. resource.drop accepts both local and imported resources — no locality
  // check.
  // 4. Validate canonical options.
  EXPECTED_TRY(validateCanonOptions(Canon.getOpCode(), Canon.getOptions()));
  // 5. Allocate the resulting core func with synthesized signature
  // [i32] -> [] (handle in, no result — matches the resource destructor
  // shape required by validate(ResourceType)).
  CompCtx.addCoreFunc(&CoreFuncType_I32_Void);
  return {};
}

Expect<void> Validator::validate(const AST::Component::Import &Im) noexcept {
  // Validation steps:
  //   1. Validate the externdesc and introduce the imported entity into
  //      its sort's index space.
  //   2. Parse the import name per the structured import-name grammar.
  //   3. Enforce the annotated-name constraints ([constructor]/[method]/
  //      [static] only on func imports).
  //   4. Reject duplicate import names (strongly-unique across imports).
  //
  // The per-annotated-name structural checks (constructor result type,
  // method `self` param, static resource name in scope) are not yet
  // implemented and tracked as follow-ups below.
  // Snapshot the type space size before validating the desc so we can
  // recover the new type index allocated by a TypeBound (sub resource).
  const uint32_t TypeSpaceBefore =
      CompCtx.getSortIndexSize(AST::Component::Sort::SortType::Type);

  EXPECTED_TRY(validate(Im.getDesc()).map_error([](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Import));
    return E;
  }));

  EXPECTED_TRY(ComponentName CName,
               ComponentName::parse(Im.getName()).map_error([](auto E) {
                 spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Import));
                 return E;
               }));

  // Annotated plainnames ([constructor], [method], [static]) can only appear
  // on func imports.
  switch (CName.getKind()) {
  case ComponentNameKind::Constructor:
  case ComponentNameKind::Method:
  case ComponentNameKind::Static:
    if (Im.getDesc().getDescType() !=
        AST::Component::ExternDesc::DescType::FuncType) {
      spdlog::error(ErrCode::Value::ComponentInvalidName);
      spdlog::error("    Import: annotated name requires func type"sv);
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Import));
      return Unexpect(ErrCode::Value::ComponentInvalidName);
    }
    break;
  default:
    break;
  }

  // Resolve the resource name referenced by an annotated name. The resource
  // must have been previously introduced by a TypeBound import or export
  // with a kebab-case label in this scope.
  // TODO: extend to the full structural checks once func-type bodies are
  // walked here:
  //   - [constructor]R: result type must be (own $R).
  //   - [method]R.f:    first param must be (borrow $R).
  //   - [static]R.f:    no `self` param of (borrow $R).
  std::string_view ResourceLabel;
  switch (CName.getKind()) {
  case ComponentNameKind::Constructor:
    ResourceLabel = CName.getDetail().get<ConstructorDetail>().Label;
    break;
  case ComponentNameKind::Method:
    ResourceLabel = CName.getDetail().get<MethodDetail>().Resource;
    break;
  case ComponentNameKind::Static:
    ResourceLabel = CName.getDetail().get<StaticDetail>().Resource;
    break;
  default:
    break;
  }
  if (!ResourceLabel.empty() && !CompCtx.hasResourceLabel(ResourceLabel)) {
    spdlog::error(ErrCode::Value::ComponentInvalidName);
    spdlog::error(
        "    Import: annotated name references unknown resource '{}'"sv,
        ResourceLabel);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Import));
    return Unexpect(ErrCode::Value::ComponentInvalidName);
  }

  if (!CompCtx.addImportedName(CName)) {
    spdlog::error(ErrCode::Value::ComponentImportNameConflict);
    spdlog::error("    Import: Duplicate import name"sv);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Import));
    return Unexpect(ErrCode::Value::ComponentImportNameConflict);
  }

  // If this import introduced a TypeBound resource with a label name,
  // register the label so subsequent annotated names can reference it.
  if (Im.getDesc().getDescType() ==
          AST::Component::ExternDesc::DescType::TypeBound &&
      CName.getKind() == ComponentNameKind::Label) {
    CompCtx.addResourceLabel(Im.getName(), TypeSpaceBefore);
  }

  return {};
}

Expect<void> Validator::validate(const AST::Component::Export &Ex) noexcept {
  // Validation steps:
  //   1. `sortidx` is in-bounds and (for core sorts) must be `core module`.
  //   2. If an `externdesc` ascription is present, it must be a supertype
  //      of the inferred externdesc of the `sortidx` (kind-only check for
  //      now; structural subtype is a follow-up).
  //   3. The export name parses under the export-name grammar.
  //   4. The export name is strongly-unique across exports.
  //   5. The export introduces a new index aliasing the definition in the
  //      component's own index space for its sort.
  //   6. If the sort is `value`, the value is consumed (value linearity).
  //
  // Not yet enforced: transitive resource-avoidance in exported types.

  // Validate the sortidx bounds.
  const auto &Sort = Ex.getSortIndex().getSort();
  uint32_t Idx = Ex.getSortIndex().getIdx();
  if (Sort.isCore()) {
    // The externdesc grammar permits only `core module` as a component-level
    // core export — no other core sort is exportable from a component.
    if (Sort.getCoreSortType() != AST::Component::Sort::CoreSortType::Module) {
      spdlog::error(ErrCode::Value::InvalidTypeReference);
      spdlog::error(
          "    Export: core sort other than `core module` is not allowed"sv);
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Export));
      return Unexpect(ErrCode::Value::InvalidTypeReference);
    }
    if (Idx >= CompCtx.getCoreSortIndexSize(Sort.getCoreSortType())) {
      spdlog::error(ErrCode::Value::DefTypeIndexOutOfBounds);
      spdlog::error("    Export: sort index {} out of bounds"sv, Idx);
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Export));
      return Unexpect(ErrCode::Value::DefTypeIndexOutOfBounds);
    }
  } else {
    if (Idx >= CompCtx.getSortIndexSize(Sort.getSortType())) {
      spdlog::error(ErrCode::Value::DefTypeIndexOutOfBounds);
      spdlog::error("    Export: sort index {} out of bounds"sv, Idx);
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Export));
      return Unexpect(ErrCode::Value::DefTypeIndexOutOfBounds);
    }
    if (Sort.getSortType() == AST::Component::Sort::SortType::Value) {
      if (auto Res = CompCtx.consumeValue(Idx); !Res) {
        spdlog::error(Res.error());
        spdlog::error("    Export: value index {} has already been consumed"sv,
                      Idx);
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Export));
        return Unexpect(Res);
      }
    }
  }

  // If an externdesc ascription is present, the spec requires it to be a
  // supertype of the inferred externdesc of the sortidx. For now we only
  // enforce that the ascription's kind matches the sortidx's sort; full
  // structural subtype between ascription and inferred type is not yet
  // implemented.
  if (Ex.getDesc().has_value() &&
      !sortMatchesDescType(Sort, Ex.getDesc()->getDescType())) {
    spdlog::error(ErrCode::Value::ExportAscriptionIncompatible);
    spdlog::error(
        "    Export: ascribed externdesc kind does not match sortidx sort"sv);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Export));
    return Unexpect(ErrCode::Value::ExportAscriptionIncompatible);
  }

  // Validate name grammar, then enforce strong-uniqueness across exports.
  EXPECTED_TRY(ComponentName CName,
               validateExportName(Ex.getName()).map_error([](auto E) {
                 spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Export));
                 return E;
               }));
  if (!CompCtx.addExportedName(CName)) {
    spdlog::error(ErrCode::Value::ComponentDuplicateName);
    spdlog::error("    Export: Duplicate export name '{}'"sv, Ex.getName());
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Export));
    return Unexpect(ErrCode::Value::ComponentDuplicateName);
  }

  // Every export introduces a new index that aliases the exported
  // definition in the component's own index space for that sort. For
  // instance-sort exports we also copy/build the export table so that a
  // later alias-export against this slot can resolve its sub-exports.
  if (Sort.isCore()) {
    CompCtx.incCoreSortIndexSize(Sort.getCoreSortType());
  } else {
    const AST::Component::InstanceType *IT = nullptr;
    const bool IsInst =
        Sort.getSortType() == AST::Component::Sort::SortType::Instance;
    const bool HasInstAscription =
        IsInst && Ex.getDesc().has_value() &&
        Ex.getDesc()->getDescType() ==
            AST::Component::ExternDesc::DescType::InstanceType;
    if (HasInstAscription) {
      IT = CompCtx.getInstanceType(Ex.getDesc()->getTypeIndex());
      if (IT != nullptr) {
        if (auto Missing = findMissingRequiredExport(Idx, *IT)) {
          spdlog::error(ErrCode::Value::ExportAscriptionIncompatible);
          spdlog::error(
              "    Export: ascribed instance type requires export '{}' "
              "not present in inferred type"sv,
              *Missing);
          spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Export));
          return Unexpect(ErrCode::Value::ExportAscriptionIncompatible);
        }
      }
    }

    uint32_t NewIdx =
        Sort.getSortType() == AST::Component::Sort::SortType::Value
            ? CompCtx.addValue(/*Consumed=*/true)
            : CompCtx.incSortIndexSize(Sort.getSortType());
    if (IsInst) {
      if (IT != nullptr) {
        populateInstanceFromType(NewIdx, *IT);
      } else {
        // Either no ascription, or the ascription's type index didn't
        // resolve to an inline InstanceType here (cross-scope type-index
        // resolution is not yet implemented). Copy the source instance's
        // inferred exports so a later alias-export on this slot can still
        // find them.
        const auto &SrcExports = CompCtx.getInstance(Idx).Exports;
        for (const auto &[Name, IE] : SrcExports) {
          CompCtx.addInstanceExport(NewIdx, Name, IE.ST, IE.IT,
                                    IE.NestedInstIdx);
        }
      }
    }
  }
  return {};
}

Expect<void>
Validator::validate(const AST::Component::ExternDesc &Desc) noexcept {
  // Validating an externdesc introduces a new entry into the index space
  // matching the descriptor's sort:
  //   * core module (CoreType) → core:module
  //   * func                   → func
  //   * value                  → value (consumed=false)
  //   * type (eq i)            → type aliased to i (resource property
  //                              propagated when i refers to a resource)
  //   * type (sub resource)    → fresh abstract resource type
  //   * component / instance   → component / instance
  //
  // GAP-ED-1: bounds-check the referenced type index. The CoreType
  // externdesc indexes the core:type space (the moduletype lives there);
  // FuncType / ComponentType / InstanceType all index the component-level
  // type space. Kind-of-type checks (e.g. that a FuncType index actually
  // resolves to a component func type, not a record) are a follow-up that
  // needs the type body to be retained on every type entry.
  switch (Desc.getDescType()) {
  case AST::Component::ExternDesc::DescType::CoreType: {
    const uint32_t RefIdx = Desc.getTypeIndex();
    const uint32_t CoreTypeSize =
        CompCtx.getCoreSortIndexSize(AST::Component::Sort::CoreSortType::Type);
    if (RefIdx >= CoreTypeSize) {
      spdlog::error(ErrCode::Value::InvalidIndex);
      spdlog::error(
          "    ExternDesc: core type index {} exceeds core:type index space size {}"sv,
          RefIdx, CoreTypeSize);
      return Unexpect(ErrCode::Value::InvalidIndex);
    }
    break;
  }
  case AST::Component::ExternDesc::DescType::FuncType:
  case AST::Component::ExternDesc::DescType::ComponentType:
  case AST::Component::ExternDesc::DescType::InstanceType: {
    const uint32_t RefIdx = Desc.getTypeIndex();
    const uint32_t TypeSize =
        CompCtx.getSortIndexSize(AST::Component::Sort::SortType::Type);
    if (RefIdx >= TypeSize) {
      spdlog::error(ErrCode::Value::InvalidIndex);
      spdlog::error(
          "    ExternDesc: referenced type index {} exceeds type index space size {}"sv,
          RefIdx, TypeSize);
      return Unexpect(ErrCode::Value::InvalidIndex);
    }
    break;
  }
  default:
    break;
  }

  switch (Desc.getDescType()) {
  case AST::Component::ExternDesc::DescType::CoreType: {
    // The CoreType externdesc is `(core module (type i))`, so it introduces
    // a new core:module slot bound to the moduletype at core:type idx i.
    const auto *CT = CompCtx.getCoreModuleType(Desc.getTypeIndex());
    CompCtx.addCoreModule(CT);
    break;
  }
  case AST::Component::ExternDesc::DescType::FuncType: {
    // Bind the imported func's declared component FuncType so that
    // downstream checks (e.g. canon lower's flatten-derived rules) can
    // introspect it.
    const auto *DT = CompCtx.getDefType(Desc.getTypeIndex());
    const AST::Component::FuncType *FT =
        (DT != nullptr && DT->isFuncType()) ? &DT->getFuncType() : nullptr;
    CompCtx.addFunc(FT);
    break;
  }
  case AST::Component::ExternDesc::DescType::ValueBound:
    CompCtx.addValue();
    break;
  case AST::Component::ExternDesc::DescType::TypeBound:
    if (Desc.isEqType()) {
      // (type (eq i)) — alias type i
      uint32_t RefIdx = Desc.getTypeIndex();
      if (RefIdx >=
          CompCtx.getSortIndexSize(AST::Component::Sort::SortType::Type)) {
        spdlog::error(ErrCode::Value::InvalidIndex);
        spdlog::error("    ExternDesc: eq type bound index {} out of bounds"sv,
                      RefIdx);
        return Unexpect(ErrCode::Value::InvalidIndex);
      }
      // (eq i): inherits the source resource's id; body lives on the
      // shared registry entry.
      uint32_t NewIdx = CompCtx.addType(nullptr, /*IsLocal=*/false);
      if (const auto *SrcInfo = CompCtx.getResource(RefIdx)) {
        CompCtx.addResource(NewIdx, {SrcInfo->Id, /*LocallyDefined=*/false});
      }
    } else {
      // (sub resource): abstract import — fresh id with no body.
      uint32_t NewIdx = CompCtx.addType(nullptr, /*IsLocal=*/false);
      CompCtx.addResource(NewIdx, {CompCtx.allocateFreshResourceId(),
                                   /*LocallyDefined=*/false});
    }
    break;
  case AST::Component::ExternDesc::DescType::ComponentType: {
    // Bind the new component slot to its ComponentType so a later
    // instantiation can pull imports/exports from it (GAP-I-1).
    const auto *CT = CompCtx.getComponentType(Desc.getTypeIndex());
    CompCtx.addComponent(CT);
    break;
  }
  case AST::Component::ExternDesc::DescType::InstanceType: {
    // GAP-ED-2: populate exports from the referenced InstanceType so
    // alias-export resolves. GAP-I-5b: also bind it to the slot.
    const auto *IT = CompCtx.getInstanceType(Desc.getTypeIndex());
    uint32_t InstIdx = CompCtx.addInstance(IT);
    if (IT != nullptr) {
      populateInstanceFromType(InstIdx, *IT);
    }
    // If the type index resolved against an outer scope's InstanceType,
    // populating from it requires cross-scope walking (the rest of
    // GAP-DECL-ED). For now the instance keeps an empty export table when
    // the InstanceType body isn't visible in this scope.
    break;
  }
  default:
    assumingUnreachable();
  }

  return {};
}

Expect<void>
Validator::validate(const AST::Component::CoreImportDesc &Desc) noexcept {
  if (Desc.isFunc()) {
    uint32_t TypeIdx = Desc.getTypeIndex();
    if (TypeIdx >= CompCtx.getCoreSortIndexSize(
                       AST::Component::Sort::CoreSortType::Type)) {
      spdlog::error(ErrCode::Value::CoreTypeIndexOutOfBounds);
      spdlog::error("    CoreImportDesc: func type index {} out of bounds"sv,
                    TypeIdx);
      return Unexpect(ErrCode::Value::CoreTypeIndexOutOfBounds);
    }
    // Bind the imported func's declared core type so later canon-lift's
    // post-return signature check (spec L3292) can compare against it.
    CompCtx.addCoreFunc(CompCtx.getCoreType(TypeIdx));
  } else if (Desc.isTable()) {
    CompCtx.addCoreTable();
  } else if (Desc.isMemory()) {
    CompCtx.addCoreMemory();
  } else if (Desc.isGlobal()) {
    CompCtx.addCoreGlobal();
  } else if (Desc.isTag()) {
    CompCtx.addCoreTag();
  } else {
    assumingUnreachable();
  }
  return {};
}

Expect<void>
Validator::validate(const AST::Component::CoreImportDecl &Decl) noexcept {
  return validate(Decl.getImportDesc());
}

Expect<void>
Validator::validate(const AST::Component::CoreExportDecl &Decl) noexcept {
  return validate(Decl.getImportDesc());
}

Expect<void>
Validator::validate(const AST::Component::CoreModuleDecl &Decl) noexcept {
  auto ReportError = [](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Decl_CoreModule));
    return E;
  };

  if (Decl.isImport()) {
    EXPECTED_TRY(validate(Decl.getImport()).map_error(ReportError));
  } else if (Decl.isType()) {
    EXPECTED_TRY(validate(*Decl.getType()).map_error(ReportError));
  } else if (Decl.isAlias()) {
    EXPECTED_TRY(validate(Decl.getAlias()).map_error(ReportError));
  } else if (Decl.isExport()) {
    EXPECTED_TRY(validate(Decl.getExport()).map_error(ReportError));
  } else {
    assumingUnreachable();
  }
  return {};
}

Expect<void>
Validator::validate(const AST::Component::ImportDecl &Decl) noexcept {
  auto ReportError = [](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Decl_Import));
    return E;
  };

  // Validate the extern descriptor (also increments sort index spaces).
  EXPECTED_TRY(validate(Decl.getExternDesc()).map_error(ReportError));

  // Parse and validate the import name.
  EXPECTED_TRY(ComponentName CName,
               ComponentName::parse(Decl.getName()).map_error(ReportError));

  // Annotated plainnames can only appear on func imports.
  switch (CName.getKind()) {
  case ComponentNameKind::Constructor:
  case ComponentNameKind::Method:
  case ComponentNameKind::Static:
    if (Decl.getExternDesc().getDescType() !=
        AST::Component::ExternDesc::DescType::FuncType) {
      spdlog::error(ErrCode::Value::ComponentInvalidName);
      spdlog::error("    ImportDecl: annotated name requires func type"sv);
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Decl_Import));
      return Unexpect(ErrCode::Value::ComponentInvalidName);
    }
    break;
  default:
    break;
  }

  // Check import name uniqueness.
  if (!CompCtx.addImportedName(CName)) {
    spdlog::error(ErrCode::Value::ComponentImportNameConflict);
    spdlog::error("    ImportDecl: Duplicate import name"sv);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Decl_Import));
    return Unexpect(ErrCode::Value::ComponentImportNameConflict);
  }

  return {};
}

Expect<void>
Validator::validate(const AST::Component::ExportDecl &Decl) noexcept {
  // Check export name grammar and uniqueness before mutating the index
  // spaces so a duplicate or malformed name doesn't widen the scope's
  // sort counts.
  EXPECTED_TRY(ComponentName CName,
               validateExportName(Decl.getName()).map_error([](auto E) {
                 spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Decl_Export));
                 return E;
               }));
  if (!CompCtx.addExportedName(CName)) {
    spdlog::error(ErrCode::Value::ComponentDuplicateName);
    spdlog::error("    ExportDecl: Duplicate export name '{}'"sv,
                  Decl.getName());
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Decl_Export));
    return Unexpect(ErrCode::Value::ComponentDuplicateName);
  }

  // Validate the extern descriptor (also increments sort index spaces and
  // performs type-index bounds checking). For nested ExportDecls whose
  // ExternDesc references a type index defined in an outer scope, the
  // current scope's lookup may return nullptr — validate(ExternDesc) handles
  // that gracefully by allocating an empty instance slot. Cross-scope
  // type-index resolution (walking the parent CompCtxs chain to resolve
  // names like `(export "f" (func (type 0)))` where type 0 lives outside
  // the InstanceType body) is the structural follow-up tracked by the rest
  // of GAP-DECL-ED / GAP-ED-1 / GAP-ED-2.
  EXPECTED_TRY(validate(Decl.getExternDesc()).map_error([](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Decl_Export));
    return E;
  }));

  return {};
}

Expect<void>
Validator::validate(const AST::Component::InstanceDecl &Decl) noexcept {
  auto ReportError = [](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Decl_Instance));
    return E;
  };

  if (Decl.isCoreType()) {
    EXPECTED_TRY(validate(*Decl.getCoreType()).map_error(ReportError));
  } else if (Decl.isType()) {
    EXPECTED_TRY(validate(*Decl.getType()).map_error(ReportError));
  } else if (Decl.isAlias()) {
    EXPECTED_TRY(validate(Decl.getAlias()).map_error(ReportError));
    const auto &A = Decl.getAlias();
    const auto &Sort = A.getSort();
    if (Sort.isCore()) {
      CompCtx.incCoreSortIndexSize(Sort.getCoreSortType());
    } else {
      uint32_t NewInstIdx = CompCtx.incSortIndexSize(Sort.getSortType());
      // Outer-aliasing a resource type keeps the resource's identity in this
      // scope so later own/borrow and (eq i) checks treat the slot correctly.
      if (A.getTargetType() == AST::Component::Alias::TargetType::Outer &&
          Sort.getSortType() == AST::Component::Sort::SortType::Type) {
        CompCtx.carryOuterResource(NewInstIdx, A.getOuter().first,
                                   A.getOuter().second);
      }
      // When aliasing an instance export, propagate either the source
      // instance's export table (for Instance targets) or the source
      // export's resource identity (for Type targets that are resources)
      // — mirrors the AliasSection handling.
      if (A.getTargetType() == AST::Component::Alias::TargetType::Export) {
        const auto SrcInstIdx = A.getExport().first;
        const auto &SrcName = A.getExport().second;
        const auto &SrcExports = CompCtx.getInstance(SrcInstIdx).Exports;
        auto It = SrcExports.find(std::string(SrcName));
        if (It != SrcExports.end()) {
          if (Sort.getSortType() == AST::Component::Sort::SortType::Instance) {
            if (It->second.IT != nullptr) {
              populateInstanceFromType(NewInstIdx, *It->second.IT);
            } else if (It->second.NestedInstIdx.has_value()) {
              const auto &NestedExports =
                  CompCtx.getInstance(*It->second.NestedInstIdx).Exports;
              for (const auto &[Name, IE] : NestedExports) {
                CompCtx.addInstanceExport(NewInstIdx, Name, IE.ST, IE.IT,
                                          IE.NestedInstIdx, IE.ResourceId);
              }
            }
          } else if (Sort.getSortType() ==
                         AST::Component::Sort::SortType::Type &&
                     It->second.ResourceId.has_value()) {
            CompCtx.addResource(NewInstIdx, {*It->second.ResourceId,
                                             /*LocallyDefined=*/false});
          }
        }
      }
    }
  } else if (Decl.isExportDecl()) {
    EXPECTED_TRY(validate(Decl.getExport()).map_error(ReportError));
  } else {
    assumingUnreachable();
  }
  return {};
}

Expect<void>
Validator::validate(const AST::Component::ComponentDecl &Decl) noexcept {
  auto ReportError = [](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Decl_Component));
    return E;
  };

  if (Decl.isImportDecl()) {
    EXPECTED_TRY(validate(Decl.getImport()).map_error(ReportError));
  } else if (Decl.isInstanceDecl()) {
    EXPECTED_TRY(validate(Decl.getInstance()).map_error(ReportError));
  } else {
    assumingUnreachable();
  }
  return {};
}

Expect<void> Validator::validate(const ComponentValType &VT) noexcept {
  if (VT.getCode() == ComponentTypeCode::TypeIndex) {
    uint32_t Idx = VT.getTypeIndex();
    if (Idx >= CompCtx.getSortIndexSize(AST::Component::Sort::SortType::Type)) {
      spdlog::error(ErrCode::Value::DefTypeIndexOutOfBounds);
      spdlog::error("    ComponentValType: type index {} out of bounds"sv, Idx);
      return Unexpect(ErrCode::Value::DefTypeIndexOutOfBounds);
    }
    const auto *DT = CompCtx.getDefType(Idx);
    if (DT != nullptr && !DT->isDefValType() && !DT->isResourceType()) {
      spdlog::error(ErrCode::Value::NotADefinedType);
      spdlog::error(
          "    ComponentValType: type index {} is not a defined value type"sv,
          Idx);
      return Unexpect(ErrCode::Value::NotADefinedType);
    }
  }
  return {};
}

Expect<void>
Validator::validate(const AST::Component::DefValType &DVT) noexcept {
  if (DVT.isOwnTy()) {
    uint32_t Idx = DVT.getOwn().Idx;
    if (Idx >= CompCtx.getSortIndexSize(AST::Component::Sort::SortType::Type)) {
      spdlog::error(ErrCode::Value::DefTypeIndexOutOfBounds);
      spdlog::error("    DefValType: own type index {} out of bounds"sv, Idx);
      return Unexpect(ErrCode::Value::DefTypeIndexOutOfBounds);
    }
    if (CompCtx.getResource(Idx) == nullptr) {
      spdlog::error(ErrCode::Value::NotADefinedType);
      spdlog::error(
          "    DefValType: own type index {} does not refer to a resource type"sv,
          Idx);
      return Unexpect(ErrCode::Value::NotADefinedType);
    }
  } else if (DVT.isBorrowTy()) {
    uint32_t Idx = DVT.getBorrow().Idx;
    if (Idx >= CompCtx.getSortIndexSize(AST::Component::Sort::SortType::Type)) {
      spdlog::error(ErrCode::Value::DefTypeIndexOutOfBounds);
      spdlog::error("    DefValType: borrow type index {} out of bounds"sv,
                    Idx);
      return Unexpect(ErrCode::Value::DefTypeIndexOutOfBounds);
    }
    if (CompCtx.getResource(Idx) == nullptr) {
      spdlog::error(ErrCode::Value::NotADefinedType);
      spdlog::error(
          "    DefValType: borrow type index {} does not refer to a resource type"sv,
          Idx);
      return Unexpect(ErrCode::Value::NotADefinedType);
    }
  } else if (DVT.isRecordTy()) {
    const auto &Rec = DVT.getRecord();
    if (Rec.LabelTypes.empty()) {
      spdlog::error(ErrCode::Value::InvalidTypeReference);
      spdlog::error("    DefValType: record must have at least one field"sv);
      return Unexpect(ErrCode::Value::InvalidTypeReference);
    }
    std::unordered_set<std::string> Seen;
    for (const auto &LT : Rec.LabelTypes) {
      if (LT.getLabel().empty()) {
        spdlog::error(ErrCode::Value::NameCannotBeEmpty);
        return Unexpect(ErrCode::Value::NameCannotBeEmpty);
      }
      if (!isKebabString(LT.getLabel())) {
        spdlog::error(ErrCode::Value::ComponentNameNotKebab);
        spdlog::error(
            "    DefValType: record field '{}' is not valid kebab-case"sv,
            LT.getLabel());
        return Unexpect(ErrCode::Value::ComponentNameNotKebab);
      }
      if (!Seen.insert(toLowerStr(LT.getLabel())).second) {
        spdlog::error(ErrCode::Value::RecordFieldNameConflicts);
        spdlog::error("    DefValType: duplicate record field '{}'"sv,
                      LT.getLabel());
        return Unexpect(ErrCode::Value::RecordFieldNameConflicts);
      }
      EXPECTED_TRY(validate(LT.getValType()));
    }
  } else if (DVT.isVariantTy()) {
    const auto &Var = DVT.getVariant();
    if (Var.Cases.empty()) {
      spdlog::error(ErrCode::Value::VariantMustHaveCase);
      return Unexpect(ErrCode::Value::VariantMustHaveCase);
    }
    std::unordered_set<std::string> Seen;
    for (const auto &C : Var.Cases) {
      if (C.first.empty()) {
        spdlog::error(ErrCode::Value::NameCannotBeEmpty);
        return Unexpect(ErrCode::Value::NameCannotBeEmpty);
      }
      if (!isKebabString(C.first)) {
        spdlog::error(ErrCode::Value::ComponentNameNotKebab);
        spdlog::error(
            "    DefValType: variant case '{}' is not valid kebab-case"sv,
            C.first);
        return Unexpect(ErrCode::Value::ComponentNameNotKebab);
      }
      if (!Seen.insert(toLowerStr(C.first)).second) {
        spdlog::error(ErrCode::Value::VariantCaseNameConflicts);
        spdlog::error("    DefValType: duplicate variant case '{}'"sv, C.first);
        return Unexpect(ErrCode::Value::VariantCaseNameConflicts);
      }
      if (C.second.has_value()) {
        EXPECTED_TRY(validate(*C.second));
      }
    }
  } else if (DVT.isTupleTy()) {
    if (DVT.getTuple().Types.empty()) {
      spdlog::error(ErrCode::Value::InvalidTypeReference);
      spdlog::error("    DefValType: tuple must have at least one element"sv);
      return Unexpect(ErrCode::Value::InvalidTypeReference);
    }
    for (const auto &T : DVT.getTuple().Types) {
      EXPECTED_TRY(validate(T));
    }
  } else if (DVT.isListTy()) {
    EXPECTED_TRY(validate(DVT.getList().ValTy));
  } else if (DVT.isOptionTy()) {
    EXPECTED_TRY(validate(DVT.getOption().ValTy));
  } else if (DVT.isResultTy()) {
    const auto &R = DVT.getResult();
    if (R.ValTy.has_value()) {
      EXPECTED_TRY(validate(*R.ValTy));
    }
    if (R.ErrTy.has_value()) {
      EXPECTED_TRY(validate(*R.ErrTy));
    }
  } else if (DVT.isFlagsTy()) {
    const auto &Flags = DVT.getFlags();
    if (Flags.Labels.empty()) {
      spdlog::error(ErrCode::Value::InvalidTypeReference);
      spdlog::error("    DefValType: flags must have at least one label"sv);
      return Unexpect(ErrCode::Value::InvalidTypeReference);
    }
    if (Flags.Labels.size() > 32) {
      spdlog::error(ErrCode::Value::CannotHaveMoreThan32Flags);
      return Unexpect(ErrCode::Value::CannotHaveMoreThan32Flags);
    }
    std::unordered_set<std::string> Seen;
    for (const auto &L : Flags.Labels) {
      if (L.empty()) {
        spdlog::error(ErrCode::Value::NameCannotBeEmpty);
        return Unexpect(ErrCode::Value::NameCannotBeEmpty);
      }
      if (!isKebabString(L)) {
        spdlog::error(ErrCode::Value::ComponentNameNotKebab);
        spdlog::error(
            "    DefValType: flags label '{}' is not valid kebab-case"sv, L);
        return Unexpect(ErrCode::Value::ComponentNameNotKebab);
      }
      if (!Seen.insert(toLowerStr(L)).second) {
        spdlog::error(ErrCode::Value::FlagNameConflicts);
        spdlog::error("    DefValType: duplicate flags label '{}'"sv, L);
        return Unexpect(ErrCode::Value::FlagNameConflicts);
      }
    }
  } else if (DVT.isEnumTy()) {
    const auto &Enm = DVT.getEnum();
    if (Enm.Labels.empty()) {
      spdlog::error(ErrCode::Value::InvalidTypeReference);
      spdlog::error("    DefValType: enum must have at least one label"sv);
      return Unexpect(ErrCode::Value::InvalidTypeReference);
    }
    std::unordered_set<std::string> Seen;
    for (const auto &L : Enm.Labels) {
      if (L.empty()) {
        spdlog::error(ErrCode::Value::NameCannotBeEmpty);
        return Unexpect(ErrCode::Value::NameCannotBeEmpty);
      }
      if (!isKebabString(L)) {
        spdlog::error(ErrCode::Value::ComponentNameNotKebab);
        spdlog::error(
            "    DefValType: enum label '{}' is not valid kebab-case"sv, L);
        return Unexpect(ErrCode::Value::ComponentNameNotKebab);
      }
      if (!Seen.insert(toLowerStr(L)).second) {
        spdlog::error(ErrCode::Value::EnumTagNameConflicts);
        spdlog::error("    DefValType: duplicate enum label '{}'"sv, L);
        return Unexpect(ErrCode::Value::EnumTagNameConflicts);
      }
    }
  } else if (DVT.isStreamTy()) {
    if (DVT.getStream().ValTy.has_value()) {
      EXPECTED_TRY(validate(*DVT.getStream().ValTy));
    }
  } else if (DVT.isFutureTy()) {
    if (DVT.getFuture().ValTy.has_value()) {
      EXPECTED_TRY(validate(*DVT.getFuture().ValTy));
    }
  }
  return {};
}

Expect<void> Validator::validate(const AST::Component::FuncType &FT) noexcept {
  // Validate param names: kebab-case + unique
  std::unordered_set<std::string_view> ParamNames;
  for (const auto &P : FT.getParamList()) {
    if (!P.getLabel().empty()) {
      if (!isKebabString(P.getLabel())) {
        spdlog::error(ErrCode::Value::ComponentNameNotKebab);
        spdlog::error(
            "    FuncType: parameter name '{}' is not valid kebab-case"sv,
            P.getLabel());
        return Unexpect(ErrCode::Value::ComponentNameNotKebab);
      }
      if (!ParamNames.insert(P.getLabel()).second) {
        spdlog::error(ErrCode::Value::ComponentDuplicateName);
        spdlog::error("    FuncType: duplicate parameter name '{}'"sv,
                      P.getLabel());
        return Unexpect(ErrCode::Value::ComponentDuplicateName);
      }
    }
    EXPECTED_TRY(validate(P.getValType()));
  }
  // Reject transitive use of borrow in results
  for (const auto &R : FT.getResultList()) {
    EXPECTED_TRY(validate(R.getValType()));
    if (containsBorrow(R.getValType())) {
      spdlog::error(ErrCode::Value::InvalidTypeReference);
      spdlog::error(
          "    FuncType: borrow type not allowed in function results"sv);
      return Unexpect(ErrCode::Value::InvalidTypeReference);
    }
  }
  return {};
}

Expect<void>
Validator::validate(const AST::Component::InstanceType &IT) noexcept {
  // Instance types are validated with an initially-empty index space.
  CompCtx.enterTypeDefinition();
  for (const auto &Decl : IT.getDecl()) {
    EXPECTED_TRY(validate(Decl).map_error([](auto E) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_DefType));
      return E;
    }));
  }
  CompCtx.exitComponent();
  return {};
}

Expect<void>
Validator::validate(const AST::Component::ComponentType &CT) noexcept {
  // Component types are validated with an initially-empty index space.
  CompCtx.enterTypeDefinition();
  for (const auto &Decl : CT.getDecl()) {
    EXPECTED_TRY(validate(Decl).map_error([](auto E) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_DefType));
      return E;
    }));
  }
  CompCtx.exitComponent();
  return {};
}

Expect<void>
Validator::validate(const AST::Component::ResourceType &RT) noexcept {
  // Resource types are not allowed inside componenttype/instancetype scopes.
  if (CompCtx.isTypeDefinitionScope()) {
    spdlog::error(ErrCode::Value::InvalidTypeReference);
    spdlog::error("    ResourceType: resource types cannot be defined inside "
                  "componenttype or instancetype"sv);
    return Unexpect(ErrCode::Value::InvalidTypeReference);
  }
  if (RT.getDestructor().has_value()) {
    uint32_t DtorIdx = *RT.getDestructor();
    if (DtorIdx >= CompCtx.getCoreSortIndexSize(
                       AST::Component::Sort::CoreSortType::Func)) {
      spdlog::error(ErrCode::Value::InvalidIndex);
      spdlog::error(
          "    ResourceType: destructor core func index {} out of bounds"sv,
          DtorIdx);
      return Unexpect(ErrCode::Value::InvalidIndex);
    }
    // Verify the destructor's signature is `[i32] -> []`. The signature is
    // only available for core funcs the validator has populated (canon
    // resource.* synthesise it; aliased-from-core-module funcs are tracked
    // as a TODO that needs CoreInstance export-type plumbing). Skip the
    // check when the SubType pointer is null — the bounds check above
    // already caught the obvious "no such func" mistake.
    // TODO: also accept `[i64] -> []` when the resource rep is i64
    // (memory64 proposal); needs ResourceType to carry rep size.
    const AST::SubType *DtorST = CompCtx.getCoreFunc(DtorIdx);
    if (DtorST != nullptr) {
      const auto &DtorType = DtorST->getCompositeType();
      const auto &ExpType = CoreFuncType_I32_Void.getCompositeType();
      if (!DtorType.isFunc() ||
          DtorType.getFuncType() != ExpType.getFuncType()) {
        spdlog::error(ErrCode::Value::InvalidTypeReference);
        spdlog::error(
            "    ResourceType: destructor core func {} must have signature [i32] -> []"sv,
            DtorIdx);
        return Unexpect(ErrCode::Value::InvalidTypeReference);
      }
    }
  }
  return {};
}

bool Validator::containsBorrow(const ComponentValType &VT) const noexcept {
  if (VT.getCode() == ComponentTypeCode::Borrow) {
    return true;
  }
  if (VT.getCode() != ComponentTypeCode::TypeIndex) {
    return false;
  }
  uint32_t Idx = VT.getTypeIndex();
  const auto *DT = CompCtx.getDefType(Idx);
  if (DT == nullptr || !DT->isDefValType()) {
    return false;
  }
  return containsBorrow(DT->getDefValType());
}

bool Validator::containsBorrow(
    const AST::Component::DefValType &DVT) const noexcept {
  if (DVT.isBorrowTy()) {
    return true;
  }
  if (DVT.isRecordTy()) {
    for (const auto &F : DVT.getRecord().LabelTypes) {
      if (containsBorrow(F.getValType())) {
        return true;
      }
    }
    return false;
  }
  if (DVT.isVariantTy()) {
    for (const auto &C : DVT.getVariant().Cases) {
      if (C.second.has_value() && containsBorrow(*C.second)) {
        return true;
      }
    }
    return false;
  }
  if (DVT.isListTy()) {
    return containsBorrow(DVT.getList().ValTy);
  }
  if (DVT.isTupleTy()) {
    for (const auto &T : DVT.getTuple().Types) {
      if (containsBorrow(T)) {
        return true;
      }
    }
    return false;
  }
  if (DVT.isOptionTy()) {
    return containsBorrow(DVT.getOption().ValTy);
  }
  if (DVT.isResultTy()) {
    const auto &R = DVT.getResult();
    return (R.ValTy.has_value() && containsBorrow(*R.ValTy)) ||
           (R.ErrTy.has_value() && containsBorrow(*R.ErrTy));
  }
  if (DVT.isStreamTy()) {
    return DVT.getStream().ValTy.has_value() &&
           containsBorrow(*DVT.getStream().ValTy);
  }
  if (DVT.isFutureTy()) {
    return DVT.getFuture().ValTy.has_value() &&
           containsBorrow(*DVT.getFuture().ValTy);
  }
  return false; // PrimValType, OwnTy, FlagsTy, EnumTy
}

} // namespace Validator
} // namespace WasmEdge
