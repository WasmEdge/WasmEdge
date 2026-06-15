// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "executor/executor.h"

#include <cstdint>

namespace WasmEdge {
namespace Executor {

// Instantiate table instance. See "include/executor/executor.h".
Expect<void> Executor::instantiate(Runtime::StackManager &StackMgr,
                                   Runtime::Instance::ModuleInstance &ModInst,
                                   const AST::TableSection &TabSec) {
  // A frame with the temporary module is pushed onto the stack by the caller.

  // Iterate through the table segments to instantiate and initialize table
  // instances.
  for (const auto &TabSeg : TabSec.getContent()) {
    if (TabSeg.getExpr().getInstrs().size() > 0) {
      // Run the initialization expression.
      EXPECTED_TRY(runExpression(StackMgr, TabSeg.getExpr().getInstrs())
                       .map_error([](auto E) {
                         spdlog::error(
                             ErrInfo::InfoAST(ASTNodeAttr::Expression));
                         return E;
                       }));
      // Keep the init result on the (GC-rooted) value stack until addTable
      // broadcasts it into the table's slots and registers that as a root.
      // Popping first would leave a managed ref unrooted, reclaimable mid-GC.
      ModInst.addTable(Allocator, TabSeg.getTableType(),
                       StackMgr.peekTop<RefVariant>());
      // Pop result from the stack.
      StackMgr.pop<ValVariant>();
    } else {
      // No init expression case. Use the null reference to initialize.
      // Normalize the type to the bottom abstract heap type so that null
      // references always carry abstract types, as ref.cast/ref.test assume.
      auto BotType = toBottomType(StackMgr, TabSeg.getTableType().getRefType());
      RefVariant InitTabValue(ValType(TypeCode::RefNull, BotType));
      ModInst.addTable(Allocator, TabSeg.getTableType(), InitTabValue);
    }
  }
  return {};
}

} // namespace Executor
} // namespace WasmEdge
