// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/wasi/fd_Close.h"
#include "executor/common.h"
#include "executor/worker/util.h"

#include <unistd.h>

namespace SSVM {
namespace Executor {

WasiFdClose::WasiFdClose(VM::WasiEnvironment &Env) : Wasi(Env) {
  appendParamDef(AST::ValType::I32);
  appendReturnDef(AST::ValType::I32);
}

ErrCode WasiFdClose::run(VM::EnvironmentManager &EnvMgr, std::vector<Value> &Args,
                      std::vector<Value> &Res,
                         StoreManager &Store,
                         Instance::ModuleInstance *ModInst) {
  /// Arg: fd(u32)
  if (Args.size() != 1) {
    return ErrCode::CallFunctionError;
  }
  ErrCode Status = ErrCode::Success;
  unsigned int Fd = retrieveValue<uint32_t>(Args[0]);

  /// Close Fd.
  int ErrNo = close(Fd);

  /// Return: errno(u32)
  if (ErrNo == 0) {
    Res[0] = uint32_t(0U);
  } else {
    /// TODO: errno
    Res[0] = uint32_t(1U);
  }
  return Status;
}

} // namespace Executor
} // namespace SSVM
