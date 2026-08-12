// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- component_validator.cpp - Component definition validation ---------===//
//
// The component-model validation entry point and the per-section rules.
//
//===----------------------------------------------------------------------===//

#include "common/errinfo.h"
#include "common/spdlog.h"
#include "validator/component_value_decode.h"
#include "validator/validator.h"

#include <optional>
#include <string>
#include <unordered_set>
#include <variant>

namespace WasmEdge {
namespace Validator {

using namespace std::literals;

Expect<void>
Validator::validate(const AST::Component::Component &Comp) noexcept {
  CompCtx.reset();
  EXPECTED_TRY(validate(Comp, *CompTypes.newShape()));
  // Deferred function-body validation of all nested core modules, resumed
  // on the checker state captured at each definition.
  for (auto &[Mod, Saved] : CompCtx.DeferredModules) {
    Checker = std::move(Saved);
    EXPECTED_TRY(validate(Mod->getCodeSection()).map_error([](auto E) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Sec_Code));
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Sec_CoreMod));
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Component));
      return E;
    }));
    const_cast<AST::Module &>(*Mod).setIsValidated();
  }
  const_cast<AST::Component::Component &>(Comp).setIsValidated();
  return {};
}

Expect<void> Validator::validate(const AST::Component::Component &Comp,
                                 Component::Shape &Out) noexcept {
  // Walk the sections in binary order, in a fresh scope. The index spaces
  // grow as definitions validate, so a reference sees only earlier entries.
  auto &S = CompCtx.enterScope(Component::Scope::Kind::Component);
  Out.DeclScope = &S;

  auto ReportError = [](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Component));
    return E;
  };
  for (const auto &Sec : Comp.getSections()) {
    auto Visitor = [&](auto &&Section) -> Expect<void> {
      using T = std::decay_t<decltype(Section)>;
      if constexpr (std::is_same_v<T, AST::CustomSection>) {
        return {};
      } else if constexpr (std::is_same_v<T, AST::Component::ImportSection> ||
                           std::is_same_v<T, AST::Component::ExportSection>) {
        // These two accumulate into the component's own external type.
        return validate(Section, Out).map_error(ReportError);
      } else {
        return validate(Section).map_error(ReportError);
      }
    };
    EXPECTED_TRY(std::visit(Visitor, Sec));
  }
  // Value linearity. Every value must have exactly one consumer.
  const auto &Values = S.Values;
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
  CompCtx.exitScope();
  return {};
}

