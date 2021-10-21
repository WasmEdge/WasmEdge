// SPDX-License-Identifier: Apache-2.0

#include "executor/executor.h"

namespace WasmEdge {
namespace Executor {

/// Instantiate global instance. See "include/executor/executor.h".
Expect<void> Executor::instantiate(Runtime::StoreManager &StoreMgr,
                                   Runtime::Instance::ModuleInstance &ModInst,
                                   const AST::GlobalSection &GlobSec) {
  /// A frame with temp. module is pushed into stack outside.
  /// Instantiate and initialize globals.
  for (const auto &GlobSeg : GlobSec.getContent()) {
    /// Insert global instance to store manager.
    uint32_t NewGlobInstAddr;
    if (InsMode == InstantiateMode::Instantiate) {
      NewGlobInstAddr = StoreMgr.pushGlobal(GlobSeg.getGlobalType());
    } else {
      NewGlobInstAddr = StoreMgr.importGlobal(GlobSeg.getGlobalType());
    }
    ModInst.addGlobalAddr(NewGlobInstAddr);

    /// Run initialize expression.
    if (auto Res = runExpression(StoreMgr, GlobSeg.getExpr().getInstrs());
        !Res) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Expression));
      return Unexpect(Res);
    }

    /// Pop result from stack.
    auto *NewGlobInst = *StoreMgr.getGlobal(NewGlobInstAddr);
    NewGlobInst->getValue() = StackMgr.pop();
  }
  return {};
}

} // namespace Executor
} // namespace WasmEdge
