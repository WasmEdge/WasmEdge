// SPDX_License_Identifier: Apache-2.0
// SPDX_FileCopyrightText: 2019-2022 Second State INC

#pragma once
#include "common/variant.h"
#include <cstdint>
#include <wasi/api.hpp>

namespace WasmEdge {
namespace Host {

enum __wasi_sockets_errno_t : uint8_t {
  /// Unknown error
  __WASI_SOCKETS_ERRNO_UNKNOW,

  /// Access denied.
  ///
  /// POSIX equivalent: EACCES, EPERM
  __WASI_SOCKETS_ERRNO_ACCESS_DENIED,

  /// The operation is not supported.
  ///
  /// POSIX equivalent: EOPNOTSUPP
  __WASI_SOCKETS_ERRNO_NOT_SUPPORTED,

  /// One of the arguments is invalid.
  ///
  /// POSIX equivalent: EINVAL
  __WASI_SOCKETS_ERRNO_INVALID_ARGUMENT,

  /// Not enough memory to complete the operation.
  ///
  /// POSIX equivalent: ENOMEM, ENOBUFS, EAI_MEMORY
  __WASI_SOCKETS_ERRNO_OUT_OF_MEMORY,

  /// The operation timed out before it could finish completely.
  __WASI_SOCKETS_ERRNO_TIMEOUT,

  /// This operation is incompatible with another asynchronous operation that is
  /// already in progress.
  __WASI_SOCKETS_ERRNO_CONCURRENCY_CONFLICT,

  /// Trying to finish an asynchronous operation that:
  /// - has not been started yet, or:
  /// - was already finished by a previous `finish-*` call.
  ///
  /// Note: this is scheduled to be removed when `future`s are natively
  /// supported.
  __WASI_SOCKETS_ERRNO_NOT_IN_PROGRESS,

  /// The operation has been aborted because it could not be completed
  /// immediately.
  ///
  /// Note: this is scheduled to be removed when `future`s are natively
  /// supported.
  __WASI_SOCKETS_ERRNO_WOULD_BLOCK,

  // ### TCP & UDP SOCKET ERRORS ###

  /// The operation is not valid in the socket's current state.
  __WASI_SOCKETS_ERRNO_INVALID_STATE,

  /// A new socket resource could not be created because of a system limit.
  __WASI_SOCKETS_ERRNO_NEW_SOCKET_LIMIT,

  /// A bind operation failed because the provided address is not an address
  /// that the `network` can bind to.
  __WASI_SOCKETS_ERRNO_ADDRESS_NOT_BINDABLE,

  /// A bind operation failed because the provided address is already in use or
  /// because there are no ephemeral ports available.
  __WASI_SOCKETS_ERRNO_ADDRESS_IN_USE,

  /// The remote address is not reachable
  __WASI_SOCKETS_ERRNO_REMOTE_UNREACHABLE,

  // ### TCP SOCKET ERRORS ###

  /// The connection was forcefully rejected
  __WASI_SOCKETS_ERRNO_CONNECTION_REFUSED,

  /// The connection was reset.
  __WASI_SOCKETS_ERRNO_CONNECTION_RESET,

  /// The connection was aborted.
  __WASI_SOCKETS_ERRNO_CONNECTION_ABORTED,

  // ### UDP SOCKET ERRORS ###
  __WASI_SOCKETS_ERRNO_DATAGRAM_TOO_LARGE,

  // ### NAME LOOKUP ERRORS ###

  /// Name does not exist or has no suitable associated IP addresses.
  __WASI_SOCKETS_ERRNO_NAME_UNRESOLVABLE,

  /// A temporary failure in name resolution occurred.
  __WASI_SOCKETS_ERRNO_TEMPORARY_RESOLVER_FAILURE,

  /// A permanent failure in name resolution occurred.
  __WASI_SOCKETS_ERRNO_PERMANENT_RESOLVER_FAILURE,
};

using __wasi_handle_t = uint32_t;
using __wasi_udp_socket_handle_t = __wasi_handle_t;
using __wasi_tcp_socket_handle_t = __wasi_handle_t;
using __wasi_udp_own_incoming_datagram_stream_handle_t = __wasi_handle_t;
using __wasi_udp_own_outgoing_datagram_stream_handle_t = __wasi_handle_t;
using __wasi_tcp_own_incoming_datagram_stream_handle_t = __wasi_handle_t;
using __wasi_tcp_own_outgoing_datagram_stream_handle_t = __wasi_handle_t;
using __wasi_resolve_address_stream_handle_t = __wasi_handle_t;

template <typename T> struct __wasi_sockets_ret_t {
  bool IsErr;
  union {
    T Res;
    __wasi_sockets_errno_t Err;
  } Val;
};

template <> struct __wasi_sockets_ret_t<void> {
  bool IsErr;
  union {
    __wasi_sockets_errno_t Err;
  } Val;
};

enum __wasi_sockets_ip_address_family_t : uint8_t {
  __WASI_SOCKETS_IP_ADDRESS_FAMILY_IPv4 = 0,
  __WASI_SOCKETS_IP_ADDRESS_FAMILY_IPv6 = 1
};

enum __wasi_sockets_type_t : uint8_t {
  __WASI_SOCKETS_TYPE_DGRAM = 1,
  __WASI_SOCKETS_TYPE_STREAM = 2
};

struct __wasi_sockets_ipv4_address_t {
  uint8_t Addr[4];
};
static_assert(sizeof(__wasi_sockets_ipv4_address_t) == 4,
              "__wasi_sockets_ipv4_address size");
struct __wasi_sockets_ipv6_address_t {
  uint16_t Addr[8];
};
static_assert(sizeof(__wasi_sockets_ipv6_address_t) == 16,
              "__wasi_sockets_ipv6_address size");

struct __wasi_sockets_ip_address_t {
  __wasi_sockets_ip_address_family_t AddressFamily;
  union {
    __wasi_sockets_ipv4_address_t IPv4;
    __wasi_sockets_ipv6_address_t IPv6;
  } Val;
};

struct __wasi_sockets_ipv4_socket_address_t {
  uint16_t Port;
  __wasi_sockets_ipv4_address_t Address;
};

struct __wasi_sockets_ipv6_socket_address_t {
  uint16_t Port;
  uint32_t FlowInfo;
  __wasi_sockets_ipv6_address_t Address;
  uint32_t ScopeId;
};

struct __wasi_sockets_ip_socket_address_t {
  __wasi_sockets_ip_address_family_t AddressFamily;
  union {
    __wasi_sockets_ipv4_socket_address_t IPv4;
    __wasi_sockets_ipv6_socket_address_t IPv6;
  } Val;
};

struct __wasi_sockets_udp_incoming_datagram_stream_t {
  __wasi_udp_own_incoming_datagram_stream_handle_t InHandle;
  __wasi_udp_own_outgoing_datagram_stream_handle_t OutHandle;
};

struct __wasi_sockets_tcp_incoming_datagram_stream_t {
  __wasi_tcp_own_incoming_datagram_stream_handle_t InHandle;
  __wasi_tcp_own_outgoing_datagram_stream_handle_t OutHandle;
};

struct __wasi_sockets_tcp_accept_tuple_t {
  __wasi_tcp_socket_handle_t Socket;
  __wasi_tcp_own_incoming_datagram_stream_handle_t InHandle;
  __wasi_tcp_own_outgoing_datagram_stream_handle_t OutHandle;
};

struct __wasi_sockets_udp_datagram {};
} // namespace Host
} // namespace WasmEdge
