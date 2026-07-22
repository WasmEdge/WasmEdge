// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- component_validator.cpp - Component definition validation ---------===//
//
// The component-model validation entry point and the per-section /
// per-definition rules: core modules and instances, nested components,
// instances, aliases, imports, exports, start, and values.
//
//===----------------------------------------------------------------------===//

#include "common/errinfo.h"
#include "common/spdlog.h"
#include "validator/component_value_decode.h"
#include "validator/validator.h"

#include <string>
#include <unordered_set>
#include <variant>

namespace WasmEdge {
namespace Validator {

using namespace std::literals;

// NOLINTBEGIN(misc-no-recursion) -- components nest by construction.

Expect<void>
Validator::validate(const AST::Component::Component &Comp) noexcept {
  CompCtx.reset();
  TypeSizeMemo.clear();
  TypeDepthMemo.clear();
  MatchWhy = ErrCode::Value::Success;
  EXPECTED_TRY(validateComponent(Comp));
  // Deferred function-body validation of all nested core modules.
  for (const auto *Mod : CompCtx.DeferredModules) {
    EXPECTED_TRY(validate(*Mod).map_error([](auto E) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Sec_CoreMod));
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Component));
      return E;
    }));
  }
  const_cast<AST::Component::Component &>(Comp).setIsValidated();
  return {};
}

Expect<const ComponentContext::ComponentInfo *>
Validator::validateComponent(const AST::Component::Component &Comp) noexcept {
  // Walk sections in binary order inside a fresh component scope; index
  // spaces grow as definitions validate, so later references only resolve
  // against entries introduced earlier. The component's external view is
  // materialized from its import/export sections along the way.
  ComponentContext::ScopedScope Guard(CompCtx,
                                      ComponentContext::Scope::Kind::Component);
  auto *Info = CompCtx.newComponentInfo();
  Info->DeclScope = &Guard.get();
  Guard.get().SelfInfo = Info;

  auto ReportError = [](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Component));
    return E;
  };
  for (const auto &Sec : Comp.getSections()) {
    auto Visitor = [&](auto &&S) -> Expect<void> {
      using T = std::decay_t<decltype(S)>;
      if constexpr (std::is_same_v<T, AST::CustomSection>) {
        return {};
      } else {
        return validate(S).map_error(ReportError);
      }
    };
    EXPECTED_TRY(std::visit(Visitor, Sec));
  }
  // Value linearity: every value must have been consumed exactly once.
  const auto &Values = Guard.get().Values;
  for (uint32_t I = 0; I < Values.size(); ++I) {
    if (!Values[I].Consumed) {
      spdlog::error(ErrCode::Value::ComponentValueNotConsumed);
      spdlog::error("    Value index {} was not consumed before the end of the "
                    "component."sv,
                    I);
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Component));
      return Unexpect(ErrCode::Value::ComponentValueNotConsumed);
    }
  }
  return Info;
}

