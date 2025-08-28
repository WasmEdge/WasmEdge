// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "common/log.h"
#include "executor/executor.h"
#include "host/preview2/wasi-sockets/api.h"
#include "host/preview2/wasi-sockets/resource_table.h"
#include "host/preview2/wasi-sockets/tcp/wasifunc.h"
#include "host/preview2/wasi-sockets/util.h"
#include "host/wasi/environ.h"
#include "runtime/instance/memory.h"
#include <atomic>

namespace WasmEdge {
namespace Host {
namespace WasiSocket {
namespace TCP {
Expect<void> CreateTCPSocket::body(const Runtime::CallingFrame &Frame,
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
  if (auto Res = Env.sockOpenPV2(WasiAddressFamily, __WASI_SOCKETS_TYPE_STREAM);
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
      Env.Table.Push<TCPSocket>(ResourceTable::NullParent, SockFd,
                                WasiAddressFamily, TCPSocketState::Default);
  RetBuf->Val.Res = Handle;
  return SocketsOk(*RetBuf);
}

// FIXME
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

  auto SocketData = Env.Table.GetById<TCPSocket>(Handle);
  if (SocketData == nullptr) {
    return SocketsErr(*RetBuf, __WASI_SOCKETS_ERRNO_ACCESS_DENIED);
  }

  auto NetworkData = Env.Table.GetById<Network::Network>(NetHandle);
  if (SocketData == nullptr) {
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

  // TODO: use NetworkData check Address
  (void)NetworkData;

  switch (SocketData->State) {
  case TCPSocketState::Default:
    SocketData->State = TCPSocketState::BindStarted;
    break;
  case TCPSocketState::BindStarted:
    return SocketsErr(*RetBuf, __WASI_SOCKETS_ERRNO_CONCURRENCY_CONFLICT);
  default:
    return SocketsErr(*RetBuf, __WASI_SOCKETS_ERRNO_INVALID_STATE);
  }

  if (auto Res = Env.sockBindPV2(SocketData->Fd, IPSocketAddress); !Res) {
    if (Res.error() == __WASI_ERRNO_AFNOSUPPORT) {
      return SocketsErr(*RetBuf, __WASI_SOCKETS_ERRNO_INVALID_ARGUMENT);
    } else {
      return SocketsErr(*RetBuf, Res.error());
    }
  }

  return SocketsOk(*RetBuf);
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

  auto SocketData = Env.Table.GetById<TCPSocket>(Handle);
  if (SocketData == nullptr) {
    return SocketsErr(*RetBuf, __WASI_SOCKETS_ERRNO_ACCESS_DENIED);
  }

  switch (SocketData->State) {
  case TCPSocketState::BindStarted:
    SocketData->State = TCPSocketState::Bound;
    return SocketsOk(*RetBuf);
  default:
    return SocketsErr(*RetBuf, __WASI_SOCKETS_ERRNO_NOT_IN_PROGRESS);
  }
}

Expect<void> StartConnect::body(const Runtime::CallingFrame &Frame,
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

  auto SocketData = Env.Table.GetById<TCPSocket>(Handle);
  if (SocketData == nullptr) {
    return SocketsErr(*RetBuf, __WASI_SOCKETS_ERRNO_ACCESS_DENIED);
  }

  auto NetworkData = Env.Table.GetById<Network::Network>(NetHandle);
  if (SocketData == nullptr) {
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

  // TODO: use NetworkData check Address
  (void)NetworkData;

  switch (SocketData->State) {
  case TCPSocketState::Default:
    break;
  case TCPSocketState::BindStarted:
  case TCPSocketState::Connecting:
  case TCPSocketState::ConnectReady:
  case TCPSocketState::ListenStarted:
    return SocketsErr(*RetBuf, __WASI_SOCKETS_ERRNO_CONCURRENCY_CONFLICT);
  default:
    return SocketsErr(*RetBuf, __WASI_SOCKETS_ERRNO_INVALID_STATE);
  }

  /*if (auto Res =
          Env.sockConnect(SocketData->Fd, SocketData->AddressFamily, Buf, Port);
      !Res) {
    if (Res.error() == __WASI_ERRNO_INPROGRESS) {
      SocketData->State = TCPSocketState::Connecting;
      RetBuf->IsErr = false;
    } else if (Res.error() == __WASI_ERRNO_AFNOSUPPORT) {
      RetBuf->IsErr = true;
      RetBuf->Val.Err = __WASI_SOCKETS_ERRNO_INVALID_ARGUMENT;
    } else {
      RetBuf->IsErr = true;
      RetBuf->Val.Err = toSockErrCode(Res.error());
    }
  } else {
    SocketData->State = TCPSocketState::ConnectReady;
    RetBuf->IsErr = false;
  }*/

  return {};
}

// FIXME
Expect<void> FinishConnect::body(const Runtime::CallingFrame &Frame,
                                 uint32_t Handle, uint32_t RetBufPtr) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return WASI::WasiUnexpect(__WASI_ERRNO_FAULT);
  }

  auto RetBuf = MemInst->getPointer<
      __wasi_sockets_ret_t<__wasi_sockets_tcp_incoming_datagram_stream_t> *>(
      RetBufPtr);
  if (RetBuf == nullptr) {
    return WASI::WasiUnexpect(__WASI_ERRNO_FAULT);
  }

  auto SocketData = Env.Table.GetById<TCPSocket>(Handle);
  if (SocketData == nullptr) {
    return SocketsErr(*RetBuf, __WASI_SOCKETS_ERRNO_ACCESS_DENIED);
  }

  switch (SocketData->State) {
  case TCPSocketState::ConnectReady:
    break;
  case TCPSocketState::Connecting: {
    // Do a test check state
    // TODO: it need Poll in wasi io....
    break;
  }
  default:
    return SocketsErr(*RetBuf, __WASI_SOCKETS_ERRNO_INVALID_STATE);
  }

  SocketData->State = TCPSocketState::Connected;
  RetBuf->Val.Res.InHandle = 0;
  RetBuf->Val.Res.OutHandle = 0;
  return SocketsOk(*RetBuf);
}

Expect<void> StartListen::body(const Runtime::CallingFrame &Frame,
                               uint32_t Handle, uint32_t RetBufPtr) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return WASI::WasiUnexpect(__WASI_ERRNO_FAULT);
  }

