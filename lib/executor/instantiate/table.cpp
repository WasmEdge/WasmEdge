// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "executor/executor.h"

#include <cstdint>

namespace WasmEdge {
namespace Executor {

// Instantiate table instance. See "include/executor/executor.h".
Expect<void> Executor::instantiate(Runtime::Instance::ModuleInstance &ModInst,
                                   const AST::TableSection &TabSec) {
  Runtime::StackManager StackMgr;

  // Iterate through the table types to instantiate table instance.
  for (const auto &Table : TabSec.getContent()) {
    StackMgr.pushFrame(&ModInst, AST::InstrView::iterator(), 0, 0);
    // Run initialize expression.
    if (auto Res = runExpression(StackMgr, Table.getInitExpr().getInstrs());
        !Res) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Expression));
      return Unexpect(Res);
    }

    // Pop result from stack.
    RefVariant InitTabValue = StackMgr.pop().get<RefVariant>();
    // Create and add the table instance into the module instance.
    ModInst.addTable(Table.getTableType(), InitTabValue);
    StackMgr.popFrame();
  }
  return {};
}

} // namespace Executor
} // namespace WasmEdge
