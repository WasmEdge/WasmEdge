// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/defines.h"

#if WASMEDGE_OS_LINUX
// Uncomment these flag to test CentOS 6
// #undef __GLIBC_MINOR__
// #define __GLIBC_MINOR__ 5

#include "common/span.h"
#include "host/wasi/vfs-linux.h"
#include "vfs.ipp"
#include <cassert>
#include <ctime>
#include <dirent.h>
#include <fcntl.h>
#include <optional>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <vector>
#if __GLIBC_PREREQ(2, 8)
#include <sys/timerfd.h>
#endif

namespace WasmEdge {
namespace Host {

namespace {

static constexpr __wasi_errno_t convertErrNo(int ErrNo) noexcept {
  switch (ErrNo) {
  default:
    return __WASI_ERRNO_SUCCESS;
  case E2BIG:
    return __WASI_ERRNO_2BIG;
  case EACCES:
    return __WASI_ERRNO_ACCES;
  case EADDRINUSE:
    return __WASI_ERRNO_ADDRINUSE;
  case EADDRNOTAVAIL:
    return __WASI_ERRNO_ADDRNOTAVAIL;
  case EAFNOSUPPORT:
    return __WASI_ERRNO_AFNOSUPPORT;
  case EAGAIN:
    return __WASI_ERRNO_AGAIN;
  case EALREADY:
    return __WASI_ERRNO_ALREADY;
  case EBADF:
    return __WASI_ERRNO_BADF;
  case EBADMSG:
    return __WASI_ERRNO_BADMSG;
  case EBUSY:
    return __WASI_ERRNO_BUSY;
  case ECANCELED:
    return __WASI_ERRNO_CANCELED;
  case ECHILD:
    return __WASI_ERRNO_CHILD;
  case ECONNABORTED:
    return __WASI_ERRNO_CONNABORTED;
  case ECONNREFUSED:
    return __WASI_ERRNO_CONNREFUSED;
  case ECONNRESET:
    return __WASI_ERRNO_CONNRESET;
  case EDEADLK:
    return __WASI_ERRNO_DEADLK;
  case EDESTADDRREQ:
    return __WASI_ERRNO_DESTADDRREQ;
  case EDOM:
    return __WASI_ERRNO_DOM;
  case EDQUOT:
    return __WASI_ERRNO_DQUOT;
  case EEXIST:
    return __WASI_ERRNO_EXIST;
  case EFAULT:
    return __WASI_ERRNO_FAULT;
  case EFBIG:
    return __WASI_ERRNO_FBIG;
  case EHOSTUNREACH:
    return __WASI_ERRNO_HOSTUNREACH;
  case EIDRM:
    return __WASI_ERRNO_IDRM;
  case EILSEQ:
    return __WASI_ERRNO_ILSEQ;
  case EINPROGRESS:
    return __WASI_ERRNO_INPROGRESS;
  case EINTR:
    return __WASI_ERRNO_INTR;
  case EINVAL:
    return __WASI_ERRNO_INVAL;
  case EIO:
    return __WASI_ERRNO_IO;
  case EISCONN:
    return __WASI_ERRNO_ISCONN;
  case EISDIR:
    return __WASI_ERRNO_ISDIR;
  case ELOOP:
    return __WASI_ERRNO_LOOP;
  case EMFILE:
    return __WASI_ERRNO_MFILE;
  case EMLINK:
    return __WASI_ERRNO_MLINK;
  case EMSGSIZE:
    return __WASI_ERRNO_MSGSIZE;
  case EMULTIHOP:
    return __WASI_ERRNO_MULTIHOP;
  case ENAMETOOLONG:
    return __WASI_ERRNO_NAMETOOLONG;
  case ENETDOWN:
    return __WASI_ERRNO_NETDOWN;
  case ENETRESET:
    return __WASI_ERRNO_NETRESET;
  case ENETUNREACH:
    return __WASI_ERRNO_NETUNREACH;
  case ENFILE:
    return __WASI_ERRNO_NFILE;
  case ENOBUFS:
    return __WASI_ERRNO_NOBUFS;
  case ENODEV:
    return __WASI_ERRNO_NODEV;
  case ENOENT:
    return __WASI_ERRNO_NOENT;
  case ENOEXEC:
    return __WASI_ERRNO_NOEXEC;
  case ENOLCK:
    return __WASI_ERRNO_NOLCK;
  case ENOLINK:
    return __WASI_ERRNO_NOLINK;
  case ENOMEM:
    return __WASI_ERRNO_NOMEM;
  case ENOMSG:
    return __WASI_ERRNO_NOMSG;
  case ENOPROTOOPT:
    return __WASI_ERRNO_NOPROTOOPT;
  case ENOSPC:
    return __WASI_ERRNO_NOSPC;
  case ENOSYS:
    return __WASI_ERRNO_NOSYS;
  case ENOTCONN:
    return __WASI_ERRNO_NOTCONN;
  case ENOTDIR:
    return __WASI_ERRNO_NOTDIR;
  case ENOTEMPTY:
    return __WASI_ERRNO_NOTEMPTY;
  case ENOTRECOVERABLE:
    return __WASI_ERRNO_NOTRECOVERABLE;
  case ENOTSOCK:
    return __WASI_ERRNO_NOTSOCK;
  case ENOTSUP:
    return __WASI_ERRNO_NOTSUP;
  case ENOTTY:
    return __WASI_ERRNO_NOTTY;
  case ENXIO:
    return __WASI_ERRNO_NXIO;
  case EOVERFLOW:
    return __WASI_ERRNO_OVERFLOW;
  case EOWNERDEAD:
    return __WASI_ERRNO_OWNERDEAD;
  case EPERM:
    return __WASI_ERRNO_PERM;
  case EPIPE:
    return __WASI_ERRNO_PIPE;
  case EPROTO:
    return __WASI_ERRNO_PROTO;
  case EPROTONOSUPPORT:
    return __WASI_ERRNO_PROTONOSUPPORT;
  case EPROTOTYPE:
    return __WASI_ERRNO_PROTOTYPE;
  case ERANGE:
    return __WASI_ERRNO_RANGE;
  case EROFS:
    return __WASI_ERRNO_ROFS;
  case ESPIPE:
    return __WASI_ERRNO_SPIPE;
  case ESRCH:
    return __WASI_ERRNO_SRCH;
  case ESTALE:
    return __WASI_ERRNO_STALE;
  case ETIMEDOUT:
    return __WASI_ERRNO_TIMEDOUT;
  case ETXTBSY:
    return __WASI_ERRNO_TXTBSY;
  case EXDEV:
    return __WASI_ERRNO_XDEV;
  }
}

static constexpr int convertAdvice(__wasi_advice_t Advice) noexcept {
  switch (Advice) {
  case __WASI_ADVICE_NORMAL:
    return POSIX_FADV_NORMAL;
  case __WASI_ADVICE_SEQUENTIAL:
    return POSIX_FADV_SEQUENTIAL;
  case __WASI_ADVICE_RANDOM:
    return POSIX_FADV_RANDOM;
  case __WASI_ADVICE_WILLNEED:
    return POSIX_FADV_WILLNEED;
  case __WASI_ADVICE_DONTNEED:
    return POSIX_FADV_DONTNEED;
  case __WASI_ADVICE_NOREUSE:
    return POSIX_FADV_NOREUSE;
  default:
    __builtin_unreachable();
  }
}

static constexpr int convertFdFlags(__wasi_fdflags_t Flags) noexcept {
  int SysFlags = 0;

  if (Flags & __WASI_FDFLAGS_NONBLOCK) {
    SysFlags |= O_NONBLOCK;
  }
  if (Flags & __WASI_FDFLAGS_APPEND) {
    SysFlags |= O_APPEND;
  }
  if (Flags & __WASI_FDFLAGS_DSYNC) {
    SysFlags |= O_DSYNC;
  }
  if (Flags & __WASI_FDFLAGS_RSYNC) {
    SysFlags |= O_RSYNC;
  }
  if (Flags & __WASI_FDFLAGS_SYNC) {
    SysFlags |= O_SYNC;
  }

  return SysFlags;
}

static constexpr __wasi_fdflags_t convertFdFlags(int SysFlags) noexcept {
  __wasi_fdflags_t Flags = static_cast<__wasi_fdflags_t>(0);

  if (SysFlags & O_NONBLOCK) {
    Flags |= __WASI_FDFLAGS_NONBLOCK;
  }
  if (SysFlags & O_APPEND) {
    Flags |= __WASI_FDFLAGS_APPEND;
  }
  if (SysFlags & O_DSYNC) {
    Flags |= __WASI_FDFLAGS_DSYNC;
  }
  if (SysFlags & O_RSYNC) {
    Flags |= __WASI_FDFLAGS_RSYNC;
  }
  if (SysFlags & O_SYNC) {
    Flags |= __WASI_FDFLAGS_SYNC;
  }

  return Flags;
}

static constexpr __wasi_filetype_t statMode2FileType(mode_t Mode) noexcept {
  switch (Mode & S_IFMT) {
  case S_IFBLK:
    return __WASI_FILETYPE_BLOCK_DEVICE;
  case S_IFCHR:
    return __WASI_FILETYPE_CHARACTER_DEVICE;
  case S_IFDIR:
    return __WASI_FILETYPE_DIRECTORY;
  case S_IFREG:
    return __WASI_FILETYPE_REGULAR_FILE;
    /// XXX: wasi-libc sees fifo files as socket files. This will be change at
    /// next snapshot. https://github.com/WebAssembly/wasi-libc/pull/107/files
  case S_IFSOCK:
  case S_IFIFO:
    return __WASI_FILETYPE_SOCKET_STREAM;
  case S_IFLNK:
    return __WASI_FILETYPE_SYMBOLIC_LINK;
  default:
    return __WASI_FILETYPE_UNKNOWN;
  }
}

static constexpr __wasi_filetype_t direntType2FileType(uint8_t Type) noexcept {
  switch (Type) {
  case DT_BLK:
    return __WASI_FILETYPE_BLOCK_DEVICE;
  case DT_CHR:
    return __WASI_FILETYPE_CHARACTER_DEVICE;
  case DT_DIR:
    return __WASI_FILETYPE_DIRECTORY;
  case DT_LNK:
    return __WASI_FILETYPE_SYMBOLIC_LINK;
  case DT_REG:
    return __WASI_FILETYPE_REGULAR_FILE;
    /// XXX: wasi-libc sees fifo files as socket files. This will be change at
    /// next snapshot. https://github.com/WebAssembly/wasi-libc/pull/107/files
  case DT_SOCK:
  case DT_FIFO:
    return __WASI_FILETYPE_SOCKET_STREAM;
  case DT_UNKNOWN:
  default:
    return __WASI_FILETYPE_UNKNOWN;
  }
}

static constexpr int convertWhence(__wasi_whence_t Whence) noexcept {
  switch (Whence) {
  case __WASI_WHENCE_CUR:
    return SEEK_CUR;
  case __WASI_WHENCE_END:
    return SEEK_END;
  case __WASI_WHENCE_SET:
    return SEEK_SET;
  default:
    __builtin_unreachable();
  }
}

static constexpr int lookupflags2At(__wasi_lookupflags_t Flags) noexcept {
  int SysFlags = AT_SYMLINK_NOFOLLOW;

  if (Flags & __WASI_LOOKUPFLAGS_SYMLINK_FOLLOW) {
    SysFlags &= ~AT_SYMLINK_NOFOLLOW;
  }

  return SysFlags;
}

static constexpr int lookupflags2O(__wasi_lookupflags_t Flags) noexcept {
  int SysFlags = 0;

  if (Flags & __WASI_LOOKUPFLAGS_SYMLINK_FOLLOW) {
    SysFlags |= O_NOFOLLOW;
  }

  return SysFlags;
}
static constexpr int convertRiFlags(__wasi_riflags_t RiFlags) noexcept {
  int SysFlags = 0;

  if (RiFlags & __WASI_RIFLAGS_RECV_PEEK) {
    RiFlags &= ~__WASI_RIFLAGS_RECV_PEEK;
    SysFlags |= MSG_PEEK;
  }
  if (RiFlags & __WASI_RIFLAGS_RECV_WAITALL) {
    RiFlags &= ~__WASI_RIFLAGS_RECV_WAITALL;
    SysFlags |= MSG_WAITALL;
  }

  return SysFlags;
}
static constexpr __wasi_roflags_t convertRoFlags(int SysRoFlags) noexcept {
  __wasi_roflags_t RoFlags = static_cast<__wasi_roflags_t>(0);
  if (SysRoFlags & MSG_TRUNC) {
    RoFlags |= __WASI_ROFLAGS_RECV_DATA_TRUNCATED;
  }

  return RoFlags;
}
static constexpr int convertSiFlags(__wasi_siflags_t) noexcept { return 0; }
static constexpr int convertSdFlags(__wasi_sdflags_t SdFlags) noexcept {
  if (SdFlags == __WASI_SDFLAGS_RD) {
    return SHUT_RD;
  } else if (SdFlags == __WASI_SDFLAGS_WR) {
    return SHUT_WR;
  } else if (SdFlags == (__WASI_SDFLAGS_RD | __WASI_SDFLAGS_WR)) {
    return SHUT_RDWR;
  }
  __builtin_unreachable();
}

static constexpr int convertClockId(__wasi_clockid_t ClockId) noexcept {
  switch (ClockId) {
  case __WASI_CLOCKID_REALTIME:
    return CLOCK_REALTIME;
  case __WASI_CLOCKID_MONOTONIC:
    return CLOCK_MONOTONIC;
  case __WASI_CLOCKID_PROCESS_CPUTIME_ID:
    return CLOCK_PROCESS_CPUTIME_ID;
  case __WASI_CLOCKID_THREAD_CPUTIME_ID:
    return CLOCK_THREAD_CPUTIME_ID;
  default:
    __builtin_unreachable();
  }
}

static constexpr __wasi_timestamp_t
timespec2Timestamp(const timespec &Time) noexcept {
  using namespace std::chrono;
  const auto Result = seconds(Time.tv_sec) + nanoseconds(Time.tv_nsec);
  return Result.count();
}

static constexpr timespec
timestamp2Timespec(__wasi_timestamp_t Timestamp) noexcept {
  using namespace std::chrono;
  const auto Total = nanoseconds(Timestamp);
  const auto Second = duration_cast<seconds>(Total);
  const auto Nano = Total - Second;
  timespec Result{};
  Result.tv_sec = Second.count();
  Result.tv_nsec = Nano.count();
  return Result;
}

#if !__GLIBC_PREREQ(2, 6)
static constexpr timeval
timestamp2Timeval(__wasi_timestamp_t Timestamp) noexcept {
  using namespace std::chrono;
  const auto Total = microseconds(Timestamp);
  const auto Second = duration_cast<seconds>(Total);
  const auto Micro = Total - Second;
  timeval Result{};
  Result.tv_sec = Second.count();
  Result.tv_usec = Micro.count();
  return Result;
}
#endif

static constexpr __wasi_filestat_t
stat2Filestat(struct stat SysFStat) noexcept {
  return {
      .dev = SysFStat.st_dev,
      .ino = SysFStat.st_ino,
      .filetype = statMode2FileType(SysFStat.st_mode),
      .nlink = SysFStat.st_nlink,
      .size = static_cast<__wasi_filesize_t>(SysFStat.st_size),
#ifndef __APPLE__
      .atim = timespec2Timestamp(SysFStat.st_atim),
      .mtim = timespec2Timestamp(SysFStat.st_mtim),
      .ctim = timespec2Timestamp(SysFStat.st_ctim),
#endif
  };
}

static std::string toPathStr(Span<const uint8_t> Path) {
  return std::string(reinterpret_cast<const char *>(Path.data()), Path.size());
}

#ifdef IOV_MAX
static inline constexpr const int32_t kIOVSMax = IOV_MAX;
#else
static inline constexpr const int32_t kIOVSMax = 1024;
#endif

template <typename T> struct ScopeExit {
  T F;
  ScopeExit(T &&F) : F(F) {}
  ~ScopeExit() { F(); }
};

} // namespace

inline WasiFile WasiFile::stdinFile() noexcept {
  using namespace std::string_literals;
  return WasiFile(kStdInDefaultRights, kNoInheritingRights, STDIN_FILENO,
                  "/dev/stderr"s);
}

inline WasiFile WasiFile::stdoutFile() noexcept {
  using namespace std::string_literals;
  return WasiFile(kStdOutDefaultRights, kNoInheritingRights, STDOUT_FILENO,
                  "/dev/stderr"s);
}

inline WasiFile WasiFile::stderrFile() noexcept {
  using namespace std::string_literals;
  return WasiFile(kStdErrDefaultRights, kNoInheritingRights, STDERR_FILENO,
                  "/dev/stderr"s);
}

inline WasiFile WasiFile::preopened(std::string Name,
                                    std::string RealPath) noexcept {
  int Flags = O_DIRECTORY;
#if defined(O_PATH)
  Flags |= O_PATH;
#endif

  int Fd = open(RealPath.c_str(), Flags);
  return WasiFile(kReadRights | kWriteRights | kCreateRights,
                  kReadRights | kWriteRights | kCreateRights, Fd,
                  std::move(Name));
}

inline void swap(WasiFile &LHS, WasiFile &RHS) noexcept {
  using std::swap;
  swap(static_cast<WasiFileBase<WasiFile> &>(LHS),
       static_cast<WasiFileBase<WasiFile> &>(RHS));
  swap(LHS.Fd, RHS.Fd);
}

inline WasiFile::WasiFile(__wasi_rights_t R, __wasi_rights_t IR, int Fd,
                          std::string PN) noexcept
    : BaseT(R, IR, std::move(PN)), Fd(Fd) {}

inline WasiFile::WasiFile(WasiFile &&RHS) noexcept
    : BaseT(std::move(RHS)), Fd(std::exchange(RHS.Fd, -1)) {}

inline WasiFile &WasiFile::operator=(WasiFile &&RHS) noexcept {
  if (this != &RHS) {
    swap(*this, RHS);
  }
  return *this;
}

inline WasiFile::~WasiFile() noexcept {
  if (Dir) {
    closedir(Dir);
  }
  if (Fd >= 0 && !BaseT::isPreopened()) {
    close(Fd);
  }
}

inline bool WasiFile::doOk() const noexcept { return Fd >= 0; }

inline __wasi_errno_t WasiFile::doFdAdvise(__wasi_filesize_t Offset,
                                           __wasi_filesize_t Len,
                                           __wasi_advice_t Advice) noexcept {
  if (unlikely(posix_fadvise(Fd, Offset, Len, convertAdvice(Advice)) != 0)) {
    return convertErrNo(errno);
  }

  return __WASI_ERRNO_SUCCESS;
}

inline __wasi_errno_t WasiFile::doFdAllocate(__wasi_filesize_t Offset,
                                             __wasi_filesize_t Len) noexcept {
  if (unlikely(posix_fallocate(Fd, Offset, Len) != 0)) {
    return convertErrNo(errno);
  }

  return __WASI_ERRNO_SUCCESS;
}

inline __wasi_errno_t WasiFile::doFdClose() noexcept {
  if (unlikely(close(Fd) != 0)) {
    return convertErrNo(errno);
  }

  return __WASI_ERRNO_SUCCESS;
}

inline __wasi_errno_t WasiFile::doFdDatasync() noexcept {
  if (unlikely(fdatasync(Fd) != 0)) {
    return convertErrNo(errno);
  }

  return __WASI_ERRNO_SUCCESS;
}

inline __wasi_errno_t
WasiFile::doFdFdstatGet(__wasi_fdstat_t &FdStat) noexcept {
  /// 1. __wasi_fdstat_t.fs_filetype
  {
    struct stat SysFStat;
    if (unlikely(fstat(Fd, &SysFStat) != 0)) {
      return convertErrNo(errno);
    }
    FdStat.fs_filetype = statMode2FileType(SysFStat.st_mode);
  }

  /// 2. __wasi_fdstat_t.fs_flags
  {
    int SysFlags = fcntl(Fd, F_GETFL);
    if (unlikely(SysFlags < 0)) {
      return convertErrNo(errno);
    }
    FdStat.fs_flags = convertFdFlags(SysFlags);
  }

  return __WASI_ERRNO_SUCCESS;
}

inline __wasi_errno_t
WasiFile::doFdFdstatSetFlags(__wasi_fdflags_t Flags) noexcept {
  if (unlikely(fcntl(Fd, F_SETFL, convertFdFlags(Flags)) != 0)) {
    return convertErrNo(errno);
  }

  return __WASI_ERRNO_SUCCESS;
}

inline __wasi_errno_t
WasiFile::doFdFilestatGet(__wasi_filestat_t &Filestat) noexcept {
  struct stat SysFStat;
  if (unlikely(fstat(Fd, &SysFStat) != 0)) {
    return convertErrNo(errno);
  }
  Filestat = stat2Filestat(SysFStat);

  return __WASI_ERRNO_SUCCESS;
}

inline __wasi_errno_t
WasiFile::doFdFilestatSetSize(__wasi_filesize_t Size) noexcept {
  if (unlikely(ftruncate(Fd, Size) == -1)) {
    return convertErrNo(errno);
  }

  return __WASI_ERRNO_SUCCESS;
}

inline __wasi_errno_t
WasiFile::doFdFilestatSetTimes(__wasi_timestamp_t ATim, __wasi_timestamp_t MTim,
                               __wasi_fstflags_t FstFlags) noexcept {
#if __GLIBC_PREREQ(2, 6)
  timespec SysTimespec[2];
  if (FstFlags & __WASI_FSTFLAGS_ATIM) {
    SysTimespec[0] = timestamp2Timespec(ATim);
  } else if (FstFlags & __WASI_FSTFLAGS_ATIM_NOW) {
    SysTimespec[0].tv_nsec = UTIME_NOW;
  } else {
    SysTimespec[0].tv_nsec = UTIME_OMIT;
  }
  if (FstFlags & __WASI_FSTFLAGS_MTIM) {
    SysTimespec[1] = timestamp2Timespec(MTim);
  } else if (FstFlags & __WASI_FSTFLAGS_MTIM_NOW) {
    SysTimespec[1].tv_nsec = UTIME_NOW;
  } else {
    SysTimespec[1].tv_nsec = UTIME_OMIT;
  }

  if (unlikely(futimens(Fd, SysTimespec) != 0)) {
    return convertErrNo(errno);
  }
#else
  bool NeedNow = false;
  bool NeedFile = false;
  if (FstFlags & __WASI_FSTFLAGS_ATIM) {
    // Nothing to do.
  } else if (FstFlags & __WASI_FSTFLAGS_ATIM_NOW) {
    NeedNow = true;
  } else {
    NeedFile = true;
  }
  if (FstFlags & __WASI_FSTFLAGS_MTIM) {
    // Nothing to do.
  } else if (FstFlags & __WASI_FSTFLAGS_MTIM_NOW) {
    NeedNow = true;
  } else {
    NeedFile = true;
  }

  timespec Now;
  if (NeedNow && unlikely(clock_gettime(CLOCK_REALTIME, &Now) != 0)) {
    return convertErrNo(errno);
  }
  struct stat Stat;
  if (NeedFile && unlikely(fstat(Fd, &Stat) != 0)) {
    return convertErrNo(errno);
  }

  timeval SysTimeval[2];
  if (FstFlags & __WASI_FSTFLAGS_ATIM) {
    SysTimeval[0] = timestamp2Timeval(ATim);
  } else if (FstFlags & __WASI_FSTFLAGS_ATIM_NOW) {
    SysTimeval[0].tv_sec = Now.tv_sec;
    SysTimeval[0].tv_usec = Now.tv_nsec / 1000;
  } else {
    SysTimeval[0].tv_sec = Stat.st_atim.tv_sec;
    SysTimeval[0].tv_usec = Stat.st_atim.tv_nsec / 1000;
  }
  if (FstFlags & __WASI_FSTFLAGS_MTIM) {
    SysTimeval[1] = timestamp2Timeval(MTim);
  } else if (FstFlags & __WASI_FSTFLAGS_MTIM_NOW) {
    SysTimeval[1].tv_sec = Now.tv_sec;
    SysTimeval[1].tv_usec = Now.tv_nsec / 1000;
  } else {
    SysTimeval[1].tv_sec = Stat.st_atim.tv_sec;
    SysTimeval[1].tv_usec = Stat.st_atim.tv_nsec / 1000;
  }

  if (unlikely(futimes(Fd, SysTimeval) != 0)) {
    return convertErrNo(errno);
  }
#endif

  return __WASI_ERRNO_SUCCESS;
}

inline __wasi_errno_t WasiFile::doFdPread(Span<const Span<uint8_t>> IOVS,
                                          __wasi_filesize_t Offset,
                                          __wasi_size_t &NRead) noexcept {
  if (unlikely(IOVS.size() > kIOVSMax)) {
    return __WASI_ERRNO_INVAL;
  }

  iovec SysIOVS[kIOVSMax];
  std::transform(IOVS.begin(), IOVS.end(), std::begin(SysIOVS),
                 [](const Span<uint8_t> &IOV) {
                   return iovec{.iov_base = IOV.data(), .iov_len = IOV.size()};
                 });

#if __GLIBC_PREREQ(2, 10)
  const auto Ret = preadv(Fd, SysIOVS, IOVS.size(), Offset);
#else
  const off_t ErrOffset = -1;
  const off_t OldOffset = lseek(Fd, 0, SEEK_CUR);
  if (OldOffset == ErrOffset) {
    NRead = -1;
    return convertErrNo(errno);
  }
  if (lseek(Fd, Offset, SEEK_SET) == ErrOffset) {
    NRead = -1;
    return convertErrNo(errno);
  }
  const auto Ret = readv(Fd, SysIOVS, IOVS.size());
  const int SavedErrNo = errno;
  if (lseek(Fd, OldOffset, SEEK_SET) == ErrOffset) {
    if (NRead != -1) {
      NRead = -1;
      return convertErrNo(errno);
    }
  }
  errno = SavedErrNo;
#endif

  if (unlikely(Ret < 0)) {
    return convertErrNo(errno);
  }
  /// Store read bytes length.
  NRead = Ret;

  return __WASI_ERRNO_SUCCESS;
}

inline __wasi_errno_t WasiFile::doFdPwrite(Span<const Span<const uint8_t>> IOVS,
                                           __wasi_filesize_t Offset,
                                           __wasi_size_t &NWritten) noexcept {
  if (unlikely(IOVS.size() > kIOVSMax)) {
    return __WASI_ERRNO_INVAL;
  }

  iovec SysIOVS[kIOVSMax];
  std::transform(IOVS.begin(), IOVS.end(), std::begin(SysIOVS),
                 [](const Span<const uint8_t> &IOV) {
                   return iovec{.iov_base = const_cast<uint8_t *>(IOV.data()),
                                .iov_len = IOV.size()};
                 });

#if __GLIBC_PREREQ(2, 10)
  const auto Ret = pwritev(Fd, SysIOVS, IOVS.size(), Offset);
#else
  const off_t ErrOffset = -1;
  const off_t OldOffset = lseek(Fd, 0, SEEK_CUR);
  if (OldOffset == ErrOffset) {
    NWritten = -1;
    return convertErrNo(errno);
  }
  if (lseek(Fd, Offset, SEEK_SET) == ErrOffset) {
    NWritten = -1;
    return convertErrNo(errno);
  }
  const auto Ret = writev(Fd, SysIOVS, IOVS.size());
  const int SavedErrNo = errno;
  if (lseek(Fd, OldOffset, SEEK_SET) == ErrOffset) {
    if (NWritten != -1) {
      NWritten = -1;
      return convertErrNo(errno);
    }
  }
  errno = SavedErrNo;
#endif

  if (unlikely(Ret < 0)) {
    return convertErrNo(errno);
  }
  /// Store write bytes length.
  NWritten = Ret;

  return __WASI_ERRNO_SUCCESS;
}

inline __wasi_errno_t WasiFile::doFdRead(Span<const Span<uint8_t>> IOVS,
                                         __wasi_size_t &NRead) noexcept {
  if (unlikely(IOVS.size() > kIOVSMax)) {
    return __WASI_ERRNO_INVAL;
  }

  iovec SysIOVS[kIOVSMax];
  std::transform(IOVS.begin(), IOVS.end(), std::begin(SysIOVS),
                 [](const Span<uint8_t> &IOV) {
                   return iovec{.iov_base = IOV.data(), .iov_len = IOV.size()};
                 });

  const auto Ret = readv(Fd, SysIOVS, IOVS.size());
  if (unlikely(Ret < 0)) {
    return convertErrNo(errno);
  }
  /// Store read bytes length.
  NRead = Ret;

  return __WASI_ERRNO_SUCCESS;
}

inline __wasi_errno_t WasiFile::doFdReaddir(Span<uint8_t> Buf,
                                            __wasi_dircookie_t Cookie,
                                            __wasi_size_t &Bufused) noexcept {
  if (unlikely(!Dir)) {
    Dir = fdopendir(Fd);
    if (Dir == nullptr) {
      return convertErrNo(errno);
    }
  }
  if (unlikely(Cookie != DirCookie)) {
    seekdir(Dir, Cookie);
    DirCookie = Cookie;
    DirentData = {};
  }

  Bufused = 0;
  do {
    if (!DirentData.empty()) {
      const auto NewDataSize =
          std::min<uint32_t>(Buf.size(), DirentData.size());
      std::copy_n(DirentData.begin(), NewDataSize, Buf.begin());
      DirentData = DirentData.subspan(NewDataSize);
      Buf = Buf.subspan(NewDataSize);
      Bufused += NewDataSize;
      if (unlikely(Buf.empty())) {
        break;
      }
    }
    errno = 0;
    dirent *SysDirent = readdir(Dir);
    if (SysDirent == nullptr) {
      if (errno != 0) {
        return convertErrNo(errno);
      }
      return __WASI_ERRNO_SUCCESS;
    }
    std::string_view Name = SysDirent->d_name;
    assert(Name.size() <= NAME_MAX);
    __wasi_dirent_t *const Dirent =
        reinterpret_cast<__wasi_dirent_t *>(DirentBuffer.data());
    Dirent->d_next = DirCookie = SysDirent->d_off;
    Dirent->d_ino = SysDirent->d_ino;
    Dirent->d_type = direntType2FileType(SysDirent->d_type);
    Dirent->d_namlen = Name.size();
    std::copy_n(Name.cbegin(), Name.size(),
                DirentBuffer.begin() + sizeof(__wasi_dirent_t));

    DirentData = {DirentBuffer.data(), sizeof(__wasi_dirent_t) + Name.size()};
  } while (!Buf.empty());

  return __WASI_ERRNO_SUCCESS;
}

inline __wasi_errno_t
WasiFile::doFdSeek(__wasi_filedelta_t Offset, __wasi_whence_t Whence,
                   __wasi_filedelta_t &NewOffset) noexcept {
  const auto Ret = lseek(Fd, Offset, convertWhence(Whence));
  if (unlikely(NewOffset < 0)) {
    return convertErrNo(errno);
  }
  NewOffset = Ret;

  return __WASI_ERRNO_SUCCESS;
}

inline __wasi_errno_t WasiFile::doFdSync() noexcept {
  if (unlikely(fsync(Fd) != 0)) {
    return convertErrNo(errno);
  }

  return __WASI_ERRNO_SUCCESS;
}

inline __wasi_errno_t WasiFile::doFdTell(__wasi_filesize_t &Offset) noexcept {
  const auto Ret = lseek(Fd, 0, SEEK_CUR);
  if (unlikely(Ret < 0)) {
    return convertErrNo(errno);
  }
  Offset = Ret;

  return __WASI_ERRNO_SUCCESS;
}

inline __wasi_errno_t WasiFile::doFdWrite(Span<const Span<const uint8_t>> IOVS,
                                          __wasi_size_t &NWritten) noexcept {
  if (unlikely(IOVS.size() > kIOVSMax)) {
    return __WASI_ERRNO_INVAL;
  }

  iovec SysIOVS[kIOVSMax];
  std::transform(IOVS.begin(), IOVS.end(), std::begin(SysIOVS),
                 [](const Span<const uint8_t> &IOV) {
                   return iovec{.iov_base = const_cast<uint8_t *>(IOV.data()),
                                .iov_len = IOV.size()};
                 });

  const auto Ret = writev(Fd, SysIOVS, IOVS.size());
  if (unlikely(Ret < 0)) {
    return convertErrNo(errno);
  }
  /// Store write bytes length.
  NWritten = Ret;

  return __WASI_ERRNO_SUCCESS;
}

inline __wasi_errno_t
WasiFile::doPathCreateDirectory(Span<const uint8_t> Path) noexcept {
  const auto PathStr = toPathStr(Path);

  if (unlikely(mkdirat(Fd, PathStr.c_str(), 0755) != 0)) {
    return convertErrNo(errno);
  }

  return __WASI_ERRNO_SUCCESS;
}

inline __wasi_errno_t
WasiFile::doPathFilestatGet(__wasi_lookupflags_t Flags,
                            Span<const uint8_t> Path,
                            __wasi_filestat_t &Buf) noexcept {
  const auto PathStr = toPathStr(Path);
  struct stat SysFStat;

  /// TODO: restrict PathStr on escaping root directory
  const auto Ret =
      fstatat(Fd, PathStr.c_str(), &SysFStat, lookupflags2At(Flags));
  if (unlikely(Ret != 0)) {
    return convertErrNo(errno);
  }
  Buf = stat2Filestat(SysFStat);

  return __WASI_ERRNO_SUCCESS;
}

inline __wasi_errno_t WasiFile::doPathFilestatSetTimes(
    __wasi_lookupflags_t Flags, Span<const uint8_t> Path,
    __wasi_timestamp_t ATim, __wasi_timestamp_t MTim,
    __wasi_fstflags_t FstFlags) noexcept {
  const auto PathStr = toPathStr(Path);

#if __GLIBC_PREREQ(2, 6)
  timespec SysTimespec[2];
  if (FstFlags & __WASI_FSTFLAGS_ATIM) {
    SysTimespec[0] = timestamp2Timespec(ATim);
  } else if (FstFlags & __WASI_FSTFLAGS_ATIM_NOW) {
    SysTimespec[0].tv_nsec = UTIME_NOW;
  } else {
    SysTimespec[0].tv_nsec = UTIME_OMIT;
  }
  if (FstFlags & __WASI_FSTFLAGS_MTIM) {
    SysTimespec[1] = timestamp2Timespec(MTim);
  } else if (FstFlags & __WASI_FSTFLAGS_MTIM_NOW) {
    SysTimespec[1].tv_nsec = UTIME_NOW;
  } else {
    SysTimespec[1].tv_nsec = UTIME_OMIT;
  }

  if (unlikely(utimensat(Fd, PathStr.c_str(), SysTimespec,
                         lookupflags2At(Flags)) != 0)) {
    return convertErrNo(errno);
  }
#else
  bool NeedNow = false;
  bool NeedFile = false;
  if (FstFlags & __WASI_FSTFLAGS_ATIM) {
    // Nothing to do.
  } else if (FstFlags & __WASI_FSTFLAGS_ATIM_NOW) {
    NeedNow = true;
  } else {
    NeedFile = true;
  }
  if (FstFlags & __WASI_FSTFLAGS_MTIM) {
    // Nothing to do.
  } else if (FstFlags & __WASI_FSTFLAGS_MTIM_NOW) {
    NeedNow = true;
  } else {
    NeedFile = true;
  }

  int SysFd = openat(Fd, PathStr.c_str(), lookupflags2O(Flags));
  if (unlikely(SysFd < 0)) {
    return convertErrNo(errno);
  }

  ScopeExit Closer([&]() { close(SysFd); });

  timespec Now;
  if (NeedNow && unlikely(clock_gettime(CLOCK_REALTIME, &Now) != 0)) {
    return convertErrNo(errno);
  }
  struct stat Stat;
  if (NeedFile && unlikely(fstat(SysFd, &Stat) != 0)) {
    return convertErrNo(errno);
  }

  timeval SysTimeval[2];
  SysTimeval[0] = timestamp2Timeval(ATim);
  SysTimeval[1] = timestamp2Timeval(MTim);
  if (unlikely(futimes(SysFd, SysTimeval) != 0)) {
    return convertErrNo(errno);
  }
#endif

  return __WASI_ERRNO_SUCCESS;
}

inline __wasi_errno_t
WasiFile::doPathLink(WasiFile &Old, __wasi_lookupflags_t OldFlags,
                     Span<const uint8_t> OldPath, WasiFile &New,
                     Span<const uint8_t> NewPath) noexcept {
  const auto OldPathStr = toPathStr(OldPath);
  const auto NewPathStr = toPathStr(NewPath);

  if (unlikely(linkat(Old.Fd, OldPathStr.c_str(), New.Fd, NewPathStr.c_str(),
                      lookupflags2At(OldFlags)) != 0)) {
    return convertErrNo(errno);
  }

  return __WASI_ERRNO_SUCCESS;
}

inline WasiExpect<WasiFile>
WasiFile::doPathOpen(Span<const uint8_t> Path, __wasi_lookupflags_t DirFlags,
                     __wasi_oflags_t OFlags, __wasi_rights_t FsRightsBase,
                     __wasi_rights_t FsRightsInheriting,
                     __wasi_fdflags_t FdFlags) noexcept {
  const bool Read =
      (FsRightsBase & (__WASI_RIGHTS_FD_READ | __WASI_RIGHTS_FD_READDIR)) != 0;
  const bool Write =
      (FsRightsBase &
       (__WASI_RIGHTS_FD_DATASYNC | __WASI_RIGHTS_FD_WRITE |
        __WASI_RIGHTS_FD_ALLOCATE | __WASI_RIGHTS_FD_FILESTAT_SET_SIZE)) != 0;

  int Flags = Write ? (Read ? O_RDWR : O_WRONLY) : O_RDONLY;
  if (OFlags & __WASI_OFLAGS_CREAT) {
    Flags |= O_CREAT;
  }
  if (OFlags & __WASI_OFLAGS_DIRECTORY) {
    Flags |= O_DIRECTORY;
  }
  if (OFlags & __WASI_OFLAGS_EXCL) {
    Flags |= O_EXCL;
  }
  if (OFlags & __WASI_OFLAGS_TRUNC) {
    Flags |= O_TRUNC;
  }

  // Convert file descriptor flags.
  if ((FdFlags & __WASI_FDFLAGS_APPEND) != 0)
    Flags |= O_APPEND;
  if ((FdFlags & __WASI_FDFLAGS_DSYNC) != 0) {
#ifdef O_DSYNC
    Flags |= O_DSYNC;
#else
    Flags |= O_SYNC;
#endif
  }
  if ((FdFlags & __WASI_FDFLAGS_NONBLOCK) != 0)
    Flags |= O_NONBLOCK;
  if ((FdFlags & __WASI_FDFLAGS_RSYNC) != 0) {
#ifdef O_RSYNC
    Flags |= O_RSYNC;
#else
    Flags |= O_SYNC;
#endif
  }
  if ((FdFlags & __WASI_FDFLAGS_SYNC) != 0) {
    Flags |= O_SYNC;
  }

  Flags |= lookupflags2O(DirFlags);

  /// Open file and store Fd.
  const auto PathStr = toPathStr(Path);

  int NewFd = openat(Fd, PathStr.c_str(), Flags, 0644);
  if (unlikely(NewFd < 0)) {
    return WasiUnexpect(convertErrNo(errno));
  }

  return WasiFile(FsRightsBase, FsRightsInheriting, NewFd, {});
}

inline __wasi_errno_t
WasiFile::doPathReadlink(Span<const uint8_t> Path, Span<uint8_t> Buf,
                         __wasi_size_t &BufUsed) noexcept {
  const auto PathStr = toPathStr(Path);

  if (unlikely(readlinkat(Fd, PathStr.c_str(),
                          reinterpret_cast<char *>(Buf.data()),
                          Buf.size()) < 0)) {
    return convertErrNo(errno);
  }

  return __WASI_ERRNO_SUCCESS;
}

inline __wasi_errno_t
WasiFile::doPathRemoveDirectory(Span<const uint8_t> Path) noexcept {
  const auto PathStr = toPathStr(Path);

  if (unlinkat(Fd, PathStr.c_str(), AT_REMOVEDIR) < 0) {
    return convertErrNo(errno);
  }

  return __WASI_ERRNO_SUCCESS;
}

inline __wasi_errno_t
WasiFile::doPathRename(WasiFile &Old, Span<const uint8_t> OldPath,
                       WasiFile &New, Span<const uint8_t> NewPath) noexcept {
  const auto OldPathStr = toPathStr(OldPath);
  const auto NewPathStr = toPathStr(NewPath);

  if (unlikely(renameat(Old.Fd, OldPathStr.c_str(), New.Fd,
                        NewPathStr.c_str()) != 0)) {
    return convertErrNo(errno);
  }

  return __WASI_ERRNO_SUCCESS;
}

inline __wasi_errno_t
WasiFile::doPathSymlink(Span<const uint8_t> OldPath,
                        Span<const uint8_t> NewPath) noexcept {
  const auto OldPathStr = toPathStr(OldPath);
  const auto NewPathStr = toPathStr(NewPath);

  if (unlikely(symlinkat(OldPathStr.c_str(), Fd, NewPathStr.c_str()) != 0)) {
    return convertErrNo(errno);
  }

  return __WASI_ERRNO_SUCCESS;
}

inline __wasi_errno_t
WasiFile::doPathUnlinkFile(Span<const uint8_t> Path) noexcept {
  const auto PathStr = toPathStr(Path);

  if (unlinkat(Fd, PathStr.c_str(), 0) < 0) {
    return convertErrNo(errno);
  }

  return __WASI_ERRNO_SUCCESS;
}

struct WasiFile::Poller {
  struct Timer {
    Timer() noexcept = default;
    Timer(const Timer &) noexcept = delete;
    Timer(Timer &&) noexcept = default;
    ~Timer() noexcept { reset(); }
#if __GLIBC_PREREQ(2, 8)
    bool create(__wasi_clockid_t ClockId, __wasi_subclockflags_t Flags,
                __wasi_timestamp_t Timeout) noexcept {
      reset();
      TimerFd =
          timerfd_create(convertClockId(ClockId), TFD_NONBLOCK | TFD_CLOEXEC);
      if (unlikely(TimerFd < 0)) {
        return false;
      }

      int SysFlags = 0;
      if (Flags & __WASI_SUBCLOCKFLAGS_SUBSCRIPTION_CLOCK_ABSTIME) {
        SysFlags |= TFD_TIMER_ABSTIME;
      }
      itimerspec ITimerSpec{timestamp2Timespec(0), timestamp2Timespec(Timeout)};
      if (unlikely(timerfd_settime(TimerFd, SysFlags, &ITimerSpec, nullptr) <
                   0)) {
        return false;
      }

      return true;
    }
    void reset() noexcept {
      if (likely(TimerFd != -1)) {
        close(TimerFd);
        TimerFd = -1;
      }
    }
    int TimerFd = -1;
#else
    bool create(__wasi_clockid_t ClockId, __wasi_subclockflags_t Flags,
                __wasi_timestamp_t Timeout) noexcept {
      reset();
      {
        int PipeFd[2] = {-1, -1};

        if (unlikely(pipe(PipeFd) != 0)) {
          return false;
        }

        TimerFd = PipeFd[0];
        NotifyFd = PipeFd[1];
      }

      {
        sigevent Event;
        Event.sigev_notify = SIGEV_THREAD;
        Event.sigev_notify_function = &sigevCallback;
        Event.sigev_value.sival_int = NotifyFd;
        Event.sigev_notify_attributes = nullptr;
        TimerId.emplace();

        if (unlikely(fcntl(TimerFd, F_SETFD, FD_CLOEXEC) != 0 ||
                     fcntl(NotifyFd, F_SETFD, FD_CLOEXEC) != 0 ||
                     timer_create(convertClockId(ClockId), &Event, &*TimerId) <
                         0)) {
          reset();
          return false;
        }
      }

      {
        int SysFlags = 0;
        if (Flags & __WASI_SUBCLOCKFLAGS_SUBSCRIPTION_CLOCK_ABSTIME) {
          SysFlags |= TIMER_ABSTIME;
        }
        itimerspec ITimerSpec{timestamp2Timespec(0),
                              timestamp2Timespec(Timeout)};

        if (unlikely(timer_settime(*TimerId, SysFlags, &ITimerSpec, nullptr) <
                     0)) {
          reset();
          return false;
        }
      }

      return true;
    }
    void reset() noexcept {
      if (likely(TimerFd != -1)) {
        close(TimerFd);
        TimerFd = -1;
      }
      if (likely(NotifyFd != -1)) {
        close(TimerFd);
        NotifyFd = -1;
      }
      if (likely(TimerId.has_value())) {
        timer_delete(*TimerId);
        TimerId.reset();
      }
    }
    static void sigevCallback(union sigval Value) noexcept {
      const uint64_t One = 1;
      write(Value.sival_int, &One, sizeof(One));
    }
    int TimerFd = -1;
    int NotifyFd = -1;
    std::optional<timer_t> TimerId;
#endif
  };

