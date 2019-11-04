#include "vm/hostfunc/wasi/environ_Get.h"
#include "executor/common.h"
#include "executor/worker/util.h"

#include <string.h>

extern char **environ;

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
  /// Arg: EnvPtr(u32), EnvBufPtr(u32)
  if (Args.size() != 2) {
    return ErrCode::CallFunctionError;
  }
  ErrCode Status = ErrCode::Success;
  unsigned int EnvPtr = retrieveValue<uint32_t>(*Args[1].get());
  unsigned int EnvBufPtr = retrieveValue<uint32_t>(*Args[0].get());

  /// Get memory instance.
  unsigned int MemoryAddr = 0;
  Instance::MemoryInstance *MemInst = nullptr;
  if ((Status = ModInst->getMemAddr(0, MemoryAddr)) != ErrCode::Success) {
    return Status;
  }
  if ((Status = Store.getMemory(MemoryAddr, MemInst)) != ErrCode::Success) {
    return Status;
  }

  /// Store **Env.
  uint32_t EnvCnt = 0;
  uint32_t EnvBufOffset = EnvBufPtr;
  char *EnvString = *environ;
  std::vector<unsigned char> EnvBuf;
  while (EnvString != nullptr) {
    /// Concate EnvString.
    EnvBuf.insert(EnvBuf.end(), EnvString, EnvString + strlen(EnvString));
    EnvBuf.push_back('\0');

    /// Calculate Env[i] offset and store.
    if ((Status = MemInst->storeValue(EnvBufOffset, EnvPtr, 4)) !=
        ErrCode::Success) {
      return Status;
    }

    /// Shift one element.
    EnvCnt++;
    EnvPtr += 4;
    EnvBufOffset += strlen(EnvString) + 1;
    EnvString = *(environ + EnvCnt);
  }

  /// Store EnvBuf.
  if ((Status = MemInst->setBytes(EnvBuf, EnvBufPtr, 0, EnvBuf.size())) !=
      ErrCode::Success) {
    return Status;
  }

  /// Return: errno(u32)
  Res.push_back(std::make_unique<ValueEntry>(0U));
  return Status;
}

} // namespace Executor
} // namespace SSVM