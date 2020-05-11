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

static uint32_t statMode2FileType(mode_t Mode) {
  switch (Mode & S_IFMT) {
  case S_IFBLK:
    return __WASI_FILETYPE_BLOCK_DEVICE;
  case S_IFCHR:
    return __WASI_FILETYPE_CHARACTER_DEVICE;
  case S_IFDIR:
    return __WASI_FILETYPE_DIRECTORY;
  case S_IFREG:
    return __WASI_FILETYPE_REGULAR_FILE;
  case S_IFSOCK:
    return __WASI_FILETYPE_SOCKET_STREAM;
  case S_IFLNK:
    return __WASI_FILETYPE_SYMBOLIC_LINK;
  case S_IFIFO:
  default:
    return __WASI_FILETYPE_UNKNOWN;
  }
}

static uint64_t timespec2Nanoseconds(const struct timespec &Time) {
  return Time.tv_nsec * UINT64_C(1000000000) + Time.tv_nsec;
}

static struct timespec nanoseconds2Timespec(uint64_t Nanoseconds) {
  struct timespec Time;
  Time.tv_sec = Nanoseconds / UINT64_C(1000000000);
  Time.tv_nsec = Nanoseconds & UINT64_C(1000000000);
  return Time;
}

static SSVM::Host::WasiEnvironment::PreStat *
getEntry(SSVM::Host::WasiEnvironment &Env, uint32_t Fd) {
  for (auto &Entry : Env.getPreStats()) {
    if (Entry.Fd != Fd) {
      continue;
    }
    return &Entry;
  }
  return nullptr;
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
  const uint64_t Resolution = timespec2Nanoseconds(SysTimespec);
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

  timespec SysTimespec = nanoseconds2Timespec(Precision);
  if (clock_gettime(SysClockId, &SysTimespec) != 0) {
    return convertErrNo(errno);
  }
  const uint64_t Time = timespec2Nanoseconds(SysTimespec);
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

  auto *const Entry = getEntry(Env, Fd);
  if (Entry == nullptr) {
    return __WASI_EBADF;
  }

  if (!Entry->checkRights(__WASI_RIGHT_FD_ADVISE)) {
    return __WASI_ENOTCAPABLE;
  }

  // TODO: check rights __WASI_RIGHT_FD_ADVISE
  if (posix_fadvise(Fd, Offset, Len, SysAdvise) != 0) {
    return convertErrNo(errno);
  }
  return UINT32_C(0);
}

Expect<uint32_t>
WasiFdAllocate::body(Runtime::Instance::MemoryInstance &MemInst, int32_t Fd,
                     uint64_t Offset, uint64_t Len) {
  auto *const Entry = getEntry(Env, Fd);
  if (Entry == nullptr) {
    return __WASI_EBADF;
  }

  if (!Entry->checkRights(__WASI_RIGHT_FD_ALLOCATE)) {
    return __WASI_ENOTCAPABLE;
  }

  if (posix_fallocate(Fd, Offset, Len) != 0) {
    return convertErrNo(errno);
  }
  return UINT32_C(0);
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
  auto *const Entry = getEntry(Env, Fd);
  if (Entry == nullptr) {
    return __WASI_EBADF;
  }

  if (!Entry->checkRights(__WASI_RIGHT_FD_DATASYNC)) {
    return __WASI_ENOTCAPABLE;
  }

  if (fdatasync(Fd) != 0) {
    return convertErrNo(errno);
  } else {
    return UINT32_C(0);
  }
}