Expect<void>
Validator::validate(const AST::Component::CoreModuleSection &ModSec) noexcept {
  // Validate the sections except the function bodies, which defer to the end
  // of the root component, matching the reference validator's ordering.
  const auto &Mod = ModSec.getContent();
  Checker.reset(true);
  auto ReportError = [](ASTNodeAttr Attr) {
    return [Attr](auto E) {
      spdlog::error(ErrInfo::InfoAST(Attr));
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Sec_CoreMod));
      return E;
    };
  };
  EXPECTED_TRY(validate(Mod.getTypeSection())
                   .map_error(ReportError(ASTNodeAttr::Sec_Type)));
  EXPECTED_TRY(validate(Mod.getImportSection())
                   .map_error(ReportError(ASTNodeAttr::Sec_Import)));
  EXPECTED_TRY(validate(Mod.getFunctionSection())
                   .map_error(ReportError(ASTNodeAttr::Sec_Function)));
  EXPECTED_TRY(validate(Mod.getTableSection())
                   .map_error(ReportError(ASTNodeAttr::Sec_Table)));
  EXPECTED_TRY(validate(Mod.getMemorySection())
                   .map_error(ReportError(ASTNodeAttr::Sec_Memory)));
  EXPECTED_TRY(validate(Mod.getGlobalSection())
                   .map_error(ReportError(ASTNodeAttr::Sec_Global)));
  EXPECTED_TRY(validate(Mod.getTagSection())
                   .map_error(ReportError(ASTNodeAttr::Sec_Tag)));
  EXPECTED_TRY(validate(Mod.getExportSection())
                   .map_error(ReportError(ASTNodeAttr::Sec_Export)));
  EXPECTED_TRY(validate(Mod.getStartSection())
                   .map_error(ReportError(ASTNodeAttr::Sec_Start)));
  EXPECTED_TRY(validate(Mod.getElementSection())
                   .map_error(ReportError(ASTNodeAttr::Sec_Element)));
  EXPECTED_TRY(validate(Mod.getDataSection())
                   .map_error(ReportError(ASTNodeAttr::Sec_Data)));
  if (Checker.getTables().size() > 1 &&
      !Conf.hasProposal(Proposal::ReferenceTypes)) {
    spdlog::error(ErrCode::Value::MultiTables);
    spdlog::error(ErrInfo::InfoProposal(Proposal::ReferenceTypes));
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Sec_CoreMod));
    return Unexpect(ErrCode::Value::MultiTables);
  }
  if (Checker.getMemories().size() > 1 &&
      !Conf.hasProposal(Proposal::MultiMemories)) {
    spdlog::error(ErrCode::Value::MultiMemories);
    spdlog::error(ErrInfo::InfoProposal(Proposal::MultiMemories));
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Sec_CoreMod));
    return Unexpect(ErrCode::Value::MultiMemories);
  }
  CompCtx.DeferredModules.emplace_back(&Mod, Checker);
  EXPECTED_TRY(const auto *Shape, CompCtx.buildCoreShape(Mod));
  CompCtx.top().CoreModules.push_back(Shape);
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
    std::unordered_map<std::string_view, const Component::CoreShape *> Args;
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
      if (!Component::Matcher(CompTypes).matchCoreExtern(Provided, Required)) {
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
    auto *Result = CompTypes.newCoreShape();
    Result->Exports = Mod->Exports;
    S.CoreInstances.push_back(Result);
    return {};
  }

  // Inline exports: project current index-space entries into a fresh
  // core instance.
  auto *Result = CompTypes.newCoreShape();
  for (const auto &Exp : Inst.getInlineExports()) {
    const auto &SI = Exp.getSortIdx();
    const uint32_t Idx = SI.getIdx();
    if (!SI.getSort().isCore()) {
      spdlog::error(ErrCode::Value::InvalidTypeReference);
      return Unexpect(ErrCode::Value::InvalidTypeReference);
    }
    Component::CoreExternInfo Ext;
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
  auto *Shape = CompTypes.newShape();
  EXPECTED_TRY(validate(CompSec.getContent(), *Shape).map_error([](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Sec_Component));
    return E;
  }));
  CompCtx.top().Components.push_back(Shape);
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
    EXPECTED_TRY(const auto *Result, CompCtx.instantiateComponentShape(
                                         *CI, Inst.getInstantiateArgs()));
    CompCtx.top().Instances.push_back(Result);
    return {};
  }

  // Inline exports. Index validity is checked before the export name.
  auto *Result = CompTypes.newShape();
  Result->DeclScope = &S;
  std::vector<Component::NameRecord> Names;
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
    EXPECTED_TRY(auto Info, CompCtx.resolveSortIndex(Exp.getSortIdx()));
    EXPECTED_TRY(Component::ExternName CN,
                 CompCtx.parseExternName(Exp.getName(), false));
    EXPECTED_TRY(
        CompCtx.addUniqueName(Names, CompCtx.makeNameRecord(CN), false));
    EXPECTED_TRY(CompCtx.checkAnnotatedName(CN, Info, false));
    EXPECTED_TRY(CompCtx.checkNameAttributes(
        CN, Exp.getImplements(), Exp.getExternalIds(), Exp.getVersionSuffixes(),
        Info.K == Component::ExternKind::InstanceType));
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
    Result->ExportOrder.emplace_back(Exp.getName());
  }
  // An inline instance nests the shapes it re-exports, so it is bounded like
  // any other declared type.
  Component::ExternInfo Probe;
  Probe.K = Component::ExternKind::InstanceType;
  Probe.Shape = Result;
  EXPECTED_TRY(CompTypes.checkTypeLimits(Probe));
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
  const bool InTypeDecl = S.K == Component::Scope::Kind::ComponentType ||
                          S.K == Component::Scope::Kind::InstanceType;

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
    if (WantCoreModule) {
      // Core-module exports of component instances.
      if (It == Inst->Exports.end() ||
          It->second.K != Component::ExternKind::CoreType) {
        spdlog::error(ErrCode::Value::ComponentExportNotAModule);
        spdlog::error("    Export '{}' of instance {} is not a core module."sv,
                      Name, InstIdx);
        return Unexpect(ErrCode::Value::ComponentExportNotAModule);
      }
      CompCtx.defineExtern(It->second);
      return {};
    }
    if (It == Inst->Exports.end()) {
      spdlog::error(ErrCode::Value::ComponentUnknownExport);
      spdlog::error("    Instance {} has no export named '{}'."sv, InstIdx,
                    Name);
      return Unexpect(ErrCode::Value::ComponentUnknownExport);
    }
    const auto ST = Sort.getSortType();
    const auto &Info = It->second;
    const bool KindOk = (ST == AST::Component::Sort::SortType::Func &&
                         Info.K == Component::ExternKind::FuncType) ||
                        (ST == AST::Component::Sort::SortType::Value &&
                         Info.K == Component::ExternKind::ValueBound) ||
                        (ST == AST::Component::Sort::SortType::Type &&
                         Info.K == Component::ExternKind::TypeBound) ||
                        (ST == AST::Component::Sort::SortType::Component &&
                         Info.K == Component::ExternKind::ComponentType) ||
                        (ST == AST::Component::Sort::SortType::Instance &&
                         Info.K == Component::ExternKind::InstanceType);
    if (!KindOk) {
      spdlog::error(ErrCode::Value::ComponentUnknownExport);
      spdlog::error("    Export '{}' of instance {} does not match the "
                    "alias sort."sv,
                    Name, InstIdx);
      return Unexpect(ErrCode::Value::ComponentUnknownExport);
    }
    CompCtx.defineExtern(Info);
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
      const auto Code = CS == AST::Component::Sort::CoreSortType::Tag
                            ? ErrCode::Value::UnknownCoreTag
                            : ErrCode::Value::ComponentUnknownExport;
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
          spdlog::error(ErrCode::Value::ComponentTypeIndexOutOfBounds);
          spdlog::error("    Aliased core type index {} out of bounds."sv, Idx);
          return Unexpect(ErrCode::Value::ComponentTypeIndexOutOfBounds);
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
        spdlog::error(ErrCode::Value::ComponentTypeIndexOutOfBounds);
        spdlog::error("    Aliased type index {} out of bounds."sv, Idx);
        return Unexpect(ErrCode::Value::ComponentTypeIndexOutOfBounds);
      }
      // A type that crosses a component boundary must carry no free resource
      // identity. A free identity leaks generativity.
      const auto *Inside = CompCtx.scopeInsideTarget(Ct);
      if (Inside != nullptr && Inside->K == Component::Scope::Kind::Component) {
        std::unordered_set<uint32_t> Ids;
        Component::ExternInfo Probe;
        Probe.K = Component::ExternKind::TypeBound;
        Probe.Type = *Entry;
        CompTypes.collectResources(Probe, Ids);
        // Ids bound by the aliased type itself are not free.
        const Component::Scope *Binder = nullptr;
        if (Entry->Inst != nullptr) {
          Binder = Entry->Inst->DeclScope;
        } else if (Entry->Comp != nullptr) {
          Binder = Entry->Comp->DeclScope;
        }
        bool HasFree = false;
        for (const uint32_t Id : Ids) {
          if (Binder == nullptr || !CompTypes.originatesIn(Id, *Binder)) {
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
    if (!Component::Matcher(CompTypes).matchValType(
            S.Values[ValIdx].Type,
            {Params[I].getValType(), FI->Home, FI->Remap})) {
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

Expect<void> Validator::validate(const AST::Component::ImportSection &ImpSec,
                                 Component::Shape &Out) noexcept {
  for (const auto &Import : ImpSec.getContent()) {
    EXPECTED_TRY(validate(Import, Out).map_error([](auto E) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Sec_Import));
      return E;
    }));
  }
  return {};
}

Expect<void> Validator::validate(const AST::Component::Import &Im,
                                 Component::Shape &Out) noexcept {
  // The descriptor is checked before the import name.
  Component::ExternInfo Resolved;
  EXPECTED_TRY(validate(Im.getDesc(), true, Resolved));
  EXPECTED_TRY(auto Info, CompCtx.defineImport(
                              Im.getName(), Resolved, Im.getImplements(),
                              Im.getExternalIds(), Im.getVersionSuffixes()));
  Out.Imports.emplace_back(std::string(Im.getName()), Info);
  return {};
}

Expect<void> Validator::validate(const AST::Component::ExportSection &ExpSec,
                                 Component::Shape &Out) noexcept {
  for (const auto &Export : ExpSec.getContent()) {
    EXPECTED_TRY(validate(Export, Out).map_error([](auto E) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Sec_Export));
      return E;
    }));
  }
  return {};
}

