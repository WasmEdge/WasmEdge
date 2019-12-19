// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/wasi/fd_Seek.h"
#include "executor/common.h"
#include "executor/worker/util.h"

#include <unistd.h>

namespace SSVM {
namespace Executor {

WasiFdSeek::WasiFdSeek(VM::WasiEnvironment &Env) : Wasi(Env) {
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I64);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendReturnDef(AST::ValType::I32);
}

ErrCode WasiFdSeek::run(VM::EnvironmentManager &EnvMgr,
                        std::vector<Value> &Args, std::vector<Value> &Res,
                        StoreManager &Store,
                        Instance::ModuleInstance *ModInst) {
  /// Arg: Fd(u32), Offset(u64), Whence(u32), NewOffsetPtr(u32)
  if (Args.size() != 4) {
    return ErrCode::CallFunctionError;
  }
  ErrCode Status = ErrCode::Success;
  unsigned int Fd = retrieveValue<uint32_t>(Args[3]);
  unsigned int Offset = retrieveValue<uint64_t>(Args[2]);
  unsigned int Whence = retrieveValue<uint32_t>(Args[1]);
  unsigned int NewOffsetPtr = retrieveValue<uint32_t>(Args[0]);
  int ErrNo = 0;

  /// Check directive whence.
  switch (Whence) {
  case __WASI_WHENCE_CUR:
    Whence = SEEK_CUR;
    break;
  case __WASI_WHENCE_END:
    Whence = SEEK_END;
    break;
  case __WASI_WHENCE_SET:
    Whence = SEEK_SET;
    break;
  default:
    /// TODO: Error handling
    break;
  }

  /// Do lseek.
  int64_t NewOffset = lseek(Fd, Offset, Whence);
  if (NewOffset == -1) {
    ErrNo = 1;
  }

  /// Store NewOffset.
  if (ErrNo == 0) {
    unsigned int MemoryAddr = 0;
    Instance::MemoryInstance *MemInst = nullptr;
    if ((Status = ModInst->getMemAddr(0, MemoryAddr)) != ErrCode::Success) {
      return Status;
    }
    if ((Status = Store.getMemory(MemoryAddr, MemInst)) != ErrCode::Success) {
      return Status;
    }
    if ((Status = MemInst->storeValue((uint64_t)NewOffset, NewOffsetPtr, 8)) !=
        ErrCode::Success) {
      return Status;
    }
  }

  /// Return: errno(u32)
  if (ErrNo == 0) {
    Res[0] = uint32_t(0U);
  } else {
    /// TODO: errno
    Res[0] = uint32_t(1U);
  }
  return Status;
}

} // namespace Executor
} // namespace SSVM
