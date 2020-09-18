// SPDX-License-Identifier: Apache-2.0
#include "runtime/instance/global.h"
#include "common/ast/section.h"
#include "interpreter/interpreter.h"
#include "runtime/instance/module.h"

namespace SSVM {
namespace Interpreter {

/// Instantiate global instance. See "include/interpreter/interpreter.h".
Expect<void>
Interpreter::instantiate(Runtime::StoreManager &StoreMgr,
                         Runtime::Instance::ModuleInstance &ModInst,
                         const AST::GlobalSection &GlobSec) {
  /// A frame with temp. module is pushed into stack outside.
  /// Instantiate and initialize globals.
  for (const auto &GlobSeg : GlobSec.getContent()) {
    /// Make a new global instance.
    auto *GlobType = GlobSeg->getGlobalType();
    auto NewGlobInst = std::make_unique<Runtime::Instance::GlobalInstance>(
        GlobType->getValueType(), GlobType->getValueMutation());

    /// Run initialize expression.
    if (auto Res = runExpression(StoreMgr, GlobSeg->getInstrs()); !Res) {
      LOG(ERROR) << ErrInfo::InfoAST(ASTNodeAttr::Expression);
      return Unexpect(Res);
    }

    /// Pop result from stack.
    NewGlobInst->getValue() = StackMgr.pop();

    /// Insert global instance to store manager.
    uint32_t NewGlobInstAddr;
    if (InsMode == InstantiateMode::Instantiate) {
      NewGlobInstAddr = StoreMgr.pushGlobal(NewGlobInst);
    } else {
      NewGlobInstAddr = StoreMgr.importGlobal(NewGlobInst);
    }
    ModInst.addGlobalAddr(NewGlobInstAddr);
  }
  return {};
}

} // namespace Interpreter
} // namespace SSVM
