#include "vm/hostfunc/wasi/fd_Write.h"
#include "executor/common.h"
#include "executor/worker/util.h"

#include <unistd.h>

namespace SSVM {
namespace Executor {

WasiFdWrite::WasiFdWrite(VM::WasiEnvironment &Env) : Wasi(Env) {
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendReturnDef(AST::ValType::I32);
}

ErrCode WasiFdWrite::run(std::vector<Value> &Args, std::vector<Value> &Res,
                         StoreManager &Store,
                         Instance::ModuleInstance *ModInst) {
  /// Arg: Fd(u32), IOVSPtr(u32), IOVSCnt(u32), NWrittenPtr(u32)
  if (Args.size() != 4) {
    return ErrCode::CallFunctionError;
  }
  ErrCode Status = ErrCode::Success;
  unsigned int Fd = retrieveValue<uint32_t>(Args[3]);
  unsigned int IOVSPtr = retrieveValue<uint32_t>(Args[2]);
  unsigned int IOVSCnt = retrieveValue<uint32_t>(Args[1]);
  unsigned int NWrittenPtr = retrieveValue<uint32_t>(Args[0]);
  int ErrNo = 0;

  /// Get memory instance.
  unsigned int MemoryAddr = 0;
  Instance::MemoryInstance *MemInst = nullptr;
  if ((Status = ModInst->getMemAddr(0, MemoryAddr)) != ErrCode::Success) {
    return Status;
  }
  if ((Status = Store.getMemory(MemoryAddr, MemInst)) != ErrCode::Success) {
    return Status;
  }

  /// Sequencially writting.
  unsigned int NWritten = 0;
  std::vector<unsigned char> Data;
  for (unsigned int I = 0; I < IOVSCnt && ErrNo == 0; I++) {
    uint64_t CIOVecBufPtr = 0;
    uint64_t CIOVecBufLen = 0;
    Data.clear();
    /// TODO: sizeof(ptr) is 32-bit in wasm now.
    /// Get data offset.
    if ((Status = MemInst->loadValue(CIOVecBufPtr, IOVSPtr, 4)) !=
        ErrCode::Success) {
      return Status;
    }
    /// Get data length.
    if ((Status = MemInst->loadValue(CIOVecBufLen, IOVSPtr + 4, 4)) !=
        ErrCode::Success) {
      return Status;
    }
    /// Get data.
    if ((Status = MemInst->getBytes(Data, (uint32_t)CIOVecBufPtr,
                                    (uint32_t)CIOVecBufLen)) !=
        ErrCode::Success) {
      return Status;
    }
    /// Write data to Fd.
    unsigned int SizeWrite = write(Fd, &Data[0], (uint32_t)CIOVecBufLen);
    if (SizeWrite != CIOVecBufLen) {
      ErrNo = 1;
    } else {
      NWritten += SizeWrite;
    }
    /// Shift one element.
    /// TODO: sizeof(__wasi_ciovec_t) is 8 in 32-bit wasm.
    IOVSPtr += 8;
  }

  /// Store written bytes length.
  if ((Status = MemInst->storeValue(NWritten, NWrittenPtr, 4)) !=
      ErrCode::Success) {
    return Status;
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
