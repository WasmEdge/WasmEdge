// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/wasi/proc_Exit.h"
#include "executor/common.h"
#include "executor/worker/util.h"

namespace SSVM {
namespace Executor {

WasiProcExit::WasiProcExit(VM::WasiEnvironment &Env) : Wasi(Env) {
  appendParamDef(AST::ValType::I32);
}

ErrCode WasiProcExit::run(std::vector<Value> &Args, std::vector<Value> &Res,
                          StoreManager &Store,
                          Instance::ModuleInstance *ModInst) {
  /// Arg: errno(u32)
  if (Args.size() != 1) {
    return ErrCode::CallFunctionError;
  }
  /// TODO: push errno to stack.

  /// Return: void
  return ErrCode::Terminated;
}

} // namespace Executor
} // namespace SSVM
