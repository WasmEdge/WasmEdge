// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "executor/executor.h"

#include <cstdint>

namespace WasmEdge {
namespace Executor {

// Instantiate global instance. See "include/executor/executor.h".
Expect<void> Executor::instantiate(Runtime::StackManager &StackMgr,
                                   Runtime::Instance::ModuleInstance &ModInst,
                                   const AST::GlobalSection &GlobSec) {
  // A frame with the temporary module is pushed onto the stack by the caller.

  // Prepare pointers for compiled functions.
  ModInst.GlobalPtrs.resize(ModInst.getGlobalNum() +
                            GlobSec.getContent().size());

  // Set the global pointers of imported globals.
  for (uint32_t I = 0; I < ModInst.getGlobalNum(); ++I) {
    ModInst.GlobalPtrs[I] = (*ModInst.getGlobal(I))->getAddress();
  }

  // Iterate through the global segments to instantiate and initialize global
  // instances.
  for (const auto &GlobSeg : GlobSec.getContent()) {
    // Run the initialization expression.
    EXPECTED_TRY(runExpression(StackMgr, GlobSeg.getExpr().getInstrs())
                     .map_error([](auto E) {
                       spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Expression));
                       return E;
                     }));

    // Keep the init result on the (GC-rooted) value stack until addGlobal
    // copies it into the GlobalInstance and registers that as a root. Popping
    // first would leave a managed reference unrooted, reclaimable
    // mid-collection.
    ModInst.addGlobal(Allocator, GlobSeg.getGlobalType(),
                      StackMgr.peekTop<ValVariant>());
    // Pop result from the stack.
    StackMgr.pop<ValVariant>();
    const auto Index = ModInst.getGlobalNum() - 1;
    Runtime::Instance::GlobalInstance *GlobInst = *ModInst.getGlobal(Index);

    // Set the global pointers of instantiated globals.
    ModInst.GlobalPtrs[Index] = GlobInst->getAddress();
  }
  return {};
}

} // namespace Executor
} // namespace WasmEdge
