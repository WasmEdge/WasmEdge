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
  ErrCode Status = ErrCode::Success;

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
    if ((Status = StoreMgr.insertGlobalInst(NewGlobInst, NewGlobInstId)) !=
        ErrCode::Success) {
      return Status;
    }

    /// Set external value (global address) to module instance.
    if ((Status = ModInst->addGlobalAddr(NewGlobInstId)) != ErrCode::Success) {
      return Status;
    }

    /// Set external value (global address) to temp module instance.
    if ((Status = TmpMod->addGlobalAddr(NewGlobInstId)) != ErrCode::Success) {
      return Status;
    }
  }

  /// Initialize the globals
  /// Insert the temp. module instance to Store
  unsigned int TmpModInstId = 0;
  if ((Status = StoreMgr.insertModuleInst(TmpMod, TmpModInstId)) !=
      ErrCode::Success) {
    return Status;
  }
  Instance::ModuleInstance *TmpModInst = nullptr;
  if ((Status = StoreMgr.getModule(TmpModInstId, TmpModInst)) !=
      ErrCode::Success) {
    return Status;
  }

  /// Make a new frame {NewModInst:{globaddrs}, locals:none} and push
  auto Frame = MemPool.allocFrameEntry(TmpModInstId, 0);
  StackMgr.push(Frame);

  /// Evaluate values and set to global instance.
  auto GlobSeg = GlobSegs.begin();
  for (unsigned int I = 0; I < TmpModInst->getGlobalNum(); I++, GlobSeg++) {
    /// Set init instrs to engine and run.
    if ((Status = Engine.runExpression((*GlobSeg)->getInstrs())) !=
        ErrCode::Success) {
      return Status;
    }

    /// Pop result from stack.
    std::unique_ptr<ValueEntry> PopVal;
    StackMgr.pop(PopVal);

    /// Get global instance from store.
    unsigned int GlobalAddr = 0;
    TmpModInst->getGlobalAddr(I, GlobalAddr);
    Instance::GlobalInstance *GlobInst;
    StoreMgr.getGlobal(GlobalAddr, GlobInst);

    /// Set value from value entry to global instance.
    AST::ValVariant Val;
    PopVal->getValue(Val);
    GlobInst->setValue(Val);
    MemPool.destroyValueEntry(std::move(PopVal));
  }

  /// Pop Frame
  Status = StackMgr.pop();

  /// TODO: Delete the temp. module instance
  return Status;
}

} // namespace Executor
} // namespace SSVM