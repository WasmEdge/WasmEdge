// SPDX-License-Identifier: Apache-2.0
#include "compiler/hostfunc/wasi/fd_Seek.h"
#include <algorithm>
#include <unistd.h>

namespace SSVM {
namespace Compiler {

uint32_t WasiFdSeek::run(uint32_t Fd, uint32_t Offset, uint32_t Whence,
                         uint32_t NewOffsetPtr) {
  /// Check directive whence.
  int NWhence;
  switch (Whence) {
  case __WASI_WHENCE_CUR:
    NWhence = SEEK_CUR;
    break;
  case __WASI_WHENCE_END:
    NWhence = SEEK_END;
    break;
  case __WASI_WHENCE_SET:
    NWhence = SEEK_SET;
    break;
  default:
    return __WASI_EINVAL;
  }

  /// Do lseek.
  int64_t NewOffset = lseek(Fd, Offset, NWhence);
  if (NewOffset == -1) {
    return convertErrno(errno);
  }

  Lib.getMemory<int64_t>(NewOffsetPtr) = NewOffset;

  /// Return: errno(u32)
  return 0;
}

} // namespace Compiler
} // namespace SSVM
