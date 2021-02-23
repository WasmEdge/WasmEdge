// SPDX-License-Identifier: Apache-2.0
#define _DEFAULT_SOURCE
#include "host/wasi/wasifunc.h"
#include "common/filesystem.h"
#include "common/log.h"
#include "runtime/instance/memory.h"

#include <boost/scope_exit.hpp>
#include <chrono>
#include <csignal>
#include <fcntl.h>
#include <limits>
#include <numeric>
#include <random>
#include <string_view>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <time.h>
#include <unistd.h>

// Uncomment these flag to test CentOS 6
// #undef __GLIBC_MINOR__
// #define __GLIBC_MINOR__ 5

#ifndef __APPLE__
#include <sys/epoll.h>
#if __GLIBC_PREREQ(2, 8)
#include <sys/timerfd.h>
#endif
#endif

namespace {

static constexpr __wasi_errno_t convertErrNo(int ErrNo) noexcept {
  switch (ErrNo) {
  case 0:
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
  default:
    assert(false);
    __builtin_unreachable();
  }
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
  case S_IFSOCK:
    return __WASI_FILETYPE_SOCKET_STREAM;
  case S_IFLNK:
    return __WASI_FILETYPE_SYMBOLIC_LINK;
  case S_IFIFO:
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
  case DT_SOCK:
    return __WASI_FILETYPE_SOCKET_STREAM;
  case DT_FIFO:
  case DT_UNKNOWN:
  default:
    return __WASI_FILETYPE_UNKNOWN;
  }
}

