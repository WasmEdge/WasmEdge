// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "common/log.h"
#include "executor/executor.h"
#include "host/preview2/wasi-sockets/api.h"
#include "host/preview2/wasi-sockets/resource_table.h"
#include "host/preview2/wasi-sockets/udp/wasifunc.h"
#include "host/preview2/wasi-sockets/util.h"
#include "host/wasi/environ.h"
#include "runtime/instance/memory.h"
#include <atomic>

namespace WasmEdge {
namespace Host {
namespace WasiSocket {
namespace UDP {
Expect<void> CreateUDPSocket::body(const Runtime::CallingFrame &Frame,
                                   int32_t AddressFamily, uint32_t RetBufPtr) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return WASI::WasiUnexpect(__WASI_ERRNO_FAULT);
  }

  auto RetBuf =
      MemInst->getPointer<__wasi_sockets_ret_t<__wasi_handle_t> *>(RetBufPtr);
  if (RetBuf == nullptr) {
    return WASI::WasiUnexpect(__WASI_ERRNO_FAULT);
  }

  __wasi_sockets_ip_address_family_t WasiAddressFamily;
  if (auto Res = cast<__wasi_sockets_ip_address_family_t>(AddressFamily);
      unlikely(!Res)) {
    return SocketsErr(*RetBuf, __WASI_SOCKETS_ERRNO_NOT_SUPPORTED);
  } else {
    WasiAddressFamily = *Res;
  }

  __wasi_fd_t SockFd;
  if (auto Res = Env.sockOpenPV2(WasiAddressFamily, __WASI_SOCKETS_TYPE_DGRAM);
      unlikely(!Res)) {
    return SocketsErr(*RetBuf, Res.error());
  } else {
    SockFd = *Res;
  }

  // On IPv6 sockets, IPV6_V6ONLY is enabled by default and can't be configured
  // otherwise.
  if (WasiAddressFamily == __WASI_SOCKETS_IP_ADDRESS_FAMILY_IPv6) {
    if (auto Res = Env.sockSetIPv6V6only(SockFd, true); unlikely(!Res)) {
      return SocketsErr(*RetBuf, Res.error());
    }
  }

  auto Handle =
      Env.Table.Push<UDPSocket>(ResourceTable::NullParent, SockFd,
                                WasiAddressFamily, SocketState::Default);
  RetBuf->Val.Res = Handle;
  return SocketsOk(*RetBuf);
}

Expect<void> StartBind::body(const Runtime::CallingFrame &Frame,
                             uint32_t Handle, uint32_t NetHandle,
                             __VARG_12_UINTS, uint32_t RetBufPtr) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return WASI::WasiUnexpect(__WASI_ERRNO_FAULT);
  }

  auto RetBuf = MemInst->getPointer<__wasi_sockets_ret_t<void> *>(RetBufPtr);
  if (RetBuf == nullptr) {
    return WASI::WasiUnexpect(__WASI_ERRNO_FAULT);
  }

  auto SocketData = Env.Table.GetById<UDPSocket>(Handle);
  if (SocketData == nullptr) {
    return SocketsErr(*RetBuf, __WASI_SOCKETS_ERRNO_ACCESS_DENIED);
  }

  auto NetworkData = Env.Table.GetById<Network::Network>(NetHandle);
  if (NetworkData == nullptr) {
    return SocketsErr(*RetBuf, __WASI_SOCKETS_ERRNO_ACCESS_DENIED);
  }

  __wasi_sockets_ip_socket_address_t IPSocketAddress;
  if (auto Res = PackIPSocketAddress(__V_12_UINTS); unlikely(!Res)) {
    return SocketsErr(*RetBuf, Res.error());
  } else {
    IPSocketAddress = *Res;
  }

  if (unlikely(!ValidateAddressFamily(IPSocketAddress))) {
    return SocketsErr(*RetBuf, __WASI_SOCKETS_ERRNO_INVALID_ARGUMENT);
  }

  switch (SocketData->State) {
  case SocketState::Default:
    break;
  case SocketState::BindStarted:
    return SocketsErr(*RetBuf, __WASI_SOCKETS_ERRNO_CONCURRENCY_CONFLICT);
  default:
    return SocketsErr(*RetBuf, __WASI_SOCKETS_ERRNO_INVALID_STATE);
  }

