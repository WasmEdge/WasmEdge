#include "vm/hostfunc/wasi/args_SizesGet.h"
#include "executor/common.h"
#include "executor/worker/util.h"

namespace SSVM {
namespace Executor {

WasiArgsSizesGet::WasiArgsSizesGet(VM::WasiEnvironment &Env) : Wasi(Env) {
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendReturnDef(AST::ValType::I32);
}

ErrCode WasiArgsSizesGet::run(std::vector<std::unique_ptr<ValueEntry>> &Args,
                              std::vector<std::unique_ptr<ValueEntry>> &Res,
                              StoreManager &Store,
                              Instance::ModuleInstance *ModInst) {
  /// Arg: argcPtr(u32), argv_bufPtr(u32)
  if (Args.size() != 2) {
    return ErrCode::CallFunctionError;
  }
  ErrCode Status = ErrCode::Success;

  /// Return: errno(u32)
  Res.push_back(std::make_unique<ValueEntry>(0U));
  return Status;
}

} // namespace Executor
} // namespace SSVM