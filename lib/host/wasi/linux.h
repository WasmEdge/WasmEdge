// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

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
#include <csignal>
#include <cstdio>
#include <ctime>
#include <dirent.h>
#include <fcntl.h>
#include <sched.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

// socket include
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <sys/stat.h>

#if defined(__GLIBC_PREREQ)
#if defined(_LIBCPP_GLIBC_PREREQ)
#undef _LIBCPP_GLIBC_PREREQ
#endif
#define _LIBCPP_GLIBC_PREREQ(a, b) 0
#else
#if defined(_LIBCPP_GLIBC_PREREQ)
#define __GLIBC_PREREQ(a, b) _LIBCPP_GLIBC_PREREQ(a, b)
#else
#define __GLIBC_PREREQ(a, b) 1
#endif
#endif

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
    assumingUnreachable();
  }
}

inline constexpr __wasi_errno_t fromEAIErrNo(int ErrNo) noexcept {
  switch (ErrNo) {
  case EAI_ADDRFAMILY:
    return __WASI_ERRNO_AIADDRFAMILY;
  case EAI_AGAIN:
    return __WASI_ERRNO_AIAGAIN;
  case EAI_BADFLAGS:
    return __WASI_ERRNO_AIBADFLAG;
  case EAI_FAIL:
    return __WASI_ERRNO_AIFAIL;
  case EAI_FAMILY:
    return __WASI_ERRNO_AIFAMILY;
  case EAI_MEMORY:
    return __WASI_ERRNO_AIMEMORY;
  case EAI_NODATA:
    return __WASI_ERRNO_AINODATA;
  case EAI_NONAME:
    return __WASI_ERRNO_AINONAME;
  case EAI_SERVICE:
    return __WASI_ERRNO_AISERVICE;
  case EAI_SOCKTYPE:
    return __WASI_ERRNO_AISOCKTYPE;
  case EAI_SYSTEM:
    return __WASI_ERRNO_AISYSTEM;
  default:
    assumingUnreachable();
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
    assumingUnreachable();
  }
}

inline constexpr timespec toTimespec(__wasi_timestamp_t Timestamp) noexcept {
  using namespace std::chrono;
  const auto Total = duration<uint64_t, std::nano>(Timestamp);
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
    assumingUnreachable();
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
    assumingUnreachable();
  }
}

inline constexpr int toSockOptLevel(__wasi_sock_opt_level_t Level) noexcept {
  switch (Level) {
  case __WASI_SOCK_OPT_LEVEL_SOL_SOCKET:
    return SOL_SOCKET;
  default:
    assumingUnreachable();
  }
}

inline constexpr int toSockOptSoName(__wasi_sock_opt_so_t SoName) noexcept {
  switch (SoName) {
  case __WASI_SOCK_OPT_SO_REUSEADDR:
    return SO_REUSEADDR;
  case __WASI_SOCK_OPT_SO_TYPE:
    return SO_TYPE;
  case __WASI_SOCK_OPT_SO_ERROR:
    return SO_ERROR;
  case __WASI_SOCK_OPT_SO_DONTROUTE:
    return SO_DONTROUTE;
  case __WASI_SOCK_OPT_SO_BROADCAST:
    return SO_BROADCAST;
  case __WASI_SOCK_OPT_SO_SNDBUF:
    return SO_SNDBUF;
  case __WASI_SOCK_OPT_SO_RCVBUF:
    return SO_RCVBUF;
  case __WASI_SOCK_OPT_SO_KEEPALIVE:
    return SO_KEEPALIVE;
  case __WASI_SOCK_OPT_SO_OOBINLINE:
    return SO_OOBINLINE;
  case __WASI_SOCK_OPT_SO_LINGER:
    return SO_LINGER;
  case __WASI_SOCK_OPT_SO_RCVLOWAT:
    return SO_RCVLOWAT;
  case __WASI_SOCK_OPT_SO_RCVTIMEO:
    return SO_RCVTIMEO;
  case __WASI_SOCK_OPT_SO_SNDTIMEO:
    return SO_SNDTIMEO;
  case __WASI_SOCK_OPT_SO_ACCEPTCONN:
    return SO_ACCEPTCONN;
  case __WASI_SOCK_OPT_SO_BINDTODEVICE:
    return SO_BINDTODEVICE;

  default:
    assumingUnreachable();
  }
}

