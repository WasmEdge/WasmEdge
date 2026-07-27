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

  // Prepare the table-size and element-buffer pointers vectors for compiled
  // functions.
  ModInst.TableSizePtrs.resize(ModInst.getTableNum() +
                               TabSec.getContent().size());
  ModInst.TableRefPtrs.resize(ModInst.getTableNum() +
                              TabSec.getContent().size());

  // Set the table pointers of imported tables.
  for (uint32_t I = 0; I < ModInst.getTableNum(); ++I) {
    auto *TabInst = ModInst.unsafeGetTable(I);
    ModInst.TableSizePtrs[I] = TabInst->getSizePtr();
    ModInst.TableRefPtrs[I] = &TabInst->getDataPtr();
  }

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
      // Pop result from the stack.
      RefVariant InitTabValue = StackMgr.pop().get<RefVariant>();
      // Create and add the table instance to the module instance.
      ModInst.addTable(TabSeg.getTableType(), InitTabValue);
    } else {
      // No init expression case. Use the null reference to initialize.
      // Normalize the type to the bottom abstract heap type so that null
      // references always carry abstract types, as ref.cast/ref.test assume.
      auto BotType = toBottomType(StackMgr, TabSeg.getTableType().getRefType());
      RefVariant InitTabValue(ValType(TypeCode::RefNull, BotType));
      ModInst.addTable(TabSeg.getTableType(), InitTabValue);
    }
    // Set the table pointers of the instantiated table.
    const auto Index = ModInst.getTableNum() - 1;
    auto *TabInst = ModInst.unsafeGetTable(Index);
    ModInst.TableSizePtrs[Index] = TabInst->getSizePtr();
    ModInst.TableRefPtrs[Index] = &TabInst->getDataPtr();
  }
  return {};
}

} // namespace Executor
} // namespace WasmEdge
