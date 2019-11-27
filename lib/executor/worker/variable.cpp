#include "executor/common.h"
#include "executor/worker.h"
#include "executor/worker/util.h"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <vector>

namespace SSVM {
namespace Executor {

ErrCode Worker::runLocalGetOp(unsigned int Idx) {
  Value *Val = nullptr;
  if (ErrCode Status = CurrentFrame->getValue(Idx, Val);
      Status != ErrCode::Success) {
    return Status;
  }
  return StackMgr.push(*Val);
}

ErrCode Worker::runLocalSetOp(unsigned int Idx) {
  Value Val;
  if (ErrCode Status = StackMgr.pop(Val); Status != ErrCode::Success) {
    return Status;
  }
  if (ErrCode Status = CurrentFrame->setValue(Idx, Val);
      Status != ErrCode::Success) {
    return Status;
  }
  return ErrCode::Success;
}

ErrCode Worker::runLocalTeeOp(unsigned int Idx) {
  Value &Val = StackMgr.getTop();
  if (ErrCode Status = CurrentFrame->setValue(Idx, Val);
      Status != ErrCode::Success) {
    return Status;
  }
  return ErrCode::Success;
}

ErrCode Worker::runGlobalGetOp(unsigned int Idx) {
  Instance::GlobalInstance *GlobInst = nullptr;
  Value Val;
  if (ErrCode Status = getGlobInstByIdx(Idx, GlobInst);
      Status != ErrCode::Success) {
    return Status;
  };
  if (ErrCode Status = GlobInst->getValue(Val); Status != ErrCode::Success) {
    return Status;
  }
  return StackMgr.push(Val);
}

ErrCode Worker::runGlobalSetOp(unsigned int Idx) {
  Value Val;
  Instance::GlobalInstance *GlobInst = nullptr;
  if (ErrCode Status = getGlobInstByIdx(Idx, GlobInst);
      Status != ErrCode::Success) {
    return Status;
  };
  if (ErrCode Status = StackMgr.pop(Val); Status != ErrCode::Success) {
    return Status;
  }
  return GlobInst->setValue(Val);
}

} // namespace Executor
} // namespace SSVM
