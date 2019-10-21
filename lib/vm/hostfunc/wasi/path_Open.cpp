#include "vm/hostfunc/wasi/path_Open.h"
#include "executor/common.h"
#include "executor/worker/util.h"

namespace SSVM {
namespace Executor {

WasiPathOpen::WasiPathOpen(VM::WasiEnvironment &Env) : Wasi(Env) {
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I64);
  appendParamDef(AST::ValType::I64);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendReturnDef(AST::ValType::I32);
}

ErrCode WasiPathOpen::run(std::vector<std::unique_ptr<ValueEntry>> &Args,
                          std::vector<std::unique_ptr<ValueEntry>> &Res,
                          StoreManager &Store,
                          Instance::ModuleInstance *ModInst) {
  /// Arg: dirfd(u32), dirflags(u32), pathPtr(u32), path_len(u32), oflags(u32),
  /// fs_rights_base(u64), fs_rights_inheriting(u64), fs_flags(u32), fdPtr(u32)
  if (Args.size() != 9) {
    return ErrCode::CallFunctionError;
  }
  ErrCode Status = ErrCode::Success;

  /// Return: errno(u32)
  Res.push_back(std::make_unique<ValueEntry>(0U));
  return Status;
}

} // namespace Executor
} // namespace SSVM