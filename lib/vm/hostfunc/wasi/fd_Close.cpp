// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/wasi/fd_Close.h"
#include <unistd.h>

namespace SSVM {
namespace Executor {

WasiFdClose::WasiFdClose(VM::WasiEnvironment &Env) : Wasi(Env) {
  initializeFuncType<WasiFdClose>();
}

ErrCode WasiFdClose::run(VM::EnvironmentManager &EnvMgr, StackManager &StackMgr,
                         Instance::MemoryInstance &MemInst) {
  return invoke<WasiFdClose>(EnvMgr, StackMgr, MemInst);
}

ErrCode WasiFdClose::body(VM::EnvironmentManager &EnvMgr,
                          Instance::MemoryInstance &MemInst, uint32_t &ErrNo,
                          int32_t Fd) {
  if (close(Fd) != 0) {
    /// TODO: errno
    ErrNo = 1U;
  } else {
    ErrNo = 0U;
  }
  return ErrCode::Success;
}

} // namespace Executor
} // namespace SSVM
