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
                      const AST::Component::ExportSection &ExportSec) {
  for (const auto &Export : ExportSec.getContent()) {
    auto Index = Export.getSortIndex().getIdx();
    const auto &Sort = Export.getSortIndex().getSort();

    if (Sort.isCore()) {
      switch (Sort.getCoreSortType()) {
      case AST::Component::Sort::CoreSortType::Module: {
        const auto *Mod = CompInst.getModuleInstance(Index);
        CompInst.addExport(Export.getName(), Mod);
        break;
      }
      case AST::Component::Sort::CoreSortType::Func:
      case AST::Component::Sort::CoreSortType::Table:
      case AST::Component::Sort::CoreSortType::Memory:
      case AST::Component::Sort::CoreSortType::Global:
      case AST::Component::Sort::CoreSortType::Type:
      case AST::Component::Sort::CoreSortType::Instance:
        // Any exported sortidx, which disallows core sorts other than core
        // module.
        spdlog::error(ErrCode::Value::InvalidCoreSort);
        spdlog::error(
            "    export core sort other than core module is invalid"sv);
        return Unexpect(ErrCode::Value::InvalidCoreSort);
      default:
        assumingUnreachable();
      }
    } else {
      switch (Sort.getSortType()) {
      case AST::Component::Sort::SortType::Func: {
        auto *FuncInst = CompInst.getFunctionInstance(Index);
        CompInst.addExport(Export.getName(), FuncInst);
        break;
      }
      case AST::Component::Sort::SortType::Value:
      case AST::Component::Sort::SortType::Type:
      case AST::Component::Sort::SortType::Instance:
      case AST::Component::Sort::SortType::Component:
        spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
        spdlog::error("    incomplete instantiate (with {})"sv,
                      Export.getName());
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
