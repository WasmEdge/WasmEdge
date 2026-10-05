// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "executor/component/executor.h"

#include "common/errinfo.h"
#include "common/spdlog.h"

#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
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
// Whether the actual core module provides what the declared module type
// requires: imports match contravariantly, exports covariantly.
Expect<void>
matchCoreModuleType(const AST::Module &Actual,
                    Span<const AST::Component::CoreModuleDecl> Decls,
                    std::string_view Provider) {
  // The core types the module type declares, and its imports and exports.
  std::vector<const AST::SubType *> DeclTypes;
  std::map<std::pair<std::string, std::string>,
           const AST::Component::CoreImportDesc *>
      DeclImports;
  std::vector<std::pair<std::string, const AST::Component::CoreImportDesc *>>
      DeclExports;
  for (const auto &Decl : Decls) {
    if (Decl.isType()) {
      const auto *CoreType = Decl.getType();
      if (CoreType != nullptr && CoreType->isRecType()) {
        for (const auto &SubTy : CoreType->getSubTypes()) {
          DeclTypes.push_back(&SubTy);
        }
      }
    } else if (Decl.isImport()) {
      const auto &Import = Decl.getImport();
      DeclImports.emplace(std::make_pair(std::string(Import.getModuleName()),
                                         std::string(Import.getName())),
                          &Import.getImportDesc());
    } else if (Decl.isExport()) {
      const auto &Export = Decl.getExport();
      DeclExports.emplace_back(std::string(Export.getName()),
                               &Export.getImportDesc());
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
  auto getFuncType = [](Span<const AST::SubType *const> Types,
                        uint32_t Idx) -> const AST::FunctionType * {
    return Idx < Types.size() ? &Types[Idx]->getCompositeType().getFuncType()
                              : nullptr;
  };
  auto getLimit = [](const AST::Limit &Lim) {
    return std::make_tuple(Lim.hasMax(), Lim.getMin(), Lim.getMax());
  };

  // Match a declared entry against the actual entry of its kind at Idx.
  auto MatchEntry = [&](const AST::Component::CoreImportDesc *Decl,
                        uint32_t Idx, bool IsImport) -> bool {
    switch (Decl->getExternalType()) {
    case ExternalType::Function: {
      const uint32_t DeclIdx = Decl->getTypeIndex();
      const auto *DeclFunc = getFuncType(DeclTypes, DeclIdx);
      const auto *ActualFunc = Idx < FuncTypeIdxs.size()
                                   ? getFuncType(ActualTypes, FuncTypeIdxs[Idx])
                                   : nullptr;
      if (DeclFunc == nullptr || ActualFunc == nullptr) {
        return false;
      }
      const bool Match =
          IsImport ? AST::TypeMatcher::matchType(ActualTypes, FuncTypeIdxs[Idx],
                                                 DeclTypes, DeclIdx)
                   : AST::TypeMatcher::matchType(
                         DeclTypes, DeclIdx, ActualTypes, FuncTypeIdxs[Idx]);
      if (!Match) {
        spdlog::error(ErrInfo::InfoMismatch(
            DeclFunc->getParamTypes(), DeclFunc->getReturnTypes(),
            ActualFunc->getParamTypes(), ActualFunc->getReturnTypes()));
      }
      return Match;
    }
    case ExternalType::Global: {
      const auto &DeclGlob = Decl->getGlobalType();
      const auto *ActualGlob = Idx < Globals.size() ? Globals[Idx] : nullptr;
      if (ActualGlob == nullptr) {
        return false;
      }
      const bool Match =
          DeclGlob.getValMut() == ActualGlob->getValMut() &&
          (IsImport
               ? AST::TypeMatcher::matchType(ActualTypes,
                                             ActualGlob->getValType(),
                                             DeclTypes, DeclGlob.getValType())
               : AST::TypeMatcher::matchType(DeclTypes, DeclGlob.getValType(),
                                             ActualTypes,
                                             ActualGlob->getValType()));
      if (!Match) {
        spdlog::error(ErrInfo::InfoMismatch(
            DeclGlob.getValType(), DeclGlob.getValMut(),
            ActualGlob->getValType(), ActualGlob->getValMut()));
      }
      return Match;
    }
    case ExternalType::Table: {
      const auto &DeclTab = Decl->getTableType();
      const auto *ActualTab = Idx < Tables.size() ? Tables[Idx] : nullptr;
      if (ActualTab == nullptr) {
        return false;
      }
      const bool Match =
          (IsImport
               ? AST::TypeMatcher::matchType(ActualTypes,
                                             ActualTab->getRefType(), DeclTypes,
                                             DeclTab.getRefType()) &&
                     AST::TypeMatcher::matchLimit(ActualTab->getLimit(),
                                                  DeclTab.getLimit())
               : AST::TypeMatcher::matchType(DeclTypes, DeclTab.getRefType(),
                                             ActualTypes,
                                             ActualTab->getRefType()) &&
                     AST::TypeMatcher::matchLimit(DeclTab.getLimit(),
                                                  ActualTab->getLimit()));
      if (!Match) {
        const auto [DeclHasMax, DeclMin, DeclMax] =
            getLimit(DeclTab.getLimit());
        const auto [ActualHasMax, ActualMin, ActualMax] =
            getLimit(ActualTab->getLimit());
        spdlog::error(ErrInfo::InfoMismatch(
            DeclTab.getRefType(), DeclHasMax, DeclMin, DeclMax,
            ActualTab->getRefType(), ActualHasMax, ActualMin, ActualMax));
      }
      return Match;
    }
    case ExternalType::Memory: {
      const auto &DeclMem = Decl->getMemoryType();
      const auto *ActualMem = Idx < Memories.size() ? Memories[Idx] : nullptr;
      if (ActualMem == nullptr) {
        return false;
      }
      const bool Match =
          IsImport ? AST::TypeMatcher::matchLimit(ActualMem->getLimit(),
                                                  DeclMem.getLimit())
                   : AST::TypeMatcher::matchLimit(DeclMem.getLimit(),
                                                  ActualMem->getLimit());
      if (!Match) {
        const auto [DeclHasMax, DeclMin, DeclMax] =
            getLimit(DeclMem.getLimit());
        const auto [ActualHasMax, ActualMin, ActualMax] =
            getLimit(ActualMem->getLimit());
        spdlog::error(ErrInfo::InfoMismatch(
            DeclHasMax, DeclMin, DeclMax, ActualHasMax, ActualMin, ActualMax));
      }
      return Match;
    }
    default:
      return true;
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
    default:
      break;
    }
    auto Iter = DeclImports.find(std::make_pair(
        std::string(Imp.getModuleName()), std::string(Imp.getExternalName())));
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
    if (!MatchEntry(Iter->second, Idx, /*IsImport=*/true)) {
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

  // Every declared export must be exported with a matching type.
  std::map<std::string, std::pair<ExternalType, uint32_t>> ActualExports;
  for (const auto &Exp : Actual.getExportSection().getContent()) {
    ActualExports.emplace(
        std::string(Exp.getExternalName()),
        std::make_pair(Exp.getExternalType(), Exp.getExternalIndex()));
  }
  for (const auto &[Name, Decl] : DeclExports) {
    auto Iter = ActualExports.find(Name);
    if (Iter == ActualExports.end()) {
      return logMatchError(ErrCode::Value::ComponentCoreModNotDefined,
                           "core export"sv, Name, Provider);
    }
    if (Iter->second.first != Decl->getExternalType()) {
      spdlog::error(
          ErrInfo::InfoMismatch(Decl->getExternalType(), Iter->second.first));
      return logMatchError(ErrCode::Value::ComponentCoreModKindMismatch,
                           "core export"sv, Name, Provider);
    }
    if (!MatchEntry(Decl, Iter->second.second, /*IsImport=*/false)) {
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
      // The host instance provides what the instance type declares.
      EXPECTED_TRY(const auto *Def, CompInst.getType(Desc.getTypeIndex()));
      if (Def != nullptr && Def->isInstanceType()) {
        EXPECTED_TRY(
            matchInstanceType(*Inst, Def->getInstanceType(), CompInst));
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

// Match a host instance against an instance type. See
// "include/executor/component/executor.h".
Expect<void> ComponentExecutor::matchInstanceType(
    const Runtime::Instance::ComponentInstance &Inst,
    const AST::Component::InstanceType &Ty,
    const Runtime::Instance::ComponentInstance &Importing) {
  const std::string_view From = Inst.getComponentName();
  // The index spaces the instance type's own declarations build up; an
  // eq-constrained export names its type through them.
  std::vector<Runtime::Instance::Component::ComponentTypeDefinition> Local;
  std::vector<const AST::Component::CoreDefType *> LocalCore;
  for (const auto &Decl : Ty.getDecl()) {
    if (Decl.isCoreType()) {
      LocalCore.push_back(Decl.getCoreType());
      continue;
    }
    if (Decl.isType()) {
      Local.push_back({Decl.getType(), nullptr, nullptr});
      continue;
    }
    if (Decl.isAlias()) {
      // An outer type alias resolves through the instantiating component.
      const auto &Alias = Decl.getAlias();
      const auto &Sort = Alias.getSort();
      if (Sort.isCore()) {
        if (Sort.getCoreSortType() ==
            AST::Component::Sort::CoreSortType::Type) {
          LocalCore.push_back(nullptr);
        }
        continue;
      }
      if (Sort.getSortType() != AST::Component::Sort::SortType::Type) {
        continue;
      }
      Runtime::Instance::Component::ComponentTypeDefinition Entry;
      if (Alias.getTargetType() == AST::Component::Alias::TargetType::Outer) {
        const auto &Outer = Alias.getOuter();
        const Runtime::Instance::ComponentInstance *Target = &Importing;
        for (uint32_t I = 1; I < Outer.first && Target != nullptr; ++I) {
          Target = Target->getParent();
        }
        if (Target != nullptr) {
          if (auto TypeDef = Target->getTypeDefinition(Outer.second)) {
            Entry = **TypeDef;
          }
        }
      }
      Local.push_back(Entry);
      continue;
    }
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
    case AST::Component::ExternDesc::DescType::InstanceType:
      if (Inst.findComponentInstance(Name) != nullptr) {
        break;
      }
      if (Inst.findFunction(Name) != nullptr) {
        return logMatchError(ErrCode::Value::ComponentImportExpectedInstance,
                             "instance"sv, Name, From);
      }
      return logUnknownError("instance"sv, Name, From,
                             ASTNodeAttr::Comp_Import);
    case AST::Component::ExternDesc::DescType::TypeBound: {
      const auto *TypeDef = Inst.findTypeDefinition(Name);
      if (TypeDef == nullptr) {
        // An eq-constrained type export is determined by its constraint.
        if (Desc.isEqType()) {
          const uint32_t Idx = Desc.getTypeIndex();
          Local.push_back(
              Idx < Local.size()
                  ? Local[Idx]
                  : Runtime::Instance::Component::ComponentTypeDefinition{});
          break;
        }
        return logUnknownError("type"sv, Name, From, ASTNodeAttr::Comp_Import);
      }
      if (!Desc.isEqType() && TypeDef->Resource == nullptr) {
        return logMatchError(ErrCode::Value::ComponentImportExpectedResource,
                             "type"sv, Name, From);
      }
      if (Desc.isEqType()) {
        // The export must carry the very resource the constraint names.
        const uint32_t Idx = Desc.getTypeIndex();
        const auto *Expected =
            Idx < Local.size() ? Local[Idx].Resource : nullptr;
        if (Expected != nullptr && TypeDef->Resource != Expected) {
          return logMatchError(ErrCode::Value::ComponentResourceMismatched,
                               "resource"sv, Name, From);
        }
      }
      Local.push_back(*TypeDef);
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
      const uint32_t Idx = Desc.getTypeIndex();
      const auto *CoreType = Idx < LocalCore.size() ? LocalCore[Idx] : nullptr;
      if (CoreType != nullptr && CoreType->isModuleType()) {
        EXPECTED_TRY(
            matchCoreModuleType(*Mod, CoreType->getModuleType(), From));
      }
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
