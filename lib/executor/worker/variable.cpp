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
  ErrCode Status = ErrCode::Success;
  Value *Val = nullptr;
  if ((Status = CurrentFrame->getValue(Idx, Val)) != ErrCode::Success) {
    return Status;
  }
  return StackMgr.push(*Val);
}

ErrCode Worker::runLocalSetOp(unsigned int Idx) {
  ErrCode Status = ErrCode::Success;
  Value Val;
  if ((Status = StackMgr.pop(Val)) != ErrCode::Success) {
    return Status;
  }
  if ((Status = CurrentFrame->setValue(Idx, Val)) != ErrCode::Success) {
    return Status;
  }
  return Status;
}

ErrCode Worker::runLocalTeeOp(unsigned int Idx) {
  ErrCode Status = ErrCode::Success;
  Value *Val = nullptr;
  if ((Status = StackMgr.getTop(Val)) != ErrCode::Success) {
    return Status;
  }
  return CurrentFrame->setValue(Idx, *Val);
}

ErrCode Worker::runGlobalGetOp(unsigned int Idx) {
  Instance::GlobalInstance *GlobInst = nullptr;
  Value Val;
  ErrCode Status = ErrCode::Success;
  if ((Status = getGlobInstByIdx(Idx, GlobInst)) != ErrCode::Success) {
    return Status;
  };
  if ((Status = GlobInst->getValue(Val)) != ErrCode::Success) {
    return Status;
  }
  return StackMgr.push(Val);
}

ErrCode Worker::runGlobalSetOp(unsigned int Idx) {
  Value Val;
  Instance::GlobalInstance *GlobInst = nullptr;
  ErrCode Status = ErrCode::Success;
  if ((Status = getGlobInstByIdx(Idx, GlobInst)) != ErrCode::Success) {
    return Status;
  };
  if ((Status = StackMgr.pop(Val)) != ErrCode::Success) {
    return Status;
  }
  return GlobInst->setValue(Val);
}

} // namespace Executor
} // namespace SSVM
