// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/wasi/fd_PrestatDirName.h"

namespace SSVM {
namespace Executor {

ErrCode WasiFdPrestatDirName::body(VM::EnvironmentManager &EnvMgr,
                                   Instance::MemoryInstance &MemInst,
                                   uint32_t &ErrNo, int32_t Fd,
                                   uint32_t PathBufPtr, uint32_t PathLen) {
  for (auto &Entry : Env.getPreStats()) {
    if (Entry.Fd != Fd) {
      continue;
    }
    if (Entry.Path.size() > PathLen) {
      ErrNo = __WASI_EINVAL;
      return ErrCode::Success;
    }

    /// Store Path and PathLen.
    if (ErrCode Status =
            MemInst.setBytes(Entry.Path, PathBufPtr, 0, Entry.Path.size());
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
