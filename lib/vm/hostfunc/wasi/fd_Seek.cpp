// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/wasi/fd_Seek.h"
#include <unistd.h>

namespace SSVM {
namespace Executor {

WasiFdSeek::WasiFdSeek(VM::WasiEnvironment &Env) : Wasi(Env) {
  initializeFuncType<WasiFdSeek>();
}

ErrCode WasiFdSeek::run(VM::EnvironmentManager &EnvMgr, StackManager &StackMgr,
                        Instance::MemoryInstance &MemInst) {
  return invoke<WasiFdSeek>(EnvMgr, StackMgr, MemInst);
}

ErrCode WasiFdSeek::body(VM::EnvironmentManager &EnvMgr,
                         Instance::MemoryInstance &MemInst, uint32_t &ErrNo,
                         int32_t Fd, int32_t Offset, uint32_t Whence,
                         uint32_t NewOffsetPtr) {
  /// Check directive whence.
  switch (Whence) {
  case __WASI_WHENCE_CUR:
    Whence = SEEK_CUR;
    break;
  case __WASI_WHENCE_END:
    Whence = SEEK_END;
    break;
  case __WASI_WHENCE_SET:
    Whence = SEEK_SET;
    break;
  default:
    ErrNo = __WASI_EINVAL;
    return ErrCode::Success;
    break;
  }

  /// Do lseek.
  int64_t NewOffset = lseek(Fd, Offset, Whence);
  if (NewOffset == -1) {
    /// TODO: errno
    ErrNo = 1U;
    return ErrCode::Success;
  }

  /// Store NewOffset.
  if (ErrCode Status = MemInst.storeValue(uint64_t(NewOffset), NewOffsetPtr, 8);
      Status != ErrCode::Success) {
    return Status;
  }

  ErrNo = 0U;
  return ErrCode::Success;
}

} // namespace Executor
} // namespace SSVM
