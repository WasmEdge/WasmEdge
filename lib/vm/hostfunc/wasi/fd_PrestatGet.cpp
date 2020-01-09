// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/wasi/fd_PrestatGet.h"

namespace SSVM {
namespace Executor {

ErrCode WasiFdPrestatGet::body(VM::EnvironmentManager &EnvMgr,
                               Instance::MemoryInstance &MemInst,
                               uint32_t &ErrNo, int32_t Fd,
                               uint32_t PreStatPtr) {
  for (const auto &Entry : Env.getPreStats()) {
    if (Entry.Fd != Fd) {
      continue;
    }

    /// Store to memory instance
    /// byte[0] : pr_type
    if (ErrCode Status =
            MemInst.storeValue(uint32_t(Entry.Type), PreStatPtr, 1);
        Status != ErrCode::Success) {
      return Status;
    }
    /// byte[5:8] : u.dir.pr_name_len
    if (ErrCode Status =
            MemInst.storeValue(uint32_t(Entry.Path.size()), PreStatPtr + 4, 4);
        Status != ErrCode::Success) {
      return Status;
    }

    ErrNo = 0U;
    return ErrCode::Success;
  }
  ErrNo = __WASI_EBADF;
  return ErrCode::Success;
}

} // namespace Executor
} // namespace SSVM
