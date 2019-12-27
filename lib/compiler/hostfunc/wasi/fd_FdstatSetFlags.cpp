// SPDX-License-Identifier: Apache-2.0
#include "compiler/hostfunc/wasi/fd_FdstatSetFlags.h"
#include <algorithm>
#include <fcntl.h>
#include <unistd.h>

namespace SSVM {
namespace Compiler {

uint32_t WasiFdFdstatSetFlags::run(uint32_t Fd, uint32_t FdFlags) {
  int Flags = 0;
  if ((FdFlags & __WASI_FDFLAG_APPEND) != 0) {
    Flags |= O_APPEND;
  }
  if ((FdFlags & __WASI_FDFLAG_DSYNC) != 0) {
#ifdef O_DSYNC
    Flags |= O_DSYNC;
#else
    Flags |= O_SYNC;
#endif
  }
  if ((FdFlags & __WASI_FDFLAG_NONBLOCK) != 0) {
    Flags |= O_NONBLOCK;
  }
  if ((FdFlags & __WASI_FDFLAG_RSYNC) != 0) {
#ifdef O_RSYNC
    Flags |= O_RSYNC;
#else
    Flags |= O_SYNC;
#endif
  }
  if ((FdFlags & __WASI_FDFLAG_SYNC) != 0) {
    Flags |= O_SYNC;
  }
  if (fcntl(Fd, F_SETFL, Flags) < 0) {
    return convertErrno(errno);
  }
  return 0;
}

} // namespace Compiler
} // namespace SSVM
