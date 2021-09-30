// SPDX-License-Identifier: Apache-2.0

#include "interpreter/interpreter.h"

namespace WasmEdge {
namespace Interpreter {

/// Instantiate global instance. See "include/interpreter/interpreter.h".
Expect<void>
Interpreter::instantiate(Runtime::StoreManager &StoreMgr,
                         Runtime::Instance::ModuleInstance &ModInst,
                         const AST::GlobalSection &GlobSec) {
  /// A frame with temp. module is pushed into stack outside.
  /// Instantiate and initialize globals.
  for (const auto &GlobSeg : GlobSec.getContent()) {
    /// Insert global instance to store manager.
    const auto &GlobType = GlobSeg.getGlobalType();
    uint32_t NewGlobInstAddr;
    if (InsMode == InstantiateMode::Instantiate) {
      NewGlobInstAddr = StoreMgr.pushGlobal(GlobType.getValueType(),
                                            GlobType.getValueMutation());
    } else {
      NewGlobInstAddr = StoreMgr.importGlobal(GlobType.getValueType(),
                                              GlobType.getValueMutation());
    }
    ModInst.addGlobalAddr(NewGlobInstAddr);

    /// Run initialize expression.
    if (auto Res = runExpression(StoreMgr, GlobSeg.getInstrs()); !Res) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Expression));
      return Unexpect(Res);
    }

    /// Pop result from stack.
    auto *NewGlobInst = *StoreMgr.getGlobal(NewGlobInstAddr);
    NewGlobInst->getValue() = StackMgr.pop();
  }
  return {};
}

} // namespace Interpreter
} // namespace WasmEdge
