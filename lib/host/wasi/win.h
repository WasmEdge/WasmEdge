// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "common/defines.h"
#if !WASMEDGE_OS_WINDOWS
#error
#endif

#include "common/errcode.h"
#include "wasi/api.hpp"
#include <boost/winapi/access_rights.hpp>
#include <boost/winapi/basic_types.hpp>
#include <boost/winapi/error_codes.hpp>
#include <boost/winapi/file_management.hpp>
#include <boost/winapi/get_last_error.hpp>
#include <boost/winapi/handles.hpp>
#include <boost/winapi/thread.hpp>
#include <cerrno>
#include <chrono>
#include <fcntl.h>
#include <io.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <winsock2.h>

#if !defined(BOOST_USE_WINDOWS_H)
extern "C" {
struct _UNICODE_STRING;
struct _OBJECT_ATTRIBUTES;
struct _IO_STATUS_BLOCK;

BOOST_WINAPI_IMPORT boost::winapi::NTSTATUS_ BOOST_WINAPI_WINAPI_CC NtOpenFile(
    boost::winapi::PHANDLE_ FileHandle,
    boost::winapi::ACCESS_MASK_ DesiredAccess,
    _OBJECT_ATTRIBUTES *ObjectAttributes, _IO_STATUS_BLOCK *IoStatusBlock,
    boost::winapi::ULONG_ ShareAccess, boost::winapi::ULONG_ OpenOptions);

BOOST_WINAPI_IMPORT boost::winapi::HANDLE_ BOOST_WINAPI_WINAPI_CC
GetStdHandle(boost::winapi::DWORD_ nStdHandle);
}
#else
#include <winternl.h>
#endif

namespace boost::winapi {

typedef struct BOOST_MAY_ALIAS _UNICODE_STRING {
  USHORT_ Length;
  USHORT_ MaximumLength;
  LPWSTR_ Buffer;
} UNICODE_STRING_, *PUNICODE_STRING_;

typedef struct BOOST_MAY_ALIAS _OBJECT_ATTRIBUTES {
  ULONG_ Length;
  HANDLE_ RootDirectory;
  PUNICODE_STRING_ ObjectName;
  ULONG_ Attributes;
  PVOID_ SecurityDescriptor;
  PVOID_ SecurityQualityOfService;
} OBJECT_ATTRIBUTES_, *POBJECT_ATTRIBUTES_;

typedef struct BOOST_MAY_ALIAS _IO_STATUS_BLOCK {
  union {
    NTSTATUS_ Status;
    PVOID_ Pointer;
  } DUMMYUNIONNAME;
  ULONG_PTR_ Information;
} IO_STATUS_BLOCK_, *PIO_STATUS_BLOCK_;

#if defined(BOOST_USE_WINDOWS_H)
BOOST_CONSTEXPR_OR_CONST DWORD_ STD_INPUT_HANDLE_ = STD_INPUT_HANDLE;
BOOST_CONSTEXPR_OR_CONST DWORD_ STD_OUTPUT_HANDLE_ = STD_OUTPUT_HANDLE;
BOOST_CONSTEXPR_OR_CONST DWORD_ STD_ERROR_HANDLE_ = STD_ERROR_HANDLE;
#else
BOOST_CONSTEXPR_OR_CONST DWORD_ STD_INPUT_HANDLE_ = static_cast<DWORD_>(-10);
BOOST_CONSTEXPR_OR_CONST DWORD_ STD_OUTPUT_HANDLE_ = static_cast<DWORD_>(-11);
BOOST_CONSTEXPR_OR_CONST DWORD_ STD_ERROR_HANDLE_ = static_cast<DWORD_>(-12);
#endif

BOOST_FORCEINLINE NTSTATUS_ NtOpenFile(PHANDLE_ FileHandle,
                                       ACCESS_MASK_ DesiredAccess,
                                       POBJECT_ATTRIBUTES_ ObjectAttributes,
                                       PIO_STATUS_BLOCK_ IoStatusBlock,
                                       ULONG_ ShareAccess, ULONG_ OpenOptions) {
  return ::NtOpenFile(
      FileHandle, DesiredAccess,
      reinterpret_cast<::_OBJECT_ATTRIBUTES *>(ObjectAttributes),
      reinterpret_cast<::_IO_STATUS_BLOCK *>(IoStatusBlock), ShareAccess,
      OpenOptions);
}

BOOST_FORCEINLINE HANDLE_ GetStdHandle(DWORD_ nStdHandle) {
  return ::GetStdHandle(nStdHandle);
}

} // namespace boost::winapi

