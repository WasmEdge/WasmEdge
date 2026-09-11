// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "executor/component/executor.h"
#include "executor/executor.h"

#include "common/errinfo.h"
#include "common/spdlog.h"

#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace std::literals;

namespace WasmEdge {
namespace Executor {

namespace {

// An import the embedder or the instantiation arguments do not satisfy.
Expect<void> logImportError(ErrCode::Value Code, std::string_view Sort,
                            std::string_view Name,
                            std::string_view Provider = {}) noexcept {
  spdlog::error(Code);
  spdlog::error(ErrInfo::InfoComponentLinking(Sort, Name, Provider));
  spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Import));
  return Unexpect(Code);
}

// Deep core-module shape check over the declared import and export types.
Expect<void>
matchModuleShape(const AST::Module &Actual,
                 Span<const AST::Component::CoreModuleDecl> Decls) {
  // The core types declared inside the module type, and its imports and
  // exports by name.
  std::vector<const AST::SubType *> DeclTypes;
  struct DeclEntry {
    ExternalType Kind;
    const AST::Component::CoreImportDesc *Desc;
  };
  std::map<std::pair<std::string, std::string>, DeclEntry> DeclImports;
  std::vector<std::pair<std::string, DeclEntry>> DeclExports;
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
                          DeclEntry{Import.getImportDesc().getExternalType(),
                                    &Import.getImportDesc()});
    } else if (Decl.isExport()) {
      const auto &Export = Decl.getExport();
      DeclExports.emplace_back(
          std::string(Export.getName()),
          DeclEntry{Export.getImportDesc().getExternalType(),
                    &Export.getImportDesc()});
    }
  }

  // The typed view of the actual module: its type section, then its imports
  // followed by its definitions in every index space.
  std::vector<const AST::SubType *> ActualTypes;
  for (const auto &SubTy : Actual.getTypeSection().getContent()) {
    ActualTypes.push_back(&SubTy);
  }
  std::vector<uint32_t> FuncTypeIdxs;
  std::vector<const AST::GlobalType *> Globals;
  std::vector<const AST::TableType *> Tables;
  std::vector<const AST::MemoryType *> Memories;
  auto FuncTypeOf = [](Span<const AST::SubType *const> Types,
                       uint32_t Idx) -> const AST::FunctionType * {
    return Idx < Types.size() ? &Types[Idx]->getCompositeType().getFuncType()
                              : nullptr;
  };
  auto LimitOf = [](const AST::Limit &Lim) {
    return std::make_tuple(Lim.hasMax(), Lim.getMin(), Lim.getMax());
  };

  // Match a declared entry against the actual entry of its kind at Idx. A
  // declared import matches contravariantly, a declared export covariantly.
  auto MatchEntry = [&](const DeclEntry &Decl, uint32_t Idx,
                        bool IsImport) -> bool {
    switch (Decl.Kind) {
    case ExternalType::Function: {
      const uint32_t DeclIdx = Decl.Desc->getTypeIndex();
      const auto *DeclFunc = FuncTypeOf(DeclTypes, DeclIdx);
      const auto *ActualFunc = Idx < FuncTypeIdxs.size()
                                   ? FuncTypeOf(ActualTypes, FuncTypeIdxs[Idx])
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
      const auto &DeclGlob = Decl.Desc->getGlobalType();
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
      const auto &DeclTab = Decl.Desc->getTableType();
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
        const auto [DeclHasMax, DeclMin, DeclMax] = LimitOf(DeclTab.getLimit());
        const auto [ActualHasMax, ActualMin, ActualMax] =
            LimitOf(ActualTab->getLimit());
        spdlog::error(ErrInfo::InfoMismatch(
            DeclTab.getRefType(), DeclHasMax, DeclMin, DeclMax,
            ActualTab->getRefType(), ActualHasMax, ActualMin, ActualMax));
      }
      return Match;
    }
    case ExternalType::Memory: {
      const auto &DeclMem = Decl.Desc->getMemoryType();
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
        const auto [DeclHasMax, DeclMin, DeclMax] = LimitOf(DeclMem.getLimit());
        const auto [ActualHasMax, ActualMin, ActualMax] =
            LimitOf(ActualMem->getLimit());
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
      return logImportError(ErrCode::Value::ComponentCoreModNotDefined,
                            "core import"sv, Name);
    }
    if (Iter->second.Kind != Imp.getExternalType()) {
      spdlog::error(
          ErrInfo::InfoMismatch(Iter->second.Kind, Imp.getExternalType()));
      return logImportError(ErrCode::Value::ComponentCoreModKindMismatch,
                            "core import"sv, Name);
    }
    if (!MatchEntry(Iter->second, Idx, /*IsImport=*/true)) {
      return logImportError(ErrCode::Value::ComponentCoreModWrongType,
                            "core import"sv, Name);
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
      return logImportError(ErrCode::Value::ComponentCoreModNotDefined,
                            "core export"sv, Name);
    }
    if (Iter->second.first != Decl.Kind) {
      spdlog::error(ErrInfo::InfoMismatch(Decl.Kind, Iter->second.first));
      return logImportError(ErrCode::Value::ComponentCoreModKindMismatch,
                            "core export"sv, Name);
    }
    if (!MatchEntry(Decl, Iter->second.second, /*IsImport=*/false)) {
      return logImportError(ErrCode::Value::ComponentCoreModWrongType,
                            "core export"sv, Name);
    }
  }
  return {};
}

