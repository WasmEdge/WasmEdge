// SPDX-License-Identifier: Apache-2.0
#include "common/defines.h"
#if !WASMEDGE_OS_LINUX
#error
#endif

// Uncomment these flag to test CentOS 6
// #undef __GLIBC_MINOR__
// #define __GLIBC_MINOR__ 5

#include "common/errcode.h"
#include "wasi/api.hpp"
#include <cerrno>
#include <chrono>
#include <dirent.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <unistd.h>

// socket include
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#if __GLIBC_PREREQ(2, 8)
#include <sys/timerfd.h>
#endif

namespace WasmEdge {
namespace Host {
namespace WASI {
inline namespace detail {

inline constexpr __wasi_errno_t fromErrNo(int ErrNo) noexcept {
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
    __builtin_unreachable();
  }
}

inline constexpr clockid_t toClockId(__wasi_clockid_t Clock) noexcept {
  switch (Clock) {
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

inline constexpr timespec toTimespec(__wasi_timestamp_t Timestamp) noexcept {
  using namespace std::chrono;
  const auto Total = nanoseconds(Timestamp);
  const auto Second = duration_cast<seconds>(Total);
  const auto Nano = Total - Second;
  timespec Result{};
  Result.tv_sec = Second.count();
  Result.tv_nsec = Nano.count();
  return Result;
}

inline constexpr __wasi_timestamp_t
fromTimespec(const timespec &Time) noexcept {
  using namespace std::chrono;
  const auto Result = seconds(Time.tv_sec) + nanoseconds(Time.tv_nsec);
  return Result.count();
}

#if !__GLIBC_PREREQ(2, 6)
inline constexpr timeval toTimeval(__wasi_timestamp_t Timestamp) noexcept {
  using namespace std::chrono;
  const auto Total = duration_cast<microseconds>(nanoseconds(Timestamp));
  const auto Second = duration_cast<seconds>(Total);
  const auto Micro = Total - Second;
  timeval Result{};
  Result.tv_sec = Second.count();
  Result.tv_usec = Micro.count();
  return Result;
}
inline constexpr timeval toTimeval(timespec Timespec) noexcept {
  using namespace std::chrono;
  timeval Result{};
  Result.tv_sec = Timespec.tv_sec;
  Result.tv_usec =
      duration_cast<microseconds>(nanoseconds(Timespec.tv_nsec)).count();
  return Result;
}
#endif

inline constexpr int toAdvice(__wasi_advice_t Advice) noexcept {
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

inline constexpr __wasi_filetype_t fromFileType(mode_t Mode) noexcept {
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

inline constexpr __wasi_filetype_t fromFileType(uint8_t Type) noexcept {
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

inline constexpr int toWhence(__wasi_whence_t Whence) noexcept {
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

} // namespace detail
} // namespace WASI
} // namespace Host
} // namespace WasmEdge
