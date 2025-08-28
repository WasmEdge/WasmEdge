// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "host/preview2/wasi-sockets/util.h"
#include "host/preview2/wasi-sockets/api.h"
#include "host/preview2/wasi-sockets/variant_helper.h"
#include "host/wasi/error.h"

#include <tuple>

namespace WasmEdge {
namespace Host {
namespace WasiSocket {

WASI::WasiExpect<__wasi_sockets_ip_socket_address_t>
PackIPSocketAddress(__VARG_12_UINTS) {
  __wasi_sockets_ip_socket_address_t Res{};
  switch (V1) {
  case 0: // IPv4
    Res.AddressFamily = __WASI_SOCKETS_IP_ADDRESS_FAMILY_IPv4;
    Res.Val.IPv4.Port = V2;
    Res.Val.IPv4.Address.Addr[0] = V3;
    Res.Val.IPv4.Address.Addr[1] = V4;
    Res.Val.IPv4.Address.Addr[2] = V5;
    Res.Val.IPv4.Address.Addr[3] = V6;
    break;
  case 1: // IPv6
    Res.AddressFamily = __WASI_SOCKETS_IP_ADDRESS_FAMILY_IPv6;
    Res.Val.IPv6.Port = V2;
    Res.Val.IPv6.FlowInfo = V3;
    Res.Val.IPv6.Address.Addr[0] = V4;
    Res.Val.IPv6.Address.Addr[1] = V5;
    Res.Val.IPv6.Address.Addr[2] = V6;
    Res.Val.IPv6.Address.Addr[3] = V7;
    Res.Val.IPv6.Address.Addr[4] = V8;
    Res.Val.IPv6.Address.Addr[5] = V9;
    Res.Val.IPv6.Address.Addr[6] = V10;
    Res.Val.IPv6.Address.Addr[7] = V11;
    Res.Val.IPv6.ScopeId = V12;
    break;
  default:
    return WASI::WasiUnexpect(__WASI_ERRNO_INVAL);
  }
  return Res;
}

bool ValidateAddressFamily(const __wasi_sockets_ip_socket_address_t &Address) {
  if (Address.AddressFamily == __WASI_SOCKETS_IP_ADDRESS_FAMILY_IPv4) {
    return true;
  } else if (Address.AddressFamily == __WASI_SOCKETS_IP_ADDRESS_FAMILY_IPv6) {
    // reject IPv4-*compatible* IPv6 addresses.
    // TODO
    return true;
  }
  return false;
}

__wasi_sockets_ip_address_t
Addr(const __wasi_sockets_ip_socket_address_t &SocketAddr) {
  __wasi_sockets_ip_address_t Res;
  Res.AddressFamily = SocketAddr.AddressFamily;
  if (SocketAddr.AddressFamily == __WASI_SOCKETS_IP_ADDRESS_FAMILY_IPv4) {
    Res.Val.IPv4 = SocketAddr.Val.IPv4.Address;
  } else if (SocketAddr.AddressFamily ==
             __WASI_SOCKETS_IP_ADDRESS_FAMILY_IPv6) {
    Res.Val.IPv6 = SocketAddr.Val.IPv6.Address;
  } else {
    __builtin_unreachable();
  }
  return Res;
}
} // namespace WasiSocket
} // namespace Host
} // namespace WasmEdge