// Runtime instance-shape check over exports, resources and module types.
Expect<void>
matchInstanceShape(const Runtime::Instance::ComponentInstance &Provided,
                   const AST::Component::InstanceType &Shape,
                   const Runtime::Instance::ComponentInstance &Importing) {
  struct ShapeTy {
    const AST::Component::DefType *Def = nullptr;
    const Runtime::Instance::Component::ResourceTypeInstance *Resource =
        nullptr;
  };
  std::vector<ShapeTy> Local;
  std::vector<const AST::Component::CoreDefType *> LocalCore;
  const auto Provider = Provided.getComponentName();
  for (const auto &Decl : Shape.getDecl()) {
    if (Decl.isCoreType()) {
      LocalCore.push_back(Decl.getCoreType());
      continue;
    }
    if (Decl.isType()) {
      Local.push_back({Decl.getType(), nullptr});
      continue;
    }
    if (Decl.isAlias()) {
      const auto &Alias = Decl.getAlias();
      const auto &Sort = Alias.getSort();
      if (!Sort.isCore() &&
          Sort.getSortType() == AST::Component::Sort::SortType::Type &&
          Alias.getTargetType() == AST::Component::Alias::TargetType::Outer) {
        const auto &Outer = Alias.getOuter();
        const Runtime::Instance::ComponentInstance *Target = &Importing;
        for (uint32_t I = 1; I < Outer.first && Target != nullptr; ++I) {
          Target = Target->getParent();
        }
        ShapeTy Entry;
        if (Target != nullptr) {
          if (auto TypeDef = Target->getTypeDefinition(Outer.second)) {
            Entry = {(*TypeDef)->Def, (*TypeDef)->Resource};
          }
        }
        Local.push_back(Entry);
      } else if (!Sort.isCore() &&
                 Sort.getSortType() == AST::Component::Sort::SortType::Type) {
        Local.push_back({});
      } else if (Sort.isCore() &&
                 Sort.getCoreSortType() ==
                     AST::Component::Sort::CoreSortType::Type) {
        LocalCore.push_back(nullptr);
      }
      continue;
    }
    if (!Decl.isExportDecl()) {
      continue;
    }
    const auto &ExportDecl = Decl.getExport();
    const auto Name = ExportDecl.getName();
    const auto &Desc = ExportDecl.getExternDesc();
    switch (Desc.getDescType()) {
    case AST::Component::ExternDesc::DescType::FuncType:
      if (Provided.findFunction(Name) == nullptr) {
        return logImportError(ErrCode::Value::ComponentImportNotFound,
                              "function"sv, Name, Provider);
      }
      break;
    case AST::Component::ExternDesc::DescType::TypeBound: {
      const auto *TypeDef = Provided.findTypeDefinition(Name);
      if (TypeDef == nullptr) {
        if (Desc.isEqType()) {
          // An eq-constrained type export resolves through the constraint.
          const uint32_t Idx = Desc.getTypeIndex();
          Local.push_back(Idx < Local.size() ? Local[Idx] : ShapeTy{});
          break;
        }
        if (Provided.findFunction(Name) != nullptr) {
          return logImportError(ErrCode::Value::ComponentImportExpectedResource,
                                "resource"sv, Name, Provider);
        }
        return logImportError(ErrCode::Value::ComponentImportNotFound, "type"sv,
                              Name, Provider);
      }
      if (Desc.isEqType()) {
        const uint32_t Idx = Desc.getTypeIndex();
        const ShapeTy Expected = Idx < Local.size() ? Local[Idx] : ShapeTy{};
        if (Expected.Resource != nullptr &&
            TypeDef->Resource != Expected.Resource) {
          return logImportError(ErrCode::Value::ComponentResourceMismatched,
                                "resource"sv, Name, Provider);
        }
      }
      Local.push_back({TypeDef->Def, TypeDef->Resource});
      break;
    }
    case AST::Component::ExternDesc::DescType::InstanceType:
      if (Provided.findComponentInstance(Name) == nullptr) {
        return logImportError(ErrCode::Value::ComponentImportNotFound,
                              "instance"sv, Name, Provider);
      }
      break;
    case AST::Component::ExternDesc::DescType::CoreType: {
      const auto *Mod = Provided.findCoreModule(Name);
      if (Mod == nullptr && Provided.findCoreModuleInstance(Name) == nullptr) {
        return logImportError(ErrCode::Value::ComponentImportNotFound,
                              "core module"sv, Name, Provider);
      }
      const uint32_t Idx = Desc.getTypeIndex();
      const auto *CoreType = Idx < LocalCore.size() ? LocalCore[Idx] : nullptr;
      if (Mod != nullptr && CoreType != nullptr && CoreType->isModuleType()) {
        EXPECTED_TRY(matchModuleShape(*Mod, CoreType->getModuleType()));
      }
      break;
    }
    case AST::Component::ExternDesc::DescType::ComponentType:
      if (Provided.findComponentDefinition(Name) == nullptr) {
        return logImportError(ErrCode::Value::ComponentImportNotFound,
                              "component"sv, Name, Provider);
      }
      break;
    default:
      break;
    }
  }
  return {};
}

} // namespace

