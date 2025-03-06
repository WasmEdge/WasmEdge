// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "executor/executor.h"

#include <cstdint>

namespace WasmEdge {
namespace Executor {

// Instantiate table instance. See "include/executor/executor.h".
Expect<void> Executor::instantiate(Runtime::StackManager &StackMgr,
                                   Runtime::Instance::ModuleInstance &ModInst,
                                   const AST::TableSection &TabSec) {
  // A frame with temp. module is pushed into the stack in caller.

  // Iterate through the table segments to instantiate and initialize table
  // instances.
  for (const auto &TabSeg : TabSec.getContent()) {
    if (TabSeg.getExpr().getInstrs().size() > 0) {
      // Run initialize expression.
      EXPECTED_TRY(runExpression(StackMgr, TabSeg.getExpr().getInstrs())
                       .map_error([](auto E) {
                         spdlog::error(
                             ErrInfo::InfoAST(ASTNodeAttr::Expression));
                         return E;
                       }));
      // Pop result from stack.
      RefVariant InitTabValue = StackMgr.pop<RefVariant>();
      // Create and add the table instance into the module instance.
      ModInst.addTable(Allocator, TabSeg.getTableType(), InitTabValue);
    } else {
      // No init expression case. Use the null reference to initialize.
      ModInst.addTable(Allocator, TabSeg.getTableType());
    }
  }
  return {};
}

} // namespace Executor
} // namespace WasmEdge
