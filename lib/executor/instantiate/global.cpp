// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "executor/executor.h"

#include <cstdint>

namespace WasmEdge {
namespace Executor {

// Instantiate global instance. See "include/executor/executor.h".
Expect<void> Executor::instantiate(Runtime::StackManager &StackMgr,
                                   Runtime::Instance::ModuleInstance &ModInst,
                                   const AST::GlobalSection &GlobSec) {
  // A frame with temp. module is pushed into the stack in caller.

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
    // Run initialize expression.
    EXPECTED_TRY(runExpression(StackMgr, GlobSeg.getExpr().getInstrs())
                     .map_error([](auto E) {
                       spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Expression));
                       return E;
                     }));

    // Pop result from stack.
    ValVariant InitValue = StackMgr.pop<ValVariant>();

    // Create and add the global instance into the module instance.
    ModInst.addGlobal(Allocator, GlobSeg.getGlobalType(), InitValue);
    const auto Index = ModInst.getGlobalNum() - 1;
    Runtime::Instance::GlobalInstance *GlobInst = *ModInst.getGlobal(Index);

    // Set the global pointers of instantiated globals.
    ModInst.GlobalPtrs[Index] = GlobInst->getAddress();
  }
  return {};
}

} // namespace Executor
} // namespace WasmEdge