  SocketData->AddressChecker = NetworkData->GetChecker();
  if (unlikely(!SocketData->AddressChecker(Addr(IPSocketAddress)))) {
    return SocketsErr(*RetBuf, __WASI_SOCKETS_ERRNO_ACCESS_DENIED);
  }

  if (auto Res = Env.sockBindPV2(SocketData->Fd, IPSocketAddress);
      unlikely(!Res)) {
    if (Res.error() == __WASI_ERRNO_AFNOSUPPORT) {
      return SocketsErr(*RetBuf, __WASI_SOCKETS_ERRNO_INVALID_ARGUMENT);
    }
    return SocketsErr(*RetBuf, Res.error());
  } else {
    SocketData->State = SocketState::BindStarted;
    return SocketsOk(*RetBuf);
  }
}

Expect<void> FinishBind::body(const Runtime::CallingFrame &Frame,
                              uint32_t Handle, uint32_t RetBufPtr) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return WASI::WasiUnexpect(__WASI_ERRNO_FAULT);
  }

  auto RetBuf = MemInst->getPointer<__wasi_sockets_ret_t<void> *>(RetBufPtr);
  if (RetBuf == nullptr) {
    return WASI::WasiUnexpect(__WASI_ERRNO_FAULT);
  }

  auto SocketData = Env.Table.GetById<UDPSocket>(Handle);
  if (SocketData == nullptr) {
    return SocketsErr(*RetBuf, __WASI_SOCKETS_ERRNO_ACCESS_DENIED);
  }

  switch (SocketData->State) {
  case SocketState::BindStarted:
    break;
  default:
    return SocketsErr(*RetBuf, __WASI_SOCKETS_ERRNO_NOT_IN_PROGRESS);
  }

  SocketData->State = SocketState::Bound;
  return SocketsOk(*RetBuf);
}

Expect<void> Stream::body(const Runtime::CallingFrame &Frame, uint32_t Handle,
                          __VARG_12_UINTS, uint32_t RetBufPtr) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return WASI::WasiUnexpect(__WASI_ERRNO_FAULT);
  }

  auto RetBuf = MemInst->getPointer<
      __wasi_sockets_ret_t<__wasi_sockets_udp_incoming_datagram_stream_t> *>(
      RetBufPtr);
  if (RetBuf == nullptr) {
    return WASI::WasiUnexpect(__WASI_ERRNO_FAULT);
  }

  auto SocketData = Env.Table.GetById<UDPSocket>(Handle);
  if (SocketData == nullptr) {
    return SocketsErr(*RetBuf, __WASI_SOCKETS_ERRNO_ACCESS_DENIED);
  }

  if (auto Children = SocketData->GetChildren(); Children) {
    for (auto CHandle : *Children) {
      if (Env.Table.GetById<IncomingDatagramStream>(CHandle) ||
          Env.Table.GetById<OutgoingDatagramStream>(CHandle)) {
        // UDP streams not dropped yet
        return WASI::WasiUnexpect(__WASI_ERRNO_NOTSUP);
      }
    }
  }

  __wasi_sockets_ip_socket_address_t IPSocketAddress;
  if (auto Res = PackIPSocketAddress(__V_12_UINTS); unlikely(!Res)) {
    return SocketsErr(*RetBuf, Res.error());
  } else {
    IPSocketAddress = *Res;
  }

  switch (SocketData->State) {
  case SocketState::Bound:
  case SocketState::Connected:
    break;
  default:
    return SocketsErr(*RetBuf, __WASI_SOCKETS_ERRNO_INVALID_STATE);
  }

  // Disconnect first
  if (SocketData->State == SocketState::Connected) {
    if (auto Res = Env.sockUDPDisconnect(SocketData->Fd); unlikely(!Res)) {
      return SocketsErr(*RetBuf, Res.error());
    }
    SocketData->State = SocketState::Bound;
  }

  if (unlikely(!ValidateAddressFamily(IPSocketAddress))) {
    return SocketsErr(*RetBuf, __WASI_SOCKETS_ERRNO_INVALID_ARGUMENT);
  }

  if (unlikely(!SocketData->AddressChecker(Addr(IPSocketAddress)))) {
    return SocketsErr(*RetBuf, __WASI_SOCKETS_ERRNO_ACCESS_DENIED);
  }

  if (auto Res = Env.sockConnectPV2(SocketData->Fd, IPSocketAddress);
      unlikely(!Res)) {
    if (Res.error() == __WASI_ERRNO_AFNOSUPPORT) {
      return SocketsErr(*RetBuf, __WASI_SOCKETS_ERRNO_INVALID_ARGUMENT);
    } else if (Res.error() == __WASI_ERRNO_INPROGRESS) {
      return SocketsErr(*RetBuf, __WASI_SOCKETS_ERRNO_UNKNOW);
    }
    return SocketsErr(*RetBuf, Res.error());
  }

  SocketData->State = SocketState::Connected;

  RetBuf->Val.Res.InHandle = Env.Table.Push<IncomingDatagramStream>(
      Handle, SocketData->Fd, IPSocketAddress);
  RetBuf->Val.Res.OutHandle = Env.Table.Push<OutgoingDatagramStream>(
      Handle, SocketData->Fd, IPSocketAddress, SocketData->AddressFamily);
  return SocketsOk(*RetBuf);
}

