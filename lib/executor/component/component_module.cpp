// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "executor/component/executor.h"

#include "common/errinfo.h"
#include "common/spdlog.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>

namespace WasmEdge {
namespace Executor {

// Instantiate a root component instance. See
// "include/executor/component/executor.h".
Expect<std::unique_ptr<Runtime::Instance::ComponentInstance>>
ComponentExecutor::instantiate(Runtime::Component::StoreManager &StoreMgr,
                               const AST::Component::Component &Comp,
                               std::optional<std::string_view> Name) {
  // Check that the component is validated.
  if (unlikely(!Comp.getIsValidated())) {
    spdlog::error(ErrCode::Value::NotValidated);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Component));
    return Unexpect(ErrCode::Value::NotValidated);
  }

  auto CompInst = std::make_unique<Runtime::Instance::ComponentInstance>(
      Name.has_value() ? *Name : std::string_view());

  // The instantiation is one embedder entry: a start function runs as a
  // task under it.
  EXPECTED_TRY(runEntry([this, &CompInst, &Comp, &StoreMgr]() {
    return instantiate(*CompInst, Comp, &StoreMgr, nullptr);
  }));

  // For a named component, register it in the store.
  if (Name.has_value()) {
    StoreMgr.registerInstance(CompInst.get());
  }
  return CompInst;
}

// Instantiate a nested component instance. See
// "include/executor/component/executor.h".
Expect<std::unique_ptr<Runtime::Instance::ComponentInstance>>
ComponentExecutor::instantiate(
    const Runtime::Instance::ComponentInstance &Provider,
    const AST::Component::Component &Comp,
    Runtime::Instance::ComponentInstance *Parent) {
  auto CompInst =
      std::make_unique<Runtime::Instance::ComponentInstance>("", Parent);
  EXPECTED_TRY(instantiate(*CompInst, Comp, nullptr, &Provider));
  return CompInst;
}

// The section walk. See "include/executor/component/executor.h".
Expect<void> ComponentExecutor::instantiate(
    Runtime::Instance::ComponentInstance &CompInst,
    const AST::Component::Component &Comp,
    Runtime::Component::StoreManager *StoreMgr,
    const Runtime::Instance::ComponentInstance *Provider) {
  auto ReportError = [](ASTNodeAttr Attr) {
    return [Attr](auto E) {
      spdlog::error(ErrInfo::InfoAST(Attr));
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Component));
      return E;
    };
  };

  // A root instance takes its own copy of the component and walks that: the
  // definitions its index spaces point into then live as long as it does,
  // like the types and the code a core module instance copies. A nested one
  // walks the copy of the tree it belongs to.
  const AST::Component::Component *Src = &Comp;
  if (CompInst.getParent() == nullptr) {
    auto Owned = std::make_unique<AST::Component::Component>();
    copyComponent(Comp, *Owned);
    Src = &CompInst.ownComponent(std::move(Owned));
  }

  // The sections instantiate in their binary order, each into the index
  // spaces of the instance.
  for (const auto &Sec : Src->getSections()) {
    EXPECTED_TRY(std::visit(
        [&](const auto &Section) -> Expect<void> {
          using T = std::decay_t<decltype(Section)>;
          if constexpr (std::is_same_v<T, AST::CustomSection>) {
            return {};
          } else if constexpr (std::is_same_v<
                                   T, AST::Component::CoreModuleSection>) {
            return instantiate(CompInst, Section)
                .map_error(ReportError(ASTNodeAttr::Comp_Sec_CoreMod));
          } else if constexpr (std::is_same_v<
                                   T, AST::Component::CoreInstanceSection>) {
            return instantiate(CompInst, Section)
                .map_error(ReportError(ASTNodeAttr::Comp_Sec_CoreInstance));
          } else if constexpr (std::is_same_v<
                                   T, AST::Component::CoreTypeSection>) {
            return instantiate(CompInst, Section)
                .map_error(ReportError(ASTNodeAttr::Comp_Sec_CoreType));
          } else if constexpr (std::is_same_v<
                                   T, AST::Component::ComponentSection>) {
            return instantiate(CompInst, Section)
                .map_error(ReportError(ASTNodeAttr::Comp_Sec_Component));
          } else if constexpr (std::is_same_v<
                                   T, AST::Component::InstanceSection>) {
            return instantiate(CompInst, Section)
                .map_error(ReportError(ASTNodeAttr::Comp_Sec_Instance));
          } else if constexpr (std::is_same_v<T,
                                              AST::Component::AliasSection>) {
            return instantiate(CompInst, Section)
                .map_error(ReportError(ASTNodeAttr::Comp_Sec_Alias));
          } else if constexpr (std::is_same_v<T, AST::Component::TypeSection>) {
            return instantiate(CompInst, Section)
                .map_error(ReportError(ASTNodeAttr::Comp_Sec_Type));
          } else if constexpr (std::is_same_v<T,
                                              AST::Component::CanonSection>) {
            return instantiate(CompInst, Section)
                .map_error(ReportError(ASTNodeAttr::Comp_Sec_Canon));
          } else if constexpr (std::is_same_v<T,
                                              AST::Component::StartSection>) {
            return instantiate(CompInst, Section)
                .map_error(ReportError(ASTNodeAttr::Comp_Sec_Start));
          } else if constexpr (std::is_same_v<T,
                                              AST::Component::ImportSection>) {
            if (StoreMgr != nullptr) {
              return instantiate(*StoreMgr, CompInst, Section)
                  .map_error(ReportError(ASTNodeAttr::Comp_Sec_Import));
            }
            return instantiate(*Provider, CompInst, Section)
                .map_error(ReportError(ASTNodeAttr::Comp_Sec_Import));
          } else if constexpr (std::is_same_v<T,
                                              AST::Component::ExportSection>) {
            return instantiate(CompInst, Section)
                .map_error(ReportError(ASTNodeAttr::Comp_Sec_Export));
          } else if constexpr (std::is_same_v<T,
                                              AST::Component::ValueSection>) {
            return instantiate(CompInst, Section)
                .map_error(ReportError(ASTNodeAttr::Comp_Sec_Value));
          } else {
            static_assert(sizeof(T) == 0, "unhandled section");
          }
        },
        Sec));
  }
  return {};
}

