// SPDX-License-Identifier: Apache-2.0
#include "runtime/instance/memory.h"
#include "host/wasi/wasifunc.h"

#include <string_view>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

extern char **environ;

namespace SSVM {
namespace Host {

ErrCode WasiArgsGet::body(Runtime::Instance::MemoryInstance &MemInst,
                          uint32_t &ErrNo, uint32_t ArgvPtr,
                          uint32_t ArgvBufPtr) {
  /// Store **Argv.
  std::vector<unsigned char> ArgvBuf;
  uint32_t ArgvBufOffset = ArgvBufPtr;
  for (const auto &Arg : Env.getCmdArgs()) {
    /// Concate Argv.
    std::copy(Arg.cbegin(), Arg.cend(), std::back_inserter(ArgvBuf));
    ArgvBuf.push_back('\0');

    /// Calcuate Argv[i] offset and store.
    if (auto Res = MemInst.storeValue(ArgvBufOffset, ArgvPtr, 4); !Res) {
      return Res.error();
    }

    /// Shift one element.
    ArgvBufOffset += Arg.size() + 1;
    ArgvPtr += 4;
  }

  /// Store nullptr
  if (auto Res = MemInst.storeValue(uint32_t(0), ArgvPtr, 4); !Res) {
    return Res.error();
  }

  /// Store ArgvBuf.
  if (auto Res = MemInst.setBytes(ArgvBuf, ArgvBufPtr, 0, ArgvBuf.size());
      !Res) {
    return Res.error();
  }

  ErrNo = 0U;
  return ErrCode::Success;
}

ErrCode WasiArgsSizesGet::body(Runtime::Instance::MemoryInstance &MemInst,
                               uint32_t &ErrNo, uint32_t ArgcPtr,
                               uint32_t ArgvBufSizePtr) {
  /// Store Argc.
  std::vector<std::string> &CmdArgs = Env.getCmdArgs();
  if (auto Res = MemInst.storeValue(uint32_t(CmdArgs.size()), ArgcPtr, 4);
      !Res) {
    return Res.error();
  }

  /// Store ArgvBufSize.
  uint32_t CmdArgsSize = 0;
  for (const auto &Arg : CmdArgs) {
    CmdArgsSize += Arg.size() + 1;
  }
  if (auto Res = MemInst.storeValue(CmdArgsSize, ArgvBufSizePtr, 4); !Res) {
    return Res.error();
  }

  ErrNo = 0U;
  return ErrCode::Success;
}

ErrCode WasiEnvironGet::body(Runtime::Instance::MemoryInstance &MemInst,
                             uint32_t &ErrNo, uint32_t EnvPtr,
                             uint32_t EnvBufPtr) {
  /// Store **Env.
  uint32_t EnvBufOffset = EnvBufPtr;
  std::vector<unsigned char> EnvBuf;
  for (uint32_t EnvCnt = 0; environ[EnvCnt] != nullptr; ++EnvCnt) {
    std::string_view EnvString(environ[EnvCnt]);

    /// Concate EnvString.
    std::copy(EnvString.cbegin(), EnvString.cend(), std::back_inserter(EnvBuf));
    EnvBuf.push_back('\0');

    /// Calculate Env[i] offset and store.
    if (auto Res = MemInst.storeValue(EnvBufOffset, EnvPtr, 4); !Res) {
      return Res.error();
    }

    /// Shift one element.
    EnvBufOffset += EnvString.size() + 1;
    EnvPtr += 4;
  }

  /// Store nullptr
  if (auto Res = MemInst.storeValue(uint32_t(0), EnvPtr, 4); !Res) {
    return Res.error();
  }

  /// Store EnvBuf.
  if (auto Res = MemInst.setBytes(EnvBuf, EnvBufPtr, 0, EnvBuf.size()); !Res) {
    return Res.error();
  }

  ErrNo = 0U;
  return ErrCode::Success;
}

ErrCode WasiEnvironSizesGet::body(Runtime::Instance::MemoryInstance &MemInst,
                                  uint32_t &ErrNo, uint32_t EnvCntPtr,
                                  uint32_t EnvBufSizePtr) {
  /// Calculate EnvCnt and EnvBufSize.
  uint32_t EnvCnt;
  uint32_t EnvBufSize = 0;

  for (EnvCnt = 0; environ[EnvCnt] != nullptr; ++EnvCnt) {
    std::string_view EnvString(environ[EnvCnt]);
    EnvBufSize += EnvString.size() + 1;
  }

  /// Store EnvCnt and EnvBufSize.
  if (auto Res = MemInst.storeValue(EnvCnt, EnvCntPtr, 4); !Res) {
  }
  if (auto Res = MemInst.storeValue(EnvBufSize, EnvBufSizePtr, 4); !Res) {
    return Res.error();
  }

  ErrNo = 0U;
  return ErrCode::Success;
}

ErrCode WasiFdClose::body(Runtime::Instance::MemoryInstance &MemInst,
                          uint32_t &ErrNo, int32_t Fd) {
  if (close(Fd) != 0) {
    /// TODO: errno
    ErrNo = 1U;
  } else {
    ErrNo = 0U;
  }
  return ErrCode::Success;
}

ErrCode WasiFdFdstatGet::body(Runtime::Instance::MemoryInstance &MemInst,
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
    if (auto Res =
            MemInst.storeValue(uint32_t(FdStat.fs_filetype), FdStatPtr, 1);
        !Res) {
      return Res.error();
    }
    /// byte[1] : ZERO
    if (auto Res = MemInst.storeValue(uint32_t(0), FdStatPtr + 1, 1); !Res) {
      return Res.error();
    }
    /// byte[2:3] : fs_flags(uint16_t)
    if (auto Res =
            MemInst.storeValue(uint32_t(FdStat.fs_flags), FdStatPtr + 2, 2);
        !Res) {
      return Res.error();
    }
    /// byte[4:7] : ZERO
    if (auto Res = MemInst.storeValue(uint32_t(0), FdStatPtr + 4, 4); !Res) {
      return Res.error();
    }
    /// byte[8:15] : fs_rights_base
    if (auto Res = MemInst.storeValue(uint64_t(FdStat.fs_rights_base),
                                      FdStatPtr + 8, 8);
        !Res) {
      return Res.error();
    }
    /// byte[16:23] : fs_rights_inheriting
    if (auto Res = MemInst.storeValue(uint64_t(FdStat.fs_rights_inheriting),
                                      FdStatPtr + 16, 8);
        !Res) {
      return Res.error();
    }
  }