Expect<void> LocalAddress::body(const Runtime::CallingFrame &Frame,
                                uint32_t Handle, uint32_t RetBufPtr) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return WASI::WasiUnexpect(__WASI_ERRNO_FAULT);
  }

  auto RetBuf = MemInst->getPointer<
      __wasi_sockets_ret_t<__wasi_sockets_ip_socket_address_t> *>(RetBufPtr);
  if (RetBuf == nullptr) {
    return WASI::WasiUnexpect(__WASI_ERRNO_FAULT);
  }

  auto SocketData = Env.Table.GetById<UDPSocket>(Handle);
  if (SocketData == nullptr) {
    return SocketsErr(*RetBuf, __WASI_SOCKETS_ERRNO_ACCESS_DENIED);
  }

  switch (SocketData->State) {
  case SocketState::Bound:
  case SocketState::Connected:
    break;
  case SocketState::BindStarted:
    return SocketsErr(*RetBuf, __WASI_SOCKETS_ERRNO_CONCURRENCY_CONFLICT);
  default:
    return SocketsErr(*RetBuf, __WASI_SOCKETS_ERRNO_INVALID_STATE);
  }

  if (auto Res = Env.sockGetLocalAddrPV2(SocketData->Fd, RetBuf->Val.Res);
      unlikely(!Res)) {
    return SocketsErr(*RetBuf, Res.error());
  }

  return SocketsOk(*RetBuf);
}

Expect<void> RemoteAddress::body(const Runtime::CallingFrame &Frame,
                                 uint32_t Handle, uint32_t RetBufPtr) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return WASI::WasiUnexpect(__WASI_ERRNO_FAULT);
  }

  auto RetBuf = MemInst->getPointer<
      __wasi_sockets_ret_t<__wasi_sockets_ip_socket_address_t> *>(RetBufPtr);
  if (RetBuf == nullptr) {
    return WASI::WasiUnexpect(__WASI_ERRNO_FAULT);
  }

  auto SocketData = Env.Table.GetById<UDPSocket>(Handle);
  if (SocketData == nullptr) {
    return SocketsErr(*RetBuf, __WASI_SOCKETS_ERRNO_ACCESS_DENIED);
  }

  switch (SocketData->State) {
  case SocketState::Connected:
    break;
  default:
    return SocketsErr(*RetBuf, __WASI_SOCKETS_ERRNO_INVALID_STATE);
  }

  if (auto Res = Env.sockGetPeerAddrPV2(SocketData->Fd, RetBuf->Val.Res);
      unlikely(!Res)) {
    return SocketsErr(*RetBuf, Res.error());
  }

  return SocketsOk(*RetBuf);
}