  auto RetBuf = MemInst->getPointer<__wasi_sockets_ret_t<void> *>(RetBufPtr);
  if (RetBuf == nullptr) {
    return WASI::WasiUnexpect(__WASI_ERRNO_FAULT);
  }

  auto SocketData = Env.Table.GetById<TCPSocket>(Handle);
  if (SocketData == nullptr) {
    return SocketsErr(*RetBuf, __WASI_SOCKETS_ERRNO_ACCESS_DENIED);
  }

  switch (SocketData->State) {
  case TCPSocketState::Bound:
    break;
  case TCPSocketState::ListenStarted:
  case TCPSocketState::Connecting:
  case TCPSocketState::ConnectReady:
  case TCPSocketState::BindStarted:
    return SocketsErr(*RetBuf, __WASI_SOCKETS_ERRNO_CONCURRENCY_CONFLICT);
  default:
    return SocketsErr(*RetBuf, __WASI_SOCKETS_ERRNO_INVALID_STATE);
  }

  if (auto Res = Env.sockListen(SocketData->Fd, SocketData->ListenBacklogSize);
      !Res) {
    return SocketsErr(*RetBuf, Res.error());
  }
  SocketData->State = TCPSocketState::ListenStarted;
  return SocketsOk(*RetBuf);
}

Expect<void> FinishListen::body(const Runtime::CallingFrame &Frame,
                                uint32_t Handle, uint32_t RetBufPtr) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return WASI::WasiUnexpect(__WASI_ERRNO_FAULT);
  }

  auto RetBuf = MemInst->getPointer<__wasi_sockets_ret_t<void> *>(RetBufPtr);
  if (RetBuf == nullptr) {
    return WASI::WasiUnexpect(__WASI_ERRNO_FAULT);
  }

  auto SocketData = Env.Table.GetById<TCPSocket>(Handle);
  if (SocketData == nullptr) {
    return SocketsErr(*RetBuf, __WASI_SOCKETS_ERRNO_ACCESS_DENIED);
  }

  switch (SocketData->State) {
  case TCPSocketState::ListenStarted:
    break;
  default:
    return SocketsErr(*RetBuf, __WASI_SOCKETS_ERRNO_NOT_IN_PROGRESS);
  }

