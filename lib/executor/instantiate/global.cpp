// SPDX-License-Identifier: Apache-2.0
#include "executor/instance/global.h"
#include "ast/section.h"
#include "executor/executor.h"
#include "executor/instance/module.h"

namespace SSVM {
namespace Executor {

/// Instantiate global instance. See "include/executor/executor.h".
ErrCode Executor::instantiate(AST::GlobalSection *GlobSec) {
  if (GlobSec == nullptr) {
    return ErrCode::Success;
  }

  /// Add a temp module to Store for initialization
  auto TmpMod = std::make_unique<Instance::ModuleInstance>();

  /// Iterate and instantiate global segments.
  auto &GlobSegs = GlobSec->getContent();
  for (auto GlobSeg = GlobSegs.begin(); GlobSeg != GlobSegs.end(); GlobSeg++) {
    /// Make a new global instance.
    auto *GlobType = (*GlobSeg)->getGlobalType();
    auto NewGlobInst = std::make_unique<Instance::GlobalInstance>(
        GlobType->getValueType(), GlobType->getValueMutation());
    unsigned int NewGlobInstId = 0;

    /// Insert global instance to store manager.
    if (ErrCode Status = StoreMgr.insertGlobalInst(NewGlobInst, NewGlobInstId);
        Status != ErrCode::Success) {
      return Status;
    }

    /// Set external value (global address) to module instance.
    if (ErrCode Status = ModInst->addGlobalAddr(NewGlobInstId);
        Status != ErrCode::Success) {
      return Status;
    }

    /// Set external value (global address) to temp module instance.
    if (ErrCode Status = TmpMod->addGlobalAddr(NewGlobInstId);
        Status != ErrCode::Success) {
      return Status;
    }
  }

  /// Initialize the globals
  /// Insert the temp. module instance to Store
  /// FIXME: Use pointer in frame instead of insert into Store.
  unsigned int TmpModInstId = 0;
  if (ErrCode Status = StoreMgr.insertModuleInst(TmpMod, TmpModInstId);
      Status != ErrCode::Success) {
    return Status;
  }
  Instance::ModuleInstance *TmpModInst = nullptr;
  if (ErrCode Status = StoreMgr.getModule(TmpModInstId, TmpModInst);
      Status != ErrCode::Success) {
    return Status;
  }

  /// Make a new frame {NewModInst:{globaddrs}, locals:none} and push
  StackMgr.pushFrame(TmpModInstId, /// Module address
                     0,            /// Arity
                     0             /// Coarity
  );

  /// Evaluate values and set to global instance.
  auto GlobSeg = GlobSegs.begin();
  for (unsigned int I = 0; I < TmpModInst->getGlobalNum(); I++, GlobSeg++) {
    /// Set init instrs to engine and run.
    if (ErrCode Status = Engine.runExpression((*GlobSeg)->getInstrs());
        Status != ErrCode::Success) {
      return Status;
    }

    /// Pop result from stack.
    Value PopVal;
    StackMgr.pop(PopVal);

    /// Get global instance from store.
    unsigned int GlobalAddr = 0;
    TmpModInst->getGlobalAddr(I, GlobalAddr);
    Instance::GlobalInstance *GlobInst;
    StoreMgr.getGlobal(GlobalAddr, GlobInst);

    /// Set value from value entry to global instance.
    GlobInst->setValue(PopVal);
  }

  /// Pop Frame
  if (ErrCode Status = StackMgr.popFrame(); Status != ErrCode::Success) {
    return Status;
  }

  /// FIXME: Use a temporary module instance instead of inserting to Store.
  return StoreMgr.popModuleInst();
}

} // namespace Executor
} // namespace SSVM
