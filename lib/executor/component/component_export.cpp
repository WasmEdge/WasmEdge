// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "executor/component/executor.h"

#include "common/errinfo.h"
#include "common/spdlog.h"

#include <cstdint>

namespace WasmEdge {
namespace Executor {

// Instantiate export section. See "include/executor/component/executor.h".
Expect<void>
ComponentExecutor::instantiate(Runtime::Instance::ComponentInstance &CompInst,
                               const AST::Component::ExportSection &ExportSec) {
  auto ReportError = [](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Export));
    return E;
  };
  for (const auto &Exp : ExportSec.getContent()) {
    EXPECTED_TRY(
        addExport(CompInst, CompInst, Exp.getName(), Exp.getSortIndex())
            .map_error(ReportError));
    // An export definition also appends the exported item to its index
    // space, as an alias does.
    const auto &SortIdx = Exp.getSortIndex();
    const uint32_t Idx = SortIdx.getIdx();
    if (SortIdx.getSort().isCore()) {
      if (SortIdx.getSort().getCoreSortType() ==
          AST::Component::Sort::CoreSortType::Module) {
        EXPECTED_TRY(const auto *Mod,
                     CompInst.getModule(Idx).map_error(ReportError));
        CompInst.addModule(*Mod);
      }
      continue;
    }
    switch (SortIdx.getSort().getSortType()) {
    case AST::Component::Sort::SortType::Func: {
      EXPECTED_TRY(auto *Func,
                   CompInst.getFunction(Idx).map_error(ReportError));
      CompInst.addFunction(Func);
      break;
    }
    case AST::Component::Sort::SortType::Value: {
      EXPECTED_TRY(const auto *Val,
                   CompInst.getValue(Idx).map_error(ReportError));
      CompInst.addValue(*Val);
      break;
    }
    case AST::Component::Sort::SortType::Type: {
      EXPECTED_TRY(const auto *Def,
                   CompInst.getTypeDefinition(Idx).map_error(ReportError));
      CompInst.addTypeDefinition(*Def);
      break;
    }
    case AST::Component::Sort::SortType::Component: {
      EXPECTED_TRY(const auto *Def,
                   CompInst.getComponentDefinition(Idx).map_error(ReportError));
      CompInst.addComponentDefinition(*Def);
      break;
    }
    case AST::Component::Sort::SortType::Instance: {
      EXPECTED_TRY(const auto *Inst,
                   CompInst.getComponentInstance(Idx).map_error(ReportError));
      CompInst.addComponentInstance(Inst);
      break;
    }
    default:
      break;
    }
  }
  return {};
}

} // namespace Executor
} // namespace WasmEdge