  SocketData->State = TCPSocketState::Listening;
  return SocketsOk(*RetBuf);
}

// FIXME
Expect<void> Accept::body(const Runtime::CallingFrame &Frame, uint32_t Handle,
                          uint32_t RetBufPtr) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return WASI::WasiUnexpect(__WASI_ERRNO_FAULT);
  }

  auto RetBuf = MemInst->getPointer<
      __wasi_sockets_ret_t<__wasi_sockets_tcp_accept_tuple_t> *>(RetBufPtr);
  if (RetBuf == nullptr) {
    return WASI::WasiUnexpect(__WASI_ERRNO_FAULT);
  }

  auto SocketData = Env.Table.GetById<TCPSocket>(Handle);
  if (SocketData == nullptr) {
    return SocketsErr(*RetBuf, __WASI_SOCKETS_ERRNO_ACCESS_DENIED);
  }

  switch (SocketData->State) {
  case TCPSocketState::Listening:
    break;
  default:
    return SocketsErr(*RetBuf, __WASI_SOCKETS_ERRNO_INVALID_STATE);
  }

  int32_t RemoteSockFd = 0;
  if (auto Res = Env.sockAccept(SocketData->Fd, __WASI_FDFLAGS_NONBLOCK);
      !Res) {
    return SocketsErr(*RetBuf, Res.error());
  } else {
    RemoteSockFd = Res.value();
  }

  // TODO: get RemoteSockFd's address family
  RetBuf->Val.Res.Socket = Env.Table.Push<TCPSocket>(
      ResourceTable::NullParent, RemoteSockFd,
      /*FIXME*/ SocketData->AddressFamily, TCPSocketState::Connected);
  RetBuf->Val.Res.InHandle = 0;
  RetBuf->Val.Res.OutHandle = 0;
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

  auto SocketData = Env.Table.GetById<TCPSocket>(Handle);
  if (SocketData == nullptr) {
    return SocketsErr(*RetBuf, __WASI_SOCKETS_ERRNO_ACCESS_DENIED);
  }

  switch (SocketData->State) {
  case TCPSocketState::Default:
    return SocketsErr(*RetBuf, __WASI_SOCKETS_ERRNO_INVALID_STATE);
  case TCPSocketState::Connecting:
  case TCPSocketState::ConnectReady:
    return SocketsErr(*RetBuf, __WASI_SOCKETS_ERRNO_CONCURRENCY_CONFLICT);
  default:
    break;
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

  auto SocketData = Env.Table.GetById<TCPSocket>(Handle);
  if (SocketData == nullptr) {
    return SocketsErr(*RetBuf, __WASI_SOCKETS_ERRNO_ACCESS_DENIED);
  }

  switch (SocketData->State) {
  case TCPSocketState::Connected:
    break;
  case TCPSocketState::Connecting:
  case TCPSocketState::ConnectReady:
    return SocketsErr(*RetBuf, __WASI_SOCKETS_ERRNO_CONCURRENCY_CONFLICT);
  default:
    return SocketsErr(*RetBuf, __WASI_SOCKETS_ERRNO_INVALID_STATE);
  }

  if (auto Res = Env.sockGetPeerAddrPV2(SocketData->Fd, RetBuf->Val.Res);
      unlikely(!Res)) {
    return SocketsErr(*RetBuf, Res.error());
  }

  return SocketsOk(*RetBuf);
}

Expect<uint32_t> Islistening::body(const Runtime::CallingFrame &,
                                   uint32_t Handle) {
  auto SocketData = Env.Table.GetById<TCPSocket>(Handle);
  if (SocketData == nullptr) {
    WASI::WasiUnexpect(__WASI_ERRNO_FAULT);
  }

  return SocketData->State == TCPSocketState::Listening ? 1 : 0;
}