Expect<void>
Validator::validate(const AST::Component::CoreModuleSection &ModSec) noexcept {
  // Function bodies are deferred to the end of the root component so
  // definition-order diagnostics match the reference validator.
  SkipFuncBodies = true;
  auto Res = validate(ModSec.getContent());
  SkipFuncBodies = false;
  EXPECTED_TRY(Res.map_error([](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Sec_CoreMod));
    return E;
  }));
  CompCtx.DeferredModules.push_back(&ModSec.getContent());
  EXPECTED_TRY(const auto *Info, buildCoreModuleInfo(ModSec.getContent()));
  CompCtx.top().CoreModules.push_back(Info);
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
Validator::validate(const AST::Component::CoreInstance &Inst) noexcept {
  auto &S = CompCtx.top();
  if (Inst.isInstantiateModule()) {
    const auto *Mod = S.getCoreModule(Inst.getModuleIndex());
    if (Mod == nullptr) {
      spdlog::error(ErrCode::Value::ComponentUnknownModule);
      spdlog::error("    Core module index {} out of bounds (size {})."sv,
                    Inst.getModuleIndex(), S.CoreModules.size());
      return Unexpect(ErrCode::Value::ComponentUnknownModule);
    }
    // Collect the named argument instances.
    std::unordered_map<std::string_view, const CtxView::CoreInstanceInfo *>
        Args;
    for (const auto &Arg : Inst.getInstantiateArgs()) {
      const auto *AI = S.getCoreInstance(Arg.getIndex());
      if (AI == nullptr) {
        spdlog::error(ErrCode::Value::DefTypeIndexOutOfBounds);
        spdlog::error("    Core instance index {} out of bounds (size {})."sv,
                      Arg.getIndex(), S.CoreInstances.size());
        return Unexpect(ErrCode::Value::DefTypeIndexOutOfBounds);
      }
      if (!Args.emplace(Arg.getName(), AI).second) {
        spdlog::error(ErrCode::Value::ComponentDuplicateModuleArg);
        spdlog::error("    Duplicate instantiation argument '{}'."sv,
                      Arg.getName());
        return Unexpect(ErrCode::Value::ComponentDuplicateModuleArg);
      }
    }
    // Every module import must be satisfied by the matching argument.
    for (const auto &[ModName, Name, Required] : Mod->Imports) {
      auto It = Args.find(ModName);
      if (It == Args.end()) {
        spdlog::error(ErrCode::Value::ComponentMissingModuleArg);
        spdlog::error("    Missing instantiation argument '{}'."sv, ModName);
        return Unexpect(ErrCode::Value::ComponentMissingModuleArg);
      }
      auto ExpIt = It->second->Exports.find(Name);
      if (ExpIt == It->second->Exports.end()) {
        spdlog::error(ErrCode::Value::ComponentUnknownExport);
        spdlog::error(
            "    Instance argument '{}' does not export '{}' required by "
            "the module."sv,
            ModName, Name);
        return Unexpect(ErrCode::Value::ComponentUnknownExport);
      }
      const auto &Provided = ExpIt->second;
      if (Provided.Kind != Required.Kind) {
        ErrCode::Value Code = ErrCode::Value::ArgTypeMismatch;
        if (Required.Kind == ExternalType::Global) {
          Code = ErrCode::Value::ComponentExpectedGlobal;
        } else if (Required.Kind == ExternalType::Function) {
          Code = ErrCode::Value::ComponentExpectedFunc;
        }
        spdlog::error(Code);
        spdlog::error("    Import '{}'.'{}' kind mismatch."sv, ModName, Name);
        return Unexpect(Code);
      }
      if (!matchCoreExtern(Provided, Required)) {
        // Distinguish the mismatch class for diagnostics.
        ErrCode::Value Code = ErrCode::Value::ArgTypeMismatch;
        switch (Required.Kind) {
        case ExternalType::Function:
          // The message prints the expected signature; a trivial one is
          // reported as `expected: (func)`.
          if (Required.Func != nullptr &&
              Required.Func->getCompositeType().isFunc() &&
              Required.Func->getCompositeType()
                  .getFuncType()
                  .getParamTypes()
                  .empty() &&
              Required.Func->getCompositeType()
                  .getFuncType()
                  .getReturnTypes()
                  .empty()) {
            Code = ErrCode::Value::ComponentExpectedFuncParen;
          }
          break;
        case ExternalType::Memory:
          if (Provided.Memory != nullptr && Required.Memory != nullptr) {
            const auto &LP = Provided.Memory->getLimit();
            const auto &LR = Required.Memory->getLimit();
            if (LP.isShared() != LR.isShared()) {
              Code = ErrCode::Value::ComponentMemorySharedMismatch;
            } else if (LP.is64() != LR.is64()) {
              Code = ErrCode::Value::ComponentMemoryIndexTypeMismatch;
            } else {
              Code = ErrCode::Value::ComponentMemoryLimitsMismatch;
            }
          }
          break;
        case ExternalType::Global:
          Code = ErrCode::Value::ArgTypeMismatch;
          break;
        case ExternalType::Table:
          if (Provided.Table != nullptr && Required.Table != nullptr &&
              !(Provided.Table->getRefType() == Required.Table->getRefType())) {
            Code = ErrCode::Value::ArgTypeMismatch;
          } else {
            Code = ErrCode::Value::ComponentTableLimitsMismatch;
          }
          break;
        default:
          break;
        }
        spdlog::error(Code);
        spdlog::error("    Import '{}'.'{}' type mismatch."sv, ModName, Name);
        return Unexpect(Code);
      }
    }
    // The new core instance exposes the module's exports.
    auto *Result = CompCtx.newCoreInstanceInfo();
    Result->Exports = Mod->Exports;
    S.CoreInstances.push_back(Result);
    return {};
  }

  // Inline exports: project current index-space entries into a fresh
  // core instance.
  auto *Result = CompCtx.newCoreInstanceInfo();
  Result->Synthetic = true;
  for (const auto &Exp : Inst.getInlineExports()) {
    const auto &SI = Exp.getSortIdx();
    const uint32_t Idx = SI.getIdx();
    if (!SI.getSort().isCore()) {
      spdlog::error(ErrCode::Value::InvalidTypeReference);
      return Unexpect(ErrCode::Value::InvalidTypeReference);
    }
    CtxView::CoreExternInfo Ext;
    switch (SI.getSort().getCoreSortType()) {
    case AST::Component::Sort::CoreSortType::Func: {
      const auto *F = S.getCoreFunc(Idx);
      if (F == nullptr) {
        spdlog::error(ErrCode::Value::DefTypeIndexOutOfBounds);
        spdlog::error("    Core function index {} out of bounds."sv, Idx);
        return Unexpect(ErrCode::Value::DefTypeIndexOutOfBounds);
      }
      Ext = {ExternalType::Function, F, nullptr, nullptr, nullptr};
      break;
    }
    case AST::Component::Sort::CoreSortType::Table: {
      if (Idx >= S.CoreTables.size()) {
        spdlog::error(ErrCode::Value::DefTypeIndexOutOfBounds);
        spdlog::error("    Core table index {} out of bounds."sv, Idx);
        return Unexpect(ErrCode::Value::DefTypeIndexOutOfBounds);
      }
      Ext = {ExternalType::Table, nullptr, S.CoreTables[Idx], nullptr, nullptr};
      break;
    }
    case AST::Component::Sort::CoreSortType::Memory: {
      if (Idx >= S.CoreMemories.size()) {
        spdlog::error(ErrCode::Value::DefTypeIndexOutOfBounds);
        spdlog::error("    Core memory index {} out of bounds."sv, Idx);
        return Unexpect(ErrCode::Value::DefTypeIndexOutOfBounds);
      }
      Ext = {ExternalType::Memory, nullptr, nullptr, S.CoreMemories[Idx],
             nullptr};
      break;
    }
    case AST::Component::Sort::CoreSortType::Global: {
      if (Idx >= S.CoreGlobals.size()) {
        spdlog::error(ErrCode::Value::DefTypeIndexOutOfBounds);
        spdlog::error("    Core global index {} out of bounds."sv, Idx);
        return Unexpect(ErrCode::Value::DefTypeIndexOutOfBounds);
      }
      Ext = {ExternalType::Global, nullptr, nullptr, nullptr,
             S.CoreGlobals[Idx]};
      break;
    }
    case AST::Component::Sort::CoreSortType::Tag: {
      if (Idx >= S.CoreTags.size()) {
        spdlog::error(ErrCode::Value::UnknownCoreTag);
        spdlog::error("    Core tag index {} out of bounds."sv, Idx);
        return Unexpect(ErrCode::Value::UnknownCoreTag);
      }
      Ext = {ExternalType::Tag, S.CoreTags[Idx], nullptr, nullptr, nullptr};
      break;
    }
    default:
      spdlog::error(ErrCode::Value::InvalidTypeReference);
      spdlog::error(
          "    Core instances can only export functions, tables, memories, "
          "globals, and tags."sv);
      return Unexpect(ErrCode::Value::InvalidTypeReference);
    }
    if (!Result->Exports.emplace(std::string(Exp.getName()), Ext).second) {
      spdlog::error(ErrCode::Value::DupExportName);
      spdlog::error("    Duplicate inline export name '{}'."sv, Exp.getName());
      return Unexpect(ErrCode::Value::DupExportName);
    }
  }
  S.CoreInstances.push_back(Result);
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
  EXPECTED_TRY(const auto *Info,
               validateComponent(CompSec.getContent()).map_error([](auto E) {
                 spdlog::error(
                     ErrInfo::InfoAST(ASTNodeAttr::Comp_Sec_Component));
                 return E;
               }));
  CompCtx.top().Components.push_back(Info);
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
Validator::validate(const AST::Component::Instance &Inst) noexcept {
  auto &S = CompCtx.top();
  if (Inst.isInstantiateModule()) {
    const auto *CI = S.getComponent(Inst.getComponentIndex());
    if (CI == nullptr) {
      spdlog::error(ErrCode::Value::ComponentUnknownComponent);
      spdlog::error("    Component index {} out of bounds (size {})."sv,
                    Inst.getComponentIndex(), S.Components.size());
      return Unexpect(ErrCode::Value::ComponentUnknownComponent);
    }
    EXPECTED_TRY(const auto *Result,
                 instantiateComponentInfo(*CI, Inst.getInstantiateArgs()));
    CompCtx.top().Instances.push_back(Result);
    return {};
  }

  // Inline exports. Index validity is checked before the export name.
  auto *Result = CompCtx.newInstanceInfo();
  Result->DeclScope = &S;
  Result->Synthetic = true;
  std::vector<CtxView::NameRecord> Names;
  for (const auto &Exp : Inst.getInlineExports()) {
    // Function indices get their own diagnostic here.
    if (!Exp.getSortIdx().getSort().isCore() &&
        Exp.getSortIdx().getSort().getSortType() ==
            AST::Component::Sort::SortType::Func &&
        S.getFunc(Exp.getSortIdx().getIdx()) == nullptr) {
      spdlog::error(ErrCode::Value::ComponentFunctionIndexOutOfBounds);
      spdlog::error("    Function index {} out of bounds (size {})."sv,
                    Exp.getSortIdx().getIdx(), S.Funcs.size());
      return Unexpect(ErrCode::Value::ComponentFunctionIndexOutOfBounds);
    }
    EXPECTED_TRY(auto Info, resolveSortIndex(Exp.getSortIdx()));
    EXPECTED_TRY(ComponentName CN, parseExportName(Exp.getName()));
    if (ComponentContext::addUniqueName(Names,
                                        ComponentContext::makeNameRecord(CN)) !=
        ComponentContext::NameClash::None) {
      spdlog::error(ErrCode::Value::ComponentNameConflict);
      spdlog::error("    Duplicate inline export name '{}'."sv, Exp.getName());
      return Unexpect(ErrCode::Value::ComponentNameConflict);
    }
    EXPECTED_TRY(checkAnnotatedName(CN, Info, false));
    EXPECTED_TRY(checkImplements(
        CN, Exp.getImplements(),
        Info.K == ComponentContext::ExternInfo::Kind::Instance));
    if (!Exp.getSortIdx().getSort().isCore() &&
        Exp.getSortIdx().getSort().getSortType() ==
            AST::Component::Sort::SortType::Value) {
      auto &VE = CompCtx.top().Values[Exp.getSortIdx().getIdx()];
      if (VE.Consumed) {
        spdlog::error(ErrCode::Value::ComponentValueAlreadyConsumed);
        return Unexpect(ErrCode::Value::ComponentValueAlreadyConsumed);
      }
      VE.Consumed = true;
    }
    Result->Exports.emplace(std::string(Exp.getName()), Info);
    Result->Order.emplace_back(Exp.getName());
  }
  CompCtx.top().Instances.push_back(Result);
  return {};
}

Expect<void>
Validator::validate(const AST::Component::AliasSection &AliasSec) noexcept {
  for (const auto &Alias : AliasSec.getContent()) {
    EXPECTED_TRY(validate(Alias).map_error([](auto E) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Sec_Alias));
      return E;
    }));
  }
  return {};
}

