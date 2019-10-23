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

ErrCode WasiFdClose::run(std::vector<std::unique_ptr<ValueEntry>> &Args,
                         std::vector<std::unique_ptr<ValueEntry>> &Res,
                         StoreManager &Store,
                         Instance::ModuleInstance *ModInst) {
  /// Arg: fd(u32)
  if (Args.size() != 1) {
    return ErrCode::CallFunctionError;
  }
  ErrCode Status = ErrCode::Success;
  unsigned int Fd = retrieveValue<uint32_t>(*Args[0].get());

  /// Close Fd.
  int ErrNo = close(Fd);

  /// Return: errno(u32)
  if (ErrNo == 0) {
    Res.push_back(std::make_unique<ValueEntry>(0U));
  } else {
    /// TODO: errno
    Res.push_back(std::make_unique<ValueEntry>(1U));
  }
  return Status;
}

} // namespace Executor
} // namespace SSVM