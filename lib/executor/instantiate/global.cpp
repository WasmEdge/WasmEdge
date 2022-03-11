// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "executor/executor.h"

#include <cstdint>

namespace WasmEdge {
namespace Executor {

// Instantiate global instance. See "include/executor/executor.h".
Expect<void> Executor::instantiate(Runtime::StoreManager &StoreMgr,
                                   Runtime::StackManager &StackMgr,
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

  // Instantiate and initialize globals.
  for (const auto &GlobSeg : GlobSec.getContent()) {
    // Insert global instance to store manager.
    Runtime::Instance::GlobalInstance *GlobInst = nullptr;
    if (InsMode == InstantiateMode::Instantiate) {
      GlobInst = StoreMgr.pushGlobal(GlobSeg.getGlobalType());
    } else {
      GlobInst = StoreMgr.importGlobal(GlobSeg.getGlobalType());
    }
    // Set the global pointers of instantiated globals.
    ModInst.GlobalPtrs[ModInst.getGlobalNum()] = &(GlobInst->getValue());
    ModInst.addGlobal(GlobInst);

    // Run initialize expression.
    if (auto Res =
            runExpression(StoreMgr, StackMgr, GlobSeg.getExpr().getInstrs());
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