Expect<void> Validator::validate(const AST::Component::Alias &Alias) noexcept {
  auto &S = CompCtx.top();
  const auto &Sort = Alias.getSort();
  const bool InTypeDecl = S.K == ComponentContext::Scope::Kind::ComponentType ||
                          S.K == ComponentContext::Scope::Kind::InstanceType;

  switch (Alias.getTargetType()) {
  case AST::Component::Alias::TargetType::Export: {
    if (Sort.isCore() &&
        Sort.getCoreSortType() != AST::Component::Sort::CoreSortType::Module) {
      spdlog::error(ErrCode::Value::MalformedAliasTarget);
      return Unexpect(ErrCode::Value::MalformedAliasTarget);
    }
    const bool WantCoreModule = Sort.isCore();
    if (InTypeDecl &&
        (WantCoreModule ||
         (Sort.getSortType() != AST::Component::Sort::SortType::Type &&
          Sort.getSortType() != AST::Component::Sort::SortType::Instance))) {
      spdlog::error(ErrCode::Value::InvalidTypeReference);
      spdlog::error("    Aliases in a component or instance type may only "
                    "refer to types or instances."sv);
      return Unexpect(ErrCode::Value::InvalidTypeReference);
    }
    const auto &[InstIdx, Name] = Alias.getExport();
    const auto *Inst = S.getInstance(InstIdx);
    if (Inst == nullptr) {
      spdlog::error(ErrCode::Value::DefTypeIndexOutOfBounds);
      spdlog::error("    Instance index {} out of bounds (size {})."sv, InstIdx,
                    S.Instances.size());
      return Unexpect(ErrCode::Value::DefTypeIndexOutOfBounds);
    }
    auto It = Inst->Exports.find(Name);
    using Kind = CtxView::ExternInfo::Kind;
    if (WantCoreModule) {
      // Core-module exports of component instances.
      if (It == Inst->Exports.end() || It->second.K != Kind::CoreModule) {
        spdlog::error(ErrCode::Value::ComponentUnknownExport);
        spdlog::error("    Instance {} has no core-module export '{}'."sv,
                      InstIdx, Name);
        return Unexpect(ErrCode::Value::ComponentUnknownExport);
      }
      defineExtern(It->second);
      return {};
    }
    if (It == Inst->Exports.end()) {
      // Inline-created instances keep the plain unknown-export diagnostic.
      const auto Code = Inst->Synthetic
                            ? ErrCode::Value::ComponentUnknownExport
                            : ErrCode::Value::ComponentUnknownExport;
      spdlog::error(Code);
      spdlog::error("    Instance {} has no export named '{}'."sv, InstIdx,
                    Name);
      return Unexpect(Code);
    }
    const auto ST = Sort.getSortType();
    const auto &Info = It->second;
    const bool KindOk =
        (ST == AST::Component::Sort::SortType::Func && Info.K == Kind::Func) ||
        (ST == AST::Component::Sort::SortType::Value &&
         Info.K == Kind::Value) ||
        (ST == AST::Component::Sort::SortType::Type && Info.K == Kind::Type) ||
        (ST == AST::Component::Sort::SortType::Component &&
         Info.K == Kind::Component) ||
        (ST == AST::Component::Sort::SortType::Instance &&
         Info.K == Kind::Instance);
    if (!KindOk) {
      spdlog::error(ErrCode::Value::ComponentUnknownExport);
      spdlog::error("    Export '{}' of instance {} does not match the "
                    "alias sort."sv,
                    Name, InstIdx);
      return Unexpect(ErrCode::Value::ComponentUnknownExport);
    }
    defineExtern(Info);
    return {};
  }
  case AST::Component::Alias::TargetType::CoreExport: {
    if (!Sort.isCore()) {
      spdlog::error(ErrCode::Value::MalformedAliasTarget);
      return Unexpect(ErrCode::Value::MalformedAliasTarget);
    }
    if (InTypeDecl) {
      spdlog::error(ErrCode::Value::InvalidTypeReference);
      spdlog::error("    Aliases in a component or instance type may only "
                    "refer to types or instances."sv);
      return Unexpect(ErrCode::Value::InvalidTypeReference);
    }
    const auto &[InstIdx, Name] = Alias.getExport();
    const auto *Inst = S.getCoreInstance(InstIdx);
    if (Inst == nullptr) {
      spdlog::error(ErrCode::Value::DefTypeIndexOutOfBounds);
      spdlog::error("    Core instance index {} out of bounds (size {})."sv,
                    InstIdx, S.CoreInstances.size());
      return Unexpect(ErrCode::Value::DefTypeIndexOutOfBounds);
    }
    auto It = Inst->Exports.find(Name);
    const auto CS = Sort.getCoreSortType();
    if (It == Inst->Exports.end()) {
      ErrCode::Value Code;
      if (CS == AST::Component::Sort::CoreSortType::Tag) {
        Code = ErrCode::Value::UnknownCoreTag;
      } else if (Inst->Synthetic) {
        Code = ErrCode::Value::ComponentUnknownExport;
      } else {
        Code = ErrCode::Value::ComponentUnknownExport;
      }
      spdlog::error(Code);
      spdlog::error("    Core instance {} has no export named '{}'."sv, InstIdx,
                    Name);
      return Unexpect(Code);
    }
    const auto &Ext = It->second;
    auto KindMatches = [&](ExternalType ET,
                           AST::Component::Sort::CoreSortType Want) noexcept {
      return Ext.Kind == ET && CS == Want;
    };
    if (KindMatches(ExternalType::Function,
                    AST::Component::Sort::CoreSortType::Func)) {
      S.CoreFuncs.push_back(Ext.Func);
    } else if (KindMatches(ExternalType::Table,
                           AST::Component::Sort::CoreSortType::Table)) {
      S.CoreTables.push_back(Ext.Table);
    } else if (KindMatches(ExternalType::Memory,
                           AST::Component::Sort::CoreSortType::Memory)) {
      S.CoreMemories.push_back(Ext.Memory);
    } else if (KindMatches(ExternalType::Global,
                           AST::Component::Sort::CoreSortType::Global)) {
      S.CoreGlobals.push_back(Ext.Global);
    } else if (KindMatches(ExternalType::Tag,
                           AST::Component::Sort::CoreSortType::Tag)) {
      S.CoreTags.push_back(Ext.Func);
    } else {
      const auto Code = CS == AST::Component::Sort::CoreSortType::Tag
                            ? ErrCode::Value::UnknownCoreTag
                            : ErrCode::Value::InvalidTypeReference;
      spdlog::error(Code);
      spdlog::error("    Core export '{}' does not match the alias sort."sv,
                    Name);
      return Unexpect(Code);
    }
    return {};
  }
  case AST::Component::Alias::TargetType::Outer: {
    const auto &[Ct, Idx] = Alias.getOuter();
    auto *Target = CompCtx.scopeUp(Ct);
    if (Target == nullptr) {
      spdlog::error(ErrCode::Value::ComponentInvalidOuterAliasCount);
      spdlog::error("    Outer alias count {} exceeds enclosing scopes."sv, Ct);
      return Unexpect(ErrCode::Value::ComponentInvalidOuterAliasCount);
    }
    if (Sort.isCore()) {
      switch (Sort.getCoreSortType()) {
      case AST::Component::Sort::CoreSortType::Module: {
        if (InTypeDecl) {
          spdlog::error(ErrCode::Value::InvalidTypeReference);
          spdlog::error("    Aliases in a component or instance type may "
                        "only refer to types or instances."sv);
          return Unexpect(ErrCode::Value::InvalidTypeReference);
        }
        const auto *Mod = Target->getCoreModule(Idx);
        if (Mod == nullptr) {
          spdlog::error(ErrCode::Value::DefTypeIndexOutOfBounds);
          spdlog::error("    Aliased core module index {} out of bounds."sv,
                        Idx);
          return Unexpect(ErrCode::Value::DefTypeIndexOutOfBounds);
        }
        S.CoreModules.push_back(Mod);
        return {};
      }
      case AST::Component::Sort::CoreSortType::Type: {
        const auto *Entry = Target->getCoreType(Idx);
        if (Entry == nullptr) {
          spdlog::error(ErrCode::Value::DefTypeIndexOutOfBounds);
          spdlog::error("    Aliased core type index {} out of bounds."sv, Idx);
          return Unexpect(ErrCode::Value::DefTypeIndexOutOfBounds);
        }
        S.CoreTypes.push_back(*Entry);
        return {};
      }
      default:
        spdlog::error(ErrCode::Value::MalformedAliasTarget);
        spdlog::error(
            "    Outer aliases may only target module, type, component."sv);
        return Unexpect(ErrCode::Value::MalformedAliasTarget);
      }
    }
    switch (Sort.getSortType()) {
    case AST::Component::Sort::SortType::Type: {
      const auto *Entry = Target->getType(Idx);
      if (Entry == nullptr) {
        spdlog::error(ErrCode::Value::DefTypeIndexOutOfBounds);
        spdlog::error("    Aliased type index {} out of bounds."sv, Idx);
        return Unexpect(ErrCode::Value::DefTypeIndexOutOfBounds);
      }
      // Crossing a component boundary requires the type to carry no free
      // resource identities (generativity would leak otherwise).
      const auto *Inside = CompCtx.scopeInsideTarget(Ct);
      if (Inside != nullptr &&
          Inside->K == ComponentContext::Scope::Kind::Component) {
        std::unordered_set<uint32_t> Ids;
        CtxView::ExternInfo Probe;
        Probe.K = CtxView::ExternInfo::Kind::Type;
        Probe.Type = *Entry;
        collectResources(Probe, Ids);
        // Ids bound by the aliased type itself are not free.
        const CtxView::Scope *Binder = nullptr;
        if (Entry->Inst != nullptr) {
          Binder = Entry->Inst->DeclScope;
        } else if (Entry->Comp != nullptr) {
          Binder = Entry->Comp->DeclScope;
        }
        bool HasFree = false;
        for (const uint32_t Id : Ids) {
          if (Binder == nullptr || !originatesIn(Id, *Binder)) {
            HasFree = true;
            break;
          }
        }
        if (HasFree) {
          spdlog::error(ErrCode::Value::ComponentAliasResourceLeak);
          spdlog::error(
              "    Cannot alias an outer type which transitively refers to "
              "resources not defined in the current component."sv);
          return Unexpect(ErrCode::Value::ComponentAliasResourceLeak);
        }
      }
      S.Types.push_back(*Entry);
      return {};
    }
    case AST::Component::Sort::SortType::Component: {
      if (InTypeDecl) {
        spdlog::error(ErrCode::Value::InvalidTypeReference);
        spdlog::error("    Aliases in a component or instance type may only "
                      "refer to types or instances."sv);
        return Unexpect(ErrCode::Value::InvalidTypeReference);
      }
      const auto *Comp = Target->getComponent(Idx);
      if (Comp == nullptr) {
        spdlog::error(ErrCode::Value::DefTypeIndexOutOfBounds);
        spdlog::error("    Aliased component index {} out of bounds."sv, Idx);
        return Unexpect(ErrCode::Value::DefTypeIndexOutOfBounds);
      }
      S.Components.push_back(Comp);
      return {};
    }
    default:
      spdlog::error(ErrCode::Value::MalformedAliasTarget);
      spdlog::error(
          "    Outer aliases may only target module, type, component."sv);
      return Unexpect(ErrCode::Value::MalformedAliasTarget);
    }
  }
  default:
    spdlog::error(ErrCode::Value::MalformedAliasTarget);
    return Unexpect(ErrCode::Value::MalformedAliasTarget);
  }
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
  for (const auto &Canon : CanonSec.getContent()) {
    EXPECTED_TRY(validate(Canon).map_error([](auto E) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Sec_Canon));
      return E;
    }));
  }
  return {};
}

