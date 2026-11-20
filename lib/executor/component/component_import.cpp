// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "executor/component/executor.h"

#include "common/errinfo.h"
#include "common/spdlog.h"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

using namespace std::literals;

namespace WasmEdge {
namespace Executor {

namespace {
Expect<void> logMatchError(ErrCode::Value Code, std::string_view Sort,
                           std::string_view Name, std::string_view Provider) {
  spdlog::error(Code);
  spdlog::error(ErrInfo::InfoComponentLinking(Sort, Name, Provider));
  spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Import));
  return Unexpect(Code);
}
// Whether the actual core module provides the declared module type: imports
// contravariant, exports covariant. FindOuter names the type of an alias.
Expect<void> matchCoreModuleType(
    const AST::Module &Actual, Span<const AST::Component::CoreModuleDecl> Decls,
    const std::function<const AST::SubType *(uint32_t, uint32_t)> &FindOuter,
    std::string_view Provider) {
  // The type index space of the module type, and its declared imports.
  std::vector<const AST::SubType *> DeclTypes;
  std::map<std::pair<std::string_view, std::string_view>,
           const AST::Component::CoreImportDesc *>
      DeclImports;
  for (const auto &Decl : Decls) {
    if (Decl.isType()) {
      for (const auto &SubTy : Decl.getType()->getSubTypes()) {
        DeclTypes.push_back(&SubTy);
      }
    } else if (Decl.isAlias()) {
      // An aliased type sits alone in this index space, so it must be a
      // function type that names no other type.
      const auto &Alias = Decl.getAlias();
      const AST::SubType *SubTy = nullptr;
      if (Alias.getComponentJump() > 0) {
        SubTy = FindOuter(Alias.getComponentJump() - 1, Alias.getIndex());
      } else if (Alias.getIndex() < DeclTypes.size()) {
        SubTy = DeclTypes[Alias.getIndex()];
      }
      auto HasTypeIndex = [](const ValType &Ty) {
        return Ty.isRefType() && !Ty.isAbsHeapType();
      };
      bool IsAlone = SubTy != nullptr &&
                     SubTy->getRecursiveInfo().RecTypeSize == 1 &&
                     SubTy->getSuperTypeIndices().empty() &&
                     SubTy->getCompositeType().isFunc();
      if (IsAlone) {
        const auto &Func = SubTy->getCompositeType().getFuncType();
        IsAlone = std::none_of(Func.getParamTypes().begin(),
                               Func.getParamTypes().end(), HasTypeIndex) &&
                  std::none_of(Func.getReturnTypes().begin(),
                               Func.getReturnTypes().end(), HasTypeIndex);
      }
      if (!IsAlone) {
        return logMatchError(ErrCode::Value::ComponentCoreModWrongType,
                             "core type"sv, std::to_string(Alias.getIndex()),
                             Provider);
      }
      DeclTypes.push_back(SubTy);
    } else if (Decl.isImport()) {
      const auto &Import = Decl.getImport();
      DeclImports.emplace(
          std::make_pair(Import.getModuleName(), Import.getName()),
          &Import.getImportDesc());
    }
  }

  // The typed view of the actual module: its imports, then its definitions.
  std::vector<const AST::SubType *> ActualTypes;
  for (const auto &SubTy : Actual.getTypeSection().getContent()) {
    ActualTypes.push_back(&SubTy);
  }
  std::vector<uint32_t> FuncTypeIdxs;
  std::vector<const AST::GlobalType *> Globals;
  std::vector<const AST::TableType *> Tables;
  std::vector<const AST::MemoryType *> Memories;
  std::vector<const AST::TagType *> Tags;
  auto GetFuncType = [](Span<const AST::SubType *const> Types,
                        uint32_t Idx) -> const AST::FunctionType * {
    return Idx < Types.size() && Types[Idx]->getCompositeType().isFunc()
               ? &Types[Idx]->getCompositeType().getFuncType()
               : nullptr;
  };
  auto GetLimit = [](const AST::Limit &Lim) {
    return std::make_tuple(Lim.hasMax(), Lim.getMin(), Lim.getMax());
  };

  // Match a declared entry against the actual entry of its kind at Idx. The
  // module expects its imports, and the module type expects the exports.
  auto MatchEntry = [&](const AST::Component::CoreImportDesc &Decl,
                        uint32_t Idx, bool IsImport) -> bool {
    Span<const AST::SubType *const> ExpTypes = DeclTypes;
    Span<const AST::SubType *const> GotTypes = ActualTypes;
    if (IsImport) {
      std::swap(ExpTypes, GotTypes);
    }
    switch (Decl.getExternalType()) {
    case ExternalType::Function: {
      if (Idx >= FuncTypeIdxs.size()) {
        return false;
      }
      uint32_t ExpIdx = Decl.getTypeIndex();
      uint32_t GotIdx = FuncTypeIdxs[Idx];
      if (IsImport) {
        std::swap(ExpIdx, GotIdx);
      }
      const auto *ExpFunc = GetFuncType(ExpTypes, ExpIdx);
      const auto *GotFunc = GetFuncType(GotTypes, GotIdx);
      if (ExpFunc == nullptr || GotFunc == nullptr) {
        return false;
      }
      const bool Match =
          AST::TypeMatcher::matchType(ExpTypes, ExpIdx, GotTypes, GotIdx);
      if (!Match) {
        spdlog::error(ErrInfo::InfoMismatch(
            ExpFunc->getParamTypes(), ExpFunc->getReturnTypes(),
            GotFunc->getParamTypes(), GotFunc->getReturnTypes()));
      }
      return Match;
    }
    case ExternalType::Tag: {
      if (Idx >= Tags.size()) {
        return false;
      }
      const AST::TagType *Exp = &Decl.getTagType();
      const AST::TagType *Got = Tags[Idx];
      if (IsImport) {
        std::swap(Exp, Got);
      }
      return AST::TypeMatcher::matchType(ExpTypes, *Exp, GotTypes, *Got);
    }
    case ExternalType::Global: {
      if (Idx >= Globals.size()) {
        return false;
      }
      const AST::GlobalType *Exp = &Decl.getGlobalType();
      const AST::GlobalType *Got = Globals[Idx];
      if (IsImport) {
        std::swap(Exp, Got);
      }
      const bool Match =
          AST::TypeMatcher::matchType(ExpTypes, *Exp, GotTypes, *Got);
      if (!Match) {
        spdlog::error(ErrInfo::InfoMismatch(Exp->getValType(), Exp->getValMut(),
                                            Got->getValType(),
                                            Got->getValMut()));
      }
      return Match;
    }
    case ExternalType::Table: {
      if (Idx >= Tables.size()) {
        return false;
      }
      const AST::TableType *Exp = &Decl.getTableType();
      const AST::TableType *Got = Tables[Idx];
      if (IsImport) {
        std::swap(Exp, Got);
      }
      const bool Match =
          AST::TypeMatcher::matchType(ExpTypes, *Exp, GotTypes, *Got);
      if (!Match) {
        const auto [ExpHasMax, ExpMin, ExpMax] = GetLimit(Exp->getLimit());
        const auto [GotHasMax, GotMin, GotMax] = GetLimit(Got->getLimit());
        spdlog::error(ErrInfo::InfoMismatch(Exp->getRefType(), ExpHasMax,
                                            ExpMin, ExpMax, Got->getRefType(),
                                            GotHasMax, GotMin, GotMax));
      }
      return Match;
    }
    case ExternalType::Memory: {
      if (Idx >= Memories.size()) {
        return false;
      }
      const AST::MemoryType *Exp = &Decl.getMemoryType();
      const AST::MemoryType *Got = Memories[Idx];
      if (IsImport) {
        std::swap(Exp, Got);
      }
      const bool Match =
          AST::TypeMatcher::matchLimit(Exp->getLimit(), Got->getLimit());
      if (!Match) {
        const auto [ExpHasMax, ExpMin, ExpMax] = GetLimit(Exp->getLimit());
        const auto [GotHasMax, GotMin, GotMax] = GetLimit(Got->getLimit());
        spdlog::error(ErrInfo::InfoMismatch(ExpHasMax, ExpMin, ExpMax,
                                            GotHasMax, GotMin, GotMax));
      }
      return Match;
    }
    default:
      assumingUnreachable();
    }
  };

  // Every actual import must match a declared import's type.
  for (const auto &Imp : Actual.getImportSection().getContent()) {
    const std::string Name = std::string(Imp.getModuleName()) +
                             "::" + std::string(Imp.getExternalName());
    uint32_t Idx = 0;
    switch (Imp.getExternalType()) {
    case ExternalType::Function:
      Idx = static_cast<uint32_t>(FuncTypeIdxs.size());
      FuncTypeIdxs.push_back(Imp.getExternalFuncTypeIdx());
      break;
    case ExternalType::Global:
      Idx = static_cast<uint32_t>(Globals.size());
      Globals.push_back(&Imp.getExternalGlobalType());
      break;
    case ExternalType::Table:
      Idx = static_cast<uint32_t>(Tables.size());
      Tables.push_back(&Imp.getExternalTableType());
      break;
    case ExternalType::Memory:
      Idx = static_cast<uint32_t>(Memories.size());
      Memories.push_back(&Imp.getExternalMemoryType());
      break;
    case ExternalType::Tag:
      Idx = static_cast<uint32_t>(Tags.size());
      Tags.push_back(&Imp.getExternalTagType());
      break;
    default:
      assumingUnreachable();
    }
    auto Iter = DeclImports.find(
        std::make_pair(Imp.getModuleName(), Imp.getExternalName()));
    if (Iter == DeclImports.end()) {
      return logMatchError(ErrCode::Value::ComponentCoreModNotDefined,
                           "core import"sv, Name, Provider);
    }
    if (Iter->second->getExternalType() != Imp.getExternalType()) {
      spdlog::error(ErrInfo::InfoMismatch(Iter->second->getExternalType(),
                                          Imp.getExternalType()));
      return logMatchError(ErrCode::Value::ComponentCoreModKindMismatch,
                           "core import"sv, Name, Provider);
    }
    if (!MatchEntry(*Iter->second, Idx, /*IsImport=*/true)) {
      return logMatchError(ErrCode::Value::ComponentCoreModWrongType,
                           "core import"sv, Name, Provider);
    }
  }
  for (const auto &Func : Actual.getFunctionSection().getContent()) {
    FuncTypeIdxs.push_back(Func);
  }
  for (const auto &Glob : Actual.getGlobalSection().getContent()) {
    Globals.push_back(&Glob.getGlobalType());
  }
  for (const auto &Tab : Actual.getTableSection().getContent()) {
    Tables.push_back(&Tab.getTableType());
  }
  for (const auto &Mem : Actual.getMemorySection().getContent()) {
    Memories.push_back(&Mem);
  }
  for (const auto &Tag : Actual.getTagSection().getContent()) {
    Tags.push_back(&Tag);
  }

  // Every declared export must be exported with a matching type.
  std::map<std::string_view, std::pair<ExternalType, uint32_t>> ActualExports;
  for (const auto &Exp : Actual.getExportSection().getContent()) {
    ActualExports.emplace(
        Exp.getExternalName(),
        std::make_pair(Exp.getExternalType(), Exp.getExternalIndex()));
  }
  for (const auto &Decl : Decls) {
    if (!Decl.isExport()) {
      continue;
    }
    const std::string_view Name = Decl.getExport().getName();
    const auto &Desc = Decl.getExport().getImportDesc();
    auto Iter = ActualExports.find(Name);
    if (Iter == ActualExports.end()) {
      return logMatchError(ErrCode::Value::ComponentCoreModNotDefined,
                           "core export"sv, Name, Provider);
    }
    if (Iter->second.first != Desc.getExternalType()) {
      spdlog::error(
          ErrInfo::InfoMismatch(Desc.getExternalType(), Iter->second.first));
      return logMatchError(ErrCode::Value::ComponentCoreModKindMismatch,
                           "core export"sv, Name, Provider);
    }
    if (!MatchEntry(Desc, Iter->second.second, /*IsImport=*/false)) {
      return logMatchError(ErrCode::Value::ComponentCoreModWrongType,
                           "core export"sv, Name, Provider);
    }
  }
  return {};
}
} // namespace

