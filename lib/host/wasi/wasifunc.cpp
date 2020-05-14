// SPDX-License-Identifier: Apache-2.0
#include "host/wasi/wasifunc.h"
#include "runtime/instance/memory.h"

#include <fcntl.h>
#include <limits>
#include <random>
#include <string_view>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

namespace {
static constexpr uint32_t convertErrNo(int ErrNo) {
  switch (ErrNo) {
  case 0:
    return __WASI_ESUCCESS;
  case E2BIG:
    return __WASI_E2BIG;
  case EACCES:
    return __WASI_EACCES;
  case EADDRINUSE:
    return __WASI_EADDRINUSE;
  case EADDRNOTAVAIL:
    return __WASI_EADDRNOTAVAIL;
  case EAFNOSUPPORT:
    return __WASI_EAFNOSUPPORT;
  case EAGAIN:
    return __WASI_EAGAIN;
  case EALREADY:
    return __WASI_EALREADY;
  case EBADF:
    return __WASI_EBADF;
  case EBADMSG:
    return __WASI_EBADMSG;
  case EBUSY:
    return __WASI_EBUSY;
  case ECANCELED:
    return __WASI_ECANCELED;
  case ECHILD:
    return __WASI_ECHILD;
  case ECONNABORTED:
    return __WASI_ECONNABORTED;
  case ECONNREFUSED:
    return __WASI_ECONNREFUSED;
  case ECONNRESET:
    return __WASI_ECONNRESET;
  case EDEADLK:
    return __WASI_EDEADLK;
  case EDESTADDRREQ:
    return __WASI_EDESTADDRREQ;
  case EDOM:
    return __WASI_EDOM;
  case EDQUOT:
    return __WASI_EDQUOT;
  case EEXIST:
    return __WASI_EEXIST;
  case EFAULT:
    return __WASI_EFAULT;
  case EFBIG:
    return __WASI_EFBIG;
  case EHOSTUNREACH:
    return __WASI_EHOSTUNREACH;
  case EIDRM:
    return __WASI_EIDRM;
  case EILSEQ:
    return __WASI_EILSEQ;
  case EINPROGRESS:
    return __WASI_EINPROGRESS;
  case EINTR:
    return __WASI_EINTR;
  case EINVAL:
    return __WASI_EINVAL;
  case EIO:
    return __WASI_EIO;
  case EISCONN:
    return __WASI_EISCONN;
  case EISDIR:
    return __WASI_EISDIR;
  case ELOOP:
    return __WASI_ELOOP;
  case EMFILE:
    return __WASI_EMFILE;
  case EMLINK:
    return __WASI_EMLINK;
  case EMSGSIZE:
    return __WASI_EMSGSIZE;
  case EMULTIHOP:
    return __WASI_EMULTIHOP;
  case ENAMETOOLONG:
    return __WASI_ENAMETOOLONG;
  case ENETDOWN:
    return __WASI_ENETDOWN;
  case ENETRESET:
    return __WASI_ENETRESET;
  case ENETUNREACH:
    return __WASI_ENETUNREACH;
  case ENFILE:
    return __WASI_ENFILE;
  case ENOBUFS:
    return __WASI_ENOBUFS;
  case ENODEV:
    return __WASI_ENODEV;
  case ENOENT:
    return __WASI_ENOENT;
  case ENOEXEC:
    return __WASI_ENOEXEC;
  case ENOLCK:
    return __WASI_ENOLCK;
  case ENOLINK:
    return __WASI_ENOLINK;
  case ENOMEM:
    return __WASI_ENOMEM;
  case ENOMSG:
    return __WASI_ENOMSG;
  case ENOPROTOOPT:
    return __WASI_ENOPROTOOPT;
  case ENOSPC:
    return __WASI_ENOSPC;
  case ENOSYS:
    return __WASI_ENOSYS;
  case ENOTCONN:
    return __WASI_ENOTCONN;
  case ENOTDIR:
    return __WASI_ENOTDIR;
  case ENOTEMPTY:
    return __WASI_ENOTEMPTY;
  case ENOTRECOVERABLE:
    return __WASI_ENOTRECOVERABLE;
  case ENOTSOCK:
    return __WASI_ENOTSOCK;
  case ENOTSUP:
    return __WASI_ENOTSUP;
  case ENOTTY:
    return __WASI_ENOTTY;
  case ENXIO:
    return __WASI_ENXIO;
  case EOVERFLOW:
    return __WASI_EOVERFLOW;
  case EOWNERDEAD:
    return __WASI_EOWNERDEAD;
  case EPERM:
    return __WASI_EPERM;
  case EPIPE:
    return __WASI_EPIPE;
  case EPROTO:
    return __WASI_EPROTO;
  case EPROTONOSUPPORT:
    return __WASI_EPROTONOSUPPORT;
  case EPROTOTYPE:
    return __WASI_EPROTOTYPE;
  case ERANGE:
    return __WASI_ERANGE;
  case EROFS:
    return __WASI_EROFS;
  case ESPIPE:
    return __WASI_ESPIPE;
  case ESRCH:
    return __WASI_ESRCH;
  case ESTALE:
    return __WASI_ESTALE;
  case ETIMEDOUT:
    return __WASI_ETIMEDOUT;
  case ETXTBSY:
    return __WASI_ETXTBSY;
  case EXDEV:
    return __WASI_EXDEV;
  default:
    assert(false);
    __builtin_unreachable();
  }
}
} // namespace