Expect<uint32_t> AddressFamily::body(const Runtime::CallingFrame &,
                                     uint32_t Handle) {
  auto SocketData = Env.Table.GetById<TCPSocket>(Handle);
  if (SocketData == nullptr) {
    WASI::WasiUnexpect(__WASI_ERRNO_FAULT);
  }

  return SocketData->AddressFamily;
}

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

  auto SocketData = Env.Table.GetById<TCPSocket>(Handle);
  if (SocketData == nullptr) {
    return SocketsErr(*RetBuf, __WASI_SOCKETS_ERRNO_ACCESS_DENIED);
  }

  if (SocketData->AddressFamily == __WASI_SOCKETS_IP_ADDRESS_FAMILY_IPv6) {
    RetBuf->IsErr = false;
    RetBuf->Val.Res = SocketData->IPv6Only;
  } else {
    RetBuf->IsErr = true;
    RetBuf->Val.Err = __WASI_SOCKETS_ERRNO_NOT_SUPPORTED;
  }
  return {};*/
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

  auto SocketData = Env.Table.GetById<TCPSocket>(Handle);
  if (SocketData == nullptr) {
    return SocketsErr(*RetBuf, __WASI_SOCKETS_ERRNO_ACCESS_DENIED);
  }

  if (SocketData->AddressFamily != __WASI_SOCKETS_IP_ADDRESS_FAMILY_IPv6) {
    RetBuf->IsErr = true;
    RetBuf->Val.Err = __WASI_SOCKETS_ERRNO_NOT_SUPPORTED;
    return {};
  }

  switch (SocketData->State) {
  case TCPSocketState::Default:
    break;
  case TCPSocketState::BindStarted:
    RetBuf->IsErr = true;
    RetBuf->Val.Err = __WASI_SOCKETS_ERRNO_CONCURRENCY_CONFLICT;
    return {};
  default:
    RetBuf->IsErr = true;
    RetBuf->Val.Err = __WASI_SOCKETS_ERRNO_INVALID_STATE;
    return {};
  }

  if (auto Res = Env.sockSetIPv6V6only(SocketData->Fd, Val); !Res) {
    RetBuf->IsErr = true;
    RetBuf->Val.Err = toSockErrCode(Res.error());
  } else {
    RetBuf->IsErr = false;
    SocketData->IPv6Only = Val;
  }
  return {};*/
}

Expect<void> SetListenBacklogSize::body(const Runtime::CallingFrame &Frame,
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

  auto SocketData = Env.Table.GetById<TCPSocket>(Handle);
  if (SocketData == nullptr) {
    return SocketsErr(*RetBuf, __WASI_SOCKETS_ERRNO_ACCESS_DENIED);
  }

  switch (SocketData->State) {
  case TCPSocketState::Default:
  case TCPSocketState::BindStarted:
  case TCPSocketState::Bound:
  case TCPSocketState::Listening:
    break;
  case TCPSocketState::Connecting:
  case TCPSocketState::ConnectReady:
  case TCPSocketState::ListenStarted:
    return SocketsErr(*RetBuf, __WASI_SOCKETS_ERRNO_CONCURRENCY_CONFLICT);
  default:
    return SocketsErr(*RetBuf, __WASI_SOCKETS_ERRNO_INVALID_STATE);
  }

  if (Val == 0) {
    return SocketsErr(*RetBuf, __WASI_SOCKETS_ERRNO_INVALID_ARGUMENT);
  }

  Val = std::min<uint64_t>(Val, INT32_MAX);

  SocketData->ListenBacklogSize = Val;
  // TODO: Update it if TCPSocketState is Listening. it maybe fail, but it is
  // fine
  return SocketsOk(*RetBuf);
}

Expect<void> KeepAliveEnabled::body(const Runtime::CallingFrame &Frame,
                                    uint32_t Handle, uint32_t RetBufPtr) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return WASI::WasiUnexpect(__WASI_ERRNO_FAULT);
  }

  auto RetBuf = MemInst->getPointer<__wasi_sockets_ret_t<bool> *>(RetBufPtr);
  if (RetBuf == nullptr) {
    return WASI::WasiUnexpect(__WASI_ERRNO_FAULT);
  }

  auto SocketData = Env.Table.GetById<TCPSocket>(Handle);
  if (SocketData == nullptr) {
    return SocketsErr(*RetBuf, __WASI_SOCKETS_ERRNO_ACCESS_DENIED);
  }

  if (auto Res = Env.sockGetKeepAlive(SocketData->Fd); !Res) {
    return SocketsErr(*RetBuf, Res.error());
  } else {
    RetBuf->Val.Res = !!Res.value();
  }
  return SocketsOk(*RetBuf);
}