// Pick the resolution side; the two differ in sorts and diagnostics.
Expect<void>
ComponentExecutor::instantiate(Component::Instantiator &Ctx,
                               const AST::Component::ImportSection &ImportSec) {
  if (Ctx.isRoot()) {
    return instantiate(Ctx.getStore(), Ctx, ImportSec);
  }
  return instantiate(Ctx.getImportManager(), Ctx, ImportSec);
}

// Instantiate the import section from the store. See executor.h.
Expect<void>
ComponentExecutor::instantiate(Runtime::Component::StoreManager &StoreMgr,
                               Component::Instantiator &Ctx,
                               const AST::Component::ImportSection &ImportSec) {
  auto &CompInst = Ctx.getInstance();
  for (const auto &Import : ImportSec.getContent()) {
    const auto &Desc = Import.getDesc();
    const auto Name = Import.getName();
    switch (Desc.getDescType()) {
    case AST::Component::ExternDesc::DescType::TypeBound: {
      // An abstract resource import mints a fresh opaque host identity.
      if (Desc.isEqType()) {
        EXPECTED_TRY(const auto *TypeDef,
                     CompInst.getTypeDefinition(Desc.getTypeIndex()));
        CompInst.addTypeDefinition(*TypeDef);
        break;
      }
      CompInst.addHostResourceType(nullptr);
      break;
    }
    case AST::Component::ExternDesc::DescType::FuncType: {
      if (auto *HostFunc = StoreMgr.findFunction(Name)) {
        Ctx.importFunction(HostFunc);
        break;
      }
      if (StoreMgr.findInstance(Name) != nullptr) {
        return logImportError(ErrCode::Value::ComponentImportExpectedFunc,
                              "function"sv, Name);
      }
      return logImportError(ErrCode::Value::ComponentImportNotFound,
                            "function"sv, Name);
    }
    case AST::Component::ExternDesc::DescType::CoreType:
      if (StoreMgr.findInstance(Name) != nullptr) {
        return logImportError(ErrCode::Value::ComponentImportExpectedModule,
                              "core module"sv, Name);
      }
      return logImportError(ErrCode::Value::ComponentImportNotFound,
                            "core module"sv, Name);
    case AST::Component::ExternDesc::DescType::ComponentType:
      if (const auto *Def = StoreMgr.findDefinition(Name)) {
        // A registered standalone definition closes over no environment.
        CompInst.addComponentDefinition({Def, nullptr});
        break;
      }
      return logImportError(ErrCode::Value::ComponentImportNotFound,
                            "component"sv, Name);
    case AST::Component::ExternDesc::DescType::ValueBound:
      // No host-side providers for values.
      return logImportError(ErrCode::Value::ComponentImportNotFound, "value"sv,
                            Name);
    case AST::Component::ExternDesc::DescType::InstanceType: {
      const auto *ImportedCompInst = StoreMgr.findInstance(Name);
      if (unlikely(ImportedCompInst == nullptr)) {
        if (StoreMgr.findFunction(Name) != nullptr) {
          return logImportError(ErrCode::Value::ComponentImportExpectedInstance,
                                "instance"sv, Name);
        }
        return logImportError(ErrCode::Value::ComponentImportNotFound,
                              "instance"sv, Name);
      }
      // Check the provided instance against the declared shape.
      EXPECTED_TRY(const auto *Def, CompInst.getType(Desc.getTypeIndex()));
      if (Def != nullptr && Def->isInstanceType()) {
        EXPECTED_TRY(matchInstanceShape(*ImportedCompInst,
                                        Def->getInstanceType(), CompInst));
      }
      Ctx.importComponentInstance(ImportedCompInst);
      break;
    }
    default:
      assumingUnreachable();
    }
  }
  return {};
}

