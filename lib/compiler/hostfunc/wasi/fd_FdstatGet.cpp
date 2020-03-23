// SPDX-License-Identifier: Apache-2.0
#include "compiler/hostfunc/wasi/fd_FdstatGet.h"
#include "executor/common.h"
#include <algorithm>
#include <fcntl.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace SSVM {
namespace Compiler {

uint32_t WasiFdFdstatGet::run(uint32_t Fd, uint32_t FdStatPtr) {
  __wasi_fdstat_t FdStat;
  /// 1. __wasi_fdstat_t.fs_filetype
  {
    struct stat SysFStat;
    if (fstat(Fd, &SysFStat) != 0) {
      return convertErrno(errno);
    }
    switch (SysFStat.st_mode & S_IFMT) {
    case S_IFSOCK:
      FdStat.fs_filetype = __WASI_FILETYPE_SOCKET_DGRAM;
      break;
    case S_IFLNK:
      FdStat.fs_filetype = __WASI_FILETYPE_SYMBOLIC_LINK;
      break;
    case S_IFREG:
      FdStat.fs_filetype = __WASI_FILETYPE_REGULAR_FILE;
      break;
    case S_IFBLK:
      FdStat.fs_filetype = __WASI_FILETYPE_BLOCK_DEVICE;
      break;
    case S_IFDIR:
      FdStat.fs_filetype = __WASI_FILETYPE_DIRECTORY;
      break;
    case S_IFCHR:
      FdStat.fs_filetype = __WASI_FILETYPE_CHARACTER_DEVICE;
      break;
    default:
      FdStat.fs_filetype = __WASI_FILETYPE_UNKNOWN;
      break;
    }
  }

  /// 2. __wasi_fdstat_t.fs_flags
  {
    const int FdFlags = fcntl(Fd, F_GETFL);
    if (FdFlags < 0) {
      return convertErrno(errno);
    }
    FdStat.fs_flags = FdFlags & O_ACCMODE;
    FdStat.fs_flags |= ((FdFlags & O_APPEND) ? __WASI_FDFLAG_APPEND : 0);
    FdStat.fs_flags |= ((FdFlags & O_DSYNC) ? __WASI_FDFLAG_DSYNC : 0);
    FdStat.fs_flags |= ((FdFlags & O_NONBLOCK) ? __WASI_FDFLAG_NONBLOCK : 0);
    FdStat.fs_flags |=
        ((FdFlags & O_SYNC) ? (__WASI_FDFLAG_RSYNC | __WASI_FDFLAG_SYNC) : 0);
  }

  /// 3. __wasi_fdstat_t.fs_rights_base
  /// TODO: get base rights
  FdStat.fs_rights_base = 0x000000001FFFFFFFULL;

  /// 4. __wasi_fdstat_t.fs_rights_inheriting
  /// TODO: get inheriting rights
  FdStat.fs_rights_inheriting = 0x000000001FFFFFFFULL;

  /// Store to memory instance
  /// byte[0] : fs_filetype(uint8_t)
  Lib.getMemory<uint8_t>(FdStatPtr) = FdStat.fs_filetype;
  /// byte[1] : ZERO
  Lib.getMemory<uint8_t>(FdStatPtr + 1) = 0;
  /// byte[2:3] : fs_flags(uint16_t)
  Lib.getMemory<uint16_t>(FdStatPtr + 2) = FdStat.fs_flags;
  /// byte[4:7] : ZERO
  Lib.getMemory<uint32_t>(FdStatPtr + 4) = 0;
  /// byte[8:15] : fs_rights_base
  Lib.getMemory<uint64_t>(FdStatPtr + 8) = FdStat.fs_rights_base;
  /// byte[16:23] : fs_rights_inheriting
  Lib.getMemory<uint64_t>(FdStatPtr + 16) = FdStat.fs_rights_inheriting;

  return 0;
}

} // namespace Compiler
} // namespace SSVM