Expect<uint32_t> AddressFamily::body(const Runtime::CallingFrame &,
                                     uint32_t Handle) {
  auto SocketData = Env.Table.GetById<UDPSocket>(Handle);
  if (SocketData == nullptr) {
    return WASI::WasiUnexpect(__WASI_ERRNO_FAULT);
  } else {
    return uint32_t(SocketData->AddressFamily);
  }
}

// https://github.com/WebAssembly/wasi-sockets/pull/93
// IPv6 Only function deleted, but maybe turn back...
Expect<void> IPv6Only::body(const Runtime::CallingFrame &, uint32_t, uint32_t) {
  return WASI::WasiUnexpect(__WASI_ERRNO_NOSYS);
  /*auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return WASI::WasiUnexpect(__WASI_ERRNO_FAULT);
  }

  auto RetBuf = MemInst->getPointer<__wasi_sockets_ret_t<bool> *>(RetBufPtr);
  if (RetBuf == nullptr) {
    return WASI::WasiUnexpect(__WASI_ERRNO_FAULT);
  }

  auto SocketData = Env.Table.GetById<UDPSocket>(Handle);
  if (SocketData == nullptr) {
    return SocketsErr(*RetBuf, __WASI_SOCKETS_ERRNO_ACCESS_DENIED);
  }

  if (SocketData->AddressFamily != __WASI_SOCKETS_IP_ADDRESS_FAMILY_IPv6) {
    return SocketsErr(*RetBuf, __WASI_SOCKETS_ERRNO_NOT_SUPPORTED);
  }

  RetBuf->Val.Res = SocketData->IPv6Only;
  return SocketsOk(*RetBuf);*/
}

Expect<void> SetIPv6Only::body(const Runtime::CallingFrame &, uint32_t,
                               uint32_t, uint32_t) {
  return WASI::WasiUnexpect(__WASI_ERRNO_NOSYS);
  /*auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return WASI::WasiUnexpect(__WASI_ERRNO_FAULT);
  }

  auto RetBuf = MemInst->getPointer<__wasi_sockets_ret_t<void> *>(RetBufPtr);
  if (RetBuf == nullptr) {
    return WASI::WasiUnexpect(__WASI_ERRNO_FAULT);
  }

  auto SocketData = Env.Table.GetById<UDPSocket>(Handle);
  if (SocketData == nullptr) {
    return SocketsErr(*RetBuf, __WASI_SOCKETS_ERRNO_ACCESS_DENIED);
  }

  if (SocketData->AddressFamily != __WASI_SOCKETS_IP_ADDRESS_FAMILY_IPv6) {
    return SocketsErr(*RetBuf, __WASI_SOCKETS_ERRNO_NOT_SUPPORTED);
  }

  switch (SocketData->State) {
  case SocketState::Default:
    break;
  case SocketState::BindStarted:
    return SocketsErr(*RetBuf, __WASI_SOCKETS_ERRNO_CONCURRENCY_CONFLICT);
  default:
    return SocketsErr(*RetBuf, __WASI_SOCKETS_ERRNO_INVALID_STATE);
  }

  if (auto Res = Env.sockSetIPv6V6only(SocketData->Fd, Val); !Res) {
    return SocketsErr(*RetBuf, Res.error());
  }

  SocketData->IPv6Only = Val;
  return SocketsOk(*RetBuf);*/
}

Expect<void> UnicastHopLimit::body(const Runtime::CallingFrame &Frame,
                                   uint32_t Handle, uint32_t RetBufPtr) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return WASI::WasiUnexpect(__WASI_ERRNO_FAULT);
  }

  auto RetBuf = MemInst->getPointer<__wasi_sockets_ret_t<uint8_t> *>(RetBufPtr);
  if (RetBuf == nullptr) {
    return WASI::WasiUnexpect(__WASI_ERRNO_FAULT);
  }

  auto SocketData = Env.Table.GetById<UDPSocket>(Handle);
  if (SocketData == nullptr) {
    return SocketsErr(*RetBuf, __WASI_SOCKETS_ERRNO_ACCESS_DENIED);
  }

  if (SocketData->AddressFamily == __WASI_SOCKETS_IP_ADDRESS_FAMILY_IPv4) {
    if (auto Res = Env.sockGetIPTTL(SocketData->Fd); !Res) {
      return SocketsErr(*RetBuf, Res.error());
    } else {
      RetBuf->Val.Res = Res.value();
      return SocketsOk(*RetBuf);
    }
  } else if (SocketData->AddressFamily ==
             __WASI_SOCKETS_IP_ADDRESS_FAMILY_IPv6) {
    if (auto Res = Env.sockGetIPv6UnicastHops(SocketData->Fd); !Res) {
      return SocketsErr(*RetBuf, Res.error());
    } else {
      RetBuf->Val.Res = Res.value();
      return SocketsOk(*RetBuf);
    }
  } else {
    __builtin_unreachable();
  }
  return {};
}