extern char **environ;

namespace SSVM {
namespace Host {

Expect<uint32_t> WasiArgsGet::body(Runtime::Instance::MemoryInstance &MemInst,
                                   uint32_t ArgvPtr, uint32_t ArgvBufPtr) {
  /// Store **Argv.
  std::vector<unsigned char> ArgvBuf;
  uint32_t ArgvBufOffset = ArgvBufPtr;
  for (const auto &Arg : Env.getCmdArgs()) {
    /// Concate Argv.
    std::copy(Arg.cbegin(), Arg.cend(), std::back_inserter(ArgvBuf));
    ArgvBuf.push_back('\0');

    /// Calcuate Argv[i] offset and store.
    if (auto Res = MemInst.storeValue(ArgvBufOffset, ArgvPtr, 4); !Res) {
      return Unexpect(Res);
    }

    /// Shift one element.
    ArgvBufOffset += Arg.size() + 1;
    ArgvPtr += 4;
  }

  /// Store nullptr
  if (auto Res = MemInst.storeValue(UINT32_C(0), ArgvPtr, 4); !Res) {
    return Unexpect(Res);
  }

  /// Store ArgvBuf.
  if (auto Res = MemInst.setBytes(ArgvBuf, ArgvBufPtr, 0, ArgvBuf.size());
      !Res) {
    return Unexpect(Res);
  }

  return UINT32_C(0);
}

Expect<uint32_t>
WasiArgsSizesGet::body(Runtime::Instance::MemoryInstance &MemInst,
                       uint32_t ArgcPtr, uint32_t ArgvBufSizePtr) {
  /// Store Argc.
  std::vector<std::string> &CmdArgs = Env.getCmdArgs();
  if (auto Res = MemInst.storeValue(uint32_t(CmdArgs.size()), ArgcPtr, 4);
      !Res) {
    return Unexpect(Res);
  }

  /// Store ArgvBufSize.
  uint32_t CmdArgsSize = 0;
  for (const auto &Arg : CmdArgs) {
    CmdArgsSize += Arg.size() + 1;
  }
  if (auto Res = MemInst.storeValue(CmdArgsSize, ArgvBufSizePtr, 4); !Res) {
    return Unexpect(Res);
  }

  return UINT32_C(0);
}

Expect<uint32_t>
WasiEnvironGet::body(Runtime::Instance::MemoryInstance &MemInst,
                     uint32_t EnvPtr, uint32_t EnvBufPtr) {
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
      return Unexpect(Res);
    }

