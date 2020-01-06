// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/wasi/path_Open.h"
#include <fcntl.h>

namespace SSVM {
namespace Executor {

WasiPathOpen::WasiPathOpen(VM::WasiEnvironment &Env) : Wasi(Env) {
  initializeFuncType<WasiPathOpen>();
}

ErrCode WasiPathOpen::run(VM::EnvironmentManager &EnvMgr,
                          StackManager &StackMgr,
                          Instance::MemoryInstance &MemInst) {
  return invoke<WasiPathOpen>(EnvMgr, StackMgr, MemInst);
}

ErrCode WasiPathOpen::body(VM::EnvironmentManager &EnvMgr,
                           Instance::MemoryInstance &MemInst, uint32_t &ErrNo,
                           int32_t DirFd, uint32_t DirFlags, uint32_t PathPtr,
                           uint32_t PathLen, uint32_t OFlags,
                           uint64_t FsRightsBase, uint64_t FsRightsInheriting,
                           uint32_t FsFlags, uint32_t FdPtr) {
  /// Get file path.
  std::vector<unsigned char> Data;
  if (ErrCode Status = MemInst.getBytes(Data, PathPtr, PathLen);
      Status != ErrCode::Success) {
    return Status;
  }
  std::string Path(Data.begin(), Data.end());

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

  if ((DirFlags & __WASI_LOOKUP_SYMLINK_FOLLOW) == 0) {
    Flags |= O_NOFOLLOW;
  }

  /// Open file and store Fd.
  int32_t Fd = open(Path.c_str(), Flags, 0644);
  if (Fd == -1) {
    /// TODO: errno
    ErrNo = 1U;
    return ErrCode::Success;
  }
  if (ErrCode Status = MemInst.storeValue(uint32_t(Fd), FdPtr, 4);
      Status != ErrCode::Success) {
    return Status;
  }
  ErrNo = 0U;
  return ErrCode::Success;
}

} // namespace Executor
} // namespace SSVM