Expect<void> SetUnicastHopLimit::body(const Runtime::CallingFrame &Frame,
                                      uint32_t Val, uint32_t Handle,
                                      uint32_t RetBufPtr) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return WASI::WasiUnexpect(__WASI_ERRNO_FAULT);
  }

  auto RetBuf = MemInst->getPointer<__wasi_sockets_ret_t<void> *>(RetBufPtr);
  if (RetBuf == nullptr) {
    return WASI::WasiUnexpect(__WASI_ERRNO_FAULT);
  }

  auto SocketData = Env.Table.GetById<UDPSocket>(Handle);
  if (SocketData == nullptr) {
    return SocketsErr(*RetBuf, __WASI_SOCKETS_ERRNO_ACCESS_DENIED);
  }

  if (SocketData->AddressFamily == __WASI_SOCKETS_IP_ADDRESS_FAMILY_IPv4) {
    if (auto Res = Env.sockSetIPTTL(SocketData->Fd, Val); !Res) {
      return SocketsErr(*RetBuf, Res.error());
    } else {
      return SocketsOk(*RetBuf);
    }
  } else if (SocketData->AddressFamily ==
             __WASI_SOCKETS_IP_ADDRESS_FAMILY_IPv6) {
    if (auto Res = Env.sockSetIPv6UnicastHops(SocketData->Fd, Val); !Res) {
      return SocketsErr(*RetBuf, Res.error());
    } else {
      return SocketsOk(*RetBuf);
    }
  } else {
    return SocketsErr(*RetBuf, __WASI_SOCKETS_ERRNO_NOT_SUPPORTED);
  }
}

Expect<void> GetReceiveBufferSize::body(const Runtime::CallingFrame &Frame,
                                        uint32_t Handle, uint32_t RetBufPtr) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return WASI::WasiUnexpect(__WASI_ERRNO_FAULT);
  }

  auto RetBuf =
      MemInst->getPointer<__wasi_sockets_ret_t<uint64_t> *>(RetBufPtr);
  if (RetBuf == nullptr) {
    return WASI::WasiUnexpect(__WASI_ERRNO_FAULT);
  }

  auto SocketData = Env.Table.GetById<UDPSocket>(Handle);
  if (SocketData == nullptr) {
    return SocketsErr(*RetBuf, __WASI_SOCKETS_ERRNO_ACCESS_DENIED);
  }

  if (auto Res = Env.sockGetRecvBufferSize(SocketData->Fd); !Res) {
    return SocketsErr(*RetBuf, Res.error());
  } else {
    RetBuf->Val.Res = Res.value();
    return SocketsOk(*RetBuf);
  }
}

Expect<void> SetReceiveBufferSize::body(const Runtime::CallingFrame &Frame,
                                        uint32_t Handle, uint64_t Val,
                                        uint32_t RetBufPtr) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return WASI::WasiUnexpect(__WASI_ERRNO_FAULT);
  }

  auto RetBuf = MemInst->getPointer<__wasi_sockets_ret_t<void> *>(RetBufPtr);
  if (RetBuf == nullptr) {
    return WASI::WasiUnexpect(__WASI_ERRNO_FAULT);
  }

  auto SocketData = Env.Table.GetById<UDPSocket>(Handle);
  if (SocketData == nullptr) {
    return SocketsErr(*RetBuf, __WASI_SOCKETS_ERRNO_ACCESS_DENIED);
  }

  if (auto Res = Env.sockSetRecvBufferSize(SocketData->Fd, Val); !Res) {
    return SocketsErr(*RetBuf, Res.error());
  } else {
    return SocketsOk(*RetBuf);
  }
}

