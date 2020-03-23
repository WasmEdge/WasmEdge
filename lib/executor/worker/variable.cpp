// SPDX-License-Identifier: Apache-2.0
#include "executor/common.h"
#include "executor/worker.h"
#include "common/value.h"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <vector>

namespace SSVM {
namespace Executor {

ErrCode Worker::runLocalGetOp(unsigned int Idx) {
  const unsigned int Offset = StackMgr.getOffset(Idx);
  Value *Var = nullptr;
  if (ErrCode Status = StackMgr.getBottomN(Offset, Var);
      Status != ErrCode::Success) {
    return Status;
  }
  return StackMgr.push(*Var);
}

ErrCode Worker::runLocalSetOp(unsigned int Idx) {
  Value Val;
  if (ErrCode Status = StackMgr.pop(Val); Status != ErrCode::Success) {
    return Status;
  }
  const unsigned int Offset = StackMgr.getOffset(Idx);
  Value *Var = nullptr;
  if (ErrCode Status = StackMgr.getBottomN(Offset, Var);
      Status != ErrCode::Success) {
    return Status;
  }
  *Var = Val;
  return ErrCode::Success;
}

ErrCode Worker::runLocalTeeOp(unsigned int Idx) {
  Value &Val = StackMgr.getTop();
  const unsigned int Offset = StackMgr.getOffset(Idx);
  Value *Var = nullptr;
  if (ErrCode Status = StackMgr.getBottomN(Offset, Var);
      Status != ErrCode::Success) {
    return Status;
  }
  *Var = Val;
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
