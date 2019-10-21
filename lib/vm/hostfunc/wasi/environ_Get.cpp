#include "vm/hostfunc/wasi/environ_Get.h"
#include "executor/common.h"
#include "executor/worker/util.h"

namespace SSVM {
namespace Executor {

WasiEnvironGet::WasiEnvironGet(VM::WasiEnvironment &Env) : Wasi(Env) {
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendReturnDef(AST::ValType::I32);
}

ErrCode WasiEnvironGet::run(std::vector<std::unique_ptr<ValueEntry>> &Args,
                            std::vector<std::unique_ptr<ValueEntry>> &Res,
                            StoreManager &Store,
                            Instance::ModuleInstance *ModInst) {
  /// Arg: environ(u32), environ_buf(u32)
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