  ~Poller() noexcept { reset(); }

  void reset() noexcept {
    if (likely(EPollFd >= 0)) {
      close(EPollFd);
      EPollFd = -1;
    }
    Timers.clear();
    Events.clear();
    Events.shrink_to_fit();
  }

  __wasi_errno_t create(size_t MaxEvents) noexcept {
    if (unlikely(EPollFd >= 0)) {
      reset();
    }

    try {
      Events.resize(MaxEvents);
    } catch (std::bad_alloc &) {
      return __WASI_ERRNO_NOMEM;
    }

#if __GLIBC_PREREQ(2, 9)
    EPollFd = epoll_create1(EPOLL_CLOEXEC);
    if (unlikely(EPollFd < 0)) {
      return convertErrNo(errno);
    }
#else
    EPollFd = epoll_create(256);
    if (unlikely(EPollFd < 0)) {
      return convertErrNo(errno);
    }

    if (unlikely(fcntl(EPollFd, F_SETFD, FD_CLOEXEC) != 0)) {
      reset();
      return convertErrNo(errno);
    }
#endif

    return __WASI_ERRNO_SUCCESS;
  }

  __wasi_errno_t addTimer(const Subscription &Sub) noexcept {
    Timers.emplace_back();
    if (unlikely(!Timers.back().create(Sub.Clock.id, Sub.Clock.flags,
                                       Sub.Clock.timeout))) {
      return convertErrNo(errno);
    }
    const int TimerFd = Timers.back().TimerFd;

    epoll_event Event;
    Event.data.ptr = const_cast<Subscription *>(&Sub);
    Event.events = EPOLLIN;
    if (unlikely(epoll_ctl(EPollFd, EPOLL_CTL_ADD, TimerFd, &Event) < 0)) {
      return convertErrNo(errno);
    }
    return __WASI_ERRNO_SUCCESS;
  }

