// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/wasi/fd_FdstatGet.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace SSVM {
namespace Executor {

ErrCode WasiFdFdstatGet::body(VM::EnvironmentManager &EnvMgr,
                              Instance::MemoryInstance &MemInst,
                              uint32_t &ErrNo, int32_t Fd, uint32_t FdStatPtr) {
  __wasi_fdstat_t FdStat;
  /// 1. __wasi_fdstat_t.fs_filetype
  {
    struct stat SysFStat;
    if (fstat(Fd, &SysFStat) != 0) {
      /// TODO: errno
      ErrNo = 1U;
      return ErrCode::Success;
    }
    FdStat.fs_filetype = __WASI_FILETYPE_UNKNOWN;
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
      break;
    }
  }

  /// 2. __wasi_fdstat_t.fs_flags
  {
    int FdFlags = fcntl(Fd, F_GETFL);
    if (FdFlags < 0) {
      /// TODO: errno
      ErrNo = 1U;
      return ErrCode::Success;
    }
    FdStat.fs_flags = FdFlags & O_ACCMODE;
    FdStat.fs_flags |= ((FdFlags & O_APPEND) ? __WASI_FDFLAG_APPEND : 0);
    FdStat.fs_flags |= ((FdFlags & O_DSYNC) ? __WASI_FDFLAG_DSYNC : 0);
    FdStat.fs_flags |= ((FdFlags & O_NONBLOCK) ? __WASI_FDFLAG_NONBLOCK : 0);
    FdStat.fs_flags |=
        ((FdFlags & O_SYNC) ? (__WASI_FDFLAG_RSYNC | __WASI_FDFLAG_SYNC) : 0);
  }

  /// 3. __wasi_fdstat_t.fs_rights_base
  {
    /// TODO: get base rights
    FdStat.fs_rights_base = 0x000000001FFFFFFFULL;
  }

  /// 4. __wasi_fdstat_t.fs_rights_inheriting
  {
    /// TODO: get inheriting rights
    FdStat.fs_rights_inheriting = 0x000000001FFFFFFFULL;
  }

  /// Store to memory instance
  {
    /// byte[0] : fs_filetype(uint8_t)
    if (ErrCode Status =
            MemInst.storeValue(uint32_t(FdStat.fs_filetype), FdStatPtr, 1);
        Status != ErrCode::Success) {
      return Status;
    }
    /// byte[1] : ZERO
    if (ErrCode Status = MemInst.storeValue(uint32_t(0), FdStatPtr + 1, 1);
        Status != ErrCode::Success) {
      return Status;
    }
    /// byte[2:3] : fs_flags(uint16_t)
    if (ErrCode Status =
            MemInst.storeValue(uint32_t(FdStat.fs_flags), FdStatPtr + 2, 2);
        Status != ErrCode::Success) {
      return Status;
    }
    /// byte[4:7] : ZERO
    if (ErrCode Status = MemInst.storeValue(uint32_t(0), FdStatPtr + 4, 4);
        Status != ErrCode::Success) {
      return Status;
    }
    /// byte[8:15] : fs_rights_base
    if (ErrCode Status = MemInst.storeValue(uint64_t(FdStat.fs_rights_base),
                                            FdStatPtr + 8, 8);
        Status != ErrCode::Success) {
      return Status;
    }
    /// byte[16:23] : fs_rights_inheriting
    if (ErrCode Status = MemInst.storeValue(
            uint64_t(FdStat.fs_rights_inheriting), FdStatPtr + 16, 8);
        Status != ErrCode::Success) {
      return Status;
    }
  }

  ErrNo = 0U;
  return ErrCode::Success;
}

} // namespace Executor
} // namespace SSVM