    /// Shift one element.
    EnvBufOffset += EnvString.size() + 1;
    EnvPtr += 4;
  }

  /// Store nullptr
  if (auto Res = MemInst.storeValue(uint32_t(0), EnvPtr, 4); !Res) {
    return Unexpect(Res);
  }

  /// Store EnvBuf.
  if (auto Res = MemInst.setBytes(EnvBuf, EnvBufPtr, 0, EnvBuf.size()); !Res) {
    return Unexpect(Res);
  }

  return UINT32_C(0);
}

Expect<uint32_t>
WasiEnvironSizesGet::body(Runtime::Instance::MemoryInstance &MemInst,
                          uint32_t EnvCntPtr, uint32_t EnvBufSizePtr) {
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
    return Unexpect(Res);
  }

  return UINT32_C(0);
}

Expect<uint32_t>
WasiClockResGet::body(Runtime::Instance::MemoryInstance &MemInst,
                      uint32_t ClockId, uint32_t ResolutionPtr) {
  clockid_t SysClockId;
  switch (ClockId) {
  case __WASI_CLOCK_REALTIME:
    SysClockId = CLOCK_REALTIME;
    break;
  case __WASI_CLOCK_MONOTONIC:
    SysClockId = CLOCK_MONOTONIC;
    break;
  case __WASI_CLOCK_PROCESS_CPUTIME_ID:
    SysClockId = CLOCK_PROCESS_CPUTIME_ID;
    break;
  case __WASI_CLOCK_THREAD_CPUTIME_ID:
    SysClockId = CLOCK_THREAD_CPUTIME_ID;
    break;
  default:
    return __WASI_EINVAL;
  }

  timespec SysTimespec;
  if (clock_getres(SysClockId, &SysTimespec) != 0) {
    return convertErrNo(errno);
  }
  const uint64_t Resolution =
      SysTimespec.tv_sec * UINT64_C(1000000000) + SysTimespec.tv_nsec;
  if (auto Res = MemInst.storeValue(Resolution, ResolutionPtr, 8); !Res) {
    return Unexpect(Res);
  }
  return UINT32_C(0);
}

Expect<uint32_t>
WasiClockTimeGet::body(Runtime::Instance::MemoryInstance &MemInst,
                       uint32_t ClockId, uint64_t Precision, uint32_t TimePtr) {
  clockid_t SysClockId;
  switch (ClockId) {
  case __WASI_CLOCK_REALTIME:
    SysClockId = CLOCK_REALTIME;
    break;
  case __WASI_CLOCK_MONOTONIC:
    SysClockId = CLOCK_MONOTONIC;
    break;
  case __WASI_CLOCK_PROCESS_CPUTIME_ID:
    SysClockId = CLOCK_PROCESS_CPUTIME_ID;
    break;
  case __WASI_CLOCK_THREAD_CPUTIME_ID:
    SysClockId = CLOCK_THREAD_CPUTIME_ID;
    break;
  default:
    return __WASI_EINVAL;
  }

  timespec SysTimespec;
  SysTimespec.tv_sec = Precision / UINT64_C(1000000000);
  SysTimespec.tv_nsec = Precision % UINT64_C(1000000000);
  if (clock_gettime(SysClockId, &SysTimespec) != 0) {
    return convertErrNo(errno);
  }
  const uint64_t Time =
      SysTimespec.tv_sec * UINT64_C(1000000000) + SysTimespec.tv_nsec;
  if (auto Res = MemInst.storeValue(Time, TimePtr, 8); !Res) {
    return Unexpect(Res);
  }
  return UINT32_C(0);
}

