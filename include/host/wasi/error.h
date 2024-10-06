// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "common/expected.h"
#include "common/spdlog.h"
#include "wasi/api.hpp"
#include <string_view>

namespace WasmEdge {
namespace Host {
namespace WASI {

/// Type aliasing for Expected<T, __wasi_errno_t>.
template <typename T> using WasiExpect = Expected<T, __wasi_errno_t>;

/// Helper function for Unexpected<ErrCode>.
constexpr auto WasiUnexpect(__wasi_errno_t Val) {
  return Unexpected<__wasi_errno_t>(Val);
}
template <typename T> constexpr auto WasiUnexpect(const WasiExpect<T> &Val) {
  return Unexpected<__wasi_errno_t>(Val.error());
}

} // namespace WASI
} // namespace Host
} // namespace WasmEdge

template <>
struct fmt::formatter<__wasi_errno_t> : fmt::formatter<std::string_view> {
  fmt::format_context::iterator
  format(__wasi_errno_t ErrNo, fmt::format_context &Ctx) const noexcept {
    fmt::memory_buffer Buffer;
    using namespace std::literals;
    auto Iter = std::back_inserter(Buffer);
    switch (ErrNo) {
    case __WASI_ERRNO_SUCCESS:
      fmt::format_to(
          Iter, "No error occurred. System call completed successfully."sv);
      break;
    case __WASI_ERRNO_2BIG:
      fmt::format_to(Iter, "Argument list too long."sv);
      break;
    case __WASI_ERRNO_ACCES:
      fmt::format_to(Iter, "Permission denied."sv);
      break;
    case __WASI_ERRNO_ADDRINUSE:
      fmt::format_to(Iter, "Address in use."sv);
      break;
    case __WASI_ERRNO_ADDRNOTAVAIL:
      fmt::format_to(Iter, "Address not available."sv);
      break;
    case __WASI_ERRNO_AFNOSUPPORT:
      fmt::format_to(Iter, "Address family not supported."sv);
      break;
    case __WASI_ERRNO_AGAIN:
      fmt::format_to(Iter, "Resource unavailable, or operation would block."sv);
      break;
    case __WASI_ERRNO_ALREADY:
      fmt::format_to(Iter, "Connection already in progress."sv);
      break;
    case __WASI_ERRNO_BADF:
      fmt::format_to(Iter, "Bad file descriptor."sv);
      break;
    case __WASI_ERRNO_BADMSG:
      fmt::format_to(Iter, "Bad message."sv);
      break;
    case __WASI_ERRNO_BUSY:
      fmt::format_to(Iter, "Device or resource busy."sv);
      break;
    case __WASI_ERRNO_CANCELED:
      fmt::format_to(Iter, "Operation canceled."sv);
      break;
    case __WASI_ERRNO_CHILD:
      fmt::format_to(Iter, "No child processes."sv);
      break;
    case __WASI_ERRNO_CONNABORTED:
      fmt::format_to(Iter, "Connection aborted."sv);
      break;
    case __WASI_ERRNO_CONNREFUSED:
      fmt::format_to(Iter, "Connection refused."sv);
      break;
    case __WASI_ERRNO_CONNRESET:
      fmt::format_to(Iter, "Connection reset."sv);
      break;
    case __WASI_ERRNO_DEADLK:
      fmt::format_to(Iter, "Resource deadlock would occur."sv);
      break;
    case __WASI_ERRNO_DESTADDRREQ:
      fmt::format_to(Iter, "Destination address required."sv);
      break;
    case __WASI_ERRNO_DOM:
      fmt::format_to(Iter, "Mathematics argument out of domain of function."sv);
      break;
    case __WASI_ERRNO_DQUOT:
      fmt::format_to(Iter, "Reserved."sv);
      break;
    case __WASI_ERRNO_EXIST:
      fmt::format_to(Iter, "File exists."sv);
      break;
    case __WASI_ERRNO_FAULT:
      fmt::format_to(Iter, "Bad address."sv);
      break;
    case __WASI_ERRNO_FBIG:
      fmt::format_to(Iter, "File too large."sv);
      break;
    case __WASI_ERRNO_HOSTUNREACH:
      fmt::format_to(Iter, "Host is unreachable."sv);
      break;
    case __WASI_ERRNO_IDRM:
      fmt::format_to(Iter, "Identifier removed."sv);
      break;
    case __WASI_ERRNO_ILSEQ:
      fmt::format_to(Iter, "Illegal byte sequence."sv);
      break;
    case __WASI_ERRNO_INPROGRESS:
      fmt::format_to(Iter, "Operation in progress."sv);
      break;
    case __WASI_ERRNO_INTR:
      fmt::format_to(Iter, "Interrupted function."sv);
      break;
    case __WASI_ERRNO_INVAL:
      fmt::format_to(Iter, "Invalid argument."sv);
      break;
    case __WASI_ERRNO_IO:
      fmt::format_to(Iter, "I/O error."sv);
      break;
    case __WASI_ERRNO_ISCONN:
      fmt::format_to(Iter, "Socket is connected."sv);
      break;
    case __WASI_ERRNO_ISDIR:
      fmt::format_to(Iter, "Is a directory."sv);
      break;
    case __WASI_ERRNO_LOOP:
      fmt::format_to(Iter, "Too many levels of symbolic links."sv);
      break;
    case __WASI_ERRNO_MFILE:
      fmt::format_to(Iter, "File descriptor value too large."sv);
      break;
    case __WASI_ERRNO_MLINK:
      fmt::format_to(Iter, "Too many links."sv);
      break;
    case __WASI_ERRNO_MSGSIZE:
      fmt::format_to(Iter, "Message too large."sv);
      break;
    case __WASI_ERRNO_MULTIHOP:
      fmt::format_to(Iter, "Reserved."sv);
      break;
    case __WASI_ERRNO_NAMETOOLONG:
      fmt::format_to(Iter, "Filename too long."sv);
      break;
    case __WASI_ERRNO_NETDOWN:
      fmt::format_to(Iter, "Network is down."sv);
      break;
    case __WASI_ERRNO_NETRESET:
      fmt::format_to(Iter, "Connection aborted by network."sv);
      break;
    case __WASI_ERRNO_NETUNREACH:
      fmt::format_to(Iter, "Network unreachable."sv);
      break;
    case __WASI_ERRNO_NFILE:
      fmt::format_to(Iter, "Too many files open in system."sv);
      break;
    case __WASI_ERRNO_NOBUFS:
      fmt::format_to(Iter, "No buffer space available."sv);
      break;
    case __WASI_ERRNO_NODEV:
      fmt::format_to(Iter, "No such device."sv);
      break;
    case __WASI_ERRNO_NOENT:
      fmt::format_to(Iter, "No such file or directory."sv);
      break;
    case __WASI_ERRNO_NOEXEC:
      fmt::format_to(Iter, "Executable file format error."sv);
      break;
    case __WASI_ERRNO_NOLCK:
      fmt::format_to(Iter, "No locks available."sv);
      break;
    case __WASI_ERRNO_NOLINK:
      fmt::format_to(Iter, "Reserved."sv);
      break;
    case __WASI_ERRNO_NOMEM:
      fmt::format_to(Iter, "Not enough space."sv);
      break;
    case __WASI_ERRNO_NOMSG:
      fmt::format_to(Iter, "No message of the desired type."sv);
      break;
    case __WASI_ERRNO_NOPROTOOPT:
      fmt::format_to(Iter, "Protocol not available."sv);
      break;
    case __WASI_ERRNO_NOSPC:
      fmt::format_to(Iter, "No space left on device."sv);
      break;
    case __WASI_ERRNO_NOSYS:
      fmt::format_to(Iter, "Function not supported."sv);
      break;
    case __WASI_ERRNO_NOTCONN:
      fmt::format_to(Iter, "The socket is not connected."sv);
      break;
    case __WASI_ERRNO_NOTDIR:
      fmt::format_to(Iter,
                     "Not a directory or a symbolic link to a directory."sv);
      break;
    case __WASI_ERRNO_NOTEMPTY:
      fmt::format_to(Iter, "Directory not empty."sv);
      break;
    case __WASI_ERRNO_NOTRECOVERABLE:
      fmt::format_to(Iter, "State not recoverable."sv);
      break;
    case __WASI_ERRNO_NOTSOCK:
      fmt::format_to(Iter, "Not a socket."sv);
      break;
    case __WASI_ERRNO_NOTSUP:
      fmt::format_to(Iter,
                     "Not supported, or operation not supported on socket."sv);
      break;
    case __WASI_ERRNO_NOTTY:
      fmt::format_to(Iter, "Inappropriate I/O control operation."sv);
      break;
    case __WASI_ERRNO_NXIO:
      fmt::format_to(Iter, "No such device or address."sv);
      break;
    case __WASI_ERRNO_OVERFLOW:
      fmt::format_to(Iter, "Value too large to be stored in data type."sv);
      break;
    case __WASI_ERRNO_OWNERDEAD:
      fmt::format_to(Iter, "Previous owner died."sv);
      break;
    case __WASI_ERRNO_PERM:
      fmt::format_to(Iter, "Operation not permitted."sv);
      break;
    case __WASI_ERRNO_PIPE:
      fmt::format_to(Iter, "Broken pipe."sv);
      break;
    case __WASI_ERRNO_PROTO:
      fmt::format_to(Iter, "Protocol error."sv);
      break;
    case __WASI_ERRNO_PROTONOSUPPORT:
      fmt::format_to(Iter, "Protocol not supported."sv);
      break;
    case __WASI_ERRNO_PROTOTYPE:
      fmt::format_to(Iter, "Protocol wrong type for socket."sv);
      break;
    case __WASI_ERRNO_RANGE:
      fmt::format_to(Iter, "Result too large."sv);
      break;
    case __WASI_ERRNO_ROFS:
      fmt::format_to(Iter, "Read-only file system."sv);
      break;
    case __WASI_ERRNO_SPIPE:
      fmt::format_to(Iter, "Invalid seek."sv);
      break;
    case __WASI_ERRNO_SRCH:
      fmt::format_to(Iter, "No such process."sv);
      break;
    case __WASI_ERRNO_STALE:
      fmt::format_to(Iter, "Reserved."sv);
      break;
    case __WASI_ERRNO_TIMEDOUT:
      fmt::format_to(Iter, "Connection timed out."sv);
      break;
    case __WASI_ERRNO_TXTBSY:
      fmt::format_to(Iter, "Text file busy."sv);
      break;
    case __WASI_ERRNO_XDEV:
      fmt::format_to(Iter, "Cross-device link."sv);
      break;
    case __WASI_ERRNO_NOTCAPABLE:
      fmt::format_to(Iter, "Extension: Capabilities insufficient."sv);
      break;
    case __WASI_ERRNO_AIADDRFAMILY:
      fmt::format_to(Iter,
                     "The specified network host does not have any network "
                     "addresses in the requested address family."sv);
      break;
    case __WASI_ERRNO_AIAGAIN:
      fmt::format_to(Iter, "Try again later."sv);
      break;
    case __WASI_ERRNO_AIBADFLAG:
      fmt::format_to(Iter, "Hints.ai_flags contains invalid flags"sv);
      break;
    case __WASI_ERRNO_AIFAIL:
      fmt::format_to(
          Iter, "The name server returned a permanent failure indication."sv);
      break;
    case __WASI_ERRNO_AIFAMILY:
      fmt::format_to(Iter, "The requested address family is not supported."sv);
      break;
    case __WASI_ERRNO_AIMEMORY:
      fmt::format_to(Iter, "Addrinfo out of memory."sv);
      break;
    case __WASI_ERRNO_AINODATA:
      fmt::format_to(Iter, "Network host exists, but does not have any network "
                           "addresses defined."sv);
      break;
    case __WASI_ERRNO_AINONAME:
      fmt::format_to(
          Iter,
          "Node or service is not known; or both node and service are NULL."sv);
      break;
    case __WASI_ERRNO_AISERVICE:
      fmt::format_to(
          Iter, "Service is not available for the requested socket type."sv);
      break;
    case __WASI_ERRNO_AISOCKTYPE:
      fmt::format_to(Iter, "The requested socket type is not supported."sv);
      break;
    case __WASI_ERRNO_AISYSTEM:
      fmt::format_to(Iter, "Other system error."sv);
      break;
    default:
      fmt::format_to(
          Iter, "Unknown error code {}"sv,
          static_cast<std::underlying_type_t<__wasi_errno_t>>(ErrNo));
      break;
    }
    return fmt::formatter<std::string_view>::format(
        std::string_view(Buffer.data(), Buffer.size()), Ctx);
  }
};
