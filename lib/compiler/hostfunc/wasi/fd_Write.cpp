// SPDX-License-Identifier: Apache-2.0
#include "compiler/hostfunc/wasi/fd_Write.h"
#include <algorithm>
#include <unistd.h>

namespace SSVM {
namespace Compiler {

uint32_t WasiFdWrite::run(uint32_t Fd, uint32_t IOVecPtr, uint32_t IOVecCnt,
                          uint32_t NWrittenPtr) {
  /// Sequencially writting.
  auto &NWritten = Lib.getMemory<uint32_t>(NWrittenPtr);
  NWritten = 0;
  for (uint32_t I = 0; I < IOVecCnt; ++I) {
    uint32_t CIOVecBufPtr = Lib.getMemory<uint32_t>(IOVecPtr + 8 * I);
    uint32_t CIOVecBufLen = Lib.getMemory<uint32_t>(IOVecPtr + 8 * I + 4);

    unsigned int SizeWrite =
        write(Fd, Lib.getMemory<char>(CIOVecBufPtr, CIOVecBufLen).begin(),
              CIOVecBufLen);
    if (SizeWrite != CIOVecBufLen) {
      return convertErrno(errno);
    }
    NWritten += SizeWrite;
  }

  return 0;
}

} // namespace Compiler
} // namespace SSVM
