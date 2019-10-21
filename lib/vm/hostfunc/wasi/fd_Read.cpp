#include "vm/hostfunc/wasi/fd_Read.h"
#include "executor/common.h"
#include "executor/worker/util.h"

namespace SSVM {
namespace Executor {

WasiFdRead::WasiFdRead(VM::WasiEnvironment &Env) : Wasi(Env) {
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendReturnDef(AST::ValType::I32);
}

ErrCode WasiFdRead::run(std::vector<std::unique_ptr<ValueEntry>> &Args,
                        std::vector<std::unique_ptr<ValueEntry>> &Res,
                        StoreManager &Store,
                        Instance::ModuleInstance *ModInst) {
  /// Arg: fd(u32), iovsPtr(u32), iovs_len(u32), nreadPtr(u32)
  if (Args.size() != 4) {
    return ErrCode::CallFunctionError;
  }
  ErrCode Status = ErrCode::Success;

  /// Return: errno(u32)
  Res.push_back(std::make_unique<ValueEntry>(0U));
  return Status;
}

} // namespace Executor
} // namespace SSVM