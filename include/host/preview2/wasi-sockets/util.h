// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC
#pragma once

#include "common/expected.h"
#include "host/preview2/wasi-sockets/api.h"
#include "host/preview2/wasi-sockets/variant_helper.h"
#include "host/wasi/environ.h"
#include "host/wasi/error.h"

#include <limits>
#include <numeric>
#include <type_traits>

namespace WasmEdge {
namespace Host {
namespace WasiSocket {

template <typename T> struct WasiRawType {
  using Type = std::underlying_type_t<T>;
};
template <> struct WasiRawType<uint8_t> {
  using Type = uint8_t;
};
template <> struct WasiRawType<uint16_t> {
  using Type = uint16_t;
};
template <> struct WasiRawType<uint32_t> {
  using Type = uint32_t;
};
template <> struct WasiRawType<uint64_t> {
  using Type = uint64_t;
};

template <typename T> using WasiRawTypeT = typename WasiRawType<T>::Type;

template <typename T> WASI::WasiExpect<T> cast(uint64_t) noexcept;

template <>
WASI::WasiExpect<__wasi_sockets_ip_address_family_t>
cast<__wasi_sockets_ip_address_family_t>(uint64_t Family) noexcept {
  switch (WasiRawTypeT<__wasi_sockets_ip_address_family_t>(Family)) {
  case __WASI_SOCKETS_IP_ADDRESS_FAMILY_IPv4:
    return __WASI_SOCKETS_IP_ADDRESS_FAMILY_IPv4;
  case __WASI_SOCKETS_IP_ADDRESS_FAMILY_IPv6:
    return __WASI_SOCKETS_IP_ADDRESS_FAMILY_IPv6;
  default:
    return WASI::WasiUnexpect(__WASI_ERRNO_INVAL);
  }
}

// TODO: fill this
__wasi_sockets_errno_t toSockErrCode(__wasi_errno_t Err) {
  switch (Err) {
  case __WASI_ERRNO_AGAIN:
  case __WASI_ERRNO_INTR:
    // case __WASI_ERRNO_WOULDBLOCK:
    return __WASI_SOCKETS_ERRNO_WOULD_BLOCK;
  case __WASI_ERRNO_PERM:
  case __WASI_ERRNO_ACCES:
    return __WASI_SOCKETS_ERRNO_ACCESS_DENIED;
  case __WASI_ERRNO_ADDRINUSE:
    return __WASI_SOCKETS_ERRNO_ADDRESS_IN_USE;
  case __WASI_ERRNO_ADDRNOTAVAIL:
    return __WASI_SOCKETS_ERRNO_ADDRESS_NOT_BINDABLE;
  case __WASI_ERRNO_ALREADY:
    return __WASI_SOCKETS_ERRNO_CONCURRENCY_CONFLICT;
  case __WASI_ERRNO_TIMEDOUT:
    return __WASI_SOCKETS_ERRNO_TIMEOUT;
  case __WASI_ERRNO_CONNREFUSED:
    return __WASI_SOCKETS_ERRNO_CONNECTION_REFUSED;
  case __WASI_ERRNO_CONNRESET:
    return __WASI_SOCKETS_ERRNO_CONNECTION_RESET;
  case __WASI_ERRNO_CONNABORTED:
    return __WASI_SOCKETS_ERRNO_CONNECTION_REFUSED;
  case __WASI_ERRNO_INVAL:
    return __WASI_SOCKETS_ERRNO_INVALID_ARGUMENT;
  case __WASI_ERRNO_HOSTUNREACH:
  // case __WASI_ERRNO_HOSTDOWN:
  case __WASI_ERRNO_NETDOWN:
  case __WASI_ERRNO_NETUNREACH:
  case __WASI_ERRNO_NOENT:
    return __WASI_SOCKETS_ERRNO_REMOTE_UNREACHABLE;
  case __WASI_ERRNO_ISCONN:
  case __WASI_ERRNO_NOTCONN:
  case __WASI_ERRNO_DESTADDRREQ:
    return __WASI_SOCKETS_ERRNO_INVALID_STATE;
  case __WASI_ERRNO_NFILE:
  case __WASI_ERRNO_MFILE:
    return __WASI_SOCKETS_ERRNO_NEW_SOCKET_LIMIT;
  case __WASI_ERRNO_MSGSIZE:
    return __WASI_SOCKETS_ERRNO_DATAGRAM_TOO_LARGE;
  case __WASI_ERRNO_NOMEM:
  case __WASI_ERRNO_NOBUFS:
    return __WASI_SOCKETS_ERRNO_OUT_OF_MEMORY;
  // case __WASI_ERRNO_OPNOTSUPP:
  case __WASI_ERRNO_NOPROTOOPT:
  case __WASI_ERRNO_NOTSUP:
  // case __WASI_ERRNO_PFNOSUPPORT:
  case __WASI_ERRNO_PROTONOSUPPORT:
  case __WASI_ERRNO_PROTOTYPE:
  case __WASI_ERRNO_AFNOSUPPORT:
    return __WASI_SOCKETS_ERRNO_NOT_SUPPORTED;
  default:
    return __WASI_SOCKETS_ERRNO_UNKNOW;
  }
}

WASI::WasiExpect<__wasi_sockets_ip_socket_address_t>
    PackIPSocketAddress(__VARG_12_UINTS);
bool ValidateAddressFamily(const __wasi_sockets_ip_socket_address_t &);

template <typename T>
Expect<void> SocketsErr(__wasi_sockets_ret_t<T> &Buf,
                        __wasi_sockets_errno_t SocketErrcode) {
  Buf.IsErr = true;
  Buf.Val.Err = SocketErrcode;
  return {};
}

template <typename T>
Expect<void> SocketsErr(__wasi_sockets_ret_t<T> &Buf,
                        __wasi_errno_t WasiErrcode) {
  Buf.IsErr = true;
  Buf.Val.Err = toSockErrCode(WasiErrcode);
  return {};
}

template <typename T> Expect<void> SocketsOk(__wasi_sockets_ret_t<T> &Buf) {
  Buf.IsErr = false;
  return {};
}

__wasi_sockets_ip_address_t
Addr(const __wasi_sockets_ip_socket_address_t &SocketAddr);
} // namespace WasiSocket
} // namespace Host
} // namespace WasmEdge