inline constexpr __wasi_aiflags_t fromAIFlags(int AIFlags) noexcept {
  __wasi_aiflags_t Result = static_cast<__wasi_aiflags_t>(0);

  if (AIFlags & AI_PASSIVE) {
    Result |= __WASI_AIFLAGS_AI_PASSIVE;
  }
  if (AIFlags & AI_CANONNAME) {
    Result |= __WASI_AIFLAGS_AI_CANONNAME;
  }
  if (AIFlags & AI_NUMERICHOST) {
    Result |= __WASI_AIFLAGS_AI_NUMERICHOST;
  }
  if (AIFlags & AI_NUMERICSERV) {
    Result |= __WASI_AIFLAGS_AI_NUMERICSERV;
  }
  if (AIFlags & AI_V4MAPPED) {
    Result |= __WASI_AIFLAGS_AI_V4MAPPED;
  }
  if (AIFlags & AI_ALL) {
    Result |= __WASI_AIFLAGS_AI_ALL;
  }
  if (AIFlags & AI_ADDRCONFIG) {
    Result |= __WASI_AIFLAGS_AI_ADDRCONFIG;
  }

  return Result;
}

inline constexpr int toAIFlags(__wasi_aiflags_t AIFlags) noexcept {
  int Result = 0;

  if (AIFlags & __WASI_AIFLAGS_AI_PASSIVE) {
    Result |= AI_PASSIVE;
  }
  if (AIFlags & __WASI_AIFLAGS_AI_CANONNAME) {
    Result |= AI_CANONNAME;
  }
  if (AIFlags & __WASI_AIFLAGS_AI_NUMERICHOST) {
    Result |= AI_NUMERICHOST;
  }
  if (AIFlags & __WASI_AIFLAGS_AI_NUMERICSERV) {
    Result |= AI_NUMERICSERV;
  }
  if (AIFlags & __WASI_AIFLAGS_AI_V4MAPPED) {
    Result |= AI_V4MAPPED;
  }
  if (AIFlags & __WASI_AIFLAGS_AI_ALL) {
    Result |= AI_ALL;
  }
  if (AIFlags & __WASI_AIFLAGS_AI_ADDRCONFIG) {
    Result |= AI_ADDRCONFIG;
  }

  return Result;
}

inline constexpr __wasi_sock_type_t fromSockType(int SockType) noexcept {
  switch (SockType) {
  case 0:
    return __WASI_SOCK_TYPE_SOCK_ANY;
  case SOCK_DGRAM:
    return __WASI_SOCK_TYPE_SOCK_DGRAM;
  case SOCK_STREAM:
    return __WASI_SOCK_TYPE_SOCK_STREAM;
  default:
    assumingUnreachable();
  }
}

inline constexpr int toSockType(__wasi_sock_type_t SockType) noexcept {
  switch (SockType) {
  case __WASI_SOCK_TYPE_SOCK_ANY:
    return 0;
  case __WASI_SOCK_TYPE_SOCK_DGRAM:
    return SOCK_DGRAM;
  case __WASI_SOCK_TYPE_SOCK_STREAM:
    return SOCK_STREAM;
  default:
    assumingUnreachable();
  }
}

inline constexpr __wasi_protocol_t fromProtocol(int Protocol) noexcept {
  switch (Protocol) {
  case IPPROTO_IP:
    return __WASI_PROTOCOL_IPPROTO_IP;
  case IPPROTO_TCP:
    return __WASI_PROTOCOL_IPPROTO_TCP;
  case IPPROTO_UDP:
    return __WASI_PROTOCOL_IPPROTO_UDP;
  default:
    assumingUnreachable();
  }
}

inline constexpr int toProtocol(__wasi_protocol_t Protocol) noexcept {
  switch (Protocol) {
  case __WASI_PROTOCOL_IPPROTO_IP:
    return IPPROTO_IP;
  case __WASI_PROTOCOL_IPPROTO_TCP:
    return IPPROTO_TCP;
  case __WASI_PROTOCOL_IPPROTO_UDP:
    return IPPROTO_UDP;
  default:
    assumingUnreachable();
  }
}

inline constexpr __wasi_address_family_t
fromAddressFamily(int AddressFamily) noexcept {
  switch (AddressFamily) {
  case PF_UNSPEC:
    return __WASI_ADDRESS_FAMILY_UNSPEC;
  case PF_INET:
    return __WASI_ADDRESS_FAMILY_INET4;
  case PF_INET6:
    return __WASI_ADDRESS_FAMILY_INET6;
  case PF_UNIX:
    return __WASI_ADDRESS_FAMILY_AF_UNIX;
  default:
    assumingUnreachable();
  }
}

inline constexpr int
toAddressFamily(__wasi_address_family_t AddressFamily) noexcept {
  switch (AddressFamily) {
  case __WASI_ADDRESS_FAMILY_UNSPEC:
    return PF_UNSPEC;
  case __WASI_ADDRESS_FAMILY_INET4:
    return PF_INET;
  case __WASI_ADDRESS_FAMILY_INET6:
    return PF_INET6;
  case __WASI_ADDRESS_FAMILY_AF_UNIX:
    return PF_UNIX;
  default:
    assumingUnreachable();
  }
}

} // namespace detail
} // namespace WASI
} // namespace Host
} // namespace WasmEdge