static constexpr int signal2SysSignal(uint8_t Signal) noexcept {
  switch (Signal) {
  case __WASI_SIGNAL_HUP:
    return SIGHUP;
  case __WASI_SIGNAL_INT:
    return SIGINT;
  case __WASI_SIGNAL_QUIT:
    return SIGQUIT;
  case __WASI_SIGNAL_ILL:
    return SIGILL;
  case __WASI_SIGNAL_TRAP:
    return SIGTRAP;
  case __WASI_SIGNAL_ABRT:
    return SIGABRT;
  case __WASI_SIGNAL_BUS:
    return SIGBUS;
  case __WASI_SIGNAL_FPE:
    return SIGFPE;
  case __WASI_SIGNAL_KILL:
    return SIGKILL;
  case __WASI_SIGNAL_USR1:
    return SIGUSR1;
  case __WASI_SIGNAL_SEGV:
    return SIGSEGV;
  case __WASI_SIGNAL_USR2:
    return SIGUSR2;
  case __WASI_SIGNAL_PIPE:
    return SIGPIPE;
  case __WASI_SIGNAL_ALRM:
    return SIGALRM;
  case __WASI_SIGNAL_TERM:
    return SIGTERM;
  case __WASI_SIGNAL_CHLD:
    return SIGCHLD;
  case __WASI_SIGNAL_CONT:
    return SIGCONT;
  case __WASI_SIGNAL_STOP:
    return SIGSTOP;
  case __WASI_SIGNAL_TSTP:
    return SIGTSTP;
  case __WASI_SIGNAL_TTIN:
    return SIGTTIN;
  case __WASI_SIGNAL_TTOU:
    return SIGTTOU;
  case __WASI_SIGNAL_URG:
    return SIGURG;
  case __WASI_SIGNAL_XCPU:
    return SIGXCPU;
  case __WASI_SIGNAL_XFSZ:
    return SIGXFSZ;
  case __WASI_SIGNAL_VTALRM:
    return SIGVTALRM;
  case __WASI_SIGNAL_PROF:
    return SIGPROF;
  case __WASI_SIGNAL_WINCH:
    return SIGWINCH;
  case __WASI_SIGNAL_POLL:
    return SIGPOLL;
  case __WASI_SIGNAL_PWR:
    return SIGPWR;
  case __WASI_SIGNAL_SYS:
    return SIGSYS;
  default:
    return 0;
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

template <typename Container>
inline constexpr uint32_t CalculateBufferSize(const Container &Array) {
  uint32_t Lengths[std::size(Array)];
  std::transform(
      std::begin(Array), std::end(Array), Lengths,
      [](const auto &String) -> uint32_t { return std::size(String) + 1; });
  return std::accumulate(Lengths, Lengths + std::size(Array), UINT32_C(0));
}

#ifdef IOV_MAX
constexpr const int32_t kIOVSMax = IOV_MAX;
#else
constexpr const int32_t kIOVSMax = 1024;
#endif

} // namespace

namespace SSVM {
namespace Host {

Expect<uint32_t> WasiArgsGet::body(Runtime::Instance::MemoryInstance *MemInst,
                                   uint32_t ArgvPtr, uint32_t ArgvBufPtr) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  /// Store **Argv.
  const std::vector<std::string> &CmdArgs = Env.getCmdArgs();
  const uint32_t ArgvSize = CmdArgs.size() + 1;
  const uint32_t ArgvBufSize = CalculateBufferSize(CmdArgs);

  /// Check for invalid address.
  uint32_t *const Argv = MemInst->getPointer<uint32_t *>(ArgvPtr, ArgvSize);
  if (unlikely(Argv == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }
  uint8_t *const ArgvBuf =
      MemInst->getPointer<uint8_t *>(ArgvBufPtr, ArgvBufSize);
  if (unlikely(ArgvBuf == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  uint8_t *ArgvBufEnd = ArgvBuf;
  for (uint32_t I = 0; I < ArgvSize - 1; ++I) {
    /// Calcuate Argv[i] offset.
    Argv[I] = ArgvBufPtr + (ArgvBufEnd - ArgvBuf);
    /// Concate Argv.
    ArgvBufEnd = std::copy(CmdArgs[I].cbegin(), CmdArgs[I].cend(), ArgvBufEnd);
    *ArgvBufEnd = '\0';
    ++ArgvBufEnd;
  }
  /// Store nullptr
  Argv[ArgvSize - 1] = UINT32_C(0);

  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t>
WasiArgsSizesGet::body(Runtime::Instance::MemoryInstance *MemInst,
                       uint32_t ArgcPtr, uint32_t ArgvBufSizePtr) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  /// Check for invalid address.
  uint32_t *const __restrict__ Argc = MemInst->getPointer<uint32_t *>(ArgcPtr);
  if (unlikely(Argc == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }
  uint32_t *const __restrict__ ArgvBufSize =
      MemInst->getPointer<uint32_t *>(ArgvBufSizePtr);
  if (unlikely(ArgvBufSize == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  const std::vector<std::string> &CmdArgs = Env.getCmdArgs();

  /// Store Argc.
  *Argc = CmdArgs.size();

  /// Calculate and store ArgvBufSize.
  *ArgvBufSize = CalculateBufferSize(CmdArgs);

  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t>
WasiEnvironGet::body(Runtime::Instance::MemoryInstance *MemInst,
                     uint32_t EnvPtr, uint32_t EnvBufPtr) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  /// Store **Env.
  const std::vector<std::string> &Environs = Env.getEnvirons();
  const uint32_t EnvSize = Environs.size() + 1;
  const uint32_t EnvBufSize = CalculateBufferSize(Environs);

  /// Check for invalid address.
  uint32_t *const Env = MemInst->getPointer<uint32_t *>(EnvPtr, EnvSize);
  if (unlikely(Env == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }
  uint8_t *const EnvBuf = MemInst->getPointer<uint8_t *>(EnvBufPtr, EnvBufSize);
  if (unlikely(EnvBuf == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  uint8_t *EnvBufEnd = EnvBuf;
  for (uint32_t I = 0; I < EnvSize - 1; ++I) {
    /// Calculate Env[i] offset.
    Env[I] = EnvBufPtr + (EnvBufEnd - EnvBuf);
    /// Concate Env.
    EnvBufEnd = std::copy(Environs[I].cbegin(), Environs[I].cend(), EnvBufEnd);
    *EnvBufEnd = '\0';
    ++EnvBufEnd;
  }
  /// Store nullptr
  Env[EnvSize - 1] = UINT32_C(0);

  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t>
WasiEnvironSizesGet::body(Runtime::Instance::MemoryInstance *MemInst,
                          uint32_t EnvCntPtr, uint32_t EnvBufSizePtr) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  /// Check for invalid address.
  uint32_t *const __restrict__ EnvCnt =
      MemInst->getPointer<uint32_t *>(EnvCntPtr);
  if (unlikely(EnvCnt == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }
  uint32_t *const __restrict__ EnvBufSize =
      MemInst->getPointer<uint32_t *>(EnvBufSizePtr);
  if (unlikely(EnvBufSize == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  const std::vector<std::string> &Environs = Env.getEnvirons();

  /// Store EnvCnt.
  *EnvCnt = Environs.size();

  /// Calculate and store EnvBufSize.
  *EnvBufSize = CalculateBufferSize(Environs);

  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t>
WasiClockResGet::body(Runtime::Instance::MemoryInstance *MemInst,
                      uint32_t ClockId, uint32_t ResolutionPtr) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  clockid_t SysClockId;
  switch (ClockId) {
  case __WASI_CLOCKID_REALTIME:
    SysClockId = CLOCK_REALTIME;
    break;
  case __WASI_CLOCKID_MONOTONIC:
    SysClockId = CLOCK_MONOTONIC;
    break;
  case __WASI_CLOCKID_PROCESS_CPUTIME_ID:
    SysClockId = CLOCK_PROCESS_CPUTIME_ID;
    break;
  case __WASI_CLOCKID_THREAD_CPUTIME_ID:
    SysClockId = CLOCK_THREAD_CPUTIME_ID;
    break;
  default:
    return __WASI_ERRNO_INVAL;
  }

  __wasi_timestamp_t *const Resolution =
      MemInst->getPointer<__wasi_timestamp_t *>(ResolutionPtr);
  if (unlikely(Resolution == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  timespec SysTimespec;
  if (unlikely(clock_getres(SysClockId, &SysTimespec) != 0)) {
    return convertErrNo(errno);
  }

  *Resolution = timespec2Timestamp(SysTimespec);
  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t>
WasiClockTimeGet::body(Runtime::Instance::MemoryInstance *MemInst,
                       uint32_t ClockId, uint64_t Precision [[maybe_unused]],
                       uint32_t TimePtr) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  clockid_t SysClockId;
  switch (ClockId) {
  case __WASI_CLOCKID_REALTIME:
    SysClockId = CLOCK_REALTIME;
    break;
  case __WASI_CLOCKID_MONOTONIC:
    SysClockId = CLOCK_MONOTONIC;
    break;
  case __WASI_CLOCKID_PROCESS_CPUTIME_ID:
    SysClockId = CLOCK_PROCESS_CPUTIME_ID;
    break;
  case __WASI_CLOCKID_THREAD_CPUTIME_ID:
    SysClockId = CLOCK_THREAD_CPUTIME_ID;
    break;
  default:
    return __WASI_ERRNO_INVAL;
  }

  __wasi_timestamp_t *const Time =
      MemInst->getPointer<__wasi_timestamp_t *>(TimePtr);
  if (unlikely(Time == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  timespec SysTimespec;
  if (unlikely(clock_gettime(SysClockId, &SysTimespec) != 0)) {
    return convertErrNo(errno);
  }

  *Time = timespec2Timestamp(SysTimespec);
  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiFdAdvise::body(Runtime::Instance::MemoryInstance *MemInst,
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
    return __WASI_ERRNO_INVAL;
  }

  const auto Entry = Env.getFile(Fd);
  if (unlikely(Entry == Env.getFileEnd())) {
    return __WASI_ERRNO_BADF;
  }

  if (unlikely(!Entry->second.checkRights(__WASI_RIGHTS_FD_ADVISE))) {
    return __WASI_ERRNO_NOTCAPABLE;
  }

  if (unlikely(posix_fadvise(Entry->second.HostFd, Offset, Len, SysAdvise) !=
               0)) {
    return convertErrNo(errno);
  }

  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t>
WasiFdAllocate::body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd,
                     uint64_t Offset, uint64_t Len) {
  const auto Entry = Env.getFile(Fd);
  if (unlikely(Entry == Env.getFileEnd())) {
    return __WASI_ERRNO_BADF;
  }

  if (unlikely(!Entry->second.checkRights(__WASI_RIGHTS_FD_ALLOCATE))) {
    return __WASI_ERRNO_NOTCAPABLE;
  }

  if (unlikely(posix_fallocate(Entry->second.HostFd, Offset, Len) != 0)) {
    return convertErrNo(errno);
  }

  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiFdClose::body(Runtime::Instance::MemoryInstance *MemInst,
                                   int32_t Fd) {
  const auto Entry = Env.getFile(Fd);
  if (unlikely(Entry == Env.getFileEnd())) {
    return __WASI_ERRNO_BADF;
  }

  if (unlikely(Entry->second.IsPreopened)) {
    return __WASI_ERRNO_NOTSUP;
  }

  if (unlikely(close(Entry->second.HostFd) != 0)) {
    return convertErrNo(errno);
  }

  Env.eraseFile(Entry);
  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t>
WasiFdDatasync::body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd) {
  const auto Entry = Env.getFile(Fd);
  if (unlikely(Entry == Env.getFileEnd())) {
    return __WASI_ERRNO_BADF;
  }

  if (unlikely(!Entry->second.checkRights(__WASI_RIGHTS_FD_DATASYNC))) {
    return __WASI_ERRNO_NOTCAPABLE;
  }

  if (unlikely(fdatasync(Entry->second.HostFd) != 0)) {
    return convertErrNo(errno);
  }

  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t>
WasiFdFdstatGet::body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd,
                      uint32_t FdStatPtr) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  const auto Entry = Env.getFile(Fd);
  if (unlikely(Entry == Env.getFileEnd())) {
    return __WASI_ERRNO_BADF;
  }

  __wasi_fdstat_t *const FdStat =
      MemInst->getPointer<__wasi_fdstat_t *>(FdStatPtr);
  if (unlikely(FdStat == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  /// 1. __wasi_fdstat_t.fs_filetype
  {
    struct stat SysFStat;
    if (unlikely(fstat(Entry->second.HostFd, &SysFStat) != 0)) {
      return convertErrNo(errno);
    }
    FdStat->fs_filetype = statMode2FileType(SysFStat.st_mode);
  }

  /// 2. __wasi_fdstat_t.fs_flags
  {
    int FdFlags = fcntl(Entry->second.HostFd, F_GETFL);
    if (unlikely(FdFlags < 0)) {
      return convertErrNo(errno);
    }
    FdStat->fs_flags = static_cast<__wasi_fdflags_t>(0);
    if (FdFlags & O_APPEND) {
      FdStat->fs_flags |= __WASI_FDFLAGS_APPEND;
    }
    if (FdFlags & O_DSYNC) {
      FdStat->fs_flags |= __WASI_FDFLAGS_DSYNC;
    }
    if (FdFlags & O_NONBLOCK) {
      FdStat->fs_flags |= __WASI_FDFLAGS_NONBLOCK;
    }
    if (FdFlags & O_SYNC) {
      FdStat->fs_flags |= __WASI_FDFLAGS_RSYNC | __WASI_FDFLAGS_SYNC;
    }
  }

  /// 3. __wasi_fdstat_t.fs_rights_base
  FdStat->fs_rights_base = Entry->second.Rights;

  /// 4. __wasi_fdstat_t.fs_rights_inheriting
  FdStat->fs_rights_inheriting = Entry->second.InheritingRights;

  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t>
WasiFdFdstatSetFlags::body(Runtime::Instance::MemoryInstance *MemInst,
                           int32_t Fd, uint32_t FsFlags) {
  const auto Entry = Env.getFile(Fd);
  if (unlikely(Entry == Env.getFileEnd())) {
    return __WASI_ERRNO_BADF;
  }

  int SysFlag = 0;
  __wasi_rights_t AdditionalRequiredRights = static_cast<__wasi_rights_t>(0);

  if (FsFlags & __WASI_FDFLAGS_NONBLOCK) {
    SysFlag |= O_NONBLOCK;
  }
  if (FsFlags & __WASI_FDFLAGS_APPEND) {
    SysFlag |= O_APPEND;
  }
  if (FsFlags & __WASI_FDFLAGS_DSYNC) {
    SysFlag |= O_DSYNC;
    AdditionalRequiredRights |= __WASI_RIGHTS_FD_DATASYNC;
  }
  if (FsFlags & __WASI_FDFLAGS_RSYNC) {
    AdditionalRequiredRights |= __WASI_RIGHTS_FD_SYNC;
    SysFlag |= O_RSYNC;
  }
  if (FsFlags & __WASI_FDFLAGS_SYNC) {
    AdditionalRequiredRights |= __WASI_RIGHTS_FD_SYNC;
    SysFlag |= O_SYNC;
  }

  if (unlikely(!Entry->second.checkRights(__WASI_RIGHTS_FD_FDSTAT_SET_FLAGS |
                                          AdditionalRequiredRights))) {
    return __WASI_ERRNO_NOTCAPABLE;
  }

  if (unlikely(fcntl(Entry->second.HostFd, F_SETFL, SysFlag) != 0)) {
    return convertErrNo(errno);
  }

  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t>
WasiFdFdstatSetRights::body(Runtime::Instance::MemoryInstance *MemInst,
                            int32_t Fd, uint64_t FsRightsBaseV,
                            uint64_t FsRightsInheritingV) {
  const __wasi_rights_t FsRightsBase =
      static_cast<__wasi_rights_t>(FsRightsBaseV);
  const __wasi_rights_t FsRightsInheriting =
      static_cast<__wasi_rights_t>(FsRightsInheritingV);

  const auto Entry = Env.getFile(Fd);
  if (unlikely(Entry == Env.getFileEnd())) {
    return __WASI_ERRNO_BADF;
  }

  if (unlikely(!Entry->second.checkRights(FsRightsBase, FsRightsInheriting))) {
    return __WASI_ERRNO_NOTCAPABLE;
  }

  Entry->second.Rights = FsRightsBase;
  Entry->second.InheritingRights = FsRightsInheriting;
  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t>
WasiFdFilestatGet::body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd,
                        uint32_t FilestatPtr) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  const auto Entry = Env.getFile(Fd);
  if (unlikely(Entry == Env.getFileEnd())) {
    return __WASI_ERRNO_BADF;
  }

  if (unlikely(!Entry->second.checkRights(__WASI_RIGHTS_FD_FILESTAT_GET))) {
    return __WASI_ERRNO_NOTCAPABLE;
  }

  auto *const Filestat = MemInst->getPointer<__wasi_filestat_t *>(FilestatPtr);
  if (unlikely(Filestat == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  struct stat SysFStat;
  if (unlikely(fstat(Entry->second.HostFd, &SysFStat) != 0)) {
    return convertErrNo(errno);
  }

  /// Store to memory instance
  Filestat->dev = SysFStat.st_dev;
  Filestat->ino = SysFStat.st_ino;
  Filestat->filetype = statMode2FileType(SysFStat.st_mode);
  Filestat->nlink = SysFStat.st_nlink;
  Filestat->size = SysFStat.st_size;
#ifndef __APPLE__
  Filestat->atim = timespec2Timestamp(SysFStat.st_atim);
  Filestat->mtim = timespec2Timestamp(SysFStat.st_mtim);
  Filestat->ctim = timespec2Timestamp(SysFStat.st_ctim);
#endif

  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t>
WasiFdFilestatSetSize::body(Runtime::Instance::MemoryInstance *MemInst,
                            int32_t Fd, uint64_t FileSize) {
  const auto Entry = Env.getFile(Fd);
  if (unlikely(Entry == Env.getFileEnd())) {
    return __WASI_ERRNO_BADF;
  }

  if (unlikely(
          !Entry->second.checkRights(__WASI_RIGHTS_FD_FILESTAT_SET_SIZE))) {
    return __WASI_ERRNO_NOTCAPABLE;
  }

  if (unlikely(ftruncate(Entry->second.HostFd, FileSize) == -1)) {
    return convertErrNo(errno);
  }

  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t>
WasiFdFilestatSetTimes::body(Runtime::Instance::MemoryInstance *MemInst,
                             int32_t Fd, uint64_t ATim, uint64_t MTim,
                             uint32_t FstFlags) {
  const auto Entry = Env.getFile(Fd);
  if (unlikely(Entry == Env.getFileEnd())) {
    return __WASI_ERRNO_BADF;
  }

  if (unlikely(
          !Entry->second.checkRights(__WASI_RIGHTS_FD_FILESTAT_SET_TIMES))) {
    return __WASI_ERRNO_NOTCAPABLE;
  }

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

  if (unlikely(futimens(Entry->second.HostFd, SysTimespec) != 0)) {
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
  if (NeedFile && unlikely(fstat(Entry->second.HostFd, &Stat) != 0)) {
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

  if (unlikely(futimes(Entry->second.HostFd, SysTimeval) != 0)) {
    return convertErrNo(errno);
  }
#endif

  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiFdPread::body(Runtime::Instance::MemoryInstance *MemInst,
                                   int32_t Fd, uint32_t IOVSPtr,
                                   int32_t IOVSLen, uint64_t Offset,
                                   uint32_t NReadPtr) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  const auto Entry = Env.getFile(Fd);
  if (unlikely(Entry == Env.getFileEnd())) {
    return __WASI_ERRNO_BADF;
  }

  if (unlikely(!Entry->second.checkRights(__WASI_RIGHTS_FD_READ |
                                          __WASI_RIGHTS_FD_SEEK))) {
    return __WASI_ERRNO_NOTCAPABLE;
  }

  if (unlikely(IOVSLen < 0 || IOVSLen > kIOVSMax)) {
    return __WASI_ERRNO_INVAL;
  }

  /// Check for invalid address.
  __wasi_iovec_t *const IOVSArray =
      MemInst->getPointer<__wasi_iovec_t *>(IOVSPtr, IOVSLen);
  if (unlikely(IOVSArray == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  int32_t *const NRead = MemInst->getPointer<int32_t *>(NReadPtr);
  if (unlikely(NRead == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  uint32_t TotalSize = 0;
  iovec SysIOVS[kIOVSMax];

  for (uint32_t I = 0; I < static_cast<uint32_t>(IOVSLen); ++I) {
    __wasi_iovec_t &IOVS = IOVSArray[I];

    /// Check for total size overflow.
    const uint32_t Overflow = std::numeric_limits<int32_t>::max() - TotalSize;
    if (unlikely(IOVS.buf_len > Overflow)) {
      return __WASI_ERRNO_INVAL;
    }
    TotalSize += IOVS.buf_len;

    void *const ReadArr =
        MemInst->getPointer<uint8_t *>(IOVS.buf, IOVS.buf_len);
    /// Check for invalid address.
    if (unlikely(ReadArr == nullptr)) {
      return __WASI_ERRNO_FAULT;
    }
    SysIOVS[I].iov_base = ReadArr;
    SysIOVS[I].iov_len = IOVS.buf_len;
  }

#if __GLIBC_PREREQ(2, 10)
  /// Store read bytes length.
  *NRead = preadv(Entry->second.HostFd, SysIOVS, IOVSLen, Offset);
#else
  const off_t ErrOffset = -1;
  const off_t OldOffset = lseek(Entry->second.HostFd, 0, SEEK_CUR);
  if (OldOffset == ErrOffset) {
    *NRead = -1;
    return convertErrNo(errno);
  }
  if (lseek(Entry->second.HostFd, Offset, SEEK_SET) == ErrOffset) {
    *NRead = -1;
    return convertErrNo(errno);
  }
  *NRead = readv(Entry->second.HostFd, SysIOVS, IOVSLen);
  const int SavedErrNo = errno;
  if (lseek(Entry->second.HostFd, OldOffset, SEEK_SET) == ErrOffset) {
    if (*NRead != -1) {
      *NRead = -1;
      return convertErrNo(errno);
    }
  }
  errno = SavedErrNo;
#endif

  if (unlikely(*NRead < 0)) {
    return convertErrNo(errno);
  }

  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t>
WasiFdPrestatDirName::body(Runtime::Instance::MemoryInstance *MemInst,
                           int32_t Fd, uint32_t PathBufPtr, uint32_t PathLen) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  const auto Entry = Env.getFile(Fd);
  if (unlikely(Entry == Env.getFileEnd())) {
    return __WASI_ERRNO_BADF;
  }

  if (unlikely(Entry->second.Path.size() > PathLen)) {
    return __WASI_ERRNO_INVAL;
  }

  const auto &Path = Entry->second.Path;

  /// Store Path and PathLen.
  char *const PathBuf = MemInst->getPointer<char *>(PathBufPtr, Path.size());
  if (unlikely(PathBuf == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  std::copy(Path.begin(), Path.end(), PathBuf);

  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t>
WasiFdPrestatGet::body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd,
                       uint32_t PreStatPtr) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  const auto Entry = Env.getFile(Fd);
  if (unlikely(Entry == Env.getFileEnd())) {
    return __WASI_ERRNO_BADF;
  }

  __wasi_prestat_t *const PreStat =
      MemInst->getPointer<__wasi_prestat_t *>(PreStatPtr);
  if (unlikely(PreStat == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  PreStat->tag = __WASI_PREOPENTYPE_DIR;
  PreStat->u.dir.pr_name_len = Entry->second.Path.size();

  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiFdPwrite::body(Runtime::Instance::MemoryInstance *MemInst,
                                    int32_t Fd, uint32_t IOVSPtr,
                                    int32_t IOVSLen, uint64_t Offset,
                                    uint32_t NWrittenPtr) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  const auto Entry = Env.getFile(Fd);
  if (unlikely(Entry == Env.getFileEnd())) {
    return __WASI_ERRNO_BADF;
  }

  if (unlikely(!Entry->second.checkRights(__WASI_RIGHTS_FD_WRITE |
                                          __WASI_RIGHTS_FD_SEEK))) {
    return __WASI_ERRNO_NOTCAPABLE;
  }

  if (unlikely(IOVSLen < 0 || IOVSLen > kIOVSMax)) {
    return __WASI_ERRNO_INVAL;
  }

  /// Check for invalid address.
  __wasi_ciovec_t *const IOVSArray =
      MemInst->getPointer<__wasi_ciovec_t *>(IOVSPtr, IOVSLen);
  if (unlikely(IOVSArray == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  int32_t *const NWritten = MemInst->getPointer<int32_t *>(NWrittenPtr);
  if (unlikely(NWritten == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  int32_t TotalSize = 0;
  iovec SysIOVS[kIOVSMax];

  for (uint32_t I = 0; I < static_cast<uint32_t>(IOVSLen); ++I) {
    __wasi_ciovec_t &IOVS = IOVSArray[I];

    /// Check for total size overflow.
    const uint32_t Overflow = std::numeric_limits<int32_t>::max() - TotalSize;
    if (unlikely(IOVS.buf_len > Overflow)) {
      return __WASI_ERRNO_INVAL;
    }
    TotalSize += IOVS.buf_len;

    void *const WriteArr =
        MemInst->getPointer<uint8_t *>(IOVS.buf, IOVS.buf_len);
    /// Check for invalid address.
    if (unlikely(WriteArr == nullptr)) {
      return __WASI_ERRNO_FAULT;
    }
    SysIOVS[I].iov_base = WriteArr;
    SysIOVS[I].iov_len = IOVS.buf_len;
  }

#if __GLIBC_PREREQ(2, 10)
  *NWritten = pwritev(Entry->second.HostFd, SysIOVS, IOVSLen, Offset);
#else
  const off_t ErrOffset = -1;
  const off_t OldOffset = lseek(Entry->second.HostFd, 0, SEEK_CUR);
  if (OldOffset == ErrOffset) {
    *NWritten = -1;
    return convertErrNo(errno);
  }
  if (lseek(Entry->second.HostFd, Offset, SEEK_SET) == ErrOffset) {
    *NWritten = -1;
    return convertErrNo(errno);
  }
  *NWritten = writev(Entry->second.HostFd, SysIOVS, IOVSLen);
  const int SavedErrNo = errno;
  if (lseek(Entry->second.HostFd, OldOffset, SEEK_SET) == ErrOffset) {
    if (*NWritten != -1) {
      *NWritten = -1;
      return convertErrNo(errno);
    }
  }
  errno = SavedErrNo;
#endif

  if (unlikely(*NWritten < 0)) {
    return convertErrNo(errno);
  }

  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiFdRead::body(Runtime::Instance::MemoryInstance *MemInst,
                                  int32_t Fd, uint32_t IOVSPtr, int32_t IOVSLen,
                                  uint32_t NReadPtr) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  const auto Entry = Env.getFile(Fd);
  if (unlikely(Entry == Env.getFileEnd())) {
    return __WASI_ERRNO_BADF;
  }

  if (unlikely(!Entry->second.checkRights(__WASI_RIGHTS_FD_READ))) {
    return __WASI_ERRNO_NOTCAPABLE;
  }

  if (unlikely(IOVSLen < 0 || IOVSLen > kIOVSMax)) {
    return __WASI_ERRNO_INVAL;
  }

  __wasi_iovec_t *const IOVSArray =
      MemInst->getPointer<__wasi_iovec_t *>(IOVSPtr, IOVSLen);
  if (unlikely(IOVSArray == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  int32_t *const NRead = MemInst->getPointer<int32_t *>(NReadPtr);
  if (unlikely(NRead == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  uint32_t TotalSize = 0;
  iovec SysIOVS[kIOVSMax];

  for (uint32_t I = 0; I < static_cast<uint32_t>(IOVSLen); ++I) {
    __wasi_iovec_t &IOVS = IOVSArray[I];

    /// Check for total size overflow.
    const uint32_t Overflow = std::numeric_limits<int32_t>::max() - TotalSize;
    if (unlikely(IOVS.buf_len > Overflow)) {
      return __WASI_ERRNO_INVAL;
    }
    TotalSize += IOVS.buf_len;

    void *const ReadArr =
        MemInst->getPointer<uint8_t *>(IOVS.buf, IOVS.buf_len);
    /// Check for invalid address.
    if (unlikely(ReadArr == nullptr)) {
      return __WASI_ERRNO_FAULT;
    }
    SysIOVS[I].iov_base = ReadArr;
    SysIOVS[I].iov_len = IOVS.buf_len;
  }

  /// Store read bytes length.
  *NRead = readv(Entry->second.HostFd, SysIOVS, IOVSLen);

  if (unlikely(*NRead < 0)) {
    return convertErrNo(errno);
  }

  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiFdReadDir::body(Runtime::Instance::MemoryInstance *MemInst,
                                     int32_t Fd, uint32_t BufPtr,
                                     uint32_t BufLen, uint64_t Cookie,
                                     uint32_t BufUsedSizePtr) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  const auto Entry = Env.getFile(Fd);
  if (unlikely(Entry == Env.getFileEnd())) {
    return __WASI_ERRNO_BADF;
  }

  if (unlikely(!Entry->second.checkRights(__WASI_RIGHTS_FD_READDIR))) {
    return __WASI_ERRNO_NOTCAPABLE;
  }

  if (unlikely(BufLen == 0)) {
    return __WASI_ERRNO_SUCCESS;
  }

  /// Check for invalid address.
  uint8_t *Buf = MemInst->getPointer<uint8_t *>(BufPtr, BufLen);
  if (unlikely(Buf == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  uint32_t *const BufUsedSize = MemInst->getPointer<uint32_t *>(BufUsedSizePtr);
  if (unlikely(BufUsedSize == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  if (unlikely(!Entry->second.Dir)) {
    DIR *D = fdopendir(Entry->second.HostFd);
    if (D == nullptr) {
      return convertErrNo(errno);
    }
    Entry->second.Dir.emplace(D);
  }
  auto &Dir = *Entry->second.Dir;

  if (unlikely(Cookie != Dir.Cookie)) {
    Dir.Buffer.clear();
    seekdir(Dir.Dir, Cookie);
  }

  *BufUsedSize = 0;
  do {
    if (!Dir.Buffer.empty()) {
      const uint32_t NewDataSize =
          std::min<uint32_t>(BufLen, Dir.Buffer.size());
      std::copy(Dir.Buffer.begin(), Dir.Buffer.begin() + NewDataSize, Buf);
      Buf += NewDataSize;
      BufLen -= NewDataSize;
      *BufUsedSize += NewDataSize;
      Dir.Buffer.erase(Dir.Buffer.begin(), Dir.Buffer.begin() + NewDataSize);
      if (unlikely(BufLen == 0)) {
        break;
      }
    }
    errno = 0;
    dirent *SysDirent = readdir(Dir.Dir);
    if (SysDirent == nullptr) {
      if (errno != 0) {
        return convertErrNo(errno);
      }
      return __WASI_ERRNO_SUCCESS;
    }
    Dir.Cookie = SysDirent->d_off;
    std::string_view Name = SysDirent->d_name;

    Dir.Buffer.resize(sizeof(__wasi_dirent_t) + Name.size());

    __wasi_dirent_t *const Dirent =
        reinterpret_cast<__wasi_dirent_t *>(Dir.Buffer.data());
    Dirent->d_next = Dir.Cookie;
    Dirent->d_ino = SysDirent->d_ino;
    Dirent->d_type = direntType2FileType(SysDirent->d_type);
    Dirent->d_namlen = Name.size();
    std::copy(Name.cbegin(), Name.cend(),
              Dir.Buffer.begin() + sizeof(__wasi_dirent_t));
  } while (BufLen > 0);

  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t>
WasiFdRenumber::body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd,
                     int32_t ToFd) {
  const auto Entry = Env.getFile(Fd);
  if (unlikely(Entry == Env.getFileEnd())) {
    return __WASI_ERRNO_BADF;
  }

  /// Refuse to renumber preopened fd.
  if (unlikely(Entry->second.IsPreopened)) {
    return __WASI_ERRNO_NOTSUP;
  }

  /// Close existed fd renumbering into.
  if (const auto OldEntry = Env.getFile(ToFd);
      unlikely(OldEntry != Env.getFileEnd())) {
    if (unlikely(OldEntry->second.IsPreopened)) {
      return __WASI_ERRNO_NOTSUP;
    }
    if (unlikely(close(OldEntry->second.HostFd) != 0)) {
      return convertErrNo(errno);
    }
    Env.eraseFile(OldEntry);
  }

  Env.changeFd(Entry, ToFd);

  return __WASI_ERRNO_SUCCESS;
}

Expect<int32_t> WasiFdSeek::body(Runtime::Instance::MemoryInstance *MemInst,
                                 int32_t Fd, int64_t Offset, uint32_t Whence,
                                 uint32_t NewOffsetPtr) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  const auto Entry = Env.getFile(Fd);
  if (unlikely(Entry == Env.getFileEnd())) {
    return __WASI_ERRNO_BADF;
  }

  if (unlikely(!Entry->second.checkRights(__WASI_RIGHTS_FD_SEEK))) {
    return __WASI_ERRNO_NOTCAPABLE;
  }

  /// Check directive whence.
  int SysWhence;
  switch (Whence) {
  case __WASI_WHENCE_CUR:
    SysWhence = SEEK_CUR;
    break;
  case __WASI_WHENCE_END:
    SysWhence = SEEK_END;
    break;
  case __WASI_WHENCE_SET:
    SysWhence = SEEK_SET;
    break;
  default:
    return __WASI_ERRNO_INVAL;
  }

  /// Check for invalid address.
  __wasi_filedelta_t *NewOffset =
      MemInst->getPointer<__wasi_filedelta_t *>(NewOffsetPtr);
  if (unlikely(NewOffset == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  /// Do lseek.
  *NewOffset = lseek(Entry->second.HostFd, Offset, SysWhence);
  if (unlikely(*NewOffset < 0)) {
    return convertErrNo(errno);
  }

  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiFdSync::body(Runtime::Instance::MemoryInstance *MemInst,
                                  int32_t Fd) {
  const auto Entry = Env.getFile(Fd);
  if (unlikely(Entry == Env.getFileEnd())) {
    return __WASI_ERRNO_BADF;
  }

  if (unlikely(!Entry->second.checkRights(__WASI_RIGHTS_FD_SYNC))) {
    return __WASI_ERRNO_NOTCAPABLE;
  }

  if (unlikely(fsync(Entry->second.HostFd) != 0)) {
    return convertErrNo(errno);
  }

  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiFdTell::body(Runtime::Instance::MemoryInstance *MemInst,
                                  int32_t Fd, int32_t OffsetPtr) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  const auto Entry = Env.getFile(Fd);
  if (unlikely(Entry == Env.getFileEnd())) {
    return __WASI_ERRNO_BADF;
  }

  if (unlikely(!Entry->second.checkRights(__WASI_RIGHTS_FD_TELL))) {
    return __WASI_ERRNO_NOTCAPABLE;
  }

  /// Check for invalid address.
  __wasi_filesize_t *Offset =
      MemInst->getPointer<__wasi_filesize_t *>(OffsetPtr);
  if (unlikely(Offset == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  /// Do lseek.
  *Offset = lseek(Entry->second.HostFd, 0, SEEK_CUR);
  if (unlikely(static_cast<off_t>(*Offset) < 0)) {
    return convertErrNo(errno);
  }

  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiFdWrite::body(Runtime::Instance::MemoryInstance *MemInst,
                                   int32_t Fd, uint32_t IOVSPtr,
                                   int32_t IOVSLen, uint32_t NWrittenPtr) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  const auto Entry = Env.getFile(Fd);
  if (unlikely(Entry == Env.getFileEnd())) {
    return __WASI_ERRNO_BADF;
  }

  if (unlikely(!Entry->second.checkRights(__WASI_RIGHTS_FD_WRITE))) {
    return __WASI_ERRNO_NOTCAPABLE;
  }

  if (unlikely(IOVSLen < 0 || IOVSLen > kIOVSMax)) {
    return __WASI_ERRNO_INVAL;
  }

  /// Check for invalid address.
  __wasi_ciovec_t *const IOVSArray =
      MemInst->getPointer<__wasi_ciovec_t *>(IOVSPtr, IOVSLen);
  if (unlikely(IOVSArray == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  int32_t *const NWritten = MemInst->getPointer<int32_t *>(NWrittenPtr);
  if (unlikely(NWritten == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  int32_t TotalSize = 0;
  iovec SysIOVS[kIOVSMax];

  for (uint32_t I = 0; I < static_cast<uint32_t>(IOVSLen); ++I) {
    __wasi_ciovec_t &IOVS = IOVSArray[I];

    /// Check for total size overflow.
    const uint32_t Overflow = std::numeric_limits<int32_t>::max() - TotalSize;
    if (unlikely(IOVS.buf_len > Overflow)) {
      return __WASI_ERRNO_INVAL;
    }
    TotalSize += IOVS.buf_len;

    void *const WriteArr =
        MemInst->getPointer<uint8_t *>(IOVS.buf, IOVS.buf_len);
    /// Check for invalid address.
    if (unlikely(WriteArr == nullptr)) {
      return __WASI_ERRNO_FAULT;
    }
    SysIOVS[I].iov_base = WriteArr;
    SysIOVS[I].iov_len = IOVS.buf_len;
  }

  *NWritten = writev(Entry->second.HostFd, SysIOVS, IOVSLen);

  if (unlikely(*NWritten < 0)) {
    return convertErrNo(errno);
  }

  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t>
WasiPathCreateDirectory::body(Runtime::Instance::MemoryInstance *MemInst,
                              int32_t Fd, uint32_t PathPtr, uint32_t PathLen) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  const auto Entry = Env.getFile(Fd);
  if (unlikely(Entry == Env.getFileEnd())) {
    return __WASI_ERRNO_BADF;
  }

  if (unlikely(
          !Entry->second.checkRights(__WASI_RIGHTS_PATH_CREATE_DIRECTORY))) {
    return __WASI_ERRNO_NOTCAPABLE;
  }

  char *const Path = MemInst->getPointer<char *>(PathPtr, PathLen);
  if (unlikely(Path == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }
  std::string PathStr(Path, PathLen);

  if (unlikely(mkdirat(Entry->second.HostFd, PathStr.c_str(), 0755) != 0)) {
    return convertErrNo(errno);
  }

  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t>
WasiPathFilestatGet::body(Runtime::Instance::MemoryInstance *MemInst,
                          int32_t Fd, uint32_t Flags, uint32_t PathPtr,
                          uint32_t PathLen, uint32_t FilestatPtr) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  const auto Entry = Env.getFile(Fd);
  if (unlikely(Entry == Env.getFileEnd())) {
    return __WASI_ERRNO_BADF;
  }

  if (unlikely(!Entry->second.checkRights(__WASI_RIGHTS_PATH_FILESTAT_GET))) {
    return __WASI_ERRNO_NOTCAPABLE;
  }

  __wasi_filestat_t *const Filestat =
      MemInst->getPointer<__wasi_filestat_t *>(FilestatPtr);
  if (unlikely(Filestat == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  char *const Path = MemInst->getPointer<char *>(PathPtr, PathLen);
  if (unlikely(Path == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }
  std::string PathStr(Path, PathLen);

  struct stat SysFStat;

  /// TODO: restrict PathStr on escaping root directory
  int Result;
  if (Flags & __WASI_LOOKUPFLAGS_SYMLINK_FOLLOW) {
    Result = fstatat(Entry->second.HostFd, PathStr.c_str(), &SysFStat, 0);
  } else {
    Result = fstatat(Entry->second.HostFd, PathStr.c_str(), &SysFStat,
                     AT_SYMLINK_NOFOLLOW);
  }
  if (unlikely(Result != 0)) {
    return convertErrNo(errno);
  }

  Filestat->dev = SysFStat.st_dev;
  Filestat->ino = SysFStat.st_ino;
  Filestat->filetype = statMode2FileType(SysFStat.st_mode);
  Filestat->nlink = SysFStat.st_nlink;
  Filestat->size = SysFStat.st_size;
#ifndef __APPLE__
  Filestat->atim = timespec2Timestamp(SysFStat.st_atim);
  Filestat->mtim = timespec2Timestamp(SysFStat.st_mtim);
  Filestat->ctim = timespec2Timestamp(SysFStat.st_ctim);
#endif

  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t>
WasiPathFilestatSetTimes::body(Runtime::Instance::MemoryInstance *MemInst,
                               int32_t Fd, uint32_t Flags, uint32_t PathPtr,
                               uint32_t PathLen, uint32_t ATim, uint32_t MTim,
                               uint32_t FstFlags) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  const auto Entry = Env.getFile(Fd);
  if (unlikely(Entry == Env.getFileEnd())) {
    return __WASI_ERRNO_BADF;
  }

  if (unlikely(
          !Entry->second.checkRights(__WASI_RIGHTS_PATH_FILESTAT_SET_TIMES))) {
    return __WASI_ERRNO_NOTCAPABLE;
  }

  /// Get file path.
  char *const Path = MemInst->getPointer<char *>(PathPtr, PathLen);
  if (unlikely(Path == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }
  std::string PathStr(Path, PathLen);

#if __GLIBC_PREREQ(2, 6)
  int SysFlags = 0;
  if ((Flags & __WASI_LOOKUPFLAGS_SYMLINK_FOLLOW) == 0) {
    SysFlags |= AT_SYMLINK_FOLLOW;
  }

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

  if (unlikely(utimensat(Entry->second.HostFd, PathStr.c_str(), SysTimespec,
                         SysFlags) != 0)) {
    return convertErrNo(errno);
  }
#else
  int SysFlags = 0;
  if ((Flags & __WASI_LOOKUPFLAGS_SYMLINK_FOLLOW) == 0) {
    SysFlags |= O_NOFOLLOW;
  }
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

  int SysFd = openat(Entry->second.HostFd, PathStr.c_str(), Flags);
  if (SysFd < 0) {
    return convertErrNo(errno);
  }
  BOOST_SCOPE_EXIT_ALL(&) { close(SysFd); };
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

Expect<uint32_t> WasiPathLink::body(Runtime::Instance::MemoryInstance *MemInst,
                                    int32_t OldFd, uint32_t OldFlags,
                                    uint32_t OldPathPtr, uint32_t OldPathLen,
                                    int32_t NewFd, uint32_t NewPathPtr,
                                    uint32_t NewPathLen) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  const auto OldEntry = Env.getFile(OldFd);
  if (unlikely(OldEntry == Env.getFileEnd())) {
    return __WASI_ERRNO_BADF;
  }

  const auto NewEntry = OldFd == NewFd ? OldEntry : Env.getFile(NewFd);
  if (unlikely(NewEntry == Env.getFileEnd())) {
    return __WASI_ERRNO_BADF;
  }

  if (unlikely(!OldEntry->second.checkRights(__WASI_RIGHTS_PATH_LINK_SOURCE))) {
    return __WASI_ERRNO_NOTCAPABLE;
  }

  if (unlikely(!NewEntry->second.checkRights(__WASI_RIGHTS_PATH_LINK_TARGET))) {
    return __WASI_ERRNO_NOTCAPABLE;
  }

  const char *const OldPath =
      MemInst->getPointer<const char *>(OldPathPtr, OldPathLen);
  if (unlikely(OldPath == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }
  const char *const NewPath =
      MemInst->getPointer<const char *>(NewPathPtr, NewPathLen);
  if (unlikely(NewPath == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }
  std::string OldPathStr(OldPath, OldPathLen);
  std::string NewPathStr(NewPath, NewPathLen);

  int SysFlags = 0;
  if (OldFlags & __WASI_LOOKUPFLAGS_SYMLINK_FOLLOW) {
    SysFlags |= AT_SYMLINK_FOLLOW;
  }

  if (unlikely(linkat(OldEntry->second.HostFd, OldPathStr.c_str(),
                      NewEntry->second.HostFd, NewPathStr.c_str(),
                      SysFlags) != 0)) {
    return convertErrNo(errno);
  }

  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiPathOpen::body(Runtime::Instance::MemoryInstance *MemInst,
                                    int32_t DirFd, uint32_t DirFlags,
                                    uint32_t PathPtr, uint32_t PathLen,
                                    uint32_t OFlags, uint64_t FsRightsBaseV,
                                    uint64_t FsRightsInheritingV,
                                    uint32_t FsFlags, uint32_t FdPtr) {
  const __wasi_rights_t FsRightsBase =
      static_cast<__wasi_rights_t>(FsRightsBaseV);
  const __wasi_rights_t FsRightsInheriting =
      static_cast<__wasi_rights_t>(FsRightsInheritingV);

  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  const auto Entry = Env.getFile(DirFd);
  if (unlikely(Entry == Env.getFileEnd())) {
    return __WASI_ERRNO_BADF;
  }

  __wasi_fd_t *const Fd = MemInst->getPointer<__wasi_fd_t *>(FdPtr);
  if (unlikely(Fd == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  /// Get file path.
  char *const Path = MemInst->getPointer<char *>(PathPtr, PathLen);
  if (unlikely(Path == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }
  std::string PathStr(Path, PathLen);

  __wasi_rights_t RequiredRights = __WASI_RIGHTS_PATH_OPEN;
  __wasi_rights_t RequiredInheritingRights = FsRightsBase | FsRightsInheriting;
  const bool Read =
      (FsRightsBase & (__WASI_RIGHTS_FD_READ | __WASI_RIGHTS_FD_READDIR)) != 0;
  const bool Write =
      (FsRightsBase &
       (__WASI_RIGHTS_FD_DATASYNC | __WASI_RIGHTS_FD_WRITE |
        __WASI_RIGHTS_FD_ALLOCATE | __WASI_RIGHTS_FD_FILESTAT_SET_SIZE)) != 0;

  int Flags = Write ? (Read ? O_RDWR : O_WRONLY) : O_RDONLY;
  if (OFlags & __WASI_OFLAGS_CREAT) {
    RequiredRights |= __WASI_RIGHTS_PATH_CREATE_FILE;
    Flags |= O_CREAT;
  }
  if (OFlags & __WASI_OFLAGS_DIRECTORY) {
    Flags |= O_DIRECTORY;
  }
  if (OFlags & __WASI_OFLAGS_EXCL) {
    Flags |= O_EXCL;
  }
  if (OFlags & __WASI_OFLAGS_TRUNC) {
    RequiredRights |= __WASI_RIGHTS_PATH_FILESTAT_SET_SIZE;
    Flags |= O_TRUNC;
  }

  // Convert file descriptor flags.
  if ((FsFlags & __WASI_FDFLAGS_APPEND) != 0)
    Flags |= O_APPEND;
  if ((FsFlags & __WASI_FDFLAGS_DSYNC) != 0) {
#ifdef O_DSYNC
    Flags |= O_DSYNC;
#else
    Flags |= O_SYNC;
#endif
  }
  if ((FsFlags & __WASI_FDFLAGS_NONBLOCK) != 0)
    Flags |= O_NONBLOCK;
  if ((FsFlags & __WASI_FDFLAGS_RSYNC) != 0) {
#ifdef O_RSYNC
    Flags |= O_RSYNC;
#else
    Flags |= O_SYNC;
#endif
  }
  if ((FsFlags & __WASI_FDFLAGS_SYNC) != 0) {
    Flags |= O_SYNC;
  }

  if ((DirFlags & __WASI_LOOKUPFLAGS_SYMLINK_FOLLOW) == 0) {
    Flags |= O_NOFOLLOW;
  }

  if (unlikely(!Entry->second.checkRights(RequiredRights,
                                          RequiredInheritingRights))) {
    return __WASI_ERRNO_NOTCAPABLE;
  }

  /// Open file and store Fd.
  int HostFd = openat(Entry->second.HostFd, PathStr.c_str(), Flags, 0644);
  if (unlikely(HostFd < 0)) {
    *Fd = -1;
    return convertErrNo(errno);
  }

  std::string NewPathStr =
      (std::filesystem::u8path(Entry->second.Path) / PathStr)
          .lexically_normal()
          .u8string();

  *Fd = Env.getNewFd();
  Env.emplaceFile(*Fd, HostFd, false, FsRightsBase, FsRightsInheriting,
                  NewPathStr);

  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t>
WasiPathReadLink::body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd,
                       uint32_t PathPtr, uint32_t PathLen, uint32_t BufPtr,
                       uint32_t BufLen) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  const auto Entry = Env.getFile(Fd);
  if (unlikely(Entry == Env.getFileEnd())) {
    return __WASI_ERRNO_BADF;
  }

  if (unlikely(!Entry->second.checkRights(__WASI_RIGHTS_PATH_READLINK))) {
    return __WASI_ERRNO_NOTCAPABLE;
  }

  char *const Path = MemInst->getPointer<char *>(PathPtr, PathLen);
  if (unlikely(Path == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }
  std::string PathStr(Path, PathLen);

  char *const Buf = MemInst->getPointer<char *>(BufPtr, BufLen);
  if (unlikely(Buf == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  if (unlikely(readlinkat(Entry->second.HostFd, PathStr.c_str(), Buf, BufLen) <
               0)) {
    return convertErrNo(errno);
  }

  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t>
WasiPathRemoveDirectory::body(Runtime::Instance::MemoryInstance *MemInst,
                              int32_t Fd, uint32_t PathPtr, uint32_t PathLen) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  const auto Entry = Env.getFile(Fd);
  if (unlikely(Entry == Env.getFileEnd())) {
    return __WASI_ERRNO_BADF;
  }

  if (unlikely(
          !Entry->second.checkRights(__WASI_RIGHTS_PATH_REMOVE_DIRECTORY))) {
    return __WASI_ERRNO_NOTCAPABLE;
  }

  char *const Path = MemInst->getPointer<char *>(PathPtr, PathLen);
  if (unlikely(Path == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }
  std::string PathStr(Path, PathLen);

  if (unlinkat(Entry->second.HostFd, PathStr.c_str(), AT_REMOVEDIR) < 0) {
    return convertErrNo(errno);
  }

  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t>
WasiPathRename::body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd,
                     uint32_t OldPathPtr, uint32_t OldPathLen, int32_t NewFd,
                     uint32_t NewPathPtr, uint32_t NewPathLen) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  const auto Entry = Env.getFile(Fd);
  if (unlikely(Entry == Env.getFileEnd())) {
    return __WASI_ERRNO_BADF;
  }

  if (unlikely(!Entry->second.checkRights(__WASI_RIGHTS_PATH_RENAME_SOURCE))) {
    return __WASI_ERRNO_NOTCAPABLE;
  }

  const auto NewEntry = NewFd == Fd ? Entry : Env.getFile(NewFd);
  if (NewFd != Fd) {
    if (unlikely(NewEntry == Env.getFileEnd())) {
      return __WASI_ERRNO_BADF;
    }

    if (unlikely(
            !NewEntry->second.checkRights(__WASI_RIGHTS_PATH_RENAME_TARGET))) {
      return __WASI_ERRNO_NOTCAPABLE;
    }
  }

  char *const OldPath = MemInst->getPointer<char *>(OldPathPtr, OldPathLen);
  if (unlikely(OldPath == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }
  char *const NewPath = MemInst->getPointer<char *>(NewPathPtr, NewPathLen);
  if (unlikely(NewPath == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }
  std::string OldPathStr(OldPath, OldPathLen);
  std::string NewPathStr(NewPath, NewPathLen);

  if (unlikely(renameat(Entry->second.HostFd, OldPathStr.c_str(),
                        NewEntry->second.HostFd, NewPathStr.c_str()) != 0)) {
    return convertErrNo(errno);
  }

  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t>
WasiPathSymlink::body(Runtime::Instance::MemoryInstance *MemInst,
                      uint32_t OldPathPtr, uint32_t OldPathLen, int32_t Fd,
                      uint32_t NewPathPtr, uint32_t NewPathLen) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  const auto Entry = Env.getFile(Fd);
  if (unlikely(Entry == Env.getFileEnd())) {
    return __WASI_ERRNO_BADF;
  }

  if (unlikely(!Entry->second.checkRights(__WASI_RIGHTS_PATH_SYMLINK))) {
    return __WASI_ERRNO_NOTCAPABLE;
  }

  char *const OldPath = MemInst->getPointer<char *>(OldPathPtr, OldPathLen);
  if (unlikely(OldPath == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }
  char *const NewPath = MemInst->getPointer<char *>(NewPathPtr, NewPathLen);
  if (unlikely(NewPath == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }
  std::string OldPathStr(OldPath, OldPathLen);
  std::string NewPathStr(NewPath, NewPathLen);

  if (unlikely(symlinkat(OldPathStr.c_str(), Entry->second.HostFd,
                         NewPathStr.c_str()) != 0)) {
    return convertErrNo(errno);
  }

  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t>
WasiPathUnlinkFile::body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd,
                         uint32_t PathPtr, uint32_t PathLen) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  const auto Entry = Env.getFile(Fd);
  if (unlikely(Entry == Env.getFileEnd())) {
    return __WASI_ERRNO_BADF;
  }

  if (unlikely(!Entry->second.checkRights(__WASI_RIGHTS_PATH_UNLINK_FILE))) {
    return __WASI_ERRNO_NOTCAPABLE;
  }

  char *const Path = MemInst->getPointer<char *>(PathPtr, PathLen);
  if (unlikely(Path == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }
  std::string PathStr(Path, PathLen);

  if (unlinkat(Entry->second.HostFd, PathStr.c_str(), 0) < 0) {
    return convertErrNo(errno);
  }

  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t>
WasiPollOneoff::body(Runtime::Instance::MemoryInstance *MemInst, uint32_t InPtr,
                     uint32_t OutPtr, uint32_t NSubscriptions,
                     uint32_t NEventsPtr) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  struct EventPoll {
    struct Timer {
      int TimerFd = -1;
      Timer() noexcept = default;
      ~Timer() noexcept {
        if (likely(TimerFd != -1)) {
          close(TimerFd);
        }
      }
      Timer(const Timer &) noexcept = delete;
      Timer(Timer &&) noexcept = default;
#if __GLIBC_PREREQ(2, 8)
      bool create(clockid_t ClockId, __wasi_subclockflags_t Flags,
                  __wasi_timestamp_t Timeout) noexcept {
        TimerFd = timerfd_create(ClockId, TFD_NONBLOCK | TFD_CLOEXEC);
        if (unlikely(TimerFd < 0)) {
          return false;
        }

        int SysFlags = 0;
        if (Flags & __WASI_SUBCLOCKFLAGS_SUBSCRIPTION_CLOCK_ABSTIME) {
          SysFlags |= TFD_TIMER_ABSTIME;
        }
        itimerspec ITimerSpec{timestamp2Timespec(0),
                              timestamp2Timespec(Timeout)};
        if (unlikely(timerfd_settime(TimerFd, SysFlags, &ITimerSpec, nullptr) <
                     0)) {
          return false;
        }

        return true;
      }
#else
      bool create(clockid_t ClockId, __wasi_subclockflags_t Flags,
                  __wasi_timestamp_t Timeout) noexcept {
        int PipeFd[2] = {-1, -1};

        if (unlikely(pipe(PipeFd) != 0)) {
          return false;
        }

        timer_t TId;
        {
          sigevent Event;
          Event.sigev_notify = SIGEV_THREAD;
          Event.sigev_notify_function = &sigevCallback;
          Event.sigev_value.sival_int = PipeFd[1];
          Event.sigev_notify_attributes = nullptr;

          if (unlikely(fcntl(PipeFd[0], F_SETFD, FD_CLOEXEC) != 0 ||
                       fcntl(PipeFd[1], F_SETFD, FD_CLOEXEC) != 0 ||
                       timer_create(ClockId, &Event, &TId) < 0)) {
            close(PipeFd[0]);
            close(PipeFd[1]);
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

          if (unlikely(timer_settime(TId, SysFlags, &ITimerSpec, nullptr) <
                       0)) {
            timer_delete(TId);
            close(PipeFd[0]);
            close(PipeFd[1]);
            return false;
          }
        }

        TimerFd = PipeFd[0];
        NotifyFd = PipeFd[1];
        TimerId = TId;
        return true;
      }
      static void sigevCallback(union sigval Value) noexcept {
        const uint64_t One = 1;
        write(Value.sival_int, &One, sizeof(One));
      }
      int NotifyFd = -1;
      timer_t TimerId;
#endif
    };
    EventPoll(uint32_t MaxEvent) : Events(MaxEvent) {}
    ~EventPoll() noexcept = default;
    bool create() noexcept {
      if (EPollFd >= 0) {
        return true;
      }
#if __GLIBC_PREREQ(2, 9)
      EPollFd = epoll_create1(EPOLL_CLOEXEC);
      return likely(EPollFd >= 0);
#else
      EPollFd = epoll_create(256);
      if (unlikely(EPollFd < 0)) {
        return false;
      }

      if (unlikely(fcntl(EPollFd, F_SETFD, FD_CLOEXEC) != 0)) {
        close(EPollFd);
        EPollFd = -1;
        return false;
      }

      return true;
#endif
    }
    bool AddFd(int Fd, const __wasi_subscription_t *Pointer,
               bool IsRead) noexcept {
      epoll_event Event;
      Event.events = (IsRead ? EPOLLIN : EPOLLOUT);
#if defined(EPOLLRDHUP)
      Event.events |= EPOLLRDHUP;
#endif
      Event.data.ptr = const_cast<__wasi_subscription_t *>(Pointer);
      if (unlikely(epoll_ctl(EPollFd, EPOLL_CTL_ADD, Fd, &Event) < 0)) {
        return false;
      }
      return true;
    }
    bool AddTimer(clockid_t ClockId,
                  const __wasi_subscription_t &Subscription) noexcept {
      Timers.emplace_back();
      if (unlikely(!Timers.back().create(ClockId, Subscription.u.u.clock.flags,
                                         Subscription.u.u.clock.timeout))) {
        return false;
      }
      const int TimerFd = Timers.back().TimerFd;

      epoll_event Event;
      Event.events = EPOLLIN;
      Event.data.ptr = const_cast<__wasi_subscription_t *>(&Subscription);
      if (unlikely(epoll_ctl(EPollFd, EPOLL_CTL_ADD, TimerFd, &Event) < 0)) {
        return false;
      }
      return true;
    }
    bool Wait() noexcept {
      sigset_t Mask;
      sigfillset(&Mask);
      sigdelset(&Mask, SIGINT);
      sigdelset(&Mask, SIGTERM);
      sigdelset(&Mask, SIGRTMIN);
#if __GLIBC_PREREQ(2, 6)
      const int Count =
          epoll_pwait(EPollFd, Events.data(), Events.size(), -1, &Mask);
#else
      sigset_t OrigMask;
      pthread_sigmask(SIG_SETMASK, &Mask, &OrigMask);
      const int Count = epoll_wait(EPollFd, Events.data(), Events.size(), -1);
      pthread_sigmask(SIG_SETMASK, &Mask, nullptr);
#endif
      if (unlikely(Count < 0)) {
        return false;
      }
      Events.resize(Count);
      return true;
    }

    int EPollFd = -1;
    std::vector<Timer> Timers;
    std::vector<epoll_event> Events;
  };

  EventPoll Event(NSubscriptions);
  if (unlikely(!Event.create())) {
    return convertErrNo(errno);
  }

  const __wasi_subscription_t *const SubscriptionArray =
      MemInst->getPointer<__wasi_subscription_t *>(InPtr, NSubscriptions);
  if (unlikely(SubscriptionArray == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  __wasi_event_t *const EventArray =
      MemInst->getPointer<__wasi_event_t *>(NEventsPtr, NSubscriptions);
  if (unlikely(EventArray == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  uint32_t *const NEvents = MemInst->getPointer<uint32_t *>(NEventsPtr);
  if (unlikely(NEvents == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  *NEvents = 0;

  auto RecordClockError = [&EventArray,
                           &NEvents](const __wasi_subscription_t &Subscription,
                                     __wasi_errno_t error) {
    EventArray[*NEvents].userdata = Subscription.userdata;
    EventArray[*NEvents].error = error;
    EventArray[*NEvents].type = Subscription.u.tag;
    ++*NEvents;
  };

  auto RecordFdError = [&EventArray,
                        &NEvents](const __wasi_subscription_t &Subscription,
                                  __wasi_errno_t error) {
    EventArray[*NEvents].userdata = Subscription.userdata;
    EventArray[*NEvents].error = error;
    EventArray[*NEvents].type = Subscription.u.tag;
    EventArray[*NEvents].fd_readwrite.nbytes = 0;
    EventArray[*NEvents].fd_readwrite.flags =
        static_cast<__wasi_eventrwflags_t>(0);
    ++*NEvents;
  };

  auto RecordClock = [&EventArray,
                      &NEvents](const __wasi_subscription_t &Subscription) {
    EventArray[*NEvents].userdata = Subscription.userdata;
    EventArray[*NEvents].error = __WASI_ERRNO_SUCCESS;
    EventArray[*NEvents].type = Subscription.u.tag;
    ++*NEvents;
  };

  auto RecordFd = [&EventArray, &NEvents](
                      const __wasi_subscription_t &Subscription,
                      __wasi_filesize_t nbytes, __wasi_eventrwflags_t flags) {
    EventArray[*NEvents].userdata = Subscription.userdata;
    EventArray[*NEvents].error = __WASI_ERRNO_SUCCESS;
    EventArray[*NEvents].type = Subscription.u.tag;
    EventArray[*NEvents].fd_readwrite.nbytes = nbytes;
    EventArray[*NEvents].fd_readwrite.flags = flags;
    ++*NEvents;
  };

  /// Validate types
  for (uint32_t I = 0; I < NSubscriptions; ++I) {
    const __wasi_subscription_t &Subscription = SubscriptionArray[I];
    switch (Subscription.u.tag) {
    case __WASI_EVENTTYPE_CLOCK:
    case __WASI_EVENTTYPE_FD_READ:
    case __WASI_EVENTTYPE_FD_WRITE:
      break;
    default:
      return __WASI_ERRNO_INVAL;
    }
  }

  for (uint32_t I = 0; I < NSubscriptions; ++I) {
    const __wasi_subscription_t &Subscription = SubscriptionArray[I];
    switch (Subscription.u.tag) {
    case __WASI_EVENTTYPE_CLOCK: {
      clockid_t SysClockId;
      switch (Subscription.u.u.clock.id) {
      case __WASI_CLOCKID_REALTIME:
        SysClockId = CLOCK_REALTIME;
        break;
      case __WASI_CLOCKID_MONOTONIC:
        SysClockId = CLOCK_MONOTONIC;
        break;
      case __WASI_CLOCKID_PROCESS_CPUTIME_ID:
        SysClockId = CLOCK_PROCESS_CPUTIME_ID;
        break;
      case __WASI_CLOCKID_THREAD_CPUTIME_ID:
        SysClockId = CLOCK_THREAD_CPUTIME_ID;
        break;
      default:
        RecordClockError(Subscription, __WASI_ERRNO_INVAL);
        continue;
      }

      if (unlikely(!Event.AddTimer(SysClockId, Subscription))) {
        RecordClockError(Subscription, convertErrNo(errno));
        continue;
      }
      continue;
    }
    case __WASI_EVENTTYPE_FD_READ: {
      const int Fd = Subscription.u.u.fd_read.file_descriptor;
      const auto Entry = Env.getFile(Fd);
      if (unlikely(Entry == Env.getFileEnd())) {
        RecordFdError(Subscription, __WASI_ERRNO_BADF);
        continue;
      }
      if (unlikely(!Entry->second.checkRights(__WASI_RIGHTS_POLL_FD_READWRITE |
                                              __WASI_RIGHTS_FD_READ))) {
        RecordFdError(Subscription, __WASI_ERRNO_NOTCAPABLE);
        continue;
      }
      if (unlikely(!Event.AddFd(Fd, &Subscription, true))) {
        RecordFdError(Subscription, convertErrNo(errno));
        continue;
      }
      continue;
    }
    case __WASI_EVENTTYPE_FD_WRITE: {
      const int Fd = Subscription.u.u.fd_write.file_descriptor;
      const auto Entry = Env.getFile(Fd);
      if (unlikely(Entry == Env.getFileEnd())) {
        RecordFdError(Subscription, __WASI_ERRNO_BADF);
        continue;
      }
      if (unlikely(!Entry->second.checkRights(__WASI_RIGHTS_POLL_FD_READWRITE |
                                              __WASI_RIGHTS_FD_WRITE))) {
        RecordFdError(Subscription, __WASI_ERRNO_NOTCAPABLE);
        continue;
      }
      if (unlikely(!Event.AddFd(Fd, &Subscription, false))) {
        RecordFdError(Subscription, convertErrNo(errno));
        continue;
      }
      continue;
    }
    default:
      __builtin_unreachable();
    }
  }

  if (unlikely(!Event.Wait())) {
    return convertErrNo(errno);
  }

  for (const epoll_event &SysEvent : Event.Events) {
    const __wasi_subscription_t &Subscription =
        *static_cast<__wasi_subscription_t *>(SysEvent.data.ptr);

    if (Subscription.u.tag == __WASI_EVENTTYPE_CLOCK) {
      RecordClock(Subscription);
    } else {
      __wasi_eventrwflags_t flags = static_cast<__wasi_eventrwflags_t>(0);
      if (SysEvent.events & EPOLLHUP) {
        flags |= __WASI_EVENTRWFLAGS_FD_READWRITE_HANGUP;
      }

      int Fd = Subscription.u.tag == __WASI_EVENTTYPE_FD_READ
                   ? Subscription.u.u.fd_read.file_descriptor
                   : Subscription.u.u.fd_write.file_descriptor;
      int NBytes = 0;
      ioctl(Fd, FIONREAD, &NBytes);

      RecordFd(Subscription, NBytes, flags);
    }
  }

  return __WASI_ERRNO_SUCCESS;
}

Expect<void> WasiProcExit::body(Runtime::Instance::MemoryInstance *MemInst,
                                int32_t ExitCode) {
  Env.setExitCode(ExitCode);
  return Unexpect(ErrCode::Terminated);
}

Expect<uint32_t> WasiProcRaise::body(Runtime::Instance::MemoryInstance *MemInst,
                                     int32_t Signal) {
  const int SysSignal = signal2SysSignal(Signal);

  if (unlikely(raise(SysSignal) != 0)) {
    return convertErrNo(errno);
  }
  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiRandomGet::body(Runtime::Instance::MemoryInstance *MemInst,
                                     uint32_t BufPtr, uint32_t BufLen) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  uint8_t *const Buf = MemInst->getPointer<uint8_t *>(BufPtr, BufLen);
  if (unlikely(Buf == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }
  Span<uint8_t> BufSpan(Buf, BufLen);

  /// Use uniform distribution to generate random bytes array
  std::random_device RandomDevice;
  std::mt19937 Generator(RandomDevice());
  std::uniform_int_distribution<> Distribution(
      std::numeric_limits<uint8_t>::min(), std::numeric_limits<uint8_t>::max());
  std::generate(BufSpan.begin(), BufSpan.end(), [&Generator, &Distribution] {
    return Distribution(Generator);
  });

  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t>
WasiSchedYield::body(Runtime::Instance::MemoryInstance *MemInst) {
  if (unlikely(sched_yield() != 0)) {
    return convertErrNo(errno);
  }

  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiSockRecv::body(Runtime::Instance::MemoryInstance *MemInst,
                                    int32_t Fd, uint32_t RiDataPtr,
                                    int32_t RiDataLen, uint32_t RiFlags,
                                    uint32_t RoDataLenPtr,
                                    uint32_t RoFlagsPtr) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  int SysFlags = 0;
  if (RiFlags & __WASI_RIFLAGS_RECV_PEEK) {
    RiFlags &= ~__WASI_RIFLAGS_RECV_PEEK;
    SysFlags |= MSG_PEEK;
  }
  if (RiFlags & __WASI_RIFLAGS_RECV_WAITALL) {
    RiFlags &= ~__WASI_RIFLAGS_RECV_WAITALL;
    SysFlags |= MSG_WAITALL;
  }
  if (unlikely(RiFlags != 0)) {
    return __WASI_ERRNO_INVAL;
  }

  const auto Entry = Env.getFile(Fd);
  if (unlikely(Entry == Env.getFileEnd())) {
    return __WASI_ERRNO_BADF;
  }

  if (unlikely(!Entry->second.checkRights(__WASI_RIGHTS_FD_READ))) {
    return __WASI_ERRNO_NOTCAPABLE;
  }

  if (unlikely(RiDataLen < 0 || RiDataLen > kIOVSMax)) {
    return __WASI_ERRNO_INVAL;
  }

  /// Check for invalid address.
  __wasi_iovec_t *const RiDataArray =
      MemInst->getPointer<__wasi_iovec_t *>(RiDataPtr, RiDataLen);
  if (unlikely(RiDataArray == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  int32_t *const RoDataLen = MemInst->getPointer<int32_t *>(RoDataLenPtr);
  if (unlikely(RoDataLen == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  __wasi_roflags_t *const RoFlags =
      MemInst->getPointer<__wasi_roflags_t *>(RoFlagsPtr);
  if (unlikely(RoFlags == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  uint32_t TotalSize = 0;
  iovec SysRiData[kIOVSMax];

  for (uint32_t I = 0; I < static_cast<uint32_t>(RiDataLen); ++I) {
    __wasi_iovec_t &RiData = RiDataArray[I];

    /// Check for total size overflow.
    const uint32_t Overflow = std::numeric_limits<int32_t>::max() - TotalSize;
    if (unlikely(RiData.buf_len > Overflow)) {
      return __WASI_ERRNO_INVAL;
    }
    TotalSize += RiData.buf_len;

    void *const RiDataArr =
        MemInst->getPointer<uint8_t *>(RiData.buf, RiData.buf_len);
    /// Check for invalid address.
    if (unlikely(RiDataArr == nullptr)) {
      return __WASI_ERRNO_FAULT;
    }
    SysRiData[I].iov_base = RiDataArr;
    SysRiData[I].iov_len = RiData.buf_len;
  }

  msghdr SysMsgHdr;
  SysMsgHdr.msg_name = nullptr;
  SysMsgHdr.msg_namelen = 0;
  SysMsgHdr.msg_iov = SysRiData;
  SysMsgHdr.msg_iovlen = RiDataLen;
  SysMsgHdr.msg_control = nullptr;
  SysMsgHdr.msg_controllen = 0;
  SysMsgHdr.msg_flags = 0;

  /// Store recv bytes length and flags.
  *RoDataLen = recvmsg(Entry->second.HostFd, &SysMsgHdr, SysFlags);
  *RoFlags = static_cast<__wasi_roflags_t>(0);
  if (SysMsgHdr.msg_flags & MSG_TRUNC) {
    *RoFlags |= __WASI_ROFLAGS_RECV_DATA_TRUNCATED;
  }

  if (unlikely(*RoDataLen < 0)) {
    return convertErrNo(errno);
  }

  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiSockSend::body(Runtime::Instance::MemoryInstance *MemInst,
                                    int32_t Fd, uint32_t SiDataPtr,
                                    int32_t SiDataLen, uint32_t SiFlags,
                                    uint32_t SoDataLenPtr) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  if (unlikely(SiFlags != 0)) {
    return __WASI_ERRNO_INVAL;
  }

  const auto Entry = Env.getFile(Fd);
  if (unlikely(Entry == Env.getFileEnd())) {
    return __WASI_ERRNO_BADF;
  }

  if (unlikely(!Entry->second.checkRights(__WASI_RIGHTS_FD_WRITE))) {
    return __WASI_ERRNO_NOTCAPABLE;
  }

  /// Check for invalid address.
  __wasi_ciovec_t *const SiDataArray =
      MemInst->getPointer<__wasi_ciovec_t *>(SiDataPtr, SiDataLen);
  if (unlikely(SiDataArray == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  int32_t *const SoDataLen = MemInst->getPointer<int32_t *>(SoDataLenPtr);
  if (unlikely(SoDataLen == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  uint32_t TotalSize = 0;
  iovec SysSiData[kIOVSMax];

  for (uint32_t I = 0; I < static_cast<uint32_t>(SiDataLen); ++I) {
    __wasi_ciovec_t &SiData = SiDataArray[I];

    /// Check for total size overflow.
    const uint32_t Overflow = std::numeric_limits<int32_t>::max() - TotalSize;
    if (unlikely(SiData.buf_len > Overflow)) {
      return __WASI_ERRNO_INVAL;
    }
    TotalSize += SiData.buf_len;

    void *const SiDataArr =
        MemInst->getPointer<uint8_t *>(SiData.buf, SiData.buf_len);
    /// Check for invalid address.
    if (unlikely(SiDataArr == nullptr)) {
      return __WASI_ERRNO_FAULT;
    }
    SysSiData[I].iov_base = SiDataArr;
    SysSiData[I].iov_len = SiData.buf_len;
  }

  msghdr SysMsgHdr;
  SysMsgHdr.msg_name = nullptr;
  SysMsgHdr.msg_namelen = 0;
  SysMsgHdr.msg_iov = SysSiData;
  SysMsgHdr.msg_iovlen = SiDataLen;
  SysMsgHdr.msg_control = nullptr;
  SysMsgHdr.msg_controllen = 0;

  /// Store send bytes length and flags.
  *SoDataLen = sendmsg(Entry->second.HostFd, &SysMsgHdr, 0);

  if (unlikely(*SoDataLen < 0)) {
    return convertErrNo(errno);
  }

  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t>
WasiSockShutdown::body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd,
                       uint32_t SdFlags) {
  int SysFlags = 0;
  switch (SdFlags) {
  case __WASI_SDFLAGS_RD:
    SysFlags = SHUT_RD;
    break;
  case __WASI_SDFLAGS_WR:
    SysFlags = SHUT_WR;
    break;
  case __WASI_SDFLAGS_RD | __WASI_SDFLAGS_WR:
    SysFlags = SHUT_RDWR;
    break;
  default:
    return __WASI_ERRNO_INVAL;
  }

  const auto Entry = Env.getFile(Fd);
  if (unlikely(Entry == Env.getFileEnd())) {
    return __WASI_ERRNO_BADF;
  }

  if (unlikely(!Entry->second.checkRights(__WASI_RIGHTS_SOCK_SHUTDOWN))) {
    return __WASI_ERRNO_NOTCAPABLE;
  }

  if (unlikely(shutdown(Entry->second.HostFd, SysFlags) < 0)) {
    return convertErrNo(errno);
  }

  return __WASI_ERRNO_SUCCESS;
}

} // namespace Host
} // namespace SSVM