  __wasi_errno_t addFd(const Subscription &Sub, int Flag, int Fd) noexcept {
    epoll_event Event;
    Event.data.ptr = const_cast<Subscription *>(&Sub);
    Event.events = Flag;
    if (unlikely(epoll_ctl(EPollFd, EPOLL_CTL_ADD, Fd, &Event) < 0)) {
      return convertErrNo(errno);
    }
    return __WASI_ERRNO_SUCCESS;
  }

  __wasi_errno_t addFdRead(const Subscription &Sub) noexcept {
    return addFd(Sub, EPOLLIN, Sub.FdRead->Fd);
  }

  __wasi_errno_t addFdWrite(const Subscription &Sub) noexcept {
    return addFd(Sub, EPOLLOUT, Sub.FdWrite->Fd);
  }

  __wasi_errno_t
  wait(std::function<void(const Subscription &)> RecordClock,
       std::function<void(const Subscription &, __wasi_filesize_t,
                          __wasi_eventrwflags_t)>
           RecordFd) noexcept {
    const int Count = epoll_wait(EPollFd, Events.data(), Events.size(), -1);
    if (unlikely(Count < 0)) {
      return convertErrNo(errno);
    }

    for (const epoll_event &SysEvent : Events) {
      const auto &Sub = *static_cast<const Subscription *>(SysEvent.data.ptr);
      switch (Sub.Tag) {
      case __WASI_EVENTTYPE_CLOCK:
        RecordClock(Sub);
        break;
      case __WASI_EVENTTYPE_FD_READ:
      case __WASI_EVENTTYPE_FD_WRITE: {
        const bool IsRead = Sub.Tag == __WASI_EVENTTYPE_FD_READ;
        const int Fd = IsRead ? Sub.FdRead->Fd : Sub.FdWrite->Fd;
        int NBytes = 0;
        if (IsRead) {
          int ReadBufUsed = 0;
          if (likely(ioctl(Fd, FIONREAD, &ReadBufUsed) == 0)) {
            NBytes = ReadBufUsed;
          }
        } else {
          int WriteBufSize = 0;
          socklen_t IntSize = sizeof(WriteBufSize);
          if (likely(getsockopt(Fd, SOL_SOCKET, SO_SNDBUF, &WriteBufSize,
                                &IntSize) == 0)) {
            int WriteBufUsed = 0;
            if (likely(ioctl(Fd, TIOCOUTQ, &WriteBufUsed) == 0)) {
              NBytes = WriteBufSize - WriteBufUsed;
            }
          }
        }
        RecordFd(Sub, NBytes, static_cast<__wasi_eventrwflags_t>(0));
        break;
      }
      default:
        __builtin_unreachable();
      }
    }

    return __WASI_ERRNO_SUCCESS;
  }