Expect<void> SetKeepAliveEnabled::body(const Runtime::CallingFrame &Frame,
                                       uint32_t Handle, uint32_t Val,
                                       uint32_t RetBufPtr) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return WASI::WasiUnexpect(__WASI_ERRNO_FAULT);
  }

  auto RetBuf = MemInst->getPointer<__wasi_sockets_ret_t<void> *>(RetBufPtr);
  if (RetBuf == nullptr) {
    return WASI::WasiUnexpect(__WASI_ERRNO_FAULT);
  }

  auto SocketData = Env.Table.GetById<TCPSocket>(Handle);
  if (SocketData == nullptr) {
    return SocketsErr(*RetBuf, __WASI_SOCKETS_ERRNO_ACCESS_DENIED);
  }

  if (SocketData->AddressFamily != __WASI_SOCKETS_IP_ADDRESS_FAMILY_IPv6) {
    return SocketsErr(*RetBuf, __WASI_SOCKETS_ERRNO_NOT_SUPPORTED);
  }

  if (auto Res = Env.sockSetKeepAlive(SocketData->Fd, Val); !Res) {
    return SocketsErr(*RetBuf, Res.error());
  }
  return SocketsOk(*RetBuf);
}

Expect<void> KeepAliveIdleTime::body(const Runtime::CallingFrame &Frame,
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

  auto SocketData = Env.Table.GetById<TCPSocket>(Handle);
  if (SocketData == nullptr) {
    return SocketsErr(*RetBuf, __WASI_SOCKETS_ERRNO_ACCESS_DENIED);
  }

  if (auto Res = Env.sockGetKeepIdle(SocketData->Fd); !Res) {
    return SocketsErr(*RetBuf, Res.error());
  } else {
    RetBuf->Val.Res = std::chrono::duration_cast<std::chrono::nanoseconds>(
                          std::chrono::seconds(Res.value()))
                          .count();
  }
  return SocketsOk(*RetBuf);
}

Expect<void> SetKeepAliveIdleTime::body(const Runtime::CallingFrame &Frame,
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

  auto SocketData = Env.Table.GetById<TCPSocket>(Handle);
  if (SocketData == nullptr) {
    return SocketsErr(*RetBuf, __WASI_SOCKETS_ERRNO_ACCESS_DENIED);
  }

  uint32_t Sec = std::chrono::duration_cast<std::chrono::seconds>(
                     std::chrono::nanoseconds(Val))
                     .count();
  if (auto Res = Env.sockSetKeepIdle(SocketData->Fd, Sec); !Res) {
    return SocketsErr(*RetBuf, Res.error());
  }
  return SocketsOk(*RetBuf);
}

Expect<void> KeepAliveInterval::body(const Runtime::CallingFrame &Frame,
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

  auto SocketData = Env.Table.GetById<TCPSocket>(Handle);
  if (SocketData == nullptr) {
    return SocketsErr(*RetBuf, __WASI_SOCKETS_ERRNO_ACCESS_DENIED);
  }

  if (auto Res = Env.sockGetAliveInterval(SocketData->Fd); !Res) {
    return SocketsErr(*RetBuf, Res.error());
  } else {
    RetBuf->Val.Res = std::chrono::duration_cast<std::chrono::nanoseconds>(
                          std::chrono::seconds(Res.value()))
                          .count();
  }
  return SocketsOk(*RetBuf);
}

Expect<void> SetKeepAliveInterval::body(const Runtime::CallingFrame &Frame,
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

  auto SocketData = Env.Table.GetById<TCPSocket>(Handle);
  if (SocketData == nullptr) {
    return SocketsErr(*RetBuf, __WASI_SOCKETS_ERRNO_ACCESS_DENIED);
  }

  uint32_t Sec = std::chrono::duration_cast<std::chrono::seconds>(
                     std::chrono::nanoseconds(Val))
                     .count();
  if (auto Res = Env.sockSetAliveInterval(SocketData->Fd, Sec); !Res) {
    return SocketsErr(*RetBuf, Res.error());
  }
  return SocketsOk(*RetBuf);
}

