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
  /// Add a temp module to Store with only imported globals for initialization.
  auto TmpMod = std::make_unique<Runtime::Instance::ModuleInstance>("");
  for (uint32_t I = 0; I < ModInst.getGlobalImportNum(); ++I) {
    TmpMod->importGlobal(*ModInst.getGlobalAddr(I));
  }

  /// Insert the temp. module instance to Store.
  uint32_t TmpModInstAddr = StoreMgr.pushModule(TmpMod);

  /// Push a new frame {TmpModInst:{globaddrs}, locals:none}
  StackMgr.pushFrame(TmpModInstAddr, 0, 0);

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

  /// Stack is ensured in validation phase.
  StackMgr.popFrame();

  /// Pop the added temp. module.
  StoreMgr.popModule();
  return {};
}

} // namespace Interpreter
} // namespace SSVM
