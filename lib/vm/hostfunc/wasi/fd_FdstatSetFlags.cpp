#include "vm/hostfunc/wasi/fd_FdstatSetFlags.h"
#include "executor/common.h"
#include "executor/worker/util.h"

namespace SSVM {
namespace Executor {

WasiFdFdstatSetFlags::WasiFdFdstatSetFlags(VM::WasiEnvironment &Env)
    : Wasi(Env) {
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendReturnDef(AST::ValType::I32);
}

ErrCode WasiFdFdstatSetFlags::run(std::vector<Value> &Args,
                                  std::vector<Value> &Res, StoreManager &Store,
                                  Instance::ModuleInstance *ModInst) {
  /// Arg: fd, iovsPtr, iovs_len, nreadPtr
  if (Args.size() != 2) {
    return ErrCode::CallFunctionError;
  }
  ErrCode Status = ErrCode::Success;

  /// Return: errno(u32)
  Res[0] = uint32_t(0U);
  return Status;
}

} // namespace Executor
} // namespace SSVM