Expect<void>
Validator::validate(const AST::Component::StartSection &StartSec) noexcept {
  const auto &Start = StartSec.getContent();
  auto &S = CompCtx.top();
  const auto *FI = S.getFunc(Start.getFunctionIndex());
  if (FI == nullptr || FI->FT == nullptr) {
    spdlog::error(ErrCode::Value::ComponentFunctionIndexOutOfBounds);
    spdlog::error("    Start function index {} out of bounds (size {})."sv,
                  Start.getFunctionIndex(), S.Funcs.size());
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Sec_Start));
    return Unexpect(ErrCode::Value::ComponentFunctionIndexOutOfBounds);
  }
  const auto Params = FI->FT->getParamList();
  const auto Args = Start.getArguments();
  if (Params.size() != Args.size()) {
    spdlog::error(ErrCode::Value::ArgTypeMismatch);
    spdlog::error("    Start takes {} arguments but the function has {} "
                  "parameters."sv,
                  Args.size(), Params.size());
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Sec_Start));
    return Unexpect(ErrCode::Value::ArgTypeMismatch);
  }
  for (size_t I = 0; I < Args.size(); ++I) {
    const uint32_t ValIdx = Args[I];
    if (ValIdx >= S.Values.size()) {
      spdlog::error(ErrCode::Value::InvalidIndex);
      spdlog::error("    Start argument value index {} out of bounds."sv,
                    ValIdx);
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Sec_Start));
      return Unexpect(ErrCode::Value::InvalidIndex);
    }
    if (S.Values[ValIdx].Consumed) {
      spdlog::error(ErrCode::Value::ComponentValueAlreadyConsumed);
      spdlog::error("    Start argument value {} was already consumed."sv,
                    ValIdx);
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Sec_Start));
      return Unexpect(ErrCode::Value::ComponentValueAlreadyConsumed);
    }
    ResourceSubst Subst;
    if (!matchValType(S.Values[ValIdx].Type,
                      {Params[I].getValType(), FI->Home, FI->Remap}, Subst)) {
      spdlog::error(ErrCode::Value::ArgTypeMismatch);
      spdlog::error("    Start argument {} type mismatch."sv, I);
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Sec_Start));
      return Unexpect(ErrCode::Value::ArgTypeMismatch);
    }
    S.Values[ValIdx].Consumed = true;
  }
  const auto Results = FI->FT->getResultList();
  if (Start.getResult() != Results.size()) {
    spdlog::error(ErrCode::Value::ArgTypeMismatch);
    spdlog::error("    Start declares {} results but the function has {}."sv,
                  Start.getResult(), Results.size());
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Sec_Start));
    return Unexpect(ErrCode::Value::ArgTypeMismatch);
  }
  for (const auto &R : Results) {
    S.Values.push_back({{R.getValType(), FI->Home, FI->Remap}, false});
  }
  return {};
}

