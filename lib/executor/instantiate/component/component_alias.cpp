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
Executor::instantiate(Runtime::StoreManager &,
                      Runtime::Instance::ComponentInstance &CompInst,
                      const AST::Component::AliasSection &AliasSec) {
  for (const auto &Alias : AliasSec.getContent()) {
    const auto &Sort = Alias.getSort();
    switch (Alias.getTargetType()) {
    case AST::Component::Alias::TargetType::Export: {
      assuming(!Sort.isCore());
      const auto &Export = Alias.getExport();
      switch (Sort.getSortType()) {
      case AST::Component::Sort::SortType::Func: {
        const auto *CInst = CompInst.getComponentInstance(Export.first);
        auto *FuncInst = CInst->findFuncExports(Export.second);
        CompInst.addFunctionInstance(FuncInst);
        break;
      }
      case AST::Component::Sort::SortType::Value:
      case AST::Component::Sort::SortType::Type:
      case AST::Component::Sort::SortType::Component:
      case AST::Component::Sort::SortType::Instance:
        spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
        spdlog::error("    incomplete alias export"sv);
        return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
      default:
        assumingUnreachable();
      }
      break;
    }
    case AST::Component::Alias::TargetType::CoreExport: {
      assuming(Sort.isCore());
      const auto &Export = Alias.getExport();
      const auto *ModInst = CompInst.getModuleInstance(Export.first);
      auto FindExports = [&](const auto &Map) {
        return ModInst->unsafeFindExports(Map, Export.second);
      };
      switch (Sort.getCoreSortType()) {
      case AST::Component::Sort::CoreSortType::Func: {
        auto *FuncInst = ModInst->getFuncExports(FindExports);
        CompInst.addCoreFunctionInstance(FuncInst);
        break;
      }
      case AST::Component::Sort::CoreSortType::Table: {
        auto *TableInst = ModInst->getTableExports(FindExports);
        CompInst.addCoreTableInstance(TableInst);
        break;
      }
      case AST::Component::Sort::CoreSortType::Memory: {
        auto *MemInst = ModInst->getMemoryExports(FindExports);
        CompInst.addCoreMemoryInstance(MemInst);
        break;
      }
      case AST::Component::Sort::CoreSortType::Global: {
        auto *GlobInst = ModInst->getGlobalExports(FindExports);
        CompInst.addCoreGlobalInstance(GlobInst);
        break;
      }
      case AST::Component::Sort::CoreSortType::Type:
      case AST::Component::Sort::CoreSortType::Module:
      case AST::Component::Sort::CoreSortType::Instance:
        spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
        spdlog::error("    incomplete alias core:export"sv);
        return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
      default:
        assumingUnreachable();
      }
      break;
    }
    case AST::Component::Alias::TargetType::Outer: {
      if (Sort.isCore()) {
        spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
        spdlog::error("    incomplete alias target outer: core:sort"sv);
        return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
      } else {
        switch (Sort.getSortType()) {
        case AST::Component::Sort::SortType::Func: {
          const auto &Outer = Alias.getOuter();
          auto *FuncInst = CompInst.getComponentInstance(Outer.first)
                               ->getCoreFunctionInstance(Outer.second);
          CompInst.addCoreFunctionInstance(FuncInst);
          break;
        }
        case AST::Component::Sort::SortType::Value:
        case AST::Component::Sort::SortType::Type:
        case AST::Component::Sort::SortType::Component:
        case AST::Component::Sort::SortType::Instance:
          spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
          spdlog::error("    incomplete alias target outer: sort"sv);
          return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
        default:
          assumingUnreachable();
        }
      }
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
