// SPDX-License-Identifier: Apache-2.0
#include "compiler/hostfunc/wasi/fd_Read.h"
#include <algorithm>
#include <unistd.h>

namespace SSVM {
namespace Compiler {

uint32_t WasiFdRead::run(uint32_t Fd, uint32_t IOVecPtr, uint32_t IOVecCnt,
                         uint32_t NReadPtr) {
  /// Sequencially reading.
  auto &NRead = Lib.getMemory<uint32_t>(NReadPtr);
  NRead = 0;
  for (uint32_t I = 0; I < IOVecCnt; ++I) {
    uint32_t CIOVecBufPtr = Lib.getMemory<uint32_t>(IOVecPtr + 8 * I);
    uint32_t CIOVecBufLen = Lib.getMemory<uint32_t>(IOVecPtr + 8 * I + 4);

    /// Read data from Fd.
    unsigned int SizeRead =
        read(Fd, Lib.getMemory<char>(CIOVecBufPtr, CIOVecBufLen).begin(),
             CIOVecBufLen);
    /// Store data.
    if (SizeRead == -1) {
      return convertErrno(errno);
    }
    NRead += SizeRead;
  }

  return 0;
}

} // namespace Compiler
} // namespace SSVM