// Log an import or alias the provider lacks. See
// "include/executor/component/executor.h".
Expect<void> ComponentExecutor::logUnknownError(std::string_view Sort,
                                                std::string_view Name,
                                                std::string_view Provider,
                                                ASTNodeAttr Node) {
  spdlog::error(ErrCode::Value::ComponentImportNotFound);
  spdlog::error(ErrInfo::InfoComponentLinking(Sort, Name, Provider));
  spdlog::error(ErrInfo::InfoAST(Node));
  return Unexpect(ErrCode::Value::ComponentImportNotFound);
}
// Instantiate imports from the store. See
// "include/executor/component/executor.h".
Expect<void>
ComponentExecutor::instantiate(Runtime::Component::StoreManager &StoreMgr,
                               Runtime::Instance::ComponentInstance &CompInst,
                               const AST::Component::ImportSection &ImportSec) {
  for (const auto &Imp : ImportSec.getContent()) {
    const auto &Desc = Imp.getDesc();
    const std::string_view Name = Imp.getName();
    switch (Desc.getDescType()) {
    case AST::Component::ExternDesc::DescType::InstanceType: {
      const auto *Inst = StoreMgr.findInstance(Name);
      if (Inst == nullptr) {
        return logUnknownError("instance"sv, Name, ""sv,
                               ASTNodeAttr::Comp_Import);
      }
      // The instance provides what the instance type declares, read in the
      // instance that owns the type.
      EXPECTED_TRY(const auto *TypeDef,
                   CompInst.getTypeDefinition(Desc.getTypeIndex()));
      if (TypeDef->Def != nullptr && TypeDef->Def->isInstanceType()) {
        EXPECTED_TRY(matchInstanceType(*Inst, TypeDef->Def->getInstanceType(),
                                       *TypeDef->Owner, {}));
      }
      // The provider stays alive while this instance imports from it.
      CompInst.addDependency(
          *const_cast<Runtime::Instance::ComponentInstance *>(Inst));
      CompInst.addComponentInstance(Inst);
      break;
    }
    case AST::Component::ExternDesc::DescType::FuncType: {
      auto *Func = StoreMgr.findFunction(Name);
      if (Func == nullptr) {
        return logUnknownError("func"sv, Name, ""sv, ASTNodeAttr::Comp_Import);
      }
      // The instance of the function stays alive, and its store runs, while
      // this instance imports from it.
      if (auto *Provider = Func->getComponentInstance(); Provider != nullptr) {
        CompInst.addDependency(*Provider);
      }
      CompInst.addFunction(Func);
      break;
    }
    case AST::Component::ExternDesc::DescType::ComponentType: {
      const auto *Def = StoreMgr.findDefinition(Name);
      if (Def == nullptr) {
        return logUnknownError("component"sv, Name, ""sv,
                               ASTNodeAttr::Comp_Import);
      }
      // The definition is instantiated as it is, so it must be validated.
      if (unlikely(!Def->getIsValidated())) {
        spdlog::error(ErrCode::Value::NotValidated);
        spdlog::error(ErrInfo::InfoComponentLinking("component"sv, Name));
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Import));
        return Unexpect(ErrCode::Value::NotValidated);
      }
      CompInst.addComponentDefinition({Def, nullptr});
      break;
    }
    case AST::Component::ExternDesc::DescType::TypeBound: {
      if (Desc.isEqType()) {
        EXPECTED_TRY(const auto *TypeDef,
                     CompInst.getTypeDefinition(Desc.getTypeIndex()));
        CompInst.addTypeDefinition(*TypeDef);
      } else {
        // An abstract resource import: the host mints an opaque resource.
        CompInst.addHostResourceType(std::function<void(uint64_t)>());
      }
      break;
    }
    case AST::Component::ExternDesc::DescType::ValueBound:
      return logUnknownError("value"sv, Name, ""sv, ASTNodeAttr::Comp_Import);
    case AST::Component::ExternDesc::DescType::CoreType:
      return logUnknownError("core module"sv, Name, ""sv,
                             ASTNodeAttr::Comp_Import);
    default:
      spdlog::error(ErrCode::Value::ComponentUnexpectedSort);
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Import));
      return Unexpect(ErrCode::Value::ComponentUnexpectedSort);
    }
  }
  return {};
}