Expect<uint32_t> WasiFdAdvise::body(Runtime::Instance::MemoryInstance &MemInst,
                                    int32_t Fd, uint64_t Offset, uint64_t Len,
                                    uint32_t Advice) {
  int SysAdvise;
  switch (Advice) {
  case __WASI_ADVICE_NORMAL:
    SysAdvise = POSIX_FADV_NORMAL;
    break;
  case __WASI_ADVICE_SEQUENTIAL:
    SysAdvise = POSIX_FADV_SEQUENTIAL;
    break;
  case __WASI_ADVICE_RANDOM:
    SysAdvise = POSIX_FADV_RANDOM;
    break;
  case __WASI_ADVICE_WILLNEED:
    SysAdvise = POSIX_FADV_WILLNEED;
    break;
  case __WASI_ADVICE_DONTNEED:
    SysAdvise = POSIX_FADV_DONTNEED;
    break;
  case __WASI_ADVICE_NOREUSE:
    SysAdvise = POSIX_FADV_NOREUSE;
    break;
  default:
    return __WASI_EINVAL;
  }

  for (auto &Entry : Env.getPreStats()) {
    if (Entry.Fd != Fd) {
      continue;
    }

    // TODO: check rights __WASI_RIGHT_FD_ADVISE
    if (posix_fadvise(Fd, Offset, Len, SysAdvise) != 0) {
      return convertErrNo(errno);
    }
    return UINT32_C(0);
  }
  return __WASI_EBADF;
}

Expect<uint32_t>
WasiFdAllocate::body(Runtime::Instance::MemoryInstance &MemInst, int32_t Fd,
                     uint64_t Offset, uint64_t Len) {
  for (auto &Entry : Env.getPreStats()) {
    if (Entry.Fd != Fd) {
      continue;
    }

    // TODO: check rights __WASI_RIGHT_FD_ALLOCATE
    if (posix_fallocate(Fd, Offset, Len) != 0) {
      return convertErrNo(errno);
    }
    return UINT32_C(0);
  }
  return __WASI_EBADF;
}

Expect<uint32_t> WasiFdClose::body(Runtime::Instance::MemoryInstance &MemInst,
                                   int32_t Fd) {
  if (close(Fd) != 0) {
    return convertErrNo(errno);
  } else {
    return UINT32_C(0);
  }
}

Expect<uint32_t>
WasiFdDatasync::body(Runtime::Instance::MemoryInstance &MemInst, int32_t Fd) {
  // TODO: check rights __WASI_RIGHT_FD_DATASYNC
  if (fdatasync(Fd) != 0) {
    return convertErrNo(errno);
  } else {
    return UINT32_C(0);
  }
}

Expect<uint32_t>
WasiFdFdstatGet::body(Runtime::Instance::MemoryInstance &MemInst, int32_t Fd,
                      uint32_t FdStatPtr) {
  __wasi_fdstat_t FdStat;
  /// 1. __wasi_fdstat_t.fs_filetype
  {
    struct stat SysFStat;
    if (fstat(Fd, &SysFStat) != 0) {
      return convertErrNo(errno);
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
      return convertErrNo(errno);
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
      return Unexpect(Res);
    }
    /// byte[1] : ZERO
    if (auto Res = MemInst.storeValue(uint32_t(0), FdStatPtr + 1, 1); !Res) {
      return Unexpect(Res);
    }
    /// byte[2:3] : fs_flags(uint16_t)
    if (auto Res =
            MemInst.storeValue(uint32_t(FdStat.fs_flags), FdStatPtr + 2, 2);
        !Res) {
      return Unexpect(Res);
    }
    /// byte[4:7] : ZERO
    if (auto Res = MemInst.storeValue(uint32_t(0), FdStatPtr + 4, 4); !Res) {
      return Unexpect(Res);
    }
    /// byte[8:15] : fs_rights_base
    if (auto Res = MemInst.storeValue(uint64_t(FdStat.fs_rights_base),
                                      FdStatPtr + 8, 8);
        !Res) {
      return Unexpect(Res);
    }
    /// byte[16:23] : fs_rights_inheriting
    if (auto Res = MemInst.storeValue(uint64_t(FdStat.fs_rights_inheriting),
                                      FdStatPtr + 16, 8);
        !Res) {
      return Unexpect(Res);
    }
  }

  return UINT32_C(0);
}

