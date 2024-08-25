// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "common/defines.h"
#if !WASMEDGE_OS_WINDOWS
#error
#endif

#include "common/errcode.h"
#include "system/winapi.h"
#include "wasi/api.hpp"
#include <cerrno>
#include <chrono>
#include <fcntl.h>
#include <io.h>
#include <sys/stat.h>
#include <sys/types.h>

namespace WasmEdge {
namespace Host {
namespace WASI {
inline namespace detail {
using namespace winapi;

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

inline __wasi_errno_t fromLastError(DWORD_ Code) noexcept {
  switch (Code) {
  case ERROR_INVALID_PARAMETER_: // MultiByteToWideChar
  case ERROR_INVALID_HANDLE_:    // GetFinalPathNameByHandleW
  case ERROR_NEGATIVE_SEEK_:     // SetFilePointerEx
    return __WASI_ERRNO_INVAL;
  case ERROR_SHARING_VIOLATION_: // CreateFile2
  case ERROR_PIPE_BUSY_:         // CreateFile2
    return __WASI_ERRNO_BUSY;
  case ERROR_ACCESS_DENIED_: // CreateFile2
    return __WASI_ERRNO_ACCES;
  case ERROR_ALREADY_EXISTS_: // CreateFile2
  case ERROR_FILE_EXISTS_:    // CreateFile2
    return __WASI_ERRNO_EXIST;
  case ERROR_FILE_NOT_FOUND_: // CreateFile2
  case ERROR_INVALID_NAME_:   // CreateFile2
    return __WASI_ERRNO_NOENT;
  case ERROR_PRIVILEGE_NOT_HELD_: // CreateSymbolicLinkW
    return __WASI_ERRNO_PERM;
  case ERROR_DIRECTORY_: // RemoveDirectoryW
    return __WASI_ERRNO_NOTDIR;
  case ERROR_DIR_NOT_EMPTY_: // RemoveDirectoryW
    return __WASI_ERRNO_NOTEMPTY;

  case ERROR_IO_PENDING_:             // ReadFileEx
  case ERROR_HANDLE_EOF_:             // ReadFileEx
  case ERROR_INSUFFICIENT_BUFFER_:    // MultiByteToWideChar
  case ERROR_INVALID_FLAGS_:          // MultiByteToWideChar
  case ERROR_NO_UNICODE_TRANSLATION_: // MultiByteToWideChar
  default:
    return __WASI_ERRNO_NOSYS;
  }
}

using FiletimeDuration = std::chrono::duration<
    uint64_t,
    std::ratio_multiply<std::ratio<100, 1>, std::chrono::nanoseconds::period>>;
/// from 1601-01-01 to 1970-01-01, 134774 days
static inline constexpr const FiletimeDuration NTToUnixEpoch =
    std::chrono::seconds{134774LL * 86400LL};

static_assert(std::chrono::duration_cast<std::chrono::seconds>(NTToUnixEpoch)
                  .count() == 11644473600LL);

static constexpr __wasi_timestamp_t fromFiletime(FILETIME_ FileTime) noexcept {
  using std::chrono::duration_cast;
  using std::chrono::nanoseconds;
  ULARGE_INTEGER_ Temp = {/* LowPart */ FileTime.dwLowDateTime,
                          /* HighPart */ FileTime.dwHighDateTime};
  auto Duration = duration_cast<nanoseconds>(FiletimeDuration{Temp.QuadPart} -
                                             NTToUnixEpoch);
  return static_cast<__wasi_timestamp_t>(Duration.count());
}

static constexpr FILETIME_ toFiletime(__wasi_timestamp_t TimeStamp) noexcept {
  using std::chrono::duration_cast;
  using std::chrono::nanoseconds;
  auto Duration =
      duration_cast<FiletimeDuration>(nanoseconds{TimeStamp}) + NTToUnixEpoch;
  ULARGE_INTEGER_ Temp = ULARGE_INTEGER_(Duration.count());
  return FILETIME_{/* dwLowDateTime */ Temp.LowPart,
                   /* dwHighDateTime */ Temp.HighPart};
}

inline __wasi_errno_t fromWSALastError() noexcept {
  switch (WSAGetLastError()) {
  case WSASYSNOTREADY_: // WSAStartup
  case WSAEWOULDBLOCK_: // closesocket
    return __WASI_ERRNO_AGAIN;
  case WSAVERNOTSUPPORTED_: // WSAStartup
    return __WASI_ERRNO_NOTSUP;
  case WSAEINPROGRESS_: // WSAStartup, socket, closesocket
    return __WASI_ERRNO_INPROGRESS;
  case WSAEPROCLIM_: // WSAStartup
    return __WASI_ERRNO_BUSY;
  case WSAEFAULT_: // WSAStartup
    return __WASI_ERRNO_FAULT;
  case WSAENETDOWN_: // socket, closesocket
    return __WASI_ERRNO_NETDOWN;
  case WSAENOTSOCK_: // closesocket
    return __WASI_ERRNO_NOTSOCK;
  case WSAEINTR_: // closesocket
    return __WASI_ERRNO_INTR;
  case WSAEAFNOSUPPORT_: // socket
    return __WASI_ERRNO_AIFAMILY;
  case WSAEMFILE_: // socket
    return __WASI_ERRNO_NFILE;
  case WSAEINVAL_: // socket
    return __WASI_ERRNO_INVAL;
  case WSAENOBUFS_: // socket
    return __WASI_ERRNO_NOBUFS;
  case WSAEPROTONOSUPPORT_: // socket
    return __WASI_ERRNO_PROTONOSUPPORT;
  case WSAEPROTOTYPE_: // socket
    return __WASI_ERRNO_PROTOTYPE;
  case WSAESOCKTNOSUPPORT_: // socket
    return __WASI_ERRNO_AISOCKTYPE;
  case WSAEINVALIDPROCTABLE_:   // socket
  case WSAEINVALIDPROVIDER_:    // socket
  case WSAEPROVIDERFAILEDINIT_: // socket
  case WSANOTINITIALISED_:      // socket, closesocket
  default:
    return __WASI_ERRNO_NOSYS;
  }
}

inline constexpr __wasi_errno_t fromWSAError(int WSAError) noexcept {
  switch (WSAError) {
  case WSATRY_AGAIN_:
    return __WASI_ERRNO_AIAGAIN;
  case WSAEINVAL_:
    return __WASI_ERRNO_AIBADFLAG;
  case WSANO_RECOVERY_:
    return __WASI_ERRNO_AIFAIL;
  case WSAEAFNOSUPPORT_:
    return __WASI_ERRNO_AIFAMILY;
  case ERROR_NOT_ENOUGH_MEMORY_:
    return __WASI_ERRNO_AIMEMORY;
  case WSAHOST_NOT_FOUND_:
    return __WASI_ERRNO_AINONAME;
  case WSATYPE_NOT_FOUND_:
    return __WASI_ERRNO_AISERVICE;
  case WSAESOCKTNOSUPPORT_:
    return __WASI_ERRNO_AISOCKTYPE;
  default:
    assumingUnreachable();
  }
}

inline WasiExpect<void> ensureWSAStartup() noexcept {
  static std::once_flag InitFlag;
  try {
    std::call_once(InitFlag, []() {
      WSADATA_ WSAData;
      if (unlikely(WSAStartup(0x0202, &WSAData) != 0)) {
        throw detail::fromWSALastError();
      }
      if (unlikely(WSAData.wVersion != 0x0202)) {
        throw __WASI_ERRNO_NOSYS;
      }
    });
    return {};
  } catch (__wasi_errno_t &Error) {
    return WasiUnexpect(Error);
  }
}

inline constexpr DWORD_ toWhence(__wasi_whence_t Whence) noexcept {
  switch (Whence) {
  case __WASI_WHENCE_SET:
    return FILE_BEGIN_;
  case __WASI_WHENCE_END:
    return FILE_END_;
  case __WASI_WHENCE_CUR:
    return FILE_CURRENT_;
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
#if NTDDI_VERSION >= NTDDI_VISTA
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
#endif

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
#if NTDDI_VERSION >= NTDDI_VISTA
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
#endif

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
  case AF_UNSPEC:
    return __WASI_ADDRESS_FAMILY_UNSPEC;
  case AF_INET:
    return __WASI_ADDRESS_FAMILY_INET4;
  case AF_INET6:
    return __WASI_ADDRESS_FAMILY_INET6;
  default:
    assumingUnreachable();
  }
}

inline constexpr int
toAddressFamily(__wasi_address_family_t AddressFamily) noexcept {
  switch (AddressFamily) {
  case __WASI_ADDRESS_FAMILY_UNSPEC:
    return AF_UNSPEC;
  case __WASI_ADDRESS_FAMILY_INET4:
    return AF_INET;
  case __WASI_ADDRESS_FAMILY_INET6:
    return AF_INET6;
  default:
    assumingUnreachable();
  }
}

} // namespace detail
} // namespace WASI
} // namespace Host
} // namespace WasmEdge
