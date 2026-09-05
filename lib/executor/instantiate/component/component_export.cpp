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
      case AST::Component::Sort::CoreSortType::Func:
      case AST::Component::Sort::CoreSortType::Table:
      case AST::Component::Sort::CoreSortType::Memory:
      case AST::Component::Sort::CoreSortType::Global:
      case AST::Component::Sort::CoreSortType::Type:
      case AST::Component::Sort::CoreSortType::Module:
        // These cases are invalid.
      default:
        assumingUnreachable();
      }
    } else {
      switch (Sort.getSortType()) {
      case AST::Component::Sort::SortType::Func:
        // Each export also aliases the exported definition into a new index in
        // the component's own index space, mirroring the validator, so later
        // references to that index resolve in bounds.
        CompInst.exportFunction(Export.getName(), Index);
        CompInst.addFunction(CompInst.getFunction(Index));
        break;
      case AST::Component::Sort::SortType::Instance:
        CompInst.exportComponentInstance(Export.getName(), Index);
        CompInst.addComponentInstance(CompInst.getComponentInstance(Index));
        break;
      case AST::Component::Sort::SortType::Type:
        // A type export introduces a new index aliasing the exported type,
        // so later definitions resolve indices as validation does.
        CompInst.exportType(Export.getName(), Index);
        if (const auto *Ty = CompInst.getType(Index)) {
          CompInst.addType(*Ty);
        }
        break;
      case AST::Component::Sort::SortType::Component:
        CompInst.exportComponent(Export.getName(), Index);
        break;
      case AST::Component::Sort::SortType::Value:
        CompInst.exportValue(Export.getName(), Index);
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
