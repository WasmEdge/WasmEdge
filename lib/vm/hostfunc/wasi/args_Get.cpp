#include "vm/hostfunc/wasi/args_Get.h"
#include "executor/common.h"
#include "executor/worker/util.h"

namespace SSVM {
namespace Executor {

WasiArgsGet::WasiArgsGet(VM::WasiEnvironment &Env) : Wasi(Env) {
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendReturnDef(AST::ValType::I32);
}

ErrCode WasiArgsGet::run(std::vector<std::unique_ptr<ValueEntry>> &Args,
                         std::vector<std::unique_ptr<ValueEntry>> &Res,
                         StoreManager &Store,
                         Instance::ModuleInstance *ModInst) {
  /// Arg: ArgvPtr(u32), ArgvBufPtr(u32)
  if (Args.size() != 2) {
    return ErrCode::CallFunctionError;
  }
  ErrCode Status = ErrCode::Success;
  unsigned int ArgvPtr = retrieveValue<uint32_t>(*Args[1].get());
  unsigned int ArgvBufPtr = retrieveValue<uint32_t>(*Args[0].get());

  /// Get memory instance.
  unsigned int MemoryAddr = 0;
  Instance::MemoryInstance *MemInst = nullptr;
  if ((Status = ModInst->getMemAddr(0, MemoryAddr)) != ErrCode::Success) {
    return Status;
  }
  if ((Status = Store.getMemory(MemoryAddr, MemInst)) != ErrCode::Success) {
    return Status;
  }

  /// Store **Argv and ArgvBuf.
  std::vector<std::string> &CmdArgs = Env.getCmdArgs();
  std::vector<unsigned char> ArgvBuf;
  for (auto It = CmdArgs.cbegin(); It != CmdArgs.cend(); It++) {
    ArgvBuf.clear();
    std::copy(It->cbegin(), It->cend(), std::back_inserter(ArgvBuf));
    ArgvBuf.push_back('\0');

    if ((Status = MemInst->setBytes(ArgvBuf, ArgvBufPtr, 0, ArgvBuf.size())) !=
        ErrCode::Success) {
      return Status;
    }
    if ((Status = MemInst->storeValue(ArgvPtr, 4, ArgvBufPtr)) !=
        ErrCode::Success) {
      return Status;
    }
    ArgvPtr += 4;
    ArgvBufPtr += ArgvBuf.size();
  }

  /// Return: errno(u32)
  Res.push_back(std::make_unique<ValueEntry>(0U));
  return Status;
}

} // namespace Executor
} // namespace SSVM