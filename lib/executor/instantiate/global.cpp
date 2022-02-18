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
  for (uint32_t I = 0; I < ModInst.getGlobalNum(); ++I) {
    ModInst.GlobalPtrs[I] =
        &(*StoreMgr.getGlobal(*ModInst.getGlobalAddr(I)))->getValue();
  }

  // Instantiate and initialize globals.
  for (const auto &GlobSeg : GlobSec.getContent()) {
    // Insert global instance to store manager.
    uint32_t NewGlobInstAddr;
    if (InsMode == InstantiateMode::Instantiate) {
      NewGlobInstAddr = StoreMgr.pushGlobal(GlobSeg.getGlobalType());
    } else {
      NewGlobInstAddr = StoreMgr.importGlobal(GlobSeg.getGlobalType());
    }
    ModInst.GlobalPtrs[ModInst.getGlobalNum()] =
        &(*StoreMgr.getGlobal(NewGlobInstAddr))->getValue();
    ModInst.addGlobalAddr(NewGlobInstAddr);

    // Run initialize expression.
    if (auto Res =
            runExpression(StoreMgr, StackMgr, GlobSeg.getExpr().getInstrs());
        !Res) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Expression));
      return Unexpect(Res);
    }

    // Pop result from stack.
    auto *NewGlobInst = *StoreMgr.getGlobal(NewGlobInstAddr);
    NewGlobInst->getValue() = StackMgr.pop();
  }
  return {};
}

} // namespace Executor
} // namespace WasmEdge
