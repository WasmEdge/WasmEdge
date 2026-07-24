// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "executor/executor.h"

#include "common/errinfo.h"
#include "common/spdlog.h"

#include <string_view>

namespace WasmEdge {
namespace Executor {

namespace {
using ResourceTypeRT = Runtime::Instance::ComponentInstance::ResourceTypeRT;
using namespace std::literals;

bool matchCoreLimits(bool ExpHasMax, uint64_t ExpMin, uint64_t ExpMax,
                     bool GotHasMax, uint64_t GotMin, uint64_t GotMax) {
  if (GotMin < ExpMin) {
    return false;
  }
  if (ExpHasMax && (!GotHasMax || GotMax > ExpMax)) {
    return false;
  }
  return true;
}

// Deep core-module shape check: every actual import must be declared with a
// compatible type, and every declared export must be provided with a
// compatible type.
Expect<void>
matchModuleShape(const AST::Module &Actual,
                 Span<const AST::Component::CoreModuleDecl> Decls) {
  using DescKind = ExternalType;
  // Local core function types declared inside the module type.
  std::vector<const AST::FunctionType *> LocalFuncTypes;
  struct DeclEntry {
    DescKind Kind;
    const AST::Component::CoreImportDesc *Desc;
  };
  std::map<std::pair<std::string, std::string>, DeclEntry> DeclImports;
  std::vector<std::pair<std::string, DeclEntry>> DeclExports;
  auto DescKindOf = [](const AST::Component::CoreImportDesc &D) {
    if (D.isFunc()) {
      return ExternalType::Function;
    }
    if (D.isTable()) {
      return ExternalType::Table;
    }
    if (D.isMemory()) {
      return ExternalType::Memory;
    }
    if (D.isGlobal()) {
      return ExternalType::Global;
    }
    return ExternalType::Tag;
  };
  for (const auto &D : Decls) {
    if (D.isType()) {
      const auto *CT = D.getType();
      if (CT != nullptr && CT->isRecType()) {
        for (const auto &ST : CT->getSubTypes()) {
          LocalFuncTypes.push_back(&ST.getCompositeType().getFuncType());
        }
      }
    } else if (D.isImport()) {
      const auto &ID = D.getImport();
      DeclImports.emplace(
          std::make_pair(std::string(ID.getModuleName()),
                         std::string(ID.getName())),
          DeclEntry{DescKindOf(ID.getImportDesc()), &ID.getImportDesc()});
    } else if (D.isExport()) {
      const auto &ED = D.getExport();
      DeclExports.emplace_back(
          std::string(ED.getName()),
          DeclEntry{DescKindOf(ED.getImportDesc()), &ED.getImportDesc()});
    }
  }
  auto DeclFuncType = [&](const AST::Component::CoreImportDesc &D)
      -> const AST::FunctionType * {
    const uint32_t Idx = D.getTypeIndex();
    return Idx < LocalFuncTypes.size() ? LocalFuncTypes[Idx] : nullptr;
  };

  // Resolve the actual module's typed view.
  const auto &Types = Actual.getTypeSection().getContent();
  auto ActualFuncType = [&](uint32_t TypeIdx) -> const AST::FunctionType * {
    if (TypeIdx < Types.size()) {
      return &Types[TypeIdx].getCompositeType().getFuncType();
    }
    return nullptr;
  };
  struct ActualItem {
    ExternalType Kind;
    const AST::FunctionType *FT = nullptr;
    const AST::GlobalType *GT = nullptr;
    const AST::TableType *TT = nullptr;
    const AST::MemoryType *MT = nullptr;
  };
  std::vector<ActualItem> Funcs, Globals, Tables, Memories;
  for (const auto &Imp : Actual.getImportSection().getContent()) {
    // Every actual import must appear in the declared imports with a
    // compatible type.
    auto It = DeclImports.find(std::make_pair(
        std::string(Imp.getModuleName()), std::string(Imp.getExternalName())));
    if (It == DeclImports.end()) {
      spdlog::error(ErrCode::Value::ComponentCoreModNotDefined);
      spdlog::error("    module import `{}::{}` not defined"sv,
                    Imp.getModuleName(), Imp.getExternalName());
      return Unexpect(ErrCode::Value::ComponentCoreModNotDefined);
    }
    if (It->second.Kind != Imp.getExternalType()) {
      spdlog::error(ErrCode::Value::ComponentCoreModKindMismatch);
      spdlog::error("    module import `{}::{}` kind mismatch"sv,
                    Imp.getModuleName(), Imp.getExternalName());
      return Unexpect(ErrCode::Value::ComponentCoreModKindMismatch);
    }
    bool TypeOk = true;
    switch (Imp.getExternalType()) {
    case ExternalType::Function: {
      const auto *DF = DeclFuncType(*It->second.Desc);
      const auto *AF = ActualFuncType(Imp.getExternalFuncTypeIdx());
      TypeOk = DF != nullptr && AF != nullptr &&
               DF->getParamTypes() == AF->getParamTypes() &&
               DF->getReturnTypes() == AF->getReturnTypes();
      break;
    }
    case ExternalType::Global: {
      const auto &DG = It->second.Desc->getGlobalType();
      const auto &AG = Imp.getExternalGlobalType();
      TypeOk = DG.getValType() == AG.getValType() &&
               DG.getValMut() == AG.getValMut();
      break;
    }
    case ExternalType::Table: {
      const auto &DTb = It->second.Desc->getTableType();
      const auto &ATb = Imp.getExternalTableType();
      TypeOk =
          DTb.getRefType() == ATb.getRefType() &&
          matchCoreLimits(ATb.getLimit().hasMax(), ATb.getLimit().getMin(),
                          ATb.getLimit().getMax(), DTb.getLimit().hasMax(),
                          DTb.getLimit().getMin(), DTb.getLimit().getMax());
      break;
    }
    case ExternalType::Memory: {
      const auto &DMm = It->second.Desc->getMemoryType();
      const auto &AMm = Imp.getExternalMemoryType();
      TypeOk =
          matchCoreLimits(AMm.getLimit().hasMax(), AMm.getLimit().getMin(),
                          AMm.getLimit().getMax(), DMm.getLimit().hasMax(),
                          DMm.getLimit().getMin(), DMm.getLimit().getMax());
      break;
    }
    default:
      break;
    }
    if (!TypeOk) {
      spdlog::error(ErrCode::Value::ComponentCoreModWrongType);
      spdlog::error("    module import `{}::{}` has the wrong type"sv,
                    Imp.getModuleName(), Imp.getExternalName());
      return Unexpect(ErrCode::Value::ComponentCoreModWrongType);
    }
    switch (Imp.getExternalType()) {
    case ExternalType::Function:
      Funcs.push_back({ExternalType::Function,
                       ActualFuncType(Imp.getExternalFuncTypeIdx()), nullptr,
                       nullptr, nullptr});
      break;
    case ExternalType::Global:
      Globals.push_back({ExternalType::Global, nullptr,
                         &Imp.getExternalGlobalType(), nullptr, nullptr});
      break;
    case ExternalType::Table:
      Tables.push_back({ExternalType::Table, nullptr, nullptr,
                        &Imp.getExternalTableType(), nullptr});
      break;
    case ExternalType::Memory:
      Memories.push_back({ExternalType::Memory, nullptr, nullptr, nullptr,
                          &Imp.getExternalMemoryType()});
      break;
    default:
      break;
    }
  }
  for (const auto &F : Actual.getFunctionSection().getContent()) {
    Funcs.push_back(
        {ExternalType::Function, ActualFuncType(F), nullptr, nullptr, nullptr});
  }
  for (const auto &G : Actual.getGlobalSection().getContent()) {
    Globals.push_back(
        {ExternalType::Global, nullptr, &G.getGlobalType(), nullptr, nullptr});
  }
  for (const auto &T : Actual.getTableSection().getContent()) {
    Tables.push_back(
        {ExternalType::Table, nullptr, nullptr, &T.getTableType(), nullptr});
  }
  for (const auto &M : Actual.getMemorySection().getContent()) {
    Memories.push_back({ExternalType::Memory, nullptr, nullptr, nullptr, &M});
  }
  // Actual export table by name.
  std::map<std::string, std::pair<ExternalType, uint32_t>> ActualExports;
  for (const auto &Exp : Actual.getExportSection().getContent()) {
    ActualExports.emplace(
        std::string(Exp.getExternalName()),
        std::make_pair(Exp.getExternalType(), Exp.getExternalIndex()));
  }
  for (const auto &[Name, Decl] : DeclExports) {
    auto It = ActualExports.find(Name);
    if (It == ActualExports.end()) {
      spdlog::error(ErrCode::Value::ComponentCoreModNotDefined);
      spdlog::error("    module export `{}` not defined"sv, Name);
      return Unexpect(ErrCode::Value::ComponentCoreModNotDefined);
    }
    if (It->second.first != Decl.Kind) {
      spdlog::error(ErrCode::Value::ComponentCoreModKindMismatch);
      spdlog::error("    module export `{}` kind mismatch"sv, Name);
      return Unexpect(ErrCode::Value::ComponentCoreModKindMismatch);
    }
    const uint32_t Idx = It->second.second;
    bool TypeOk = true;
    switch (Decl.Kind) {
    case ExternalType::Function: {
      const auto *DF = DeclFuncType(*Decl.Desc);
      const auto *AF = Idx < Funcs.size() ? Funcs[Idx].FT : nullptr;
      TypeOk = DF != nullptr && AF != nullptr &&
               DF->getParamTypes() == AF->getParamTypes() &&
               DF->getReturnTypes() == AF->getReturnTypes();
      break;
    }
    case ExternalType::Global: {
      const auto *AG = Idx < Globals.size() ? Globals[Idx].GT : nullptr;
      const auto &DG = Decl.Desc->getGlobalType();
      TypeOk = AG != nullptr && DG.getValType() == AG->getValType() &&
               DG.getValMut() == AG->getValMut();
      break;
    }
    case ExternalType::Table: {
      const auto *AT = Idx < Tables.size() ? Tables[Idx].TT : nullptr;
      const auto &DT = Decl.Desc->getTableType();
      TypeOk =
          AT != nullptr && DT.getRefType() == AT->getRefType() &&
          matchCoreLimits(DT.getLimit().hasMax(), DT.getLimit().getMin(),
                          DT.getLimit().getMax(), AT->getLimit().hasMax(),
                          AT->getLimit().getMin(), AT->getLimit().getMax());
      break;
    }
    case ExternalType::Memory: {
      const auto *AM = Idx < Memories.size() ? Memories[Idx].MT : nullptr;
      const auto &DM = Decl.Desc->getMemoryType();
      TypeOk =
          AM != nullptr &&
          matchCoreLimits(DM.getLimit().hasMax(), DM.getLimit().getMin(),
                          DM.getLimit().getMax(), AM->getLimit().hasMax(),
                          AM->getLimit().getMin(), AM->getLimit().getMax());
      break;
    }
    default:
      break;
    }
    if (!TypeOk) {
      spdlog::error(ErrCode::Value::ComponentCoreModWrongType);
      spdlog::error("    module export `{}` has the wrong type"sv, Name);
      return Unexpect(ErrCode::Value::ComponentCoreModWrongType);
    }
  }
  return {};
}

// Runtime instance-shape check: exports must exist with compatible kinds,
// resource identities must line up with eq-bounds, and core-module exports
// must satisfy their declared module type.
// NOLINTNEXTLINE(misc-no-recursion)
Expect<void>
matchInstanceShape(const Runtime::Instance::ComponentInstance &Provided,
                   const AST::Component::InstanceType &Shape,
                   const Runtime::Instance::ComponentInstance &Importing) {
  using DescType = AST::Component::ExternDesc::DescType;
  struct ShapeTy {
    const AST::Component::DefType *DT = nullptr;
    const ResourceTypeRT *RT = nullptr;
  };
  std::vector<ShapeTy> Local;
  std::vector<const AST::Component::CoreDefType *> LocalCore;
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
      const auto &A = Decl.getAlias();
      const auto &Sort = A.getSort();
      if (!Sort.isCore() &&
          Sort.getSortType() == AST::Component::Sort::SortType::Type &&
          A.getTargetType() == AST::Component::Alias::TargetType::Outer) {
        const auto &Outer = A.getOuter();
        const Runtime::Instance::ComponentInstance *Target = &Importing;
        for (uint32_t I = 1; I < Outer.first && Target != nullptr; ++I) {
          Target = Target->getParent();
        }
        if (Target != nullptr) {
          Local.push_back({Target->getType(Outer.second),
                           Target->getTypeResource(Outer.second)});
        } else {
          Local.push_back({});
        }
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
    const auto &ED = Decl.getExport();
    const auto Name = ED.getName();
    const auto &Desc = ED.getExternDesc();
    switch (Desc.getDescType()) {
    case DescType::FuncType:
      if (Provided.findFunction(Name) == nullptr) {
        spdlog::error(ErrCode::Value::ComponentImportNotFound);
        spdlog::error("    instance export `{}` was not found"sv, Name);
        return Unexpect(ErrCode::Value::ComponentImportNotFound);
      }
      break;
    case DescType::TypeBound: {
      const auto *RT = Provided.findTypeResource(Name);
      const auto *DT = Provided.findType(Name);
      if (RT == nullptr && DT == nullptr) {
        if (Desc.isEqType()) {
          // Equality-constrained type exports need not be supplied; the
          // binding resolves through the constraint.
          const uint32_t Idx = Desc.getTypeIndex();
          Local.push_back(Idx < Local.size() ? Local[Idx] : ShapeTy{});
          break;
        }
        if (Provided.findFunction(Name) != nullptr) {
          spdlog::error(ErrCode::Value::ComponentImportExpectedResource);
          spdlog::error("    instance export `{}`: expected resource"sv, Name);
          return Unexpect(ErrCode::Value::ComponentImportExpectedResource);
        }
        spdlog::error(ErrCode::Value::ComponentImportNotFound);
        spdlog::error("    instance export `{}` was not found"sv, Name);
        return Unexpect(ErrCode::Value::ComponentImportNotFound);
      }
      if (Desc.isEqType()) {
        const uint32_t Idx = Desc.getTypeIndex();
        const ShapeTy Exp = Idx < Local.size() ? Local[Idx] : ShapeTy{};
        if (Exp.RT != nullptr && RT != Exp.RT) {
          spdlog::error(ErrCode::Value::ComponentResourceMismatched);
          spdlog::error("    instance export `{}`: mismatched resource "
                        "types"sv,
                        Name);
          return Unexpect(ErrCode::Value::ComponentResourceMismatched);
        }
      }
      Local.push_back({DT, RT});
      break;
    }
    case DescType::InstanceType: {
      const auto *Nested = Provided.findComponentInstance(Name);
      if (Nested == nullptr) {
        spdlog::error(ErrCode::Value::ComponentImportNotFound);
        spdlog::error("    instance export `{}` was not found"sv, Name);
        return Unexpect(ErrCode::Value::ComponentImportNotFound);
      }
      // The nested shape index space is component-level; resolve it in the
      // importing component's view when available.
      break;
    }
    case DescType::CoreType: {
      const auto *Mod = Provided.findCoreModule(Name);
      if (Mod == nullptr && Provided.findCoreModuleInstance(Name) == nullptr) {
        spdlog::error(ErrCode::Value::ComponentImportNotFound);
        spdlog::error("    instance export `{}` was not found"sv, Name);
        return Unexpect(ErrCode::Value::ComponentImportNotFound);
      }
      const uint32_t Idx = Desc.getTypeIndex();
      const auto *CT = Idx < LocalCore.size() ? LocalCore[Idx] : nullptr;
      if (Mod != nullptr && CT != nullptr && CT->isModuleType()) {
        EXPECTED_TRY(matchModuleShape(*Mod, CT->getModuleType()));
      }
      break;
    }
    case DescType::ComponentType:
      if (Provided.findComponentEntry(Name) == nullptr) {
        spdlog::error(ErrCode::Value::ComponentImportNotFound);
        spdlog::error("    instance export `{}` was not found"sv, Name);
        return Unexpect(ErrCode::Value::ComponentImportNotFound);
      }
      break;
    default:
      break;
    }
  }
  return {};
}
} // namespace

using namespace std::literals;

Expect<void>
Executor::instantiate(Runtime::StoreManager &StoreMgr,
                      Runtime::Instance::ComponentInstance &CompInst,
                      const AST::Component::ImportSection &ImportSec) {
  for (const auto &Import : ImportSec.getContent()) {
    const auto &Desc = Import.getDesc();
    switch (Desc.getDescType()) {
    case AST::Component::ExternDesc::DescType::TypeBound:
      // Type imports fill the type index space: eq-bound targets are used,
      // and an abstract resource import mints a fresh opaque host resource
      // identity (abstract types have no store-provider concept).
      if (Desc.isEqType()) {
        CompInst.addTypeWithResource(
            CompInst.getType(Desc.getTypeIndex()),
            CompInst.getTypeResource(Desc.getTypeIndex()));
        break;
      }
      CompInst.addHostResourceType(nullptr);
      break;
    case AST::Component::ExternDesc::DescType::FuncType: {
      if (auto *HostFunc = StoreMgr.findComponentFunction(Import.getName())) {
        CompInst.addFunction(HostFunc);
        break;
      }
      if (StoreMgr.findComponent(Import.getName()) != nullptr) {
        spdlog::error(ErrCode::Value::ComponentImportExpectedFunc);
        spdlog::error("    import name: {}"sv, Import.getName());
        return Unexpect(ErrCode::Value::ComponentImportExpectedFunc);
      }
      spdlog::error(ErrCode::Value::ComponentImportNotFound);
      spdlog::error("    import `{}` was not found"sv, Import.getName());
      return Unexpect(ErrCode::Value::ComponentImportNotFound);
    }
    case AST::Component::ExternDesc::DescType::CoreType:
      if (StoreMgr.findComponent(Import.getName()) != nullptr) {
        spdlog::error(ErrCode::Value::ComponentImportExpectedModule);
        spdlog::error("    import name: {}"sv, Import.getName());
        return Unexpect(ErrCode::Value::ComponentImportExpectedModule);
      }
      spdlog::error(ErrCode::Value::ComponentImportNotFound);
      spdlog::error("    import `{}` was not found"sv, Import.getName());
      return Unexpect(ErrCode::Value::ComponentImportNotFound);
    case AST::Component::ExternDesc::DescType::ComponentType:
      if (const auto *Def =
              StoreMgr.findComponentDefinition(Import.getName())) {
        // A registered standalone definition closes over no environment.
        CompInst.addComponentEntry(Def, nullptr);
        break;
      }
      [[fallthrough]];
    case AST::Component::ExternDesc::DescType::ValueBound:
      // No host-side providers for these sorts.
      spdlog::error(ErrCode::Value::ComponentImportNotFound);
      spdlog::error("    import `{}` was not found"sv, Import.getName());
      return Unexpect(ErrCode::Value::ComponentImportNotFound);
    case AST::Component::ExternDesc::DescType::InstanceType: {
      auto CompName = Import.getName();
      const auto *ImportedCompInst = StoreMgr.findComponent(CompName);
      if (unlikely(ImportedCompInst == nullptr)) {
        if (StoreMgr.findComponentFunction(CompName) != nullptr) {
          spdlog::error(ErrCode::Value::ComponentImportExpectedInstance);
          spdlog::error("    import name: {}"sv, CompName);
          return Unexpect(ErrCode::Value::ComponentImportExpectedInstance);
        }
        spdlog::error(ErrCode::Value::ComponentImportNotFound);
        spdlog::error("    import `{}` was not found"sv, CompName);
        return Unexpect(ErrCode::Value::ComponentImportNotFound);
      }
      // Check the provided instance against the declared shape.
      if (const auto *DT = CompInst.getType(Desc.getTypeIndex());
          DT != nullptr && DT->isInstanceType()) {
        EXPECTED_TRY(matchInstanceShape(*ImportedCompInst,
                                        DT->getInstanceType(), CompInst));
      }
      CompInst.addComponentInstance(ImportedCompInst);
      break;
    }
    default:
      assumingUnreachable();
    }
  }
  return {};
}

Expect<void>
Executor::instantiate(Runtime::Instance::ComponentImportManager &ImportMgr,
                      Runtime::Instance::ComponentInstance &CompInst,
                      const AST::Component::ImportSection &ImportSec) {
  for (const auto &Import : ImportSec.getContent()) {
    const auto &Desc = Import.getDesc();
    switch (Desc.getDescType()) {
    case AST::Component::ExternDesc::DescType::FuncType: {
      auto *Func = ImportMgr.findFunction(Import.getName());
      if (unlikely(Func == nullptr)) {
        spdlog::error(ErrCode::Value::UnknownImport);
        spdlog::error("    function name: {}"sv, Import.getName());
        return Unexpect(ErrCode::Value::UnknownImport);
      }
      CompInst.addFunction(Func);
      break;
    }
    case AST::Component::ExternDesc::DescType::TypeBound: {
      // Type imports fill the type index space: an argument-supplied type
      // when present, otherwise the eq-bound target.
      const auto *Ty = ImportMgr.findType(Import.getName());
      const auto *RT = static_cast<
          const Runtime::Instance::ComponentInstance::ResourceTypeRT *>(
          ImportMgr.findTypeResource(Import.getName()));
      if (Ty == nullptr && RT == nullptr && Desc.isEqType()) {
        CompInst.addTypeWithResource(
            CompInst.getType(Desc.getTypeIndex()),
            CompInst.getTypeResource(Desc.getTypeIndex()));
      } else {
        CompInst.addTypeWithResource(Ty, RT);
      }
      break;
    }
    case AST::Component::ExternDesc::DescType::ComponentType: {
      if (ImportMgr.hasComponent(Import.getName())) {
        CompInst.addComponentEntry(
            ImportMgr.findComponent(Import.getName()),
            ImportMgr.findComponentEnv(Import.getName()));
        break;
      }
      spdlog::error(ErrCode::Value::UnknownImport);
      spdlog::error("    component name: {}"sv, Import.getName());
      return Unexpect(ErrCode::Value::UnknownImport);
    }
    case AST::Component::ExternDesc::DescType::CoreType: {
      const auto *M = ImportMgr.findCoreModule(Import.getName());
      if (unlikely(M == nullptr)) {
        spdlog::error(ErrCode::Value::UnknownImport);
        spdlog::error("    core module name: {}"sv, Import.getName());
        return Unexpect(ErrCode::Value::UnknownImport);
      }
      CompInst.addModule(*M);
      break;
    }
    case AST::Component::ExternDesc::DescType::ValueBound: {
      const auto *V = ImportMgr.findValue(Import.getName());
      if (unlikely(V == nullptr)) {
        spdlog::error(ErrCode::Value::UnknownImport);
        spdlog::error("    value name: {}"sv, Import.getName());
        return Unexpect(ErrCode::Value::UnknownImport);
      }
      CompInst.addValue(*V);
      break;
    }
    case AST::Component::ExternDesc::DescType::InstanceType: {
      // TODO: COMPONENT - type matching for the instance type.
      auto CompName = Import.getName();
      const auto *ImportedCompInst = ImportMgr.findComponentInstance(CompName);
      if (unlikely(ImportedCompInst == nullptr)) {
        spdlog::error(ErrCode::Value::UnknownImport);
        spdlog::error("    component name: {}"sv, CompName);
        return Unexpect(ErrCode::Value::UnknownImport);
      }
      CompInst.addComponentInstance(ImportedCompInst);
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
