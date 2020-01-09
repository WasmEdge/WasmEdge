// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/wasi/fd_Write.h"
#include <unistd.h>

namespace SSVM {
namespace Executor {

ErrCode WasiFdWrite::body(VM::EnvironmentManager &EnvMgr,
                          Instance::MemoryInstance &MemInst, uint32_t &ErrNo,
                          int32_t Fd, uint32_t IOVSPtr, uint32_t IOVSCnt,
                          uint32_t NWrittenPtr) {
  /// Sequencially writting.
  uint32_t NWritten = 0;
  for (uint32_t I = 0; I < IOVSCnt; I++) {
    uint32_t CIOVecBufPtr = 0;
    uint32_t CIOVecBufLen = 0;
    /// Get data offset.
    if (ErrCode Status = MemInst.loadValue(CIOVecBufPtr, IOVSPtr, 4);
        Status != ErrCode::Success) {
      return Status;
    }
    /// Get data length.
    if (ErrCode Status = MemInst.loadValue(CIOVecBufLen, IOVSPtr + 4, 4);
        Status != ErrCode::Success) {
      return Status;
    }
    /// Write data to Fd.
    unsigned char *WriteArr = MemInst.getPointer<unsigned char *>(CIOVecBufPtr);
    int32_t SizeWrite = write(Fd, WriteArr, CIOVecBufLen);
    if (SizeWrite == -1) {
      /// Store read bytes length.
      if (ErrCode Status = MemInst.storeValue(NWritten, NWrittenPtr, 4);
          Status != ErrCode::Success) {
        return Status;
      }
      /// TODO: errno
      ErrNo = 1U;
      return ErrCode::Success;
    }

    NWritten += SizeWrite;
    /// Shift one element.
    IOVSPtr += 8;
  }

  /// Store read bytes length.
  if (ErrCode Status = MemInst.storeValue(NWritten, NWrittenPtr, 4);
      Status != ErrCode::Success) {
    return Status;
  }
  ErrNo = 0U;
  return ErrCode::Success;
}

} // namespace Executor
} // namespace SSVM
