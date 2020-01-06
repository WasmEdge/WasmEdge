// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/wasi/proc_Exit.h"

namespace SSVM {
namespace Executor {

WasiProcExit::WasiProcExit(VM::WasiEnvironment &Env) : Wasi(Env) {
  initializeFuncType<WasiProcExit>();
}

ErrCode WasiProcExit::run(VM::EnvironmentManager &EnvMgr,
                          StackManager &StackMgr,
                          Instance::MemoryInstance &MemInst) {
  return invoke<WasiProcExit>(EnvMgr, StackMgr, MemInst);
}

ErrCode WasiProcExit::body(VM::EnvironmentManager &EnvMgr,
                           Instance::MemoryInstance &MemInst, int32_t Status) {
  Env.setStatus(Status);
  return ErrCode::Terminated;
}

} // namespace Executor
} // namespace SSVM