Expect<void> Validator::validate(const AST::Component::Export &Ex,
                                 Component::Shape &Out) noexcept {
  auto &S = CompCtx.top();
  // The index bounds are diagnosed before the export name. A valid name
  // upgrades a bounds error to a per-sort code.
  {
    const auto &SI = Ex.getSortIndex();
    Component::ExternName CN;
    const bool NameOk = CN.parse(Ex.getName()).has_value();
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
  EXPECTED_TRY(auto Inferred, CompCtx.resolveSortIndex(Ex.getSortIndex()));
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
    if (CompTypes.containsBorrow(VE.Type)) {
      spdlog::error(ErrCode::Value::InvalidTypeReference);
      spdlog::error("    Exported value types cannot contain borrows."sv);
      return Unexpect(ErrCode::Value::InvalidTypeReference);
    }
  }
  EXPECTED_TRY(
      Component::ExternName CN,
      CompCtx.registerExportName(
          Ex.getName(), Inferred.K == Component::ExternKind::InstanceType,
          Ex.getImplements(), Ex.getExternalIds(), Ex.getVersionSuffixes()));
  std::optional<Component::ExternInfo> Ascribed;
  if (Ex.getDesc().has_value()) {
    EXPECTED_TRY(validate(*Ex.getDesc(), false, Ascribed.emplace()));
  }
  EXPECTED_TRY(auto Result, CompCtx.defineExport(CN, Inferred, Ascribed));
  // The re-exported value index is born consumed.
  if (Result.K == Component::ExternKind::ValueBound) {
    CompCtx.top().Values.back().Consumed = true;
  }
  Out.Exports.emplace(std::string(Ex.getName()), Result);
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
    Component::ValueDecoder<decltype(Resolver)> Decoder(Value.getData(),
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

} // namespace Validator
} // namespace WasmEdge