Expect<void>
Validator::validate(const AST::Component::ImportSection &ImpSec) noexcept {
  for (const auto &Import : ImpSec.getContent()) {
    EXPECTED_TRY(validate(Import).map_error([](auto E) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Sec_Import));
      return E;
    }));
  }
  return {};
}

Expect<void> Validator::validate(const AST::Component::Import &Im) noexcept {
  EXPECTED_TRY(auto Info,
               defineImport(Im.getName(), Im.getDesc(), Im.getImplements()));
  auto &S = CompCtx.top();
  if (S.SelfInfo != nullptr) {
    S.SelfInfo->Imports.emplace_back(std::string(Im.getName()), Info);
  }
  return {};
}

Expect<void>
Validator::validate(const AST::Component::ExportSection &ExpSec) noexcept {
  for (const auto &Export : ExpSec.getContent()) {
    EXPECTED_TRY(validate(Export).map_error([](auto E) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Sec_Export));
      return E;
    }));
  }
  return {};
}

Expect<void> Validator::validate(const AST::Component::Export &Ex) noexcept {
  auto &S = CompCtx.top();
  // Index bounds are diagnosed before the export name; a valid name
  // upgrades module/instance/function bounds errors to specific codes.
  {
    const auto &SI = Ex.getSortIndex();
    const bool NameOk = ComponentName::parse(Ex.getName()).has_value();
    const uint32_t Idx = SI.getIdx();
    bool OOB = false;
    ErrCode::Value Code = ErrCode::Value::DefTypeIndexOutOfBounds;
    if (SI.getSort().isCore()) {
      if (SI.getSort().getCoreSortType() ==
              AST::Component::Sort::CoreSortType::Module &&
          S.getCoreModule(Idx) == nullptr) {
        OOB = true;
        if (NameOk) {
          Code = ErrCode::Value::ComponentModuleIndexOutOfBounds;
        }
      }
    } else {
      switch (SI.getSort().getSortType()) {
      case AST::Component::Sort::SortType::Func:
        if (S.getFunc(Idx) == nullptr) {
          OOB = true;
          if (NameOk) {
            Code = ErrCode::Value::ComponentFunctionIndexOutOfBounds;
          }
        }
        break;
      case AST::Component::Sort::SortType::Instance:
        if (S.getInstance(Idx) == nullptr) {
          OOB = true;
          if (NameOk) {
            Code = ErrCode::Value::ComponentInstanceIndexOutOfBounds;
          }
        }
        break;
      default:
        break;
      }
    }
    if (OOB) {
      spdlog::error(Code);
      spdlog::error("    Export index {} out of bounds."sv, Idx);
      return Unexpect(Code);
    }
  }
  EXPECTED_TRY(auto Inferred, resolveSortIndex(Ex.getSortIndex()));
  // Exporting a value consumes it.
  if (!Ex.getSortIndex().getSort().isCore() &&
      Ex.getSortIndex().getSort().getSortType() ==
          AST::Component::Sort::SortType::Value) {
    auto &VE = S.Values[Ex.getSortIndex().getIdx()];
    if (VE.Consumed) {
      spdlog::error(ErrCode::Value::ComponentValueAlreadyConsumed);
      spdlog::error("    Exported value {} was already consumed."sv,
                    Ex.getSortIndex().getIdx());
      return Unexpect(ErrCode::Value::ComponentValueAlreadyConsumed);
    }
    VE.Consumed = true;
    // Exported values must not transitively contain borrow handles.
    if (containsBorrow(VE.Type)) {
      spdlog::error(ErrCode::Value::InvalidTypeReference);
      spdlog::error("    Exported value types cannot contain borrows."sv);
      return Unexpect(ErrCode::Value::InvalidTypeReference);
    }
  }
  EXPECTED_TRY(auto Result, defineExport(Ex.getName(), Inferred, Ex.getDesc(),
                                         Ex.getImplements()));
  // The re-exported value index is born consumed.
  if (Result.K == CtxView::ExternInfo::Kind::Value) {
    CompCtx.top().Values.back().Consumed = true;
  }
  if (S.SelfInfo != nullptr) {
    S.SelfInfo->Exports.emplace(std::string(Ex.getName()), Result);
  }
  return {};
}