  ErrNo = 0U;
  return ErrCode::Success;
}

ErrCode WasiFdFdstatSetFlags::body(Runtime::Instance::MemoryInstance &MemInst,
                                   uint32_t &ErrNo, int32_t Fd,
                                   uint32_t FsFlags) {
  /// TODO: implement
  ErrNo = 0U;
  return ErrCode::Success;
}

ErrCode WasiFdPrestatDirName::body(Runtime::Instance::MemoryInstance &MemInst,
                                   uint32_t &ErrNo, int32_t Fd,
                                   uint32_t PathBufPtr, uint32_t PathLen) {
  for (auto &Entry : Env.getPreStats()) {
    if (Entry.Fd != Fd) {
      continue;
    }
    if (Entry.Path.size() > PathLen) {
      ErrNo = __WASI_EINVAL;
      return ErrCode::Success;
    }

    /// Store Path and PathLen.
    if (auto Res =
            MemInst.setBytes(Entry.Path, PathBufPtr, 0, Entry.Path.size());
        !Res) {
      return Res.error();
    }

    ErrNo = 0U;
    return ErrCode::Success;
  }
  ErrNo = __WASI_EBADF;
  return ErrCode::Success;
}

ErrCode WasiFdPrestatGet::body(Runtime::Instance::MemoryInstance &MemInst,
                               uint32_t &ErrNo, int32_t Fd,
                               uint32_t PreStatPtr) {
  for (const auto &Entry : Env.getPreStats()) {
    if (Entry.Fd != Fd) {
      continue;
    }

    /// Store to memory instance
    /// byte[0] : pr_type
    if (auto Res = MemInst.storeValue(uint32_t(Entry.Type), PreStatPtr, 1);
        !Res) {
      return Res.error();
    }
    /// byte[5:8] : u.dir.pr_name_len
    if (auto Res =
            MemInst.storeValue(uint32_t(Entry.Path.size()), PreStatPtr + 4, 4);
        !Res) {
      return Res.error();
    }

    ErrNo = 0U;
    return ErrCode::Success;
  }
  ErrNo = __WASI_EBADF;
  return ErrCode::Success;
}

ErrCode WasiFdRead::body(Runtime::Instance::MemoryInstance &MemInst,
                         uint32_t &ErrNo, int32_t Fd, uint32_t IOVSPtr,
                         uint32_t IOVSCnt, uint32_t NReadPtr) {
  /// Sequencially reading.
  uint32_t NRead = 0;
  for (uint32_t I = 0; I < IOVSCnt; I++) {
    uint32_t CIOVecBufPtr = 0;
    uint32_t CIOVecBufLen = 0;
    /// Get data offset.
    if (auto Res = MemInst.loadValue(CIOVecBufPtr, IOVSPtr, 4); !Res) {
      return Res.error();
    }
    /// Get data length.
    if (auto Res = MemInst.loadValue(CIOVecBufLen, IOVSPtr + 4, 4); !Res) {
      return Res.error();
    }
    /// Read data from Fd.
    unsigned char *ReadArr = MemInst.getPointer<unsigned char *>(CIOVecBufPtr);
    int32_t SizeRead = read(Fd, ReadArr, CIOVecBufLen);
    /// Store data.
    if (SizeRead == -1) {
      /// Store read bytes length.
      if (auto Res = MemInst.storeValue(NRead, NReadPtr, 4); !Res) {
        return Res.error();
      }
      /// TODO: errno
      ErrNo = 1U;
      return ErrCode::Success;
    }

    NRead += SizeRead;
    /// Shift one element.
    IOVSPtr += 8;
  }

  /// Store read bytes length.
  if (auto Res = MemInst.storeValue(NRead, NReadPtr, 4); !Res) {
    return Res.error();
  }
  ErrNo = 0U;
  return ErrCode::Success;
}

ErrCode WasiFdSeek::body(Runtime::Instance::MemoryInstance &MemInst,
                         uint32_t &ErrNo, int32_t Fd, int32_t Offset,
                         uint32_t Whence, uint32_t NewOffsetPtr) {
  /// Check directive whence.
  switch (Whence) {
  case __WASI_WHENCE_CUR:
    Whence = SEEK_CUR;
    break;
  case __WASI_WHENCE_END:
    Whence = SEEK_END;
    break;
  case __WASI_WHENCE_SET:
    Whence = SEEK_SET;
    break;
  default:
    ErrNo = __WASI_EINVAL;
    return ErrCode::Success;
    break;
  }

  /// Do lseek.
  int64_t NewOffset = lseek(Fd, Offset, Whence);
  if (NewOffset == -1) {
    /// TODO: errno
    ErrNo = 1U;
    return ErrCode::Success;
  }

  /// Store NewOffset.
  if (auto Res = MemInst.storeValue(uint64_t(NewOffset), NewOffsetPtr, 8);
      !Res) {
    return Res.error();
  }

  ErrNo = 0U;
  return ErrCode::Success;
}

ErrCode WasiFdWrite::body(Runtime::Instance::MemoryInstance &MemInst,
                          uint32_t &ErrNo, int32_t Fd, uint32_t IOVSPtr,
                          uint32_t IOVSCnt, uint32_t NWrittenPtr) {
  /// Sequencially writting.
  uint32_t NWritten = 0;
  for (uint32_t I = 0; I < IOVSCnt; I++) {
    uint32_t CIOVecBufPtr = 0;
    uint32_t CIOVecBufLen = 0;
    /// Get data offset.
    if (auto Res = MemInst.loadValue(CIOVecBufPtr, IOVSPtr, 4); !Res) {
      return Res.error();
    }
    /// Get data length.
    if (auto Res = MemInst.loadValue(CIOVecBufLen, IOVSPtr + 4, 4); !Res) {
      return Res.error();
    }
    /// Write data to Fd.
    unsigned char *WriteArr = MemInst.getPointer<unsigned char *>(CIOVecBufPtr);
    int32_t SizeWrite = write(Fd, WriteArr, CIOVecBufLen);
    if (SizeWrite == -1) {
      /// Store read bytes length.
      if (auto Res = MemInst.storeValue(NWritten, NWrittenPtr, 4); !Res) {
        return Res.error();
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
  if (auto Res = MemInst.storeValue(NWritten, NWrittenPtr, 4); !Res) {
    return Res.error();
  }
  ErrNo = 0U;
  return ErrCode::Success;
}

ErrCode WasiPathOpen::body(Runtime::Instance::MemoryInstance &MemInst,
                           uint32_t &ErrNo, int32_t DirFd, uint32_t DirFlags,
                           uint32_t PathPtr, uint32_t PathLen, uint32_t OFlags,
                           uint64_t FsRightsBase, uint64_t FsRightsInheriting,
                           uint32_t FsFlags, uint32_t FdPtr) {
  /// Get file path.
  std::vector<unsigned char> Data;
  if (auto Res = MemInst.getBytes(PathPtr, PathLen)) {
    Data = *Res;
  } else {
    return Res.error();
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
  if (auto Res = MemInst.storeValue(uint32_t(Fd), FdPtr, 4); !Res) {
    return Res.error();
  }
  ErrNo = 0U;
  return ErrCode::Success;
}

ErrCode WasiProcExit::body(Runtime::Instance::MemoryInstance &MemInst,
                           int32_t Status) {
  Env.setStatus(Status);
  return ErrCode::Terminated;
}

} // namespace Host
} // namespace SSVM