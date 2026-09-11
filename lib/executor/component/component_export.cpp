// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "executor/component/executor.h"
#include "executor/executor.h"

#include <string_view>

namespace WasmEdge {
namespace Executor {

// Instantiate export section. See executor.h.
Expect<void>
ComponentExecutor::instantiate(Component::Instantiator &Ctx,
                               const AST::Component::ExportSection &ExportSec) {
  auto &CompInst = Ctx.getInstance();
  for (const auto &Export : ExportSec.getContent()) {
    const uint32_t Idx = Export.getSortIndex().getIdx();
    const auto &Sort = Export.getSortIndex().getSort();
    const auto Name = Export.getName();

    // An export aliases its definition into a new index, like validation.
    if (Sort.isCore()) {
      switch (Sort.getCoreSortType()) {
      case AST::Component::Sort::CoreSortType::Instance: {
        EXPECTED_TRY(const auto *ModInst, Ctx.getCoreModuleInstance(Idx));
        CompInst.exportCoreModuleInstance(Name, ModInst);
        Ctx.importCoreModuleInstance(ModInst);
        break;
      }
      case AST::Component::Sort::CoreSortType::Module: {
        EXPECTED_TRY(const auto *Mod, CompInst.getModule(Idx));
        CompInst.exportCoreModule(Name, Idx);
        CompInst.addModule(*Mod);
        break;
      }
      default:
        // Validation admits no other core sort in an export.
        assumingUnreachable();
      }
    } else {
      switch (Sort.getSortType()) {
      case AST::Component::Sort::SortType::Func: {
        EXPECTED_TRY(auto *Func, Ctx.getFunction(Idx));
        CompInst.exportFunction(Name, Func);
        Ctx.importFunction(Func);
        break;
      }
      case AST::Component::Sort::SortType::Instance: {
        EXPECTED_TRY(const auto *Inst, Ctx.getComponentInstance(Idx));
        CompInst.exportComponentInstance(Name, Inst);
        Ctx.importComponentInstance(Inst);
        break;
      }
      case AST::Component::Sort::SortType::Type: {
        EXPECTED_TRY(const auto *TypeDef, CompInst.getTypeDefinition(Idx));
        CompInst.exportType(Name, Idx);
        CompInst.addTypeDefinition(*TypeDef);
        break;
      }
      case AST::Component::Sort::SortType::Component: {
        EXPECTED_TRY(const auto *CompDef, CompInst.getComponentDefinition(Idx));
        CompInst.exportComponent(Name, Idx);
        CompInst.addComponentDefinition(*CompDef);
        break;
      }
      case AST::Component::Sort::SortType::Value: {
        EXPECTED_TRY(const auto *Val, Ctx.getValue(Idx));
        CompInst.exportValue(Name, *Val);
        Ctx.addValue(*Val);
        break;
      }
      default:
        assumingUnreachable();
      }
    }
  }
  return {};
}

} // namespace Executor
} // namespace WasmEdge