namespace WasmEdge {
namespace Host {
namespace WASI {
inline namespace detail {

std::chrono::nanoseconds getResolution() noexcept;
extern long(__stdcall *NtQueryTimerResolution)(
    unsigned long *LowestResolution, unsigned long *HighestResolution,
    unsigned long *CurrentResolution);

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

inline constexpr __wasi_errno_t
fromLastError(boost::winapi::DWORD_ LastError) noexcept {
  switch (LastError) {
  case boost::winapi::ERROR_ACCESS_DENIED_:
    return __WASI_ERRNO_PERM;
  case boost::winapi::ERROR_ADDRESS_ALREADY_ASSOCIATED_:
    return __WASI_ERRNO_ADDRINUSE;
  case boost::winapi::ERROR_ALREADY_EXISTS_:
    return __WASI_ERRNO_EXIST;
  case boost::winapi::ERROR_BAD_PATHNAME_:
    return __WASI_ERRNO_NOENT;
  case boost::winapi::ERROR_BAD_PIPE_:
    return __WASI_ERRNO_PIPE;
  case boost::winapi::ERROR_BEGINNING_OF_MEDIA_:
    return __WASI_ERRNO_IO;
  case boost::winapi::ERROR_BROKEN_PIPE_:
    return __WASI_ERRNO_PIPE;
  case boost::winapi::ERROR_BUFFER_OVERFLOW_:
    return __WASI_ERRNO_FAULT;
  case boost::winapi::ERROR_BUS_RESET_:
    return __WASI_ERRNO_IO;
  case boost::winapi::ERROR_CANNOT_MAKE_:
    return __WASI_ERRNO_NOSPC;
  case boost::winapi::ERROR_CANT_ACCESS_FILE_:
    return __WASI_ERRNO_ACCES;
  case boost::winapi::ERROR_CANT_RESOLVE_FILENAME_:
    return __WASI_ERRNO_LOOP;
  case boost::winapi::ERROR_CONNECTION_ABORTED_:
    return __WASI_ERRNO_CONNABORTED;
  case boost::winapi::ERROR_CONNECTION_REFUSED_:
    return __WASI_ERRNO_CONNREFUSED;
  case boost::winapi::ERROR_CRC_:
    return __WASI_ERRNO_IO;
  case boost::winapi::ERROR_DEVICE_DOOR_OPEN_:
    return __WASI_ERRNO_IO;
  case boost::winapi::ERROR_DEVICE_REQUIRES_CLEANING_:
    return __WASI_ERRNO_IO;
  case boost::winapi::ERROR_DIRECTORY_:
    return __WASI_ERRNO_NOENT;
  case boost::winapi::ERROR_DIR_NOT_EMPTY_:
    return __WASI_ERRNO_NOTEMPTY;
  case boost::winapi::ERROR_DISK_CORRUPT_:
    return __WASI_ERRNO_IO;
  case boost::winapi::ERROR_DISK_FULL_:
    return __WASI_ERRNO_NOSPC;
  case boost::winapi::ERROR_EA_TABLE_FULL_:
    return __WASI_ERRNO_NOSPC;
  case boost::winapi::ERROR_ELEVATION_REQUIRED_:
    return __WASI_ERRNO_ACCES;
  case boost::winapi::ERROR_END_OF_MEDIA_:
    return __WASI_ERRNO_NOSPC;
  case boost::winapi::ERROR_ENVVAR_NOT_FOUND_:
    return __WASI_ERRNO_NOENT;
  case boost::winapi::ERROR_EOM_OVERFLOW_:
    return __WASI_ERRNO_IO;
  case boost::winapi::ERROR_FILEMARK_DETECTED_:
    return __WASI_ERRNO_IO;
  case boost::winapi::ERROR_FILENAME_EXCED_RANGE_:
    return __WASI_ERRNO_NAMETOOLONG;
  case boost::winapi::ERROR_FILE_EXISTS_:
    return __WASI_ERRNO_EXIST;
  case boost::winapi::ERROR_FILE_NOT_FOUND_:
    return __WASI_ERRNO_NOENT;
  case boost::winapi::ERROR_GEN_FAILURE_:
    return __WASI_ERRNO_IO;
  case boost::winapi::ERROR_HANDLE_DISK_FULL_:
    return __WASI_ERRNO_NOSPC;
  case boost::winapi::ERROR_HOST_UNREACHABLE_:
    return __WASI_ERRNO_HOSTUNREACH;
  case boost::winapi::ERROR_INSUFFICIENT_BUFFER_:
    return __WASI_ERRNO_INVAL;
  case boost::winapi::ERROR_INVALID_BLOCK_LENGTH_:
    return __WASI_ERRNO_IO;
  case boost::winapi::ERROR_INVALID_DATA_:
    return __WASI_ERRNO_INVAL;
  case boost::winapi::ERROR_INVALID_DRIVE_:
    return __WASI_ERRNO_NOENT;
  case boost::winapi::ERROR_INVALID_FLAGS_:
    return __WASI_ERRNO_BADF;
  case boost::winapi::ERROR_INVALID_FUNCTION_:
    return __WASI_ERRNO_ISDIR;
  case boost::winapi::ERROR_INVALID_HANDLE_:
    return __WASI_ERRNO_BADF;
  case boost::winapi::ERROR_INVALID_NAME_:
    return __WASI_ERRNO_NOENT;
  case boost::winapi::ERROR_INVALID_PARAMETER_:
    return __WASI_ERRNO_INVAL;
  case boost::winapi::ERROR_INVALID_REPARSE_DATA_:
    return __WASI_ERRNO_NOENT;
  case boost::winapi::ERROR_IO_DEVICE_:
    return __WASI_ERRNO_IO;
  case boost::winapi::ERROR_LOCK_VIOLATION_:
    return __WASI_ERRNO_BUSY;
  case boost::winapi::ERROR_META_EXPANSION_TOO_LONG_:
    return __WASI_ERRNO_2BIG;
  case boost::winapi::ERROR_MOD_NOT_FOUND_:
    return __WASI_ERRNO_NOENT;
  case boost::winapi::ERROR_NETNAME_DELETED_:
    return __WASI_ERRNO_CONNRESET;
  case boost::winapi::ERROR_NETWORK_UNREACHABLE_:
    return __WASI_ERRNO_NETUNREACH;
  case boost::winapi::ERROR_NOACCESS_:
    return __WASI_ERRNO_ACCES;
  case boost::winapi::ERROR_NOT_CONNECTED_:
    return __WASI_ERRNO_NOTCONN;
  case boost::winapi::ERROR_NOT_ENOUGH_MEMORY_:
    return __WASI_ERRNO_NOMEM;
  case boost::winapi::ERROR_NOT_SAME_DEVICE_:
    return __WASI_ERRNO_XDEV;
  case boost::winapi::ERROR_NOT_SUPPORTED_:
    return __WASI_ERRNO_NOTSUP;
  case boost::winapi::ERROR_NO_DATA_:
    return __WASI_ERRNO_PIPE;
  case boost::winapi::ERROR_NO_DATA_DETECTED_:
    return __WASI_ERRNO_IO;
  case boost::winapi::ERROR_NO_SIGNAL_SENT_:
    return __WASI_ERRNO_IO;
  case boost::winapi::ERROR_OPEN_FAILED_:
    return __WASI_ERRNO_IO;
  case boost::winapi::ERROR_OPERATION_ABORTED_:
    return __WASI_ERRNO_CANCELED;
  case boost::winapi::ERROR_OUTOFMEMORY_:
    return __WASI_ERRNO_NOMEM;
  case boost::winapi::ERROR_PATH_NOT_FOUND_:
    return __WASI_ERRNO_NOENT;
  case boost::winapi::ERROR_PIPE_BUSY_:
    return __WASI_ERRNO_BUSY;
  case boost::winapi::ERROR_PIPE_NOT_CONNECTED_:
    return __WASI_ERRNO_PIPE;
  case boost::winapi::ERROR_PRIVILEGE_NOT_HELD_:
    return __WASI_ERRNO_PERM;
  case boost::winapi::ERROR_SEM_TIMEOUT_:
    return __WASI_ERRNO_TIMEDOUT;
  case boost::winapi::ERROR_SETMARK_DETECTED_:
    return __WASI_ERRNO_IO;
  case boost::winapi::ERROR_SHARING_VIOLATION_:
    return __WASI_ERRNO_BUSY;
  case boost::winapi::ERROR_SIGNAL_REFUSED_:
    return __WASI_ERRNO_IO;
  case boost::winapi::ERROR_SYMLINK_NOT_SUPPORTED_:
    return __WASI_ERRNO_INVAL;
  case boost::winapi::ERROR_TOO_MANY_LINKS_:
    return __WASI_ERRNO_MLINK;
  case boost::winapi::ERROR_TOO_MANY_OPEN_FILES_:
    return __WASI_ERRNO_MFILE;
  case boost::winapi::ERROR_WAIT_NO_CHILDREN_:
    return __WASI_ERRNO_CHILD;
  case boost::winapi::ERROR_WRITE_PROTECT_:
    return __WASI_ERRNO_ROFS;
  case boost::winapi::WSAEACCES_:
    return __WASI_ERRNO_ACCES;
  case boost::winapi::WSAEADDRINUSE_:
    return __WASI_ERRNO_ADDRINUSE;
  case boost::winapi::WSAEADDRNOTAVAIL_:
    return __WASI_ERRNO_ADDRNOTAVAIL;
  case boost::winapi::WSAEAFNOSUPPORT_:
    return __WASI_ERRNO_AFNOSUPPORT;
  case boost::winapi::WSAEALREADY_:
    return __WASI_ERRNO_ALREADY;
  case boost::winapi::WSAEBADF_:
    return __WASI_ERRNO_BADF;
  case boost::winapi::WSAECONNABORTED_:
    return __WASI_ERRNO_CONNABORTED;
  case boost::winapi::WSAECONNREFUSED_:
    return __WASI_ERRNO_CONNREFUSED;
  case boost::winapi::WSAECONNRESET_:
    return __WASI_ERRNO_CONNRESET;
  case boost::winapi::WSAEDQUOT_:
    return __WASI_ERRNO_DQUOT;
  case boost::winapi::WSAEFAULT_:
    return __WASI_ERRNO_FAULT;
  case boost::winapi::WSAEHOSTDOWN_:
    return __WASI_ERRNO_HOSTUNREACH;
  case boost::winapi::WSAEHOSTUNREACH_:
    return __WASI_ERRNO_HOSTUNREACH;
  case boost::winapi::WSAEINTR_:
    return __WASI_ERRNO_INTR;
  case boost::winapi::WSAEINVAL_:
    return __WASI_ERRNO_INVAL;
  case boost::winapi::WSAEISCONN_:
    return __WASI_ERRNO_ISCONN;
  case boost::winapi::WSAELOOP_:
    return __WASI_ERRNO_LOOP;
  case boost::winapi::WSAEMFILE_:
    return __WASI_ERRNO_MFILE;
  case boost::winapi::WSAEMSGSIZE_:
    return __WASI_ERRNO_MSGSIZE;
  case boost::winapi::WSAENAMETOOLONG_:
    return __WASI_ERRNO_NAMETOOLONG;
  case boost::winapi::WSAENETDOWN_:
    return __WASI_ERRNO_NETDOWN;
  case boost::winapi::WSAENETRESET_:
    return __WASI_ERRNO_NETRESET;
  case boost::winapi::WSAENETUNREACH_:
    return __WASI_ERRNO_NETUNREACH;
  case boost::winapi::WSAENOBUFS_:
    return __WASI_ERRNO_NOBUFS;
  case boost::winapi::WSAENOTCONN_:
    return __WASI_ERRNO_NOTCONN;
  case boost::winapi::WSAENOTEMPTY_:
    return __WASI_ERRNO_NOTEMPTY;
  case boost::winapi::WSAENOTSOCK_:
    return __WASI_ERRNO_NOTSOCK;
  case boost::winapi::WSAEOPNOTSUPP_:
    return __WASI_ERRNO_NOTSUP;
  case boost::winapi::WSAEPFNOSUPPORT_:
    return __WASI_ERRNO_NOTSUP;
  case boost::winapi::WSAEPROTONOSUPPORT_:
    return __WASI_ERRNO_PROTONOSUPPORT;
  case boost::winapi::WSAESHUTDOWN_:
    return __WASI_ERRNO_PIPE;
  case boost::winapi::WSAESOCKTNOSUPPORT_:
    return __WASI_ERRNO_NOTSUP;
  case boost::winapi::WSAESTALE_:
    return __WASI_ERRNO_STALE;
  case boost::winapi::WSAETIMEDOUT_:
    return __WASI_ERRNO_TIMEDOUT;
  case boost::winapi::WSAETOOMANYREFS_:
    return __WASI_ERRNO_NFILE;
  case boost::winapi::WSAEWOULDBLOCK_:
    return __WASI_ERRNO_AGAIN;
  case boost::winapi::WSAHOST_NOT_FOUND_:
    return __WASI_ERRNO_NOENT;
  case boost::winapi::WSANO_DATA_:
    return __WASI_ERRNO_NOENT;
  default:
    return __WASI_ERRNO_NOSYS;
  }
}

inline constexpr __wasi_errno_t fromWSALastError(int WSALastError) noexcept {
  return fromLastError(static_cast<boost::winapi::DWORD_>(WSALastError));
}

inline constexpr __wasi_errno_t fromWSAToEAIError(int WSALastError) noexcept {
  switch (WSALastError) {
  case boost::winapi::WSATRY_AGAIN_:
    return __WASI_ERRNO_AIAGAIN;
  case boost::winapi::WSAEINVAL_:
    return __WASI_ERRNO_AIBADFLAG;
  case boost::winapi::WSANO_RECOVERY_:
    return __WASI_ERRNO_AIFAIL;
  case boost::winapi::ERROR_NOT_ENOUGH_MEMORY_:
    return __WASI_ERRNO_AIMEMORY;
  case boost::winapi::WSAHOST_NOT_FOUND_:
    return __WASI_ERRNO_AINONAME;
  case boost::winapi::WSATYPE_NOT_FOUND_:
    return __WASI_ERRNO_AISERVICE;
  case boost::winapi::WSAESOCKTNOSUPPORT_:
    return __WASI_ERRNO_AISOCKTYPE;
  default:
    return fromWSALastError(WSALastError);
  }
}

using Days = std::chrono::duration<uint64_t, std::ratio<86400>>;
using NS = std::chrono::nanoseconds;
using HundredNS = std::chrono::duration<uint64_t, std::ratio<1, 10000000>>;
/// from 1601-01-01 to 1970-01-01, 134774 days
constexpr const HundredNS kWinUnixTimeBaseDiff(Days(134774));

inline constexpr __wasi_timestamp_t fromFileTyime(uint64_t Value) noexcept {
  NS Time = HundredNS(Value) - kWinUnixTimeBaseDiff;
  return static_cast<__wasi_timestamp_t>(Time.count());
}

inline constexpr uint64_t toFileTyime(__wasi_timestamp_t Value) noexcept {
  HundredNS Time =
      std::chrono::duration_cast<HundredNS>(NS(Value)) + kWinUnixTimeBaseDiff;
  return static_cast<uint64_t>(Time.count());
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
    return SO_REUSEADDR;
  case __WASI_SOCK_OPT_SO_DONTROUTE:
    return SO_TYPE;
  case __WASI_SOCK_OPT_SO_BROADCAST:
    return SO_REUSEADDR;
  case __WASI_SOCK_OPT_SO_SNDBUF:
    return SO_TYPE;
  case __WASI_SOCK_OPT_SO_RCVBUF:
    return SO_REUSEADDR;
  case __WASI_SOCK_OPT_SO_KEEPALIVE:
    return SO_TYPE;
  case __WASI_SOCK_OPT_SO_OOBINLINE:
    return SO_REUSEADDR;
  case __WASI_SOCK_OPT_SO_LINGER:
    return SO_TYPE;
  case __WASI_SOCK_OPT_SO_RCVLOWAT:
    return SO_REUSEADDR;
  case __WASI_SOCK_OPT_SO_RCVTIMEO:
    return SO_TYPE;
  case __WASI_SOCK_OPT_SO_SNDTIMEO:
    return SO_REUSEADDR;
  case __WASI_SOCK_OPT_SO_ACCEPTCONN:
    return SO_TYPE;

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
  default:
    assumingUnreachable();
  }
}

} // namespace detail
} // namespace WASI
} // namespace Host
} // namespace WasmEdge
