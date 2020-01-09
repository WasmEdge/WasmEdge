// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/wasi/proc_Exit.h"

namespace SSVM {
namespace Executor {

ErrCode WasiProcExit::body(VM::EnvironmentManager &EnvMgr,
                           Instance::MemoryInstance &MemInst, int32_t Status) {
  Env.setStatus(Status);
  return ErrCode::Terminated;
}

} // namespace Executor
} // namespace SSVM