// Instantiate the import section from the arguments. See executor.h.
Expect<void>
ComponentExecutor::instantiate(Runtime::Component::ImportManager &ImportMgr,
                               Component::Instantiator &Ctx,
                               const AST::Component::ImportSection &ImportSec) {
  auto &CompInst = Ctx.getInstance();
  for (const auto &Import : ImportSec.getContent()) {
    const auto &Desc = Import.getDesc();
    const auto Name = Import.getName();
    switch (Desc.getDescType()) {
    case AST::Component::ExternDesc::DescType::FuncType: {
      auto *Func = ImportMgr.findFunction(Name);
      if (unlikely(Func == nullptr)) {
        return logImportError(ErrCode::Value::UnknownImport, "function"sv,
                              Name);
      }
      Ctx.importFunction(Func);
      break;
    }
    case AST::Component::ExternDesc::DescType::TypeBound: {
      // Type imports take the argument type, else the eq-bound target.
      if (const auto *TypeDef = ImportMgr.findTypeDefinition(Name)) {
        CompInst.addTypeDefinition(*TypeDef);
        break;
      }
      if (Desc.isEqType()) {
        EXPECTED_TRY(const auto *TypeDef,
                     CompInst.getTypeDefinition(Desc.getTypeIndex()));
        CompInst.addTypeDefinition(*TypeDef);
        break;
      }
      CompInst.addTypeDefinition({});
      break;
    }
    case AST::Component::ExternDesc::DescType::ComponentType: {
      const auto *CompDef = ImportMgr.findComponentDefinition(Name);
      if (unlikely(CompDef == nullptr)) {
        return logImportError(ErrCode::Value::UnknownImport, "component"sv,
                              Name);
      }
      CompInst.addComponentDefinition(*CompDef);
      break;
    }
    case AST::Component::ExternDesc::DescType::CoreType: {
      const auto *Mod = ImportMgr.findCoreModule(Name);
      if (unlikely(Mod == nullptr)) {
        return logImportError(ErrCode::Value::UnknownImport, "core module"sv,
                              Name);
      }
      CompInst.addModule(*Mod);
      break;
    }
    case AST::Component::ExternDesc::DescType::ValueBound: {
      const auto *Val = ImportMgr.findValue(Name);
      if (unlikely(Val == nullptr)) {
        return logImportError(ErrCode::Value::UnknownImport, "value"sv, Name);
      }
      Ctx.addValue(*Val);
      break;
    }
    case AST::Component::ExternDesc::DescType::InstanceType: {
      const auto *ImportedCompInst = ImportMgr.findComponentInstance(Name);
      if (unlikely(ImportedCompInst == nullptr)) {
        return logImportError(ErrCode::Value::UnknownImport, "instance"sv,
                              Name);
      }
      // Check the provided instance against the declared shape.
      EXPECTED_TRY(const auto *Def, CompInst.getType(Desc.getTypeIndex()));
      if (Def != nullptr && Def->isInstanceType()) {
        EXPECTED_TRY(matchInstanceShape(*ImportedCompInst,
                                        Def->getInstanceType(), CompInst));
      }
      Ctx.importComponentInstance(ImportedCompInst);
      break;
    }
    default:
      assumingUnreachable();
    }
  }
  return {};
}

} // namespace Executor
} // namespace WasmEdge
