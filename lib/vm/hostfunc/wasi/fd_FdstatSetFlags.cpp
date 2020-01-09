// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/wasi/fd_FdstatSetFlags.h"

namespace SSVM {
namespace Executor {

ErrCode WasiFdFdstatSetFlags::body(VM::EnvironmentManager &EnvMgr,
                                   Instance::MemoryInstance &MemInst,
                                   uint32_t &ErrNo, int32_t Fd,
                                   uint32_t FsFlags) {
  /// TODO: implement
  ErrNo = 0U;
  return ErrCode::Success;
}

} // namespace Executor
} // namespace SSVM
