// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

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
    ModInst.GlobalPtrs[I] = &((*ModInst.getGlobal(I))->getValue());
  }

  // Iterate through the global segments to instantiate and initialize global
  // instances.
  for (const auto &GlobSeg : GlobSec.getContent()) {
    // Create and add the global instance into the module instance.
    ModInst.addGlobal(GlobSeg.getGlobalType());
    const auto Index = ModInst.getGlobalNum() - 1;
    Runtime::Instance::GlobalInstance *GlobInst = *ModInst.getGlobal(Index);

    // Set the global pointers of instantiated globals.
    ModInst.GlobalPtrs[Index] = &(GlobInst->getValue());

    // Run initialize expression.
    if (auto Res = runExpression(StackMgr, GlobSeg.getExpr().getInstrs());
        !Res) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Expression));
      return Unexpect(Res);
    }

    // Pop result from stack.
    GlobInst->getValue() = StackMgr.pop();
  }
  return {};
}

} // namespace Executor
} // namespace WasmEdge