Expect<uint32_t>
WasiFdFdstatSetFlags::body(Runtime::Instance::MemoryInstance &MemInst,
                           int32_t Fd, uint32_t FsFlags) {
  /// TODO: implement
  return UINT32_C(0);
}

Expect<uint32_t>
WasiFdPrestatDirName::body(Runtime::Instance::MemoryInstance &MemInst,
                           int32_t Fd, uint32_t PathBufPtr, uint32_t PathLen) {
  for (auto &Entry : Env.getPreStats()) {
    if (Entry.Fd != Fd) {
      continue;
    }
    if (Entry.Path.size() > PathLen) {
      return __WASI_EINVAL;
    }

    /// Store Path and PathLen.
    if (auto Res =
            MemInst.setBytes(Entry.Path, PathBufPtr, 0, Entry.Path.size());
        !Res) {
      return Unexpect(Res);
    }

    return UINT32_C(0);
  }
  return __WASI_EBADF;
}

Expect<uint32_t>
WasiFdPrestatGet::body(Runtime::Instance::MemoryInstance &MemInst, int32_t Fd,
                       uint32_t PreStatPtr) {
  for (const auto &Entry : Env.getPreStats()) {
    if (Entry.Fd != Fd) {
      continue;
    }

    /// Store to memory instance
    /// byte[0] : pr_type
    if (auto Res = MemInst.storeValue(uint32_t(Entry.Type), PreStatPtr, 1);
        !Res) {
      return Unexpect(Res);
    }
    /// byte[5:8] : u.dir.pr_name_len
    if (auto Res =
            MemInst.storeValue(uint32_t(Entry.Path.size()), PreStatPtr + 4, 4);
        !Res) {
      return Unexpect(Res);
    }

    return UINT32_C(0);
  }
  return __WASI_EBADF;
}

Expect<uint32_t> WasiFdRead::body(Runtime::Instance::MemoryInstance &MemInst,
                                  int32_t Fd, uint32_t IOVSPtr,
                                  uint32_t IOVSCnt, uint32_t NReadPtr) {
  /// Sequencially reading.
  uint32_t NRead = 0;
  for (uint32_t I = 0; I < IOVSCnt; I++) {
    uint32_t CIOVecBufPtr = 0;
    uint32_t CIOVecBufLen = 0;
    /// Get data offset.
    if (auto Res = MemInst.loadValue(CIOVecBufPtr, IOVSPtr, 4); !Res) {
      return Unexpect(Res);
    }
    /// Get data length.
    if (auto Res = MemInst.loadValue(CIOVecBufLen, IOVSPtr + 4, 4); !Res) {
      return Unexpect(Res);
    }
    /// Read data from Fd.
    unsigned char *ReadArr = MemInst.getPointer<unsigned char *>(CIOVecBufPtr);
    int32_t SizeRead = read(Fd, ReadArr, CIOVecBufLen);
    /// Store data.
    if (SizeRead == -1) {
      /// Store read bytes length.
      if (auto Res = MemInst.storeValue(NRead, NReadPtr, 4); !Res) {
        return Unexpect(Res);
      }
      return convertErrNo(errno);
    }

    NRead += SizeRead;
    /// Shift one element.
    IOVSPtr += 8;
  }

  /// Store read bytes length.
  if (auto Res = MemInst.storeValue(NRead, NReadPtr, 4); !Res) {
    return Unexpect(Res);
  }
  return UINT32_C(0);
}

Expect<int32_t> WasiFdSeek::body(Runtime::Instance::MemoryInstance &MemInst,
                                 int32_t Fd, int64_t Offset, uint32_t Whence,
                                 uint32_t NewOffsetPtr) {
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
    if (auto Res = MemInst.storeValue(UINT64_C(-1), NewOffsetPtr, 8); !Res) {
      return Unexpect(Res);
    }
    return __WASI_EINVAL;
  }

  /// Do lseek.
  uint64_t NewOffset = lseek(Fd, Offset, Whence);
  if (auto Res = MemInst.storeValue(NewOffset, NewOffsetPtr, 8); !Res) {
    return Unexpect(Res);
  }
  return convertErrNo(errno);
}

