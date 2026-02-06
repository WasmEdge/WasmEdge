// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "executor/executor.h"

#include "common/errinfo.h"
#include "common/spdlog.h"

#include <string_view>

namespace WasmEdge {
namespace Executor {

using namespace std::literals;

Expect<void>
Executor::instantiate(Runtime::Instance::ComponentInstance &CompInst,
                      const AST::Component::CoreInstanceSection &CoreInstSec) {
  // Instantiate the core module instance with the imports and add the instances
  // into the component model index space.
  for (const auto &Expr : CoreInstSec.getContent()) {
    if (Expr.isInstantiateModule()) {
      // Instantiate with args case.
      // Create an import manager to implement the isolation of imports.
      Runtime::Instance::ComponentImportManager ImportMgr;
      for (const auto &Arg : Expr.getInstantiateArgs()) {
        ImportMgr.exportCoreModuleInstance(
            Arg.getName(), CompInst.getCoreModuleInstance(Arg.getIndex()));
      }
      const AST::Module &Mod = CompInst.getModule(Expr.getModuleIndex());
      EXPECTED_TRY(auto NewModInst, instantiate(ImportMgr, Mod));
      CompInst.addCoreModuleInstance(std::move(NewModInst));
    } else {
      // Inline exports case.
      // Create a core module instance with the exports.
      auto Mod = std::make_unique<Runtime::Instance::ModuleInstance>("");
      uint32_t ExpIdx[4] = {0, 0, 0, 0};

      for (const auto &Exp : Expr.getInlineExports()) {
        const auto &SortIdx = Exp.getSortIdx();
        const uint32_t Idx = SortIdx.getIdx();
        switch (SortIdx.getSort().getCoreSortType()) {
        case AST::Component::Sort::CoreSortType::Func:
          Mod->importFunction(CompInst.getCoreFunction(Idx));
          Mod->exportFunction(Exp.getName(), ExpIdx[0]);
          ExpIdx[0]++;
          break;
        case AST::Component::Sort::CoreSortType::Table:
          Mod->importTable(CompInst.getCoreTable(Idx));
          Mod->exportTable(Exp.getName(), ExpIdx[1]);
          ExpIdx[1]++;
          break;
        case AST::Component::Sort::CoreSortType::Memory:
          Mod->importMemory(CompInst.getCoreMemory(Idx));
          Mod->exportMemory(Exp.getName(), ExpIdx[2]);
          ExpIdx[2]++;
          break;
        case AST::Component::Sort::CoreSortType::Global:
          Mod->importGlobal(CompInst.getCoreGlobal(Idx));
          Mod->exportGlobal(Exp.getName(), ExpIdx[3]);
          ExpIdx[3]++;
          break;
        case AST::Component::Sort::CoreSortType::Type:
        case AST::Component::Sort::CoreSortType::Module:
        case AST::Component::Sort::CoreSortType::Instance:
          spdlog::error(ErrCode::Value::CoreInvalidExport);
          spdlog::error("    A module instance cannot exports types, modules,"sv
                        " or instances"sv);
          return Unexpect(ErrCode::Value::CoreInvalidExport);
        default:
          assumingUnreachable();
        }
      }

      // Add this core module instance into the component instance index space.
      CompInst.addCoreModuleInstance(std::move(Mod));
    }
  }
  return {};
}

Expect<void>
Executor::instantiate(Runtime::Instance::ComponentInstance &CompInst,
                      const AST::Component::InstanceSection &InstSec) {
  for (const auto &Expr : InstSec.getContent()) {
    if (Expr.isInstantiateModule()) {
      // Create an import manager to implement the isolation of imports.
      Runtime::Instance::ComponentImportManager ImportMgr;
      for (const auto &Arg : Expr.getInstantiateArgs()) {
        const auto &SortIdx = Arg.getIndex();
        const auto &Sort = SortIdx.getSort();
        if (Sort.isCore()) {
          switch (Sort.getCoreSortType()) {
          case AST::Component::Sort::CoreSortType::Func:
            ImportMgr.exportCoreFunctionInstance(
                Arg.getName(), CompInst.getCoreFunction(SortIdx.getIdx()));
            break;
          case AST::Component::Sort::CoreSortType::Table:
            ImportMgr.exportCoreTableInstance(
                Arg.getName(), CompInst.getCoreTable(SortIdx.getIdx()));
            break;
          case AST::Component::Sort::CoreSortType::Memory:
            ImportMgr.exportCoreMemoryInstance(
                Arg.getName(), CompInst.getCoreMemory(SortIdx.getIdx()));
            break;
          case AST::Component::Sort::CoreSortType::Global:
            ImportMgr.exportCoreGlobalInstance(
                Arg.getName(), CompInst.getCoreGlobal(SortIdx.getIdx()));
            break;
          case AST::Component::Sort::CoreSortType::Instance:
            ImportMgr.exportCoreModuleInstance(
                Arg.getName(),
                CompInst.getCoreModuleInstance(SortIdx.getIdx()));
            break;
          case AST::Component::Sort::CoreSortType::Type:
          case AST::Component::Sort::CoreSortType::Module:
            // TODO: COMPONENT - complete the instance instantiation.
            spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
            spdlog::error("    incomplete instantiate (with {})"sv,
                          Arg.getName());
            return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
          default:
            assumingUnreachable();
          }
        } else {
          switch (Sort.getSortType()) {
          case AST::Component::Sort::SortType::Func:
            ImportMgr.exportFunction(Arg.getName(),
                                     CompInst.getFunction(SortIdx.getIdx()));
            break;
          case AST::Component::Sort::SortType::Instance:
            ImportMgr.exportComponentInstance(
                Arg.getName(), CompInst.getComponentInstance(SortIdx.getIdx()));
            break;
          case AST::Component::Sort::SortType::Value:
          case AST::Component::Sort::SortType::Type:
          case AST::Component::Sort::SortType::Component:
            // TODO: COMPONENT - complete the instance instantiation.
            spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
            spdlog::error("    incomplete instantiate (with {})"sv,
                          Arg.getName());
            return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
          default:
            assumingUnreachable();
          }
        }
      }
      const AST::Component::Component &Comp =
          CompInst.getComponent(Expr.getComponentIndex());
      EXPECTED_TRY(auto NewCompInst, instantiate(ImportMgr, Comp));
      CompInst.addComponentInstance(std::move(NewCompInst));
    } else {
      // Inline exports case.
      // Create a component instance with the exports.
      auto Comp = std::make_unique<Runtime::Instance::ComponentInstance>("");
      uint32_t CoreExpIdx[7] = {0, 0, 0, 0, 0, 0, 0};
      uint32_t ExpIdx[5] = {0, 0, 0, 0, 0};

      for (const auto &Exp : Expr.getInlineExports()) {
        const auto &SortIdx = Exp.getSortIdx();
        const uint32_t Idx = SortIdx.getIdx();
        const auto &Sort = SortIdx.getSort();

        if (Sort.isCore()) {
          switch (Sort.getCoreSortType()) {
          case AST::Component::Sort::CoreSortType::Func:
            Comp->addCoreFunction(CompInst.getCoreFunction(Idx));
            Comp->exportCoreFunction(Exp.getName().Name, CoreExpIdx[0]);
            CoreExpIdx[0]++;
            break;
          case AST::Component::Sort::CoreSortType::Table:
            Comp->addCoreTable(CompInst.getCoreTable(Idx));
            Comp->exportCoreTable(Exp.getName().Name, CoreExpIdx[1]);
            CoreExpIdx[1]++;
            break;
          case AST::Component::Sort::CoreSortType::Memory:
            Comp->addCoreMemory(CompInst.getCoreMemory(Idx));
            Comp->exportCoreMemory(Exp.getName().Name, CoreExpIdx[2]);
            CoreExpIdx[2]++;
            break;
          case AST::Component::Sort::CoreSortType::Global:
            Comp->addCoreGlobal(CompInst.getCoreGlobal(Idx));
            Comp->exportCoreGlobal(Exp.getName().Name, CoreExpIdx[3]);
            CoreExpIdx[3]++;
            break;
          case AST::Component::Sort::CoreSortType::Type:
          case AST::Component::Sort::CoreSortType::Module:
          case AST::Component::Sort::CoreSortType::Instance:
            // TODO: COMPONENT - complete the instance instantiation.
            spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
            spdlog::error("    incomplete inline export {}"sv,
                          Exp.getName().Name);
            return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
          default:
            assumingUnreachable();
          }
        } else {
          switch (Sort.getSortType()) {
          case AST::Component::Sort::SortType::Func:
            Comp->addFunction(CompInst.getFunction(Idx));
            Comp->exportFunction(Exp.getName().Name, ExpIdx[0]);
            ExpIdx[0]++;
            break;
          case AST::Component::Sort::SortType::Instance:
            Comp->addComponentInstance(CompInst.getComponentInstance(Idx));
            Comp->exportComponentInstance(Exp.getName().Name, ExpIdx[4]);
            ExpIdx[4]++;
            break;
          case AST::Component::Sort::SortType::Value:
          case AST::Component::Sort::SortType::Type:
          case AST::Component::Sort::SortType::Component:
            // TODO: COMPONENT - complete the instance instantiation.
            spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
            spdlog::error("    incomplete inline export {}"sv,
                          Exp.getName().Name);
            return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
          default:
            assumingUnreachable();
          }
        }
      }

      // Add this component instance into the component instance index space.
      CompInst.addComponentInstance(std::move(Comp));
    }
  }
  return {};
}

} // namespace Executor
} // namespace WasmEdge
