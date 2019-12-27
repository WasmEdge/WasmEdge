// SPDX-License-Identifier: Apache-2.0
#include "compiler/hostfunc/wasi/path_Open.h"
#include <fcntl.h>

namespace SSVM {
namespace Compiler {

uint32_t WasiPathOpen::run(uint32_t DirFd, uint32_t DirFlags, uint32_t PathPtr,
                           uint32_t PathLen, uint32_t OFlags,
                           uint32_t FsRightsBase, uint32_t FsRightsInheriting,
                           uint32_t FsFlags, uint32_t FdPtr) {
  /// Get file path.
  auto Path = Lib.getMemory<char>(PathPtr, PathLen);
  std::string SPath(Path.begin(), Path.end());

  const bool Read =
      (FsRightsBase & (__WASI_RIGHT_FD_READ | __WASI_RIGHT_FD_READDIR)) != 0;
  const bool Write =
      (FsRightsBase &
       (__WASI_RIGHT_FD_DATASYNC | __WASI_RIGHT_FD_WRITE |
        __WASI_RIGHT_FD_ALLOCATE | __WASI_RIGHT_FD_FILESTAT_SET_SIZE)) != 0;

  int Flags = Write ? (Read ? O_RDWR : O_WRONLY) : O_RDONLY;
  if ((OFlags & __WASI_O_CREAT) != 0) {
    Flags |= O_CREAT;
  }
  if ((OFlags & __WASI_O_DIRECTORY) != 0)
    Flags |= O_DIRECTORY;
  if ((OFlags & __WASI_O_EXCL) != 0)
    Flags |= O_EXCL;
  if ((OFlags & __WASI_O_TRUNC) != 0) {
    Flags |= O_TRUNC;
  }

  // Convert file descriptor flags.
  if ((FsFlags & __WASI_FDFLAG_APPEND) != 0)
    Flags |= O_APPEND;
  if ((FsFlags & __WASI_FDFLAG_DSYNC) != 0) {
#ifdef O_DSYNC
    Flags |= O_DSYNC;
#else
    Flags |= O_SYNC;
#endif
  }
  if ((FsFlags & __WASI_FDFLAG_NONBLOCK) != 0)
    Flags |= O_NONBLOCK;
  if ((FsFlags & __WASI_FDFLAG_RSYNC) != 0) {
#ifdef O_RSYNC
    Flags |= O_RSYNC;
#else
    Flags |= O_SYNC;
#endif
  }
  if ((FsFlags & __WASI_FDFLAG_SYNC) != 0) {
    Flags |= O_SYNC;
  }

  /// Open file and store Fd.
  int Fd = open(SPath.c_str(), Flags, 0644);
  if (Fd == -1) {
    return convertErrno(errno);
  }

  Lib.getMemory<uint32_t>(FdPtr) = Fd;

  return 0;
}

} // namespace Compiler
} // namespace SSVM