// Instantiate imports from the arguments. See
// "include/executor/component/executor.h".
Expect<void> ComponentExecutor::instantiate(
    const Runtime::Instance::ComponentInstance &Provider,
    Runtime::Instance::ComponentInstance &CompInst,
    const AST::Component::ImportSection &ImportSec) {
  const std::string_view From = Provider.getComponentName();
  for (const auto &Imp : ImportSec.getContent()) {
    const auto &Desc = Imp.getDesc();
    const std::string_view Name = Imp.getName();
    switch (Desc.getDescType()) {
    case AST::Component::ExternDesc::DescType::InstanceType: {
      const auto *Inst = Provider.findComponentInstance(Name);
      if (Inst == nullptr) {
        return logUnknownError("instance"sv, Name, From,
                               ASTNodeAttr::Comp_Import);
      }
      CompInst.addComponentInstance(Inst);
      break;
    }
    case AST::Component::ExternDesc::DescType::FuncType: {
      auto *Func = Provider.findFunction(Name);
      if (Func == nullptr) {
        return logUnknownError("func"sv, Name, From, ASTNodeAttr::Comp_Import);
      }
      CompInst.addFunction(Func);
      break;
    }
    case AST::Component::ExternDesc::DescType::ComponentType: {
      const auto *Def = Provider.findComponentDefinition(Name);
      if (Def == nullptr) {
        return logUnknownError("component"sv, Name, From,
                               ASTNodeAttr::Comp_Import);
      }
      CompInst.addComponentDefinition(*Def);
      break;
    }
    case AST::Component::ExternDesc::DescType::TypeBound: {
      const auto *TypeDef = Provider.findTypeDefinition(Name);
      if (TypeDef == nullptr) {
        return logUnknownError("type"sv, Name, From, ASTNodeAttr::Comp_Import);
      }
      if (!Desc.isEqType() && TypeDef->Resource == nullptr) {
        return logMatchError(ErrCode::Value::ComponentImportExpectedResource,
                             "type"sv, Name, From);
      }
      CompInst.addTypeDefinition(*TypeDef);
      break;
    }
    case AST::Component::ExternDesc::DescType::ValueBound: {
      const auto *Val = Provider.findValue(Name);
      if (Val == nullptr) {
        return logUnknownError("value"sv, Name, From, ASTNodeAttr::Comp_Import);
      }
      CompInst.addValue(*Val);
      break;
    }
    case AST::Component::ExternDesc::DescType::CoreType: {
      const auto *Mod = Provider.findCoreModule(Name);
      if (Mod == nullptr) {
        return logUnknownError("core module"sv, Name, From,
                               ASTNodeAttr::Comp_Import);
      }
      CompInst.addModule(*Mod);
      break;
    }
    default:
      spdlog::error(ErrCode::Value::ComponentUnexpectedSort);
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Import));
      return Unexpect(ErrCode::Value::ComponentUnexpectedSort);
    }
  }
  return {};
}

