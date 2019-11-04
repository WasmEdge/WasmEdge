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
  /// Arg: ArgcPtr(u32), ArgvBufSizePtr(u32)
  if (Args.size() != 2) {
    return ErrCode::CallFunctionError;
  }
  ErrCode Status = ErrCode::Success;
  unsigned int ArgcPtr = retrieveValue<uint32_t>(*Args[1].get());
  unsigned int ArgvBufSizePtr = retrieveValue<uint32_t>(*Args[0].get());

  /// Get memory instance.
  unsigned int MemoryAddr = 0;
  Instance::MemoryInstance *MemInst = nullptr;
  if ((Status = ModInst->getMemAddr(0, MemoryAddr)) != ErrCode::Success) {
    return Status;
  }
  if ((Status = Store.getMemory(MemoryAddr, MemInst)) != ErrCode::Success) {
    return Status;
  }

  /// Store Argc.
  std::vector<std::string> &CmdArgs = Env.getCmdArgs();
  if ((Status = MemInst->storeValue((uint32_t)CmdArgs.size(), ArgcPtr, 4)) !=
      ErrCode::Success) {
    return Status;
  }

  /// Store ArgvBufSize.
  uint32_t CmdArgsSize = 0;
  for (auto It = CmdArgs.cbegin(); It != CmdArgs.cend(); It++) {
    CmdArgsSize += It->length() + 1;
  }
  if ((Status = MemInst->storeValue(CmdArgsSize, ArgvBufSizePtr, 4)) !=
      ErrCode::Success) {
    return Status;
  }

  /// Return: errno(u32)
  Res.push_back(std::make_unique<ValueEntry>(0U));
  return Status;
}

} // namespace Executor
} // namespace SSVM