// Instantiate a core module with arguments. See
// "include/executor/component/executor.h".
Expect<std::unique_ptr<Runtime::Instance::ModuleInstance>>
ComponentExecutor::instantiateModule(
    Runtime::Instance::ComponentInstance &CompInst, const AST::Module &Mod,
    Span<const AST::Component::InstantiateArg<uint32_t>> Args) {
  auto ModInst = std::make_unique<Runtime::Instance::ModuleInstance>("");
  // A two-level import name resolves through the argument of the module
  // name, a core instance.
  Executor::CoreModuleFinder ModuleFinder = [&CompInst,
                                             Args](std::string_view ModName)
      -> const Runtime::Instance::ModuleInstance * {
    for (const auto &Arg : Args) {
      if (Arg.getName() == ModName) {
        auto Found = CompInst.getCoreModuleInstance(Arg.getIndex());
        return Found ? *Found : nullptr;
      }
    }
    return nullptr;
  };
  // A start function of the module may suspend, so it runs as a task.
  if (Mod.getStartSection().getContent().has_value()) {
    EXPECTED_TRY(runImplicitTask(
        &CompInst, [this, &ModuleFinder, &ModInst, &Mod]() -> Expect<void> {
          return Core.instantiateModule(ModuleFinder, *ModInst, Mod);
        }));
  } else {
    EXPECTED_TRY(Core.instantiateModule(ModuleFinder, *ModInst, Mod));
  }
  return ModInst;
}