// Match an instance against an instance type. See
// "include/executor/component/executor.h".
Expect<void> ComponentExecutor::matchInstanceType(
    const Runtime::Instance::ComponentInstance &Inst,
    const AST::Component::InstanceType &Ty,
    const Runtime::Instance::ComponentInstance &TypeInst,
    Span<const std::pair<const AST::Component::InstanceType *,
                         const Runtime::Instance::ComponentInstance *>>
        Outer) {
  const std::string_view From = Inst.getComponentName();
  // The instance types an index of Ty resolves through, Ty innermost.
  std::vector<std::pair<const AST::Component::InstanceType *,
                        const Runtime::Instance::ComponentInstance *>>
      Scopes(Outer.begin(), Outer.end());
  Scopes.emplace_back(&Ty, &Inst);

  // The type at Idx of the innermost Depth of Scopes. Depth is left at the
  // scopes its declaration sits in, none for a type of a component.
  auto GetType = [&Scopes, &TypeInst](size_t &Depth, uint32_t Idx)
      -> Runtime::Instance::Component::ComponentTypeDefinition {
    uint32_t Count = 0;
    while (Count < Depth) {
      Depth -= Count;
      Count = 0;
      const auto &[ScopeTy, ScopeInst] = Scopes[Depth - 1];
      const AST::Component::InstanceDecl *Found = nullptr;
      uint32_t Num = 0;
      for (const auto &Decl : ScopeTy->getDecl()) {
        const bool IsType =
            Decl.isType() ||
            (Decl.isAlias() && !Decl.getAlias().getSort().isCore() &&
             Decl.getAlias().getSort().getSortType() ==
                 AST::Component::Sort::SortType::Type) ||
            (Decl.isExportDecl() &&
             Decl.getExport().getExternDesc().getDescType() ==
                 AST::Component::ExternDesc::DescType::TypeBound);
        if (IsType && Num++ == Idx) {
          Found = &Decl;
          break;
        }
      }
      if (Found == nullptr) {
        return {};
      }
      if (Found->isType()) {
        return {Found->getType(), nullptr, nullptr};
      }
      if (Found->isExportDecl()) {
        // An eq-constrained export is the type it names; a resource export is
        // the resource the instance provides.
        const auto &Exp = Found->getExport();
        if (Exp.getExternDesc().isEqType()) {
          Idx = Exp.getExternDesc().getTypeIndex();
          continue;
        }
        const auto *TypeDef = ScopeInst->findTypeDefinition(Exp.getName());
        return TypeDef != nullptr
                   ? *TypeDef
                   : Runtime::Instance::Component::ComponentTypeDefinition();
      }
      const auto &Alias = Found->getAlias();
      if (Alias.getTargetType() != AST::Component::Alias::TargetType::Outer) {
        return {};
      }
      Count = Alias.getOuter().first;
      Idx = Alias.getOuter().second;
    }
    // Past the instance types: the component of TypeInst, then its parents.
    const Runtime::Instance::ComponentInstance *Target = &TypeInst;
    for (Count -= static_cast<uint32_t>(Depth); Count > 0 && Target != nullptr;
         --Count) {
      Target = Target->getParent();
    }
    Depth = 0;
    if (Target != nullptr) {
      if (auto TypeDef = Target->getTypeDefinition(Idx)) {
        return **TypeDef;
      }
    }
    return {};
  };
  // The same for the core type Count scopes out: null when it names none.
  auto GetCoreType =
      [&Scopes, &TypeInst](
          size_t &Depth, uint32_t Count,
          uint32_t Idx) -> std::variant<const AST::SubType *,
                                        const AST::Component::CoreDefType *> {
    while (Count < Depth) {
      Depth -= Count;
      Count = 0;
      const AST::Component::Alias *Found = nullptr;
      uint32_t Num = 0;
      for (const auto &Decl : Scopes[Depth - 1].first->getDecl()) {
        if (Decl.isCoreType()) {
          // A module type takes one index, a recursive group one per type.
          const auto *CoreTy = Decl.getCoreType();
          if (CoreTy->isModuleType()) {
            if (Num++ == Idx) {
              return CoreTy;
            }
            continue;
          }
          const auto SubTypes = CoreTy->getSubTypes();
          if (Idx - Num < SubTypes.size()) {
            return &SubTypes[Idx - Num];
          }
          Num += static_cast<uint32_t>(SubTypes.size());
        } else if (Decl.isAlias() && Decl.getAlias().getSort().isCore() &&
                   Decl.getAlias().getSort().getCoreSortType() ==
                       AST::Component::Sort::CoreSortType::Type &&
                   Num++ == Idx) {
          Found = &Decl.getAlias();
          break;
        }
      }
      if (Found == nullptr ||
          Found->getTargetType() != AST::Component::Alias::TargetType::Outer) {
        return {};
      }
      Count = Found->getOuter().first;
      Idx = Found->getOuter().second;
    }
    const Runtime::Instance::ComponentInstance *Target = &TypeInst;
    for (Count -= static_cast<uint32_t>(Depth); Count > 0 && Target != nullptr;
         --Count) {
      Target = Target->getParent();
    }
    Depth = 0;
    if (Target != nullptr) {
      if (auto CoreTy = Target->getCoreType(Idx)) {
        return *CoreTy;
      }
    }
    return {};
  };

  for (const auto &Decl : Ty.getDecl()) {
    if (!Decl.isExportDecl()) {
      continue;
    }
    const auto &Exp = Decl.getExport();
    const std::string_view Name = Exp.getName();
    const auto &Desc = Exp.getExternDesc();
    switch (Desc.getDescType()) {
    case AST::Component::ExternDesc::DescType::FuncType:
      if (Inst.findFunction(Name) != nullptr) {
        break;
      }
      if (Inst.findComponentInstance(Name) != nullptr) {
        return logMatchError(ErrCode::Value::ComponentImportExpectedFunc,
                             "func"sv, Name, From);
      }
      return logUnknownError("func"sv, Name, From, ASTNodeAttr::Comp_Import);
    case AST::Component::ExternDesc::DescType::InstanceType: {
      const auto *Nested = Inst.findComponentInstance(Name);
      if (Nested == nullptr) {
        if (Inst.findFunction(Name) != nullptr) {
          return logMatchError(ErrCode::Value::ComponentImportExpectedInstance,
                               "instance"sv, Name, From);
        }
        return logUnknownError("instance"sv, Name, From,
                               ASTNodeAttr::Comp_Import);
      }
      // The nested instance provides what its own instance type declares,
      // matched in the scopes that type is declared in.
      size_t Depth = Scopes.size();
      const auto Entry = GetType(Depth, Desc.getTypeIndex());
      if (Entry.Def == nullptr || !Entry.Def->isInstanceType()) {
        break;
      }
      EXPECTED_TRY(
          matchInstanceType(*Nested, Entry.Def->getInstanceType(),
                            Entry.Owner != nullptr ? *Entry.Owner : TypeInst,
                            {Scopes.data(), Depth}));
      break;
    }
    case AST::Component::ExternDesc::DescType::TypeBound: {
      const auto *TypeDef = Inst.findTypeDefinition(Name);
      if (!Desc.isEqType()) {
        if (TypeDef == nullptr) {
          return logUnknownError("type"sv, Name, From,
                                 ASTNodeAttr::Comp_Import);
        }
        if (TypeDef->Resource == nullptr) {
          return logMatchError(ErrCode::Value::ComponentImportExpectedResource,
                               "type"sv, Name, From);
        }
        break;
      }
      // An eq-constrained type export is determined by its constraint; one
      // the instance provides must carry the very resource it names.
      if (TypeDef != nullptr) {
        size_t Depth = Scopes.size();
        const auto *Expected = GetType(Depth, Desc.getTypeIndex()).Resource;
        if (Expected != nullptr && TypeDef->Resource != Expected) {
          return logMatchError(ErrCode::Value::ComponentResourceMismatched,
                               "resource"sv, Name, From);
        }
      }
      break;
    }
    case AST::Component::ExternDesc::DescType::ValueBound:
      if (Inst.findValue(Name) == nullptr) {
        return logUnknownError("value"sv, Name, From, ASTNodeAttr::Comp_Import);
      }
      break;
    case AST::Component::ExternDesc::DescType::ComponentType:
      if (Inst.findComponentDefinition(Name) == nullptr) {
        return logUnknownError("component"sv, Name, From,
                               ASTNodeAttr::Comp_Import);
      }
      break;
    case AST::Component::ExternDesc::DescType::CoreType: {
      const auto *Mod = Inst.findCoreModule(Name);
      if (Mod == nullptr) {
        if (Inst.findComponentInstance(Name) != nullptr) {
          return logMatchError(ErrCode::Value::ComponentImportExpectedModule,
                               "core module"sv, Name, From);
        }
        return logUnknownError("core module"sv, Name, From,
                               ASTNodeAttr::Comp_Import);
      }
      // The module provides what its declared module type names; an alias
      // in it counts from the scope the module type is declared in.
      size_t Depth = Scopes.size();
      const auto CoreTy = GetCoreType(Depth, 0, Desc.getTypeIndex());
      const auto *ModTy =
          std::get_if<const AST::Component::CoreDefType *>(&CoreTy);
      if (ModTy == nullptr) {
        spdlog::error(ErrCode::Value::ComponentUnexpectedType);
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Import));
        return Unexpect(ErrCode::Value::ComponentUnexpectedType);
      }
      EXPECTED_TRY(matchCoreModuleType(
          *Mod, (*ModTy)->getModuleType(),
          [&GetCoreType, Depth](uint32_t Count,
                                uint32_t Idx) -> const AST::SubType * {
            size_t Reach = Depth;
            const auto Aliased = GetCoreType(Reach, Count, Idx);
            const auto *SubTy = std::get_if<const AST::SubType *>(&Aliased);
            return SubTy != nullptr ? *SubTy : nullptr;
          },
          From));
      break;
    }
    default:
      break;
    }
  }
  return {};
}

} // namespace Executor
} // namespace WasmEdge