  int EPollFd = -1;
  std::vector<Timer> Timers;
  std::vector<epoll_event> Events;
};

inline __wasi_errno_t WasiFile::doSockRecv(Span<const Span<uint8_t>> RiData,
                                           __wasi_riflags_t RiFlags,
                                           __wasi_size_t &RoDatalen,
                                           __wasi_roflags_t &RoFlags) noexcept {
  if (unlikely(RiData.size() > kIOVSMax)) {
    return __WASI_ERRNO_INVAL;
  }

  iovec SysRiData[kIOVSMax];
  std::transform(RiData.begin(), RiData.end(), std::begin(SysRiData),
                 [](const Span<uint8_t> &IOV) {
                   return iovec{.iov_base = IOV.data(), .iov_len = IOV.size()};
                 });

  msghdr SysMsgHdr;
  SysMsgHdr.msg_name = nullptr;
  SysMsgHdr.msg_namelen = 0;
  SysMsgHdr.msg_iov = SysRiData;
  SysMsgHdr.msg_iovlen = RiData.size();
  SysMsgHdr.msg_control = nullptr;
  SysMsgHdr.msg_controllen = 0;
  SysMsgHdr.msg_flags = 0;

  /// Store recv bytes length and flags.
  if (const auto Ret = recvmsg(Fd, &SysMsgHdr, convertRiFlags(RiFlags));
      unlikely(Ret < 0)) {
    return convertErrNo(errno);
  } else {
    RoDatalen = Ret;
    RoFlags = convertRoFlags(SysMsgHdr.msg_flags);
  }

  return __WASI_ERRNO_SUCCESS;
}

inline __wasi_errno_t
WasiFile::doSockSend(Span<const Span<const uint8_t>> SiData,
                     __wasi_siflags_t SiFlags,
                     __wasi_size_t &SoDatalen) noexcept {
  if (unlikely(SiData.size() > kIOVSMax)) {
    return __WASI_ERRNO_INVAL;
  }

  iovec SysSiData[kIOVSMax];
  std::transform(SiData.begin(), SiData.end(), std::begin(SysSiData),
                 [](const Span<const uint8_t> &IOV) {
                   return iovec{.iov_base = const_cast<uint8_t *>(IOV.data()),
                                .iov_len = IOV.size()};
                 });

  msghdr SysMsgHdr;
  SysMsgHdr.msg_name = nullptr;
  SysMsgHdr.msg_namelen = 0;
  SysMsgHdr.msg_iov = SysSiData;
  SysMsgHdr.msg_iovlen = SiData.size();
  SysMsgHdr.msg_control = nullptr;
  SysMsgHdr.msg_controllen = 0;
  SysMsgHdr.msg_flags = 0;

  /// Store recv bytes length and flags.
  if (const auto Ret = sendmsg(Fd, &SysMsgHdr, convertSiFlags(SiFlags));
      unlikely(Ret < 0)) {
    return convertErrNo(errno);
  } else {
    SoDatalen = Ret;
  }

  return __WASI_ERRNO_SUCCESS;
}

inline __wasi_errno_t WasiFile::doSockShutdown(__wasi_sdflags_t How) noexcept {
  if (unlikely(shutdown(Fd, convertSdFlags(How)) < 0)) {
    return convertErrNo(errno);
  }

  return __WASI_ERRNO_SUCCESS;
}

} // namespace Host
} // namespace WasmEdge

#endif