Expect<uint32_t> WasiFdWrite::body(Runtime::Instance::MemoryInstance &MemInst,
                                   int32_t Fd, uint32_t IOVSPtr,
                                   uint32_t IOVSCnt, uint32_t NWrittenPtr) {
  /// Sequencially writting.
  uint32_t NWritten = 0;
  for (uint32_t I = 0; I < IOVSCnt; I++) {
    uint32_t CIOVecBufPtr = 0;
    uint32_t CIOVecBufLen = 0;
    /// Get data offset.
    if (auto Res = MemInst.loadValue(CIOVecBufPtr, IOVSPtr, 4); !Res) {
      return Unexpect(Res);
    }
    /// Get data length.
    if (auto Res = MemInst.loadValue(CIOVecBufLen, IOVSPtr + 4, 4); !Res) {
      return Unexpect(Res);
    }
    /// Write data to Fd.
    unsigned char *WriteArr = MemInst.getPointer<unsigned char *>(CIOVecBufPtr);
    int32_t SizeWrite = write(Fd, WriteArr, CIOVecBufLen);
    if (SizeWrite == -1) {
      /// Store read bytes length.
      if (auto Res = MemInst.storeValue(NWritten, NWrittenPtr, 4); !Res) {
        return Unexpect(Res);
      }
      return convertErrNo(errno);
    }

    NWritten += SizeWrite;
    /// Shift one element.
    IOVSPtr += 8;
  }

  /// Store read bytes length.
  if (auto Res = MemInst.storeValue(NWritten, NWrittenPtr, 4); !Res) {
    return Unexpect(Res);
  }
  return UINT32_C(0);
}

Expect<uint32_t> WasiPathOpen::body(Runtime::Instance::MemoryInstance &MemInst,
                                    int32_t DirFd, uint32_t DirFlags,
                                    uint32_t PathPtr, uint32_t PathLen,
                                    uint32_t OFlags, uint64_t FsRightsBase,
                                    uint64_t FsRightsInheriting,
                                    uint32_t FsFlags, uint32_t FdPtr) {
  /// Get file path.
  std::vector<unsigned char> Data;
  if (auto Res = MemInst.getBytes(PathPtr, PathLen)) {
    Data = *Res;
  } else {
    return Unexpect(Res);
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
    return convertErrNo(errno);
  }
  if (auto Res = MemInst.storeValue(uint32_t(Fd), FdPtr, 4); !Res) {
    return Unexpect(Res);
  }
  return UINT32_C(0);
}

Expect<void> WasiProcExit::body(Runtime::Instance::MemoryInstance &MemInst,
                                int32_t Status) {
  Env.setStatus(Status);
  return Unexpect(ErrCode::Terminated);
}

Expect<void> WasiRandomGet::body(Runtime::Instance::MemoryInstance &MemInst,
                                 uint32_t BufPtr, uint32_t BufLen) {
  /// Use uniform distribution to generate random bytes array
  std::vector<uint8_t> RandomBytes(BufLen);
  std::random_device RandomDevice;
  std::mt19937 Generator(RandomDevice());
  std::uniform_int_distribution<> Distribution(
      std::numeric_limits<uint8_t>::min(), std::numeric_limits<uint8_t>::max());
  std::generate(
      RandomBytes.begin(), RandomBytes.end(),
      [&Generator, &Distribution] { return Distribution(Generator); });
  /// Store random bytes array into buffer pointer
  if (auto Res = MemInst.setBytes(RandomBytes, BufPtr, 0, BufLen); !Res) {
    return Unexpect(Res);
  }
  return {};
}

} // namespace Host
} // namespace SSVM