Expect<void>
Validator::validate(const AST::Component::ValueSection &ValSec) noexcept {
  for (const auto &Value : ValSec.getContent()) {
    EXPECTED_TRY(validate(Value.getType()).map_error([](auto E) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Sec_Value));
      return E;
    }));
    auto Resolver = [this](uint32_t Idx) -> const AST::Component::DefValType * {
      const auto *Entry = CompCtx.top().getType(Idx);
      return Entry != nullptr && Entry->DT != nullptr &&
                     Entry->DT->isDefValType()
                 ? &Entry->DT->getDefValType()
                 : nullptr;
    };
    ValueDecoder<decltype(Resolver)> Decoder(Value.getData(),
                                             std::move(Resolver));
    auto Decoded = Decoder.decode(Value.getType());
    if (!Decoded) {
      spdlog::error(ErrCode::Value::ComponentMalformedValue);
      spdlog::error("    Value definition payload does not match its type."sv);
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Sec_Value));
      return Unexpect(ErrCode::Value::ComponentMalformedValue);
    }
    Value.setDecoded(std::move(*Decoded));
    CompCtx.top().Values.push_back(
        {{Value.getType(), &CompCtx.top(), nullptr}, false});
  }
  return {};
}

// NOLINTEND(misc-no-recursion)

} // namespace Validator
} // namespace WasmEdge
