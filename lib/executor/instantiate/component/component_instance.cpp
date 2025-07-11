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
Executor::instantiate(Runtime::StoreManager &StoreMgr,
                      Runtime::Instance::ComponentInstance &CompInst,
                      const AST::Component::CoreInstanceSection &CoreInstSec) {
  // In this function, we will create a new module instance and insert it into
  // component module instance index space.
  for (const auto &Expr : CoreInstSec.getContent()) {
    if (Expr.isInstantiateModule()) {
      for (const auto &Arg : Expr.getInstantiateArgs()) {
        // Expr a list of `(with (name $instance))`
        // Each $instance get named as `name` as statement tell.
        StoreMgr.addNamedModule(Arg.getName(),
                                CompInst.getModuleInstance(Arg.getIndex()));
      }
      const AST::Module &Mod = CompInst.getModule(Expr.getModuleIndex());
      EXPECTED_TRY(auto ModInst, instantiate(StoreMgr, Mod));
      CompInst.addModuleInstance(std::move(ModInst));
    } else {
      // Create an immediate anonymous module instance.
      // This happened usually at `(with xxx)` statement, where we can have a
      // module instance expression.
      auto Mod = std::make_unique<Runtime::Instance::ModuleInstance>("");

      for (const auto &Exp : Expr.getInlineExports()) {
        const auto &SortIdx = Exp.getSortIdx();
        switch (SortIdx.getSort().getCoreSortType()) {
        case AST::Component::Sort::CoreSortType::Func:
          // The module instance takes functions and export them.
          Mod->exportFunction(Exp.getName(), CompInst.getCoreFunctionInstance(
                                                 SortIdx.getIdx()));
          break;
        case AST::Component::Sort::CoreSortType::Table:
          Mod->exportTable(Exp.getName(),
                           CompInst.getCoreTableInstance(SortIdx.getIdx()));
          break;
        case AST::Component::Sort::CoreSortType::Memory:
          Mod->exportMemory(Exp.getName(),
                            CompInst.getCoreMemoryInstance(SortIdx.getIdx()));
          break;
        case AST::Component::Sort::CoreSortType::Global:
          Mod->exportGlobal(Exp.getName(),
                            CompInst.getCoreGlobalInstance(SortIdx.getIdx()));
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

      // Insert this immediate module instance into the component instance.
      CompInst.addModuleInstance(std::move(Mod));
    }
  }
  return {};
}

Expect<void>
Executor::instantiate(Runtime::StoreManager &StoreMgr,
                      Runtime::Instance::ComponentInstance &CompInst,
                      const AST::Component::InstanceSection &InstSec) {
  for (const auto &Expr : InstSec.getContent()) {
    if (Expr.isInstantiateModule()) {
      for (const auto &Arg : Expr.getInstantiateArgs()) {
        const auto &SortIdx = Arg.getIndex();
        const auto &Sort = SortIdx.getSort();
        if (Sort.isCore()) {
          // TODO: insert below into mapping
          switch (Sort.getCoreSortType()) {
          case AST::Component::Sort::CoreSortType::Func:
          case AST::Component::Sort::CoreSortType::Table:
          case AST::Component::Sort::CoreSortType::Memory:
          case AST::Component::Sort::CoreSortType::Global:
          case AST::Component::Sort::CoreSortType::Type:
          case AST::Component::Sort::CoreSortType::Module:
            spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
            spdlog::error("    incomplete instantiate (with {})"sv,
                          Arg.getName());
            return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
          case AST::Component::Sort::CoreSortType::Instance:
            StoreMgr.addNamedModule(
                Arg.getName(), CompInst.getModuleInstance(SortIdx.getIdx()));
            break;
          default:
            assumingUnreachable();
          }
        } else {
          switch (Sort.getSortType()) {
          case AST::Component::Sort::SortType::Func:
          case AST::Component::Sort::SortType::Value:
          case AST::Component::Sort::SortType::Type:
          case AST::Component::Sort::SortType::Instance:
            spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
            spdlog::error("    incomplete instantiate (with {})"sv,
                          Arg.getName());
            return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
          case AST::Component::Sort::SortType::Component:
            EXPECTED_TRY(
                StoreMgr
                    .registerComponent(
                        Arg.getName(),
                        CompInst.getComponentInstance(SortIdx.getIdx()))
                    .map_error([](auto E) {
                      spdlog::error(E);
                      spdlog::error(
                          "    failed to register component instance"sv);
                      return E;
                    }));
            break;
          default:
            assumingUnreachable();
          }
        }
      }
      EXPECTED_TRY(auto Inst,
                   instantiate(StoreMgr, CompInst.getComponent(
                                             Expr.getComponentIndex())));
      CompInst.addComponentInstance(std::move(Inst));
    } else {
      // TODO: complete inline exports
      spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
      return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
    }
  }
  return {};
}

} // namespace Executor
} // namespace WasmEdge