// Synthesize a core module instance from inline exports. See
// "include/executor/component/executor.h".
Expect<std::unique_ptr<Runtime::Instance::ModuleInstance>>
ComponentExecutor::instantiateModule(
    Runtime::Instance::ComponentInstance &CompInst,
    Span<const AST::Component::InlineExport> Exports) {
  auto ModInst = std::make_unique<Runtime::Instance::ModuleInstance>("");
  uint32_t NumFuncs = 0, NumTables = 0, NumMems = 0, NumGlobs = 0, NumTags = 0;
  for (const auto &Exp : Exports) {
    const auto &SortIdx = Exp.getSortIdx();
    const uint32_t Idx = SortIdx.getIdx();
    if (!SortIdx.getSort().isCore()) {
      spdlog::error(ErrCode::Value::ComponentUnexpectedSort);
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_CoreInlineExport));
      return Unexpect(ErrCode::Value::ComponentUnexpectedSort);
    }
    switch (SortIdx.getSort().getCoreSortType()) {
    case AST::Component::Sort::CoreSortType::Func: {
      EXPECTED_TRY(auto *Func, CompInst.getCoreFunction(Idx));
      // A canonical function has no module of its own: the synthesized
      // instance registers its type.
      if (Func->isHostFunction() && Func->getModule() == nullptr) {
        ModInst->importHostFunction(Func);
      } else {
        ModInst->importFunction(Func);
      }
      ModInst->exportFunction(Exp.getName(), NumFuncs);
      ++NumFuncs;
      break;
    }
    case AST::Component::Sort::CoreSortType::Table: {
      EXPECTED_TRY(auto *Tab, CompInst.getCoreTable(Idx));
      ModInst->importTable(Tab);
      ModInst->exportTable(Exp.getName(), NumTables);
      ++NumTables;
      break;
    }
    case AST::Component::Sort::CoreSortType::Memory: {
      EXPECTED_TRY(auto *Mem, CompInst.getCoreMemory(Idx));
      ModInst->importMemory(Mem);
      ModInst->exportMemory(Exp.getName(), NumMems);
      ++NumMems;
      break;
    }
    case AST::Component::Sort::CoreSortType::Global: {
      EXPECTED_TRY(auto *Glob, CompInst.getCoreGlobal(Idx));
      ModInst->importGlobal(Glob);
      ModInst->exportGlobal(Exp.getName(), NumGlobs);
      ++NumGlobs;
      break;
    }
    case AST::Component::Sort::CoreSortType::Tag: {
      EXPECTED_TRY(auto *Tag, CompInst.getCoreTag(Idx));
      ModInst->importTag(Tag);
      ModInst->exportTag(Exp.getName(), NumTags);
      ++NumTags;
      break;
    }
    default:
      spdlog::error(ErrCode::Value::ComponentUnexpectedSort);
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_CoreInlineExport));
      return Unexpect(ErrCode::Value::ComponentUnexpectedSort);
    }
  }
  ModInst->finalizeInstantiation();
  return ModInst;
}

// Export an item of an index space. See
// "include/executor/component/executor.h".
Expect<void>
ComponentExecutor::addExport(Runtime::Instance::ComponentInstance &CompInst,
                             Runtime::Instance::ComponentInstance &DstInst,
                             std::string_view Name,
                             const AST::Component::SortIndex &SortIdx) {
  const uint32_t Idx = SortIdx.getIdx();
  if (SortIdx.getSort().isCore()) {
    switch (SortIdx.getSort().getCoreSortType()) {
    case AST::Component::Sort::CoreSortType::Module: {
      EXPECTED_TRY(const auto *Mod, CompInst.getModule(Idx));
      DstInst.exportCoreModule(Name, *Mod);
      return {};
    }
    default:
      break;
    }
  } else {
    switch (SortIdx.getSort().getSortType()) {
    case AST::Component::Sort::SortType::Func: {
      EXPECTED_TRY(auto *Func, CompInst.getFunction(Idx));
      DstInst.exportFunction(Name, Func);
      return {};
    }
    case AST::Component::Sort::SortType::Value: {
      EXPECTED_TRY(const auto *Val, CompInst.getValue(Idx));
      DstInst.exportValue(Name, *Val);
      return {};
    }
    case AST::Component::Sort::SortType::Type: {
      EXPECTED_TRY(const auto *Def, CompInst.getTypeDefinition(Idx));
      DstInst.exportType(Name, *Def);
      return {};
    }
    case AST::Component::Sort::SortType::Component: {
      EXPECTED_TRY(const auto *Def, CompInst.getComponentDefinition(Idx));
      DstInst.exportComponent(Name, *Def);
      return {};
    }
    case AST::Component::Sort::SortType::Instance: {
      EXPECTED_TRY(const auto *Inst, CompInst.getComponentInstance(Idx));
      DstInst.exportComponentInstance(Name, Inst);
      return {};
    }
    default:
      break;
    }
  }
  spdlog::error(ErrCode::Value::ComponentUnexpectedSort);
  spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Sort));
  return Unexpect(ErrCode::Value::ComponentUnexpectedSort);
}

// Instantiate core module section. See "include/executor/component/executor.h".
Expect<void> ComponentExecutor::instantiate(
    Runtime::Instance::ComponentInstance &CompInst,
    const AST::Component::CoreModuleSection &CoreModSec) {
  // A core module definition: instantiated by a core instance expression.
  CompInst.addModule(CoreModSec.getContent());
  return {};
}

// Instantiate component section. See "include/executor/component/executor.h".
Expect<void> ComponentExecutor::instantiate(
    Runtime::Instance::ComponentInstance &CompInst,
    const AST::Component::ComponentSection &CompSec) {
  // A nested component definition closes over this instance.
  CompInst.addComponent(CompSec.getContent());
  return {};
}

} // namespace Executor
} // namespace WasmEdge