Expect<uint32_t>
WasiFdFdstatGet::body(Runtime::Instance::MemoryInstance &MemInst, int32_t Fd,
                      uint32_t FdStatPtr) {
  auto *const Entry = getEntry(Env, Fd);
  if (Entry == nullptr) {
    return __WASI_EBADF;
  }

  __wasi_fdstat_t FdStat;
  /// 1. __wasi_fdstat_t.fs_filetype
  {
    struct stat SysFStat;
    if (fstat(Fd, &SysFStat) != 0) {
      return convertErrNo(errno);
    }
    FdStat.fs_filetype = statMode2FileType(SysFStat.st_mode);
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
  FdStat.fs_rights_base = Entry->Rights;

  /// 4. __wasi_fdstat_t.fs_rights_inheriting
  FdStat.fs_rights_inheriting = Entry->InheritingRights;

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
  auto *const Entry = getEntry(Env, Fd);
  if (Entry == nullptr) {
    return __WASI_EBADF;
  }

  int SysFlag = 0;
  __wasi_rights_t AdditionalRequiredRights = 0;

  if (FsFlags & __WASI_FDFLAG_NONBLOCK) {
    SysFlag |= O_NONBLOCK;
  }
  if (FsFlags & __WASI_FDFLAG_APPEND) {
    SysFlag |= O_APPEND;
  }
  if (FsFlags & __WASI_FDFLAG_DSYNC) {
    SysFlag |= O_DSYNC;
    AdditionalRequiredRights |= __WASI_RIGHT_FD_DATASYNC;
  }
  if (FsFlags & __WASI_FDFLAG_RSYNC) {
    AdditionalRequiredRights |= __WASI_RIGHT_FD_SYNC;
    SysFlag |= O_RSYNC;
  }
  if (FsFlags & __WASI_FDFLAG_SYNC) {
    AdditionalRequiredRights |= __WASI_RIGHT_FD_SYNC;
    SysFlag |= O_SYNC;
  }

  if (!Entry->checkRights(__WASI_RIGHT_FD_FDSTAT_SET_FLAGS |
                          AdditionalRequiredRights)) {
    return __WASI_ENOTCAPABLE;
  }

  if (fcntl(Fd, F_SETFL, SysFlag) != 0) {
    return convertErrNo(errno);
  }

  return UINT32_C(0);
}

Expect<uint32_t>
WasiFdFdstatSetRights::body(Runtime::Instance::MemoryInstance &MemInst,
                            int32_t Fd, uint64_t FsRightsBase,
                            uint64_t FsRightsInheriting) {
  /// TODO: implement
  return UINT32_C(0);
}

Expect<uint32_t>
WasiFdFilestatGet::body(Runtime::Instance::MemoryInstance &MemInst, int32_t Fd,
                        uint32_t FilestatPtr) {
  auto *const Entry = getEntry(Env, Fd);
  if (Entry == nullptr) {
    return __WASI_EBADF;
  }

  if (!Entry->checkRights(__WASI_RIGHT_FD_FILESTAT_GET)) {
    return __WASI_ENOTCAPABLE;
  }

  struct stat SysFStat;
  if (fstat(Fd, &SysFStat) != 0) {
    return convertErrNo(errno);
  }

  /// Store to memory instance
  /// byte[0:8] : st_dev
  if (auto Res = MemInst.storeValue(uint64_t(SysFStat.st_dev), FilestatPtr, 8); !Res) {
    return Unexpect(Res);
  }
  /// byte[9:16] : st_ino
  if (auto Res = MemInst.storeValue(uint64_t(SysFStat.st_ino), FilestatPtr + 8, 8);
      !Res) {
    return Unexpect(Res);
  }
  /// byte[16] : st_mode
  if (auto Res =
          MemInst.storeValue(statMode2FileType(SysFStat.st_mode), FilestatPtr + 16, 1);
      !Res) {
    return Unexpect(Res);
  }
  /// byte[20:28] : st_nlink
  if (auto Res = MemInst.storeValue(uint64_t(SysFStat.st_nlink), FilestatPtr + 20, 8);
      !Res) {
    return Unexpect(Res);
  }
  /// byte[28:36] : st_size
  if (auto Res = MemInst.storeValue(uint64_t(SysFStat.st_size), FilestatPtr + 28, 8);
      !Res) {
    return Unexpect(Res);
  }
  /// byte[36:44] : st_atim
  if (auto Res = MemInst.storeValue(timespec2Nanoseconds(SysFStat.st_atim),
                                    FilestatPtr + 36, 8);
      !Res) {
    return Unexpect(Res);
  }
  /// byte[44:52] : st_mtim
  if (auto Res = MemInst.storeValue(timespec2Nanoseconds(SysFStat.st_mtim),
                                    FilestatPtr + 44, 8);
      !Res) {
    return Unexpect(Res);
  }
  /// byte[52:60] : st_ctim
  if (auto Res = MemInst.storeValue(timespec2Nanoseconds(SysFStat.st_ctim),
                                    FilestatPtr + 52, 8);
      !Res) {
    return Unexpect(Res);
  }

  return UINT32_C(0);
}

Expect<uint32_t>
WasiFdFilestatSetSize::body(Runtime::Instance::MemoryInstance &MemInst,
                            int32_t Fd, uint64_t FileSize) {
  /// TODO: implement
  return UINT32_C(0);
}

Expect<uint32_t>
WasiFdFilestatSetTimes::body(Runtime::Instance::MemoryInstance &MemInst,
                             int32_t Fd, uint32_t ATimPtr, uint32_t MTimPtr,
                             uint32_t FstFlags) {
  /// TODO: implement
  return UINT32_C(0);
}

Expect<uint32_t> WasiFdPread::body(Runtime::Instance::MemoryInstance &MemInst,
                                   int32_t Fd, uint32_t IOVSPtr,
                                   uint32_t Offset, uint32_t NRead) {
  /// TODO: implement
  return UINT32_C(0);
}

Expect<uint32_t>
WasiFdPrestatDirName::body(Runtime::Instance::MemoryInstance &MemInst,
                           int32_t Fd, uint32_t PathBufPtr, uint32_t PathLen) {
  auto *const Entry = getEntry(Env, Fd);
  if (Entry == nullptr) {
    return __WASI_EBADF;
  }

  if (Entry->Path.size() > PathLen) {
    return __WASI_EINVAL;
  }

  /// Store Path and PathLen.
  if (auto Res = MemInst.setBytes(
          Span<const Byte>(reinterpret_cast<const Byte *>(Entry->Path.data()),
                           Entry->Path.size()),
          PathBufPtr, 0, Entry->Path.size());
      !Res) {
    return Unexpect(Res);
  }

  return UINT32_C(0);
}

Expect<uint32_t>
WasiFdPrestatGet::body(Runtime::Instance::MemoryInstance &MemInst, int32_t Fd,
                       uint32_t PreStatPtr) {
  auto *const Entry = getEntry(Env, Fd);
  if (Entry == nullptr) {
    return __WASI_EBADF;
  }

  /// Store to memory instance
  /// byte[0] : pr_type
  if (auto Res = MemInst.storeValue(uint32_t(__WASI_PREOPENTYPE_DIR), PreStatPtr, 1);
      !Res) {
    return Unexpect(Res);
  }
  /// byte[5:8] : u.dir.pr_name_len
  if (auto Res =
          MemInst.storeValue(uint32_t(Entry->Path.size()), PreStatPtr + 4, 4);
      !Res) {
    return Unexpect(Res);
  }

  return UINT32_C(0);
}

Expect<uint32_t> WasiFdPwrite::body(Runtime::Instance::MemoryInstance &MemInst,
                                    int32_t Fd, uint32_t IOVSPtr,
                                    uint32_t Offset, uint32_t NWritten) {
  /// TODO: implement
  return UINT32_C(0);
}

Expect<uint32_t> WasiFdRead::body(Runtime::Instance::MemoryInstance &MemInst,
                                  int32_t Fd, uint32_t IOVSPtr,
                                  uint32_t IOVSCnt, uint32_t NReadPtr) {
  auto *const Entry = getEntry(Env, Fd);
  if (Entry == nullptr) {
    return __WASI_EBADF;
  }

  if (!Entry->checkRights(__WASI_RIGHT_FD_READ)) {
    return __WASI_ENOTCAPABLE;
  }

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

Expect<uint32_t> WasiFdReadDir::body(Runtime::Instance::MemoryInstance &MemInst,
                                     int32_t Fd, uint32_t BufPtr,
                                     uint32_t BufLen, uint64_t Cookie,
                                     uint32_t BufUsedSize) {
  /// TODO: implement
  return UINT32_C(0);
}

Expect<uint32_t>
WasiFdRenumber::body(Runtime::Instance::MemoryInstance &MemInst, int32_t Fd,
                     int32_t ToFd) {
  /// TODO: implement
  return UINT32_C(0);
}

Expect<int32_t> WasiFdSeek::body(Runtime::Instance::MemoryInstance &MemInst,
                                 int32_t Fd, int64_t Offset, uint32_t Whence,
                                 uint32_t NewOffsetPtr) {
  auto *const Entry = getEntry(Env, Fd);
  if (Entry == nullptr) {
    return __WASI_EBADF;
  }

  if (!Entry->checkRights(__WASI_RIGHT_FD_SEEK)) {
    return __WASI_ENOTCAPABLE;
  }

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
    return __WASI_EINVAL;
  }

  /// Do lseek.
  uint64_t NewOffset = lseek(Fd, Offset, Whence);
  if (auto Res = MemInst.storeValue(NewOffset, NewOffsetPtr, 8); !Res) {
    return Unexpect(Res);
  }
  return convertErrNo(errno);
}

Expect<uint32_t> WasiFdSync::body(Runtime::Instance::MemoryInstance &MemInst,
                                  int32_t Fd) {
  /// TODO: implement
  return UINT32_C(0);
}
Expect<uint32_t> WasiFdTell::body(Runtime::Instance::MemoryInstance &MemInst,
                                  int32_t Fd, int64_t Offset) {
  /// TODO: implement
  return UINT32_C(0);
}

Expect<uint32_t> WasiFdWrite::body(Runtime::Instance::MemoryInstance &MemInst,
                                   int32_t Fd, uint32_t IOVSPtr,
                                   uint32_t IOVSCnt, uint32_t NWrittenPtr) {
  auto *const Entry = getEntry(Env, Fd);
  if (Entry == nullptr) {
    return __WASI_EBADF;
  }

  if (!Entry->checkRights(__WASI_RIGHT_FD_WRITE)) {
    return __WASI_ENOTCAPABLE;
  }

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

Expect<uint32_t>
WasiPathCreateDirectory::body(Runtime::Instance::MemoryInstance &MemInst,
                              int32_t Fd, uint32_t PathPtr) {
  /// TODO: implement
  return UINT32_C(0);
}

Expect<uint32_t>
WasiPathFilestatGet::body(Runtime::Instance::MemoryInstance &MemInst,
                          int32_t Fd, uint32_t Flags, uint32_t PathPtr,
                          uint32_t BufPtr) {
  /// TODO: implement
  return UINT32_C(0);
}

Expect<uint32_t> WasiPathFilestatSetTimes::body(
    Runtime::Instance::MemoryInstance &MemInst, int32_t Fd, uint32_t Flags,
    uint32_t PathPtr, uint32_t ATim, uint32_t MTim, uint32_t FstFlags) {
  /// TODO: implement
  return UINT32_C(0);
}

Expect<uint32_t> WasiPathLink::body(Runtime::Instance::MemoryInstance &MemInst,
                                    int32_t OldFd, uint32_t OldFlags,
                                    uint32_t OldPathPtr, int32_t NewFd,
                                    uint32_t NewPathPtr) {
  /// TODO: implement
  return UINT32_C(0);
}

Expect<uint32_t> WasiPathOpen::body(Runtime::Instance::MemoryInstance &MemInst,
                                    int32_t DirFd, uint32_t DirFlags,
                                    uint32_t PathPtr, uint32_t PathLen,
                                    uint32_t OFlags, uint64_t FsRightsBase,
                                    uint64_t FsRightsInheriting,
                                    uint32_t FsFlags, uint32_t FdPtr) {
  auto *const Entry = getEntry(Env, DirFd);
  if (Entry == nullptr) {
    return __WASI_EBADF;
  }

  /// Get file path.
  std::vector<unsigned char> Data;
  if (auto Res = MemInst.getBytes(PathPtr, PathLen)) {
    Data = *Res;
  } else {
    return Unexpect(Res);
  }
  std::string Path(Data.begin(), Data.end());

  __wasi_rights_t RequiredRights = __WASI_RIGHT_PATH_OPEN;
  __wasi_rights_t RequiredInheritingRights = FsRightsBase | FsRightsInheriting;
  const bool Read =
      (FsRightsBase & (__WASI_RIGHT_FD_READ | __WASI_RIGHT_FD_READDIR)) != 0;
  const bool Write =
      (FsRightsBase &
       (__WASI_RIGHT_FD_DATASYNC | __WASI_RIGHT_FD_WRITE |
        __WASI_RIGHT_FD_ALLOCATE | __WASI_RIGHT_FD_FILESTAT_SET_SIZE)) != 0;

  int Flags = Write ? (Read ? O_RDWR : O_WRONLY) : O_RDONLY;
  if (OFlags & __WASI_O_CREAT) {
    RequiredRights |= __WASI_RIGHT_PATH_CREATE_FILE;
    Flags |= O_CREAT;
  }
  if (OFlags & __WASI_O_DIRECTORY) {
    Flags |= O_DIRECTORY;
  }
  if (OFlags & __WASI_O_EXCL) {
    Flags |= O_EXCL;
  }
  if (OFlags & __WASI_O_TRUNC) {
    RequiredRights |= __WASI_RIGHT_PATH_FILESTAT_SET_SIZE;
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

  if (!Entry->checkRights(RequiredInheritingRights, RequiredInheritingRights)) {
    return __WASI_ENOTCAPABLE;
  }

  /// Open file and store Fd.
  __wasi_fd_t Fd = open(Path.c_str(), Flags, 0644);
  if (Fd == -1) {
    return convertErrNo(errno);
  }
  Env.getPreStats().emplace_back(Fd, FsRightsBase, FsRightsInheriting, Path);
  if (auto Res = MemInst.storeValue(uint32_t(Fd), FdPtr, 4); !Res) {
    return Unexpect(Res);
  }
  return UINT32_C(0);
}

Expect<uint32_t>
WasiPathReadLink::body(Runtime::Instance::MemoryInstance &MemInst, int32_t Fd,
                       uint32_t PathPtr, uint32_t BufPtr, uint32_t BufLen) {
  /// TODO: implement
  return UINT32_C(0);
}

Expect<uint32_t>
WasiPathRemoveDirectory::body(Runtime::Instance::MemoryInstance &MemInst,
                              int32_t Fd, uint32_t PathPtr) {
  /// TODO: implement
  return UINT32_C(0);
}

Expect<uint32_t>
WasiPathRename::body(Runtime::Instance::MemoryInstance &MemInst, int32_t Fd,
                     uint32_t OldPathPtr, int32_t NewFd, uint32_t NewPathPtr) {
  /// TODO: implement
  return UINT32_C(0);
}

Expect<uint32_t>
WasiPathSymlink::body(Runtime::Instance::MemoryInstance &MemInst,
                      uint32_t OldPathPtr, int32_t Fd, uint32_t NewPathPtr) {
  /// TODO: implement
  return UINT32_C(0);
}

Expect<uint32_t>
WasiPathUnlinkFile::body(Runtime::Instance::MemoryInstance &MemInst, int32_t Fd,
                         uint32_t PathPtr) {
  /// TODO: implement
  return UINT32_C(0);
}

Expect<uint32_t>
WasiPollOneoff::body(Runtime::Instance::MemoryInstance &MemInst, uint32_t InPtr,
                     uint32_t OutPtr, uint32_t NSubscriptions,
                     uint32_t NEvents) {
  /// TODO: implement
  return UINT32_C(0);
}

Expect<void> WasiProcExit::body(Runtime::Instance::MemoryInstance &MemInst,
                                int32_t Status) {
  Env.setStatus(Status);
  return Unexpect(ErrCode::Terminated);
}

Expect<uint32_t> WasiProcRaise::body(Runtime::Instance::MemoryInstance &MemInst,
                                     int32_t Signal) {
  /// TODO: implement
  return UINT32_C(0);
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

Expect<uint32_t>
WasiSchedYield::body(Runtime::Instance::MemoryInstance &MemInst) {
  /// TODO: implement
  return UINT32_C(0);
}

Expect<uint32_t> WasiSockRecv::body(Runtime::Instance::MemoryInstance &MemInst,
                                    int32_t Fd, uint32_t RiDataPtr,
                                    uint32_t RiFlags, uint32_t RoDataLen,
                                    uint32_t RoFlags) {
  /// TODO: implement
  return UINT32_C(0);
}

Expect<uint32_t> WasiSockSend::body(Runtime::Instance::MemoryInstance &MemInst,
                                    int32_t Fd, uint32_t SiDataPtr,
                                    uint32_t SiFlags, uint32_t SoDataLen) {
  /// TODO: implement
  return UINT32_C(0);
}

Expect<uint32_t>
WasiSockShutdown::body(Runtime::Instance::MemoryInstance &MemInst, int32_t Fd,
                       uint32_t SdFlags) {
  /// TODO: implement
  return UINT32_C(0);
}

} // namespace Host
} // namespace SSVM
