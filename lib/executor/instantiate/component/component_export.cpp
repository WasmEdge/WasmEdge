// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "executor/executor.h"

#include "common/errinfo.h"
#include "common/spdlog.h"

#include <string_view>

namespace WasmEdge {
namespace Executor {

using namespace std::literals;

Expect<void>
Executor::instantiate(Runtime::Instance::ComponentInstance &CompInst,
                      const AST::Component::ExportSection &ExportSec) {
  for (const auto &Export : ExportSec.getContent()) {
    auto Index = Export.getSortIndex().getIdx();
    const auto &Sort = Export.getSortIndex().getSort();

    if (Sort.isCore()) {
      switch (Sort.getCoreSortType()) {
      case AST::Component::Sort::CoreSortType::Instance:
        CompInst.exportCoreModuleInstance(Export.getName(), Index);
        break;
      case AST::Component::Sort::CoreSortType::Module:
        CompInst.exportCoreModule(Export.getName(), Index);
        if (const auto *Mod = CompInst.getModule(Index)) {
          CompInst.addModule(*Mod);
        }
        break;
      case AST::Component::Sort::CoreSortType::Func:
      case AST::Component::Sort::CoreSortType::Table:
      case AST::Component::Sort::CoreSortType::Memory:
      case AST::Component::Sort::CoreSortType::Global:
      case AST::Component::Sort::CoreSortType::Type:
        // These cases are invalid.
      default:
        assumingUnreachable();
      }
    } else {
      // Exports introduce a new index aliasing the exported definition, so
      // later definitions resolve indices consistently with validation.
      switch (Sort.getSortType()) {
      case AST::Component::Sort::SortType::Func:
        CompInst.exportFunction(Export.getName(), Index);
        if (auto *Func = CompInst.getFunction(Index)) {
          CompInst.addFunction(Func);
        }
        break;
      case AST::Component::Sort::SortType::Instance:
        CompInst.exportComponentInstance(Export.getName(), Index);
        if (const auto *Inst = CompInst.getComponentInstance(Index)) {
          CompInst.addComponentInstance(Inst);
        }
        break;
      case AST::Component::Sort::SortType::Type:
        CompInst.exportType(Export.getName(), Index);
        CompInst.addTypeWithResource(CompInst.getType(Index),
                                     CompInst.getTypeResource(Index));
        break;
      case AST::Component::Sort::SortType::Component:
        CompInst.exportComponent(Export.getName(), Index);
        CompInst.addComponentEntry(CompInst.getComponent(Index),
                                   CompInst.getComponentEnv(Index));
        break;
      case AST::Component::Sort::SortType::Value:
        CompInst.exportValue(Export.getName(), CompInst.getValue(Index));
        CompInst.addValue(CompInst.getValue(Index));
        break;
      default:
        assumingUnreachable();
      }
    }
  }
  return {};
}

} // namespace Executor
} // namespace WasmEdge
