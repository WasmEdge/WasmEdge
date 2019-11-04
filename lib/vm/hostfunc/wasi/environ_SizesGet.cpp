#include "vm/hostfunc/wasi/environ_SizesGet.h"
#include "executor/common.h"
#include "executor/worker/util.h"

#include <string.h>

extern char **environ;

namespace SSVM {
namespace Executor {

WasiEnvironSizesGet::WasiEnvironSizesGet(VM::WasiEnvironment &Env) : Wasi(Env) {
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendReturnDef(AST::ValType::I32);
}

ErrCode WasiEnvironSizesGet::run(std::vector<std::unique_ptr<ValueEntry>> &Args,
                                 std::vector<std::unique_ptr<ValueEntry>> &Res,
                                 StoreManager &Store,
                                 Instance::ModuleInstance *ModInst) {
  /// Arg: EnvCntPtr(u32), EnvBufSizePtr(u32)
  if (Args.size() != 2) {
    return ErrCode::CallFunctionError;
  }
  ErrCode Status = ErrCode::Success;
  unsigned int EnvCntPtr = retrieveValue<uint32_t>(*Args[1].get());
  unsigned int EnvBufSizePtr = retrieveValue<uint32_t>(*Args[0].get());

  /// Get memory instance.
  unsigned int MemoryAddr = 0;
  Instance::MemoryInstance *MemInst = nullptr;
  if ((Status = ModInst->getMemAddr(0, MemoryAddr)) != ErrCode::Success) {
    return Status;
  }
  if ((Status = Store.getMemory(MemoryAddr, MemInst)) != ErrCode::Success) {
    return Status;
  }

  /// Calculate EnvCnt and EnvBufSize.
  uint32_t EnvCnt = 0;
  uint32_t EnvBufSize = 0;
  char *EnvString = *environ;
  while (EnvString != nullptr) {
    EnvBufSize += strlen(EnvString) + 1;
    EnvCnt++;
    EnvString = *(environ + EnvCnt);
  }

  /// Store EnvCnt and EnvBufSize.
  if ((Status = MemInst->storeValue(EnvCnt, EnvCntPtr, 4)) !=
      ErrCode::Success) {
    return Status;
  }
  if ((Status = MemInst->storeValue(EnvBufSize, EnvBufSizePtr, 4)) !=
      ErrCode::Success) {
    return Status;
  }

  /// Return: errno(u32)
  Res.push_back(std::make_unique<ValueEntry>(0U));
  return Status;
}

} // namespace Executor
} // namespace SSVM