Expect<void> KeepAliveCount::body(const Runtime::CallingFrame &Frame,
                                  uint32_t Handle, uint32_t RetBufPtr) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return WASI::WasiUnexpect(__WASI_ERRNO_FAULT);
  }

  auto RetBuf =
      MemInst->getPointer<__wasi_sockets_ret_t<uint32_t> *>(RetBufPtr);
  if (RetBuf == nullptr) {
    return WASI::WasiUnexpect(__WASI_ERRNO_FAULT);
  }

  auto SocketData = Env.Table.GetById<TCPSocket>(Handle);
  if (SocketData == nullptr) {
    return SocketsErr(*RetBuf, __WASI_SOCKETS_ERRNO_ACCESS_DENIED);
  }

  if (auto Res = Env.sockGetKeepAliveCount(SocketData->Fd); !Res) {
    return SocketsErr(*RetBuf, Res.error());
  } else {
    RetBuf->Val.Res = Res.value();
  }
  return SocketsOk(*RetBuf);
}

Expect<void> SetKeepAliveCount::body(const Runtime::CallingFrame &Frame,
                                     uint32_t Handle, uint32_t Val,
                                     uint32_t RetBufPtr) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return WASI::WasiUnexpect(__WASI_ERRNO_FAULT);
  }

  auto RetBuf = MemInst->getPointer<__wasi_sockets_ret_t<void> *>(RetBufPtr);
  if (RetBuf == nullptr) {
    return WASI::WasiUnexpect(__WASI_ERRNO_FAULT);
  }

  auto SocketData = Env.Table.GetById<TCPSocket>(Handle);
  if (SocketData == nullptr) {
    return SocketsErr(*RetBuf, __WASI_SOCKETS_ERRNO_ACCESS_DENIED);
  }

  if (auto Res = Env.sockSetKeepAliveCount(SocketData->Fd, Val); !Res) {
    return SocketsErr(*RetBuf, Res.error());
  }
  return SocketsOk(*RetBuf);
}

Expect<void> HopLimit::body(const Runtime::CallingFrame &Frame, uint32_t Handle,
                            uint32_t RetBufPtr) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return WASI::WasiUnexpect(__WASI_ERRNO_FAULT);
  }

  auto RetBuf = MemInst->getPointer<__wasi_sockets_ret_t<uint8_t> *>(RetBufPtr);
  if (RetBuf == nullptr) {
    return WASI::WasiUnexpect(__WASI_ERRNO_FAULT);
  }

  auto SocketData = Env.Table.GetById<TCPSocket>(Handle);
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

Expect<void> SetHopLimit::body(const Runtime::CallingFrame &Frame,
                               uint32_t Handle, uint32_t Val,
                               uint32_t RetBufPtr) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return WASI::WasiUnexpect(__WASI_ERRNO_FAULT);
  }

  auto RetBuf = MemInst->getPointer<__wasi_sockets_ret_t<void> *>(RetBufPtr);
  if (RetBuf == nullptr) {
    return WASI::WasiUnexpect(__WASI_ERRNO_FAULT);
  }

  auto SocketData = Env.Table.GetById<TCPSocket>(Handle);
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

  auto SocketData = Env.Table.GetById<TCPSocket>(Handle);
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

  auto SocketData = Env.Table.GetById<TCPSocket>(Handle);
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

  auto SocketData = Env.Table.GetById<TCPSocket>(Handle);
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

  auto SocketData = Env.Table.GetById<TCPSocket>(Handle);
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

Expect<void> Shutdown::body(const Runtime::CallingFrame &Frame, uint32_t Handle,
                            uint32_t How, uint32_t RetBufPtr) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return WASI::WasiUnexpect(__WASI_ERRNO_FAULT);
  }

  auto RetBuf = MemInst->getPointer<__wasi_sockets_ret_t<void> *>(RetBufPtr);
  if (RetBuf == nullptr) {
    return WASI::WasiUnexpect(__WASI_ERRNO_FAULT);
  }

  auto SocketData = Env.Table.GetById<TCPSocket>(Handle);
  if (SocketData == nullptr) {
    return SocketsErr(*RetBuf, __WASI_SOCKETS_ERRNO_ACCESS_DENIED);
  }

  if (auto Res = Env.sockShutdown(SocketData->Fd, __wasi_sdflags_t(How));
      !Res) {
    return SocketsErr(*RetBuf, Res.error());
  } else {
    return SocketsOk(*RetBuf);
  }
  return {};
}
} // namespace TCP
} // namespace WasiSocket
} // namespace Host
} // namespace WasmEdge