Expect<void> GetSendBufferSize::body(const Runtime::CallingFrame &Frame,
                                     uint32_t Handle, uint32_t RetBufPtr) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return WASI::WasiUnexpect(__WASI_ERRNO_FAULT);
  }

  auto RetBuf =
      MemInst->getPointer<__wasi_sockets_ret_t<uint64_t> *>(RetBufPtr);
  if (RetBuf == nullptr) {
    return WASI::WasiUnexpect(__WASI_ERRNO_FAULT);
  }

  auto SocketData = Env.Table.GetById<UDPSocket>(Handle);
  if (SocketData == nullptr) {
    return SocketsErr(*RetBuf, __WASI_SOCKETS_ERRNO_ACCESS_DENIED);
  }

  if (auto Res = Env.sockGetSendBufferSize(SocketData->Fd); !Res) {
    return SocketsErr(*RetBuf, Res.error());
  } else {
    RetBuf->Val.Res = Res.value();
    return SocketsOk(*RetBuf);
  }
}

Expect<void> SetSendBufferSize::body(const Runtime::CallingFrame &Frame,
                                     uint32_t Handle, uint64_t Val,
                                     uint32_t RetBufPtr) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return WASI::WasiUnexpect(__WASI_ERRNO_FAULT);
  }

  auto RetBuf = MemInst->getPointer<__wasi_sockets_ret_t<void> *>(RetBufPtr);
  if (RetBuf == nullptr) {
    return WASI::WasiUnexpect(__WASI_ERRNO_FAULT);
  }

  auto SocketData = Env.Table.GetById<UDPSocket>(Handle);
  if (SocketData == nullptr) {
    return SocketsErr(*RetBuf, __WASI_SOCKETS_ERRNO_ACCESS_DENIED);
  }

  if (auto Res = Env.sockSetSendBufferSize(SocketData->Fd, Val); !Res) {
    return SocketsErr(*RetBuf, Res.error());
  } else {
    return SocketsOk(*RetBuf);
  }
  return {};
}

// FIXME
Expect<uint32_t> UDPSocketsSubscribe::body(const Runtime::CallingFrame &,
                                           uint32_t Handle) {
  auto SocketData = Env.Table.GetById<UDPSocket>(Handle);
  if (SocketData == nullptr) {
    return WASI::WasiUnexpect(__WASI_ERRNO_FAULT);
  }
  return 0;
}

Expect<void> UDPSocketsDrop::body(const Runtime::CallingFrame &,
                                  uint32_t Handle) {
  auto SocketData = Env.Table.GetById<UDPSocket>(Handle);
  if (SocketData == nullptr) {
    return {};
  }

  if (auto Res = Env.fdClose(SocketData->Fd); unlikely(!Res)) {
    return {};
  }

  Env.Table.Drop(Handle);
  return {};
}

Expect<void>
IncomingDatagramStreamReceive::body(const Runtime::CallingFrame &Frame,
                                    uint32_t Handle, uint64_t MaxResults,
                                    uint32_t RetBufPtr) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return WASI::WasiUnexpect(__WASI_ERRNO_FAULT);
  }

  auto RetBuf = MemInst->getPointer<__wasi_sockets_ret_t<void> *>(RetBufPtr);
  if (RetBuf == nullptr) {
    return WASI::WasiUnexpect(__WASI_ERRNO_FAULT);
  }

  auto InStreamData = Env.Table.GetById<IncomingDatagramStream>(Handle);
  if (InStreamData == nullptr) {
    return {};
  }

  std::vector<uint8_t> Buf(MaxResults);
  uint32_t NRead;
  if (auto Res = Env.sockRecvPV2(InStreamData->Fd, Buf, NRead);
      unlikely(!Res)) {
    return SocketsErr(*RetBuf, Res.error());
  }
  return {};
}

} // namespace UDP
} // namespace WasiSocket
} // namespace Host
} // namespace WasmEdge
