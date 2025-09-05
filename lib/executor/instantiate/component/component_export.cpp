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
        // The cases are invalid.
      default:
        assumingUnreachable();
      }
    } else {
      switch (Sort.getSortType()) {
      case AST::Component::Sort::SortType::Func:
        CompInst.exportFunction(Export.getName(), Index);
        break;
      case AST::Component::Sort::SortType::Instance:
        CompInst.exportComponentInstance(Export.getName(), Index);
        break;
      case AST::Component::Sort::SortType::Type:
        CompInst.exportType(Export.getName(), Index);
        break;
      case AST::Component::Sort::SortType::Value:
      case AST::Component::Sort::SortType::Component:
        // TODO: COMPONENT - complete the export instantiation.
        spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
        spdlog::error("    incomplete export {}"sv, Export.getName());
        return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
      default:
        assumingUnreachable();
      }
    }
  }
  return {};
}

} // namespace Executor
} // namespace WasmEdge
