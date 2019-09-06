#include "executor/common.h"
#include "executor/worker.h"
#include "executor/worker/util.h"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <vector>

namespace SSVM {
namespace Executor {

/// Helper function of getting global instance by index.
ErrCode getGlobInstByIdx(StoreManager &Store, FrameEntry &CurrFrame,
                         unsigned int Idx,
                         Instance::GlobalInstance *&GlobInst) {
  ErrCode Status = ErrCode::Success;
  Instance::ModuleInstance *ModInst = nullptr;
  unsigned int GlobalAddr = 0;
  unsigned int ModuleAddr = CurrFrame.getModuleAddr();
  if ((Status = Store.getModule(ModuleAddr, ModInst)) != ErrCode::Success) {
    return Status;
  };
  if ((Status = ModInst->getGlobalAddr(Idx, GlobalAddr)) != ErrCode::Success) {
    return Status;
  };
  return Store.getGlobal(GlobalAddr, GlobInst);
}

ErrCode Worker::runLocalGetOp(unsigned int Idx) {
  ErrCode Status = ErrCode::Success;
  ValueEntry *ValEntry = nullptr;
  if ((Status = CurrentFrame->getValue(Idx, ValEntry)) != ErrCode::Success) {
    return Status;
  }
  return StackMgr.push(std::make_unique<ValueEntry>(*ValEntry));
}

ErrCode Worker::runLocalSetOp(unsigned int Idx) {
  ErrCode Status = ErrCode::Success;
  std::unique_ptr<ValueEntry> ValEntry;
  if ((Status = StackMgr.pop(ValEntry)) != ErrCode::Success) {
    return Status;
  }
  return CurrentFrame->setValue(Idx, *ValEntry.get());
}

ErrCode Worker::runLocalTeeOp(unsigned int Idx) {
  ErrCode Status = ErrCode::Success;
  ValueEntry *ValEntry = nullptr;
  if ((Status = StackMgr.getTop(ValEntry)) != ErrCode::Success) {
    return Status;
  }
  return CurrentFrame->setValue(Idx, *ValEntry);
}

ErrCode Worker::runGlobalGetOp(unsigned int Idx) {
  Instance::GlobalInstance *GlobInst = nullptr;
  AST::ValVariant Val;
  ErrCode Status = ErrCode::Success;
  if ((Status = getGlobInstByIdx(StoreMgr, *CurrentFrame, Idx, GlobInst)) !=
      ErrCode::Success) {
    return Status;
  };
  if ((Status = GlobInst->getValue(Val)) != ErrCode::Success) {
    return Status;
  }
  return StackMgr.push(
      std::make_unique<ValueEntry>(GlobInst->getValType(), Val));
}

ErrCode Worker::runGlobalSetOp(unsigned int Idx) {
  std::unique_ptr<ValueEntry> ValEntry;
  AST::ValVariant Val;
  Instance::GlobalInstance *GlobInst = nullptr;
  ErrCode Status = ErrCode::Success;
  if ((Status = getGlobInstByIdx(StoreMgr, *CurrentFrame, Idx, GlobInst)) !=
      ErrCode::Success) {
    return Status;
  };
  if ((Status = StackMgr.pop(ValEntry)) != ErrCode::Success) {
    return Status;
  }
  if ((Status = ValEntry->getValue(Val)) != ErrCode::Success) {
    return Status;
  }
  return GlobInst->setValue(Val);
}

} // namespace Executor
} // namespace SSVM