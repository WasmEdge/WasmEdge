// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "host/wasi/p2/sockets.h"

#include <array>
#include <cstring>
#include <memory>
#include <utility>

namespace WasmEdge {
namespace Host {
namespace WasiP2 {

namespace {

NetworkError errorOf(__wasi_errno_t Errno) noexcept {
  switch (Errno) {
  case __WASI_ERRNO_ACCES:
  case __WASI_ERRNO_PERM:
  case __WASI_ERRNO_NOTCAPABLE:
    return NetworkError::AccessDenied;
  case __WASI_ERRNO_NOTSUP:
  case __WASI_ERRNO_AFNOSUPPORT:
  case __WASI_ERRNO_PROTONOSUPPORT:
  case __WASI_ERRNO_PROTOTYPE:
    return NetworkError::NotSupported;
  case __WASI_ERRNO_INVAL:
  case __WASI_ERRNO_DOM:
    return NetworkError::InvalidArgument;
  case __WASI_ERRNO_NOMEM:
  case __WASI_ERRNO_NOBUFS:
    return NetworkError::OutOfMemory;
  case __WASI_ERRNO_TIMEDOUT:
    return NetworkError::Timeout;
  case __WASI_ERRNO_AGAIN:
    return NetworkError::WouldBlock;
  case __WASI_ERRNO_ISCONN:
  case __WASI_ERRNO_NOTCONN:
  case __WASI_ERRNO_ALREADY:
  case __WASI_ERRNO_INPROGRESS:
  case __WASI_ERRNO_DESTADDRREQ:
    return NetworkError::InvalidState;
  case __WASI_ERRNO_MFILE:
  case __WASI_ERRNO_NFILE:
    return NetworkError::NewSocketLimit;
  case __WASI_ERRNO_ADDRNOTAVAIL:
    return NetworkError::AddressNotBindable;
  case __WASI_ERRNO_ADDRINUSE:
    return NetworkError::AddressInUse;
  case __WASI_ERRNO_NETUNREACH:
  case __WASI_ERRNO_HOSTUNREACH:
  case __WASI_ERRNO_NETDOWN:
    return NetworkError::RemoteUnreachable;
  case __WASI_ERRNO_CONNREFUSED:
    return NetworkError::ConnectionRefused;
  case __WASI_ERRNO_CONNRESET:
  case __WASI_ERRNO_NETRESET:
  case __WASI_ERRNO_PIPE:
    return NetworkError::ConnectionReset;
  case __WASI_ERRNO_CONNABORTED:
    return NetworkError::ConnectionAborted;
  case __WASI_ERRNO_MSGSIZE:
    return NetworkError::DatagramTooLarge;
  case __WASI_ERRNO_AIAGAIN:
    return NetworkError::TemporaryResolverFailure;
  case __WASI_ERRNO_AINONAME:
  case __WASI_ERRNO_AINODATA:
    return NetworkError::NameUnresolvable;
  case __WASI_ERRNO_AIBADFLAG:
  case __WASI_ERRNO_AIFAMILY:
  case __WASI_ERRNO_AISOCKTYPE:
  case __WASI_ERRNO_AISERVICE:
    return NetworkError::InvalidArgument;
  case __WASI_ERRNO_AIFAIL:
  case __WASI_ERRNO_AIMEMORY:
  case __WASI_ERRNO_AISYSTEM:
    return NetworkError::PermanentResolverFailure;
  default:
    return NetworkError::Unknown;
  }
}

template <typename T> NetResult<T> fail(NetworkError E) noexcept {
  return Unexpected<NetworkError>(E);
}

NetStatus statusOf(WASI::WasiExpect<void> Res) noexcept {
  if (!Res) {
    return Unexpected<NetworkError>(errorOf(Res.error()));
  }
  return Runtime::Component::Unit{};
}

template <typename T>
NetResult<T> optionResult(WASI::WasiExpect<uint64_t> Res) noexcept {
  if (!Res) {
    return Unexpected<NetworkError>(errorOf(Res.error()));
  }
  return static_cast<T>(*Res);
}

} // namespace

Expect<Runtime::Component::Own<Network>>
InstanceNetwork::body(Runtime::Component::CallingFrame &) {
  return Runtime::Component::Own<Network>{1};
}

Expect<NetResult<Runtime::Component::Own<ResolveAddressStream>>>
ResolveAddresses::body(Runtime::Component::CallingFrame &, NetworkBorrow,
                       std::string Name) {
  if (Name.empty()) {
    return NetResult<Runtime::Component::Own<ResolveAddressStream>>(
        fail<Runtime::Component::Own<ResolveAddressStream>>(
            NetworkError::InvalidArgument));
  }
  auto Res = WasiP3::resolveHostname(Name);
  if (!Res) {
    return NetResult<Runtime::Component::Own<ResolveAddressStream>>(
        fail<Runtime::Component::Own<ResolveAddressStream>>(
            errorOf(Res.error())));
  }
  auto Stream = std::make_shared<ResolveAddressStream>();
  Stream->Addresses = std::move(*Res);
  return NetResult<Runtime::Component::Own<ResolveAddressStream>>(
      Runtime::Component::Own<ResolveAddressStream>{
          Sockets.Resolves.add(std::move(Stream))});
}

Expect<NetResult<std::optional<IpAddress>>>
ResolveNextAddress::body(Runtime::Component::CallingFrame &,
                         ResolveBorrow Self) {
  auto Stream = Sockets.Resolves.get(Self.Rep);
  if (!Stream) {
    return NetResult<std::optional<IpAddress>>(
        fail<std::optional<IpAddress>>(NetworkError::InvalidState));
  }
  if (Stream->Next >= Stream->Addresses.size()) {
    return NetResult<std::optional<IpAddress>>(std::optional<IpAddress>{});
  }
  return NetResult<std::optional<IpAddress>>(
      std::optional<IpAddress>{Stream->Addresses[Stream->Next++]});
}

Expect<Runtime::Component::Own<PollSource>>
ResolveSubscribe::body(Runtime::Component::CallingFrame &, ResolveBorrow) {
  return Runtime::Component::Own<PollSource>{
      Sockets.Io.pollables().add(std::make_shared<AlwaysReady>())};
}

Expect<NetResult<Runtime::Component::Own<TcpSocket>>>
CreateTcpSocket::body(Runtime::Component::CallingFrame &,
                      IpAddressFamily Family) {
  auto Node = WASI::VINode::sockOpen(WasiP3::wasiFamilyOf(Family),
                                     __WASI_SOCK_TYPE_SOCK_STREAM);
  if (!Node) {
    return NetResult<Runtime::Component::Own<TcpSocket>>(
        fail<Runtime::Component::Own<TcpSocket>>(errorOf(Node.error())));
  }
  auto Sock = std::make_shared<TcpSocket>();
  Sock->Node = std::move(*Node);
  Sock->Family = Family;
  return NetResult<Runtime::Component::Own<TcpSocket>>(
      Runtime::Component::Own<TcpSocket>{
          Sockets.TcpSockets.add(std::move(Sock))});
}

Expect<NetStatus> TcpStart::body(Runtime::Component::CallingFrame &,
                                 TcpBorrow Self, NetworkBorrow,
                                 IpSocketAddress Address) {
  auto Sock = Sockets.socket(Self);
  if (!Sock) {
    return NetStatus(Unexpected<NetworkError>(NetworkError::InvalidState));
  }
  if (Sock->PendingBind || Sock->PendingConnect || Sock->PendingListen) {
    return NetStatus(
        Unexpected<NetworkError>(NetworkError::ConcurrencyConflict));
  }
  const bool Ipv4 = Address.Family == IpAddressFamily::Ipv4;
  if (Ipv4 != (Sock->Family == IpAddressFamily::Ipv4)) {
    return NetStatus(Unexpected<NetworkError>(NetworkError::InvalidArgument));
  }
  const auto Raw = Address.raw();
  if (Which == Op::Bind) {
    if (Sock->St != TcpSocket::State::Unbound) {
      return NetStatus(Unexpected<NetworkError>(NetworkError::InvalidState));
    }
    auto Res = Sock->Node->sockBind(Raw.Family, Raw.bytes(), Raw.Port);
    Sock->PendingBind = true;
    Sock->Pending.reset();
    if (!Res) {
      Sock->Pending = errorOf(Res.error());
    } else {
      Sock->St = TcpSocket::State::Bound;
    }
    return NetStatus(Runtime::Component::Unit{});
  }
  if (Sock->St != TcpSocket::State::Unbound &&
      Sock->St != TcpSocket::State::Bound) {
    return NetStatus(Unexpected<NetworkError>(NetworkError::InvalidState));
  }
  if (Raw.Port == 0) {
    return NetStatus(Unexpected<NetworkError>(NetworkError::InvalidArgument));
  }
  auto Res = Sock->Node->sockConnect(Raw.Family, Raw.bytes(), Raw.Port);
  Sock->PendingConnect = true;
  Sock->Pending.reset();
  if (!Res) {
    Sock->Pending = errorOf(Res.error());
    Sock->St = TcpSocket::State::Failed;
  } else {
    Sock->St = TcpSocket::State::Connected;
  }
  return NetStatus(Runtime::Component::Unit{});
}

Expect<NetStatus> TcpFinish::body(Runtime::Component::CallingFrame &,
                                  TcpBorrow Self) {
  auto Sock = Sockets.socket(Self);
  if (!Sock) {
    return NetStatus(Unexpected<NetworkError>(NetworkError::InvalidState));
  }
  bool &Flag = Which == Op::Bind ? Sock->PendingBind : Sock->PendingListen;
  if (!Flag) {
    return NetStatus(Unexpected<NetworkError>(NetworkError::NotInProgress));
  }
  Flag = false;
  if (Sock->Pending) {
    const auto E = *Sock->Pending;
    Sock->Pending.reset();
    return NetStatus(Unexpected<NetworkError>(E));
  }
  return NetStatus(Runtime::Component::Unit{});
}

Expect<NetResult<std::tuple<Runtime::Component::Own<InputStream>,
                            Runtime::Component::Own<OutputStream>>>>
TcpFinishConnect::body(Runtime::Component::CallingFrame &, TcpBorrow Self) {
  using Streams = std::tuple<Runtime::Component::Own<InputStream>,
                             Runtime::Component::Own<OutputStream>>;
  auto Sock = Sockets.socket(Self);
  if (!Sock) {
    return NetResult<Streams>(fail<Streams>(NetworkError::InvalidState));
  }
  if (!Sock->PendingConnect) {
    return NetResult<Streams>(fail<Streams>(NetworkError::NotInProgress));
  }
  Sock->PendingConnect = false;
  if (Sock->Pending) {
    const auto E = *Sock->Pending;
    Sock->Pending.reset();
    return NetResult<Streams>(fail<Streams>(E));
  }
  return NetResult<Streams>(Sockets.Io.socketStreams(Sock->Node));
}

Expect<NetStatus> TcpStartListen::body(Runtime::Component::CallingFrame &,
                                       TcpBorrow Self) {
  auto Sock = Sockets.socket(Self);
  if (!Sock) {
    return NetStatus(Unexpected<NetworkError>(NetworkError::InvalidState));
  }
  if (Sock->PendingBind || Sock->PendingConnect || Sock->PendingListen) {
    return NetStatus(
        Unexpected<NetworkError>(NetworkError::ConcurrencyConflict));
  }
  if (Sock->St != TcpSocket::State::Bound) {
    return NetStatus(Unexpected<NetworkError>(NetworkError::InvalidState));
  }
  auto Res = Sock->Node->sockListen(static_cast<int32_t>(Sock->Backlog));
  Sock->PendingListen = true;
  Sock->Pending.reset();
  if (!Res) {
    Sock->Pending = errorOf(Res.error());
  } else {
    Sock->St = TcpSocket::State::Listening;
  }
  return NetStatus(Runtime::Component::Unit{});
}

Expect<NetResult<std::tuple<Runtime::Component::Own<TcpSocket>,
                            Runtime::Component::Own<InputStream>,
                            Runtime::Component::Own<OutputStream>>>>
TcpAccept::body(Runtime::Component::CallingFrame &, TcpBorrow Self) {
  using Accepted = std::tuple<Runtime::Component::Own<TcpSocket>,
                              Runtime::Component::Own<InputStream>,
                              Runtime::Component::Own<OutputStream>>;
  auto Sock = Sockets.socket(Self);
  if (!Sock) {
    return NetResult<Accepted>(fail<Accepted>(NetworkError::InvalidState));
  }
  if (Sock->St != TcpSocket::State::Listening) {
    return NetResult<Accepted>(fail<Accepted>(NetworkError::InvalidState));
  }
  if (!Sockets.Io.readable(*Sock->Node)) {
    return NetResult<Accepted>(fail<Accepted>(NetworkError::WouldBlock));
  }
  auto Node = Sock->Node->sockAccept(static_cast<__wasi_fdflags_t>(0));
  if (!Node) {
    return NetResult<Accepted>(fail<Accepted>(errorOf(Node.error())));
  }
  auto Client = std::make_shared<TcpSocket>();
  Client->Node = std::move(*Node);
  Client->Family = Sock->Family;
  Client->St = TcpSocket::State::Connected;
  auto Streams = Sockets.Io.socketStreams(Client->Node);
  Runtime::Component::Own<TcpSocket> Handle{
      Sockets.TcpSockets.add(std::move(Client))};
  return NetResult<Accepted>(
      Accepted{Handle, std::get<0>(Streams), std::get<1>(Streams)});
}

Expect<NetResult<IpSocketAddress>>
TcpAddress::body(Runtime::Component::CallingFrame &, TcpBorrow Self) {
  auto Sock = Sockets.socket(Self);
  if (!Sock) {
    return NetResult<IpSocketAddress>(
        fail<IpSocketAddress>(NetworkError::InvalidState));
  }
  if (Remote ? Sock->St != TcpSocket::State::Connected
             : Sock->St == TcpSocket::State::Unbound) {
    return NetResult<IpSocketAddress>(
        fail<IpSocketAddress>(NetworkError::InvalidState));
  }
  auto Res = WasiP3::nodeAddress(*Sock->Node, Remote);
  if (!Res) {
    return NetResult<IpSocketAddress>(
        fail<IpSocketAddress>(errorOf(Res.error())));
  }
  return NetResult<IpSocketAddress>(std::move(*Res));
}

Expect<bool> TcpIsListening::body(Runtime::Component::CallingFrame &,
                                  TcpBorrow Self) {
  auto Sock = Sockets.socket(Self);
  return Sock && Sock->St == TcpSocket::State::Listening;
}

Expect<IpAddressFamily>
TcpAddressFamily::body(Runtime::Component::CallingFrame &, TcpBorrow Self) {
  auto Sock = Sockets.socket(Self);
  return Sock ? Sock->Family : IpAddressFamily::Ipv4;
}

Expect<NetStatus>
TcpSetListenBacklogSize::body(Runtime::Component::CallingFrame &,
                              TcpBorrow Self, uint64_t Value) {
  auto Sock = Sockets.socket(Self);
  if (!Sock) {
    return NetStatus(Unexpected<NetworkError>(NetworkError::InvalidState));
  }
  if (Value == 0) {
    return NetStatus(Unexpected<NetworkError>(NetworkError::InvalidArgument));
  }
  if (Sock->St == TcpSocket::State::Connected ||
      Sock->St == TcpSocket::State::Failed) {
    return NetStatus(Unexpected<NetworkError>(NetworkError::InvalidState));
  }
  Sock->Backlog = std::min<uint64_t>(Value, 4096);
  if (Sock->St == TcpSocket::State::Listening) {
    return statusOf(
        Sock->Node->sockListen(static_cast<int32_t>(Sock->Backlog)));
  }
  return NetStatus(Runtime::Component::Unit{});
}

Expect<NetResult<bool>>
TcpGetBoolOption::body(Runtime::Component::CallingFrame &, TcpBorrow Self) {
  auto Sock = Sockets.socket(Self);
  if (!Sock) {
    return NetResult<bool>(fail<bool>(NetworkError::InvalidState));
  }
  return optionResult<bool>(
      WasiP3::getOption(*Sock->Node, Sock->Family, Option));
}

Expect<NetStatus> TcpSetBoolOption::body(Runtime::Component::CallingFrame &,
                                         TcpBorrow Self, bool Value) {
  auto Sock = Sockets.socket(Self);
  if (!Sock) {
    return NetStatus(Unexpected<NetworkError>(NetworkError::InvalidState));
  }
  return statusOf(
      WasiP3::setOption(*Sock->Node, Sock->Family, Option, Value ? 1 : 0));
}

Expect<NetResult<uint64_t>>
TcpGetU64Option::body(Runtime::Component::CallingFrame &, TcpBorrow Self) {
  auto Sock = Sockets.socket(Self);
  if (!Sock) {
    return NetResult<uint64_t>(fail<uint64_t>(NetworkError::InvalidState));
  }
  return optionResult<uint64_t>(
      WasiP3::getOption(*Sock->Node, Sock->Family, Option));
}

Expect<NetStatus> TcpSetU64Option::body(Runtime::Component::CallingFrame &,
                                        TcpBorrow Self, uint64_t Value) {
  auto Sock = Sockets.socket(Self);
  if (!Sock) {
    return NetStatus(Unexpected<NetworkError>(NetworkError::InvalidState));
  }
  if (Value == 0) {
    return NetStatus(Unexpected<NetworkError>(NetworkError::InvalidArgument));
  }
  return statusOf(WasiP3::setOption(*Sock->Node, Sock->Family, Option, Value));
}

Expect<NetResult<uint32_t>>
TcpGetU32Option::body(Runtime::Component::CallingFrame &, TcpBorrow Self) {
  auto Sock = Sockets.socket(Self);
  if (!Sock) {
    return NetResult<uint32_t>(fail<uint32_t>(NetworkError::InvalidState));
  }
  return optionResult<uint32_t>(
      WasiP3::getOption(*Sock->Node, Sock->Family, Option));
}

Expect<NetStatus> TcpSetU32Option::body(Runtime::Component::CallingFrame &,
                                        TcpBorrow Self, uint32_t Value) {
  auto Sock = Sockets.socket(Self);
  if (!Sock) {
    return NetStatus(Unexpected<NetworkError>(NetworkError::InvalidState));
  }
  if (Value == 0) {
    return NetStatus(Unexpected<NetworkError>(NetworkError::InvalidArgument));
  }
  return statusOf(WasiP3::setOption(*Sock->Node, Sock->Family, Option, Value));
}

Expect<NetResult<uint8_t>>
TcpGetU8Option::body(Runtime::Component::CallingFrame &, TcpBorrow Self) {
  auto Sock = Sockets.socket(Self);
  if (!Sock) {
    return NetResult<uint8_t>(fail<uint8_t>(NetworkError::InvalidState));
  }
  return optionResult<uint8_t>(
      WasiP3::getOption(*Sock->Node, Sock->Family, Option));
}

Expect<NetStatus> TcpSetU8Option::body(Runtime::Component::CallingFrame &,
                                       TcpBorrow Self, uint8_t Value) {
  auto Sock = Sockets.socket(Self);
  if (!Sock) {
    return NetStatus(Unexpected<NetworkError>(NetworkError::InvalidState));
  }
  if (Value == 0) {
    return NetStatus(Unexpected<NetworkError>(NetworkError::InvalidArgument));
  }
  return statusOf(WasiP3::setOption(*Sock->Node, Sock->Family, Option, Value));
}

Expect<Runtime::Component::Own<PollSource>>
TcpSubscribe::body(Runtime::Component::CallingFrame &, TcpBorrow Self) {
  auto Sock = Sockets.socket(Self);
  const bool Active = Sock && (Sock->St == TcpSocket::State::Listening ||
                               Sock->St == TcpSocket::State::Connected);
  return Sockets.Io.readySource(Sock ? Sock->Node : nullptr, Active, false);
}

Expect<NetStatus> TcpShutdown::body(Runtime::Component::CallingFrame &,
                                    TcpBorrow Self, ShutdownType Type) {
  auto Sock = Sockets.socket(Self);
  if (!Sock) {
    return NetStatus(Unexpected<NetworkError>(NetworkError::InvalidState));
  }
  if (Sock->St != TcpSocket::State::Connected) {
    return NetStatus(Unexpected<NetworkError>(NetworkError::InvalidState));
  }
  uint32_t Flags = 0;
  if (Type != ShutdownType::Send) {
    Flags |= __WASI_SDFLAGS_RD;
  }
  if (Type != ShutdownType::Receive) {
    Flags |= __WASI_SDFLAGS_WR;
  }
  return statusOf(
      Sock->Node->sockShutdown(static_cast<__wasi_sdflags_t>(Flags)));
}

Expect<NetResult<Runtime::Component::Own<UdpSocket>>>
CreateUdpSocket::body(Runtime::Component::CallingFrame &,
                      IpAddressFamily Family) {
  auto Node = WASI::VINode::sockOpen(WasiP3::wasiFamilyOf(Family),
                                     __WASI_SOCK_TYPE_SOCK_DGRAM);
  if (!Node) {
    return NetResult<Runtime::Component::Own<UdpSocket>>(
        fail<Runtime::Component::Own<UdpSocket>>(errorOf(Node.error())));
  }
  auto Sock = std::make_shared<UdpSocket>();
  Sock->Node = std::move(*Node);
  Sock->Family = Family;
  return NetResult<Runtime::Component::Own<UdpSocket>>(
      Runtime::Component::Own<UdpSocket>{
          Sockets.UdpSockets.add(std::move(Sock))});
}

Expect<NetStatus> UdpStartBind::body(Runtime::Component::CallingFrame &,
                                     UdpBorrow Self, NetworkBorrow,
                                     IpSocketAddress Local) {
  auto Sock = Sockets.socket(Self);
  if (!Sock) {
    return NetStatus(Unexpected<NetworkError>(NetworkError::InvalidState));
  }
  if (Sock->PendingBind) {
    return NetStatus(
        Unexpected<NetworkError>(NetworkError::ConcurrencyConflict));
  }
  if (Sock->Bound) {
    return NetStatus(Unexpected<NetworkError>(NetworkError::InvalidState));
  }
  if ((Local.Family == IpAddressFamily::Ipv4) !=
      (Sock->Family == IpAddressFamily::Ipv4)) {
    return NetStatus(Unexpected<NetworkError>(NetworkError::InvalidArgument));
  }
  const auto Raw = Local.raw();
  auto Res = Sock->Node->sockBind(Raw.Family, Raw.bytes(), Raw.Port);
  Sock->PendingBind = true;
  Sock->Pending.reset();
  if (!Res) {
    Sock->Pending = errorOf(Res.error());
  } else {
    Sock->Bound = true;
  }
  return NetStatus(Runtime::Component::Unit{});
}

Expect<NetStatus> UdpFinishBind::body(Runtime::Component::CallingFrame &,
                                      UdpBorrow Self) {
  auto Sock = Sockets.socket(Self);
  if (!Sock) {
    return NetStatus(Unexpected<NetworkError>(NetworkError::InvalidState));
  }
  if (!Sock->PendingBind) {
    return NetStatus(Unexpected<NetworkError>(NetworkError::NotInProgress));
  }
  Sock->PendingBind = false;
  if (Sock->Pending) {
    const auto E = *Sock->Pending;
    Sock->Pending.reset();
    return NetStatus(Unexpected<NetworkError>(E));
  }
  return NetStatus(Runtime::Component::Unit{});
}

Expect<NetResult<std::tuple<Runtime::Component::Own<IncomingDatagramStream>,
                            Runtime::Component::Own<OutgoingDatagramStream>>>>
UdpStream::body(Runtime::Component::CallingFrame &, UdpBorrow Self,
                std::optional<IpSocketAddress> Remote) {
  using Streams = std::tuple<Runtime::Component::Own<IncomingDatagramStream>,
                             Runtime::Component::Own<OutgoingDatagramStream>>;
  auto Sock = Sockets.socket(Self);
  if (!Sock) {
    return NetResult<Streams>(fail<Streams>(NetworkError::InvalidState));
  }
  if (!Sock->Bound) {
    return NetResult<Streams>(fail<Streams>(NetworkError::InvalidState));
  }
  if (Remote) {
    if ((Remote->Family == IpAddressFamily::Ipv4) !=
        (Sock->Family == IpAddressFamily::Ipv4)) {
      return NetResult<Streams>(fail<Streams>(NetworkError::InvalidArgument));
    }
    const auto Raw = Remote->raw();
    if (Raw.Port == 0) {
      return NetResult<Streams>(fail<Streams>(NetworkError::InvalidArgument));
    }
    if (auto Res = Sock->Node->sockConnect(Raw.Family, Raw.bytes(), Raw.Port);
        !Res) {
      return NetResult<Streams>(fail<Streams>(errorOf(Res.error())));
    }
  }
  Sock->Remote = std::move(Remote);
  auto In = std::make_shared<IncomingDatagramStream>();
  In->Socket = Sock;
  auto OutStream = std::make_shared<OutgoingDatagramStream>();
  OutStream->Socket = Sock;
  return NetResult<Streams>(
      Streams{Runtime::Component::Own<IncomingDatagramStream>{
                  Sockets.Incoming.add(std::move(In))},
              Runtime::Component::Own<OutgoingDatagramStream>{
                  Sockets.Outgoing.add(std::move(OutStream))}});
}

Expect<NetResult<IpSocketAddress>>
UdpAddress::body(Runtime::Component::CallingFrame &, UdpBorrow Self) {
  auto Sock = Sockets.socket(Self);
  if (!Sock) {
    return NetResult<IpSocketAddress>(
        fail<IpSocketAddress>(NetworkError::InvalidState));
  }
  if (Remote) {
    if (!Sock->Remote) {
      return NetResult<IpSocketAddress>(
          fail<IpSocketAddress>(NetworkError::InvalidState));
    }
    return NetResult<IpSocketAddress>(*Sock->Remote);
  }
  if (!Sock->Bound) {
    return NetResult<IpSocketAddress>(
        fail<IpSocketAddress>(NetworkError::InvalidState));
  }
  auto Res = WasiP3::nodeAddress(*Sock->Node, false);
  if (!Res) {
    return NetResult<IpSocketAddress>(
        fail<IpSocketAddress>(errorOf(Res.error())));
  }
  return NetResult<IpSocketAddress>(std::move(*Res));
}

Expect<IpAddressFamily>
UdpAddressFamily::body(Runtime::Component::CallingFrame &, UdpBorrow Self) {
  auto Sock = Sockets.socket(Self);
  return Sock ? Sock->Family : IpAddressFamily::Ipv4;
}

Expect<NetResult<uint64_t>>
UdpGetU64Option::body(Runtime::Component::CallingFrame &, UdpBorrow Self) {
  auto Sock = Sockets.socket(Self);
  if (!Sock) {
    return NetResult<uint64_t>(fail<uint64_t>(NetworkError::InvalidState));
  }
  return optionResult<uint64_t>(
      WasiP3::getOption(*Sock->Node, Sock->Family, Option));
}

Expect<NetStatus> UdpSetU64Option::body(Runtime::Component::CallingFrame &,
                                        UdpBorrow Self, uint64_t Value) {
  auto Sock = Sockets.socket(Self);
  if (!Sock) {
    return NetStatus(Unexpected<NetworkError>(NetworkError::InvalidState));
  }
  if (Value == 0) {
    return NetStatus(Unexpected<NetworkError>(NetworkError::InvalidArgument));
  }
  return statusOf(WasiP3::setOption(*Sock->Node, Sock->Family, Option, Value));
}

Expect<NetResult<uint8_t>>
UdpGetU8Option::body(Runtime::Component::CallingFrame &, UdpBorrow Self) {
  auto Sock = Sockets.socket(Self);
  if (!Sock) {
    return NetResult<uint8_t>(fail<uint8_t>(NetworkError::InvalidState));
  }
  return optionResult<uint8_t>(
      WasiP3::getOption(*Sock->Node, Sock->Family, Option));
}

Expect<NetStatus> UdpSetU8Option::body(Runtime::Component::CallingFrame &,
                                       UdpBorrow Self, uint8_t Value) {
  auto Sock = Sockets.socket(Self);
  if (!Sock) {
    return NetStatus(Unexpected<NetworkError>(NetworkError::InvalidState));
  }
  if (Value == 0) {
    return NetStatus(Unexpected<NetworkError>(NetworkError::InvalidArgument));
  }
  return statusOf(WasiP3::setOption(*Sock->Node, Sock->Family, Option, Value));
}

Expect<Runtime::Component::Own<PollSource>>
UdpSubscribe::body(Runtime::Component::CallingFrame &, UdpBorrow Self) {
  auto Sock = Sockets.socket(Self);
  return Sockets.Io.readySource(Sock ? Sock->Node : nullptr,
                                Sock && Sock->Bound, false);
}

Expect<NetResult<std::vector<IncomingDatagram>>>
IncomingReceive::body(Runtime::Component::CallingFrame &, IncomingBorrow Self,
                      uint64_t MaxResults) {
  auto Stream = Sockets.Incoming.get(Self.Rep);
  if (!Stream || !Stream->Socket) {
    return NetResult<std::vector<IncomingDatagram>>(
        fail<std::vector<IncomingDatagram>>(NetworkError::InvalidState));
  }
  auto &Sock = *Stream->Socket;
  std::vector<IncomingDatagram> Datagrams;
  std::vector<uint8_t> Buffer(65536);
  while (Datagrams.size() < MaxResults && Sockets.Io.readable(*Sock.Node)) {
    std::array<Span<uint8_t>, 1> IOVs{Span<uint8_t>(Buffer)};
    __wasi_address_family_t Family = __WASI_ADDRESS_FAMILY_UNSPEC;
    std::array<uint8_t, 16> Bytes{};
    uint16_t Port = 0;
    __wasi_size_t Read = 0;
    __wasi_roflags_t RoFlags = static_cast<__wasi_roflags_t>(0);
    auto Res = Sock.Node->sockRecvFrom(IOVs, static_cast<__wasi_riflags_t>(0),
                                       &Family, Bytes, &Port, Read, RoFlags);
    if (!Res) {
      if (Res.error() == __WASI_ERRNO_AGAIN) {
        break;
      }
      if (Datagrams.empty()) {
        return NetResult<std::vector<IncomingDatagram>>(
            fail<std::vector<IncomingDatagram>>(errorOf(Res.error())));
      }
      break;
    }
    auto From = WasiP3::socketAddressOf(Family, Bytes, WasiP3::portOf(Port));
    if (Sock.Remote && From.raw() != Sock.Remote->raw()) {
      continue;
    }
    Datagrams.push_back(IncomingDatagram{
        std::vector<uint8_t>(Buffer.begin(), Buffer.begin() + Read),
        std::move(From)});
  }
  return NetResult<std::vector<IncomingDatagram>>(std::move(Datagrams));
}

Expect<Runtime::Component::Own<PollSource>>
IncomingSubscribe::body(Runtime::Component::CallingFrame &,
                        IncomingBorrow Self) {
  auto Stream = Sockets.Incoming.get(Self.Rep);
  const bool Active = Stream && Stream->Socket;
  return Sockets.Io.readySource(Active ? Stream->Socket->Node : nullptr, Active,
                                false);
}

Expect<NetResult<uint64_t>>
OutgoingCheckSend::body(Runtime::Component::CallingFrame &,
                        OutgoingBorrow Self) {
  auto Stream = Sockets.Outgoing.get(Self.Rep);
  if (!Stream || !Stream->Socket) {
    return NetResult<uint64_t>(fail<uint64_t>(NetworkError::InvalidState));
  }
  return NetResult<uint64_t>(Sockets.Io.writable(*Stream->Socket->Node) ? 1024
                                                                        : 0);
}

Expect<NetResult<uint64_t>>
OutgoingSend::body(Runtime::Component::CallingFrame &, OutgoingBorrow Self,
                   std::vector<OutgoingDatagram> Datagrams) {
  auto Stream = Sockets.Outgoing.get(Self.Rep);
  if (!Stream || !Stream->Socket) {
    return NetResult<uint64_t>(fail<uint64_t>(NetworkError::InvalidState));
  }
  auto &Sock = *Stream->Socket;
  uint64_t Sent = 0;
  for (auto &D : Datagrams) {
    const IpSocketAddress *To = nullptr;
    if (D.Remote) {
      if (Sock.Remote && D.Remote->raw() != Sock.Remote->raw()) {
        return NetResult<uint64_t>(
            fail<uint64_t>(NetworkError::InvalidArgument));
      }
      To = &*D.Remote;
    } else if (Sock.Remote) {
      To = &*Sock.Remote;
    } else {
      return NetResult<uint64_t>(fail<uint64_t>(NetworkError::InvalidArgument));
    }
    if ((To->Family == IpAddressFamily::Ipv4) !=
        (Sock.Family == IpAddressFamily::Ipv4)) {
      return NetResult<uint64_t>(fail<uint64_t>(NetworkError::InvalidArgument));
    }
    const auto Raw = To->raw();
    if (Raw.Port == 0) {
      return NetResult<uint64_t>(fail<uint64_t>(NetworkError::InvalidArgument));
    }
    if (!Sockets.Io.writable(*Sock.Node)) {
      break;
    }
    // A connected socket sends to its remote without naming it.
    std::array<Span<const uint8_t>, 1> IOVs{Span<const uint8_t>(D.Data)};
    __wasi_size_t Written = 0;
    auto Res =
        Sock.Remote
            ? Sock.Node->sockSend(IOVs, static_cast<__wasi_siflags_t>(0),
                                  Written)
            : Sock.Node->sockSendTo(IOVs, static_cast<__wasi_siflags_t>(0),
                                    Raw.Family, Raw.bytes(), Raw.Port, Written);
    if (!Res) {
      if (Res.error() == __WASI_ERRNO_AGAIN) {
        break;
      }
      if (Sent == 0) {
        return NetResult<uint64_t>(fail<uint64_t>(errorOf(Res.error())));
      }
      break;
    }
    ++Sent;
  }
  return NetResult<uint64_t>(Sent);
}

Expect<Runtime::Component::Own<PollSource>>
OutgoingSubscribe::body(Runtime::Component::CallingFrame &,
                        OutgoingBorrow Self) {
  auto Stream = Sockets.Outgoing.get(Self.Rep);
  const bool Active = Stream && Stream->Socket;
  return Sockets.Io.readySource(Active ? Stream->Socket->Node : nullptr, Active,
                                true);
}

NetworkInstance::NetworkInstance(SocketsHost &Sockets)
    : ComponentInstance("wasi:sockets/network@0.2.12") {
  const uint32_t NetIdx = addHostResourceType<Network>([](uint64_t) {});
  exportType("network", NetIdx);
  Sockets.NetworkType = *getTypeResource(NetIdx);
  const auto Mint = getTypeMinter();
  exportType("error-code",
             Runtime::Component::Wit<NetworkError>::type(Mint).getTypeIndex());
  exportType(
      "ip-address-family",
      Runtime::Component::Wit<IpAddressFamily>::type(Mint).getTypeIndex());
  exportType("ipv4-address",
             Runtime::Component::Wit<Ipv4Address>::type(Mint).getTypeIndex());
  exportType("ipv6-address",
             Runtime::Component::Wit<Ipv6Address>::type(Mint).getTypeIndex());
  exportType("ip-address",
             Runtime::Component::Wit<IpAddress>::type(Mint).getTypeIndex());
  exportType(
      "ipv4-socket-address",
      Runtime::Component::Wit<Ipv4SocketAddress>::type(Mint).getTypeIndex());
  exportType(
      "ipv6-socket-address",
      Runtime::Component::Wit<Ipv6SocketAddress>::type(Mint).getTypeIndex());
  exportType(
      "ip-socket-address",
      Runtime::Component::Wit<IpSocketAddress>::type(Mint).getTypeIndex());
}

InstanceNetworkInstance::InstanceNetworkInstance(SocketsHost &Sockets)
    : ComponentInstance("wasi:sockets/instance-network@0.2.12") {
  exportType("network", addSharedResourceType<Network>(Sockets.NetworkType));
  addHostFunc("instance-network", std::make_unique<InstanceNetwork>(Sockets));
}

IpNameLookupInstance::IpNameLookupInstance(SocketsHost &Sockets)
    : ComponentInstance("wasi:sockets/ip-name-lookup@0.2.12") {
  exportType("pollable",
             addSharedResourceType<PollSource>(Sockets.Io.PollableType));
  exportType("network", addSharedResourceType<Network>(Sockets.NetworkType));
  const auto Mint = getTypeMinter();
  exportType("error-code",
             Runtime::Component::Wit<NetworkError>::type(Mint).getTypeIndex());
  exportType("ip-address",
             Runtime::Component::Wit<IpAddress>::type(Mint).getTypeIndex());
  exportType("resolve-address-stream",
             addHostResourceType<ResolveAddressStream>(
                 [&Sockets](uint64_t Rep) { Sockets.Resolves.remove(Rep); }));
  addHostFunc("resolve-addresses", std::make_unique<ResolveAddresses>(Sockets));
  addHostFunc("[method]resolve-address-stream.resolve-next-address",
              std::make_unique<ResolveNextAddress>(Sockets));
  addHostFunc("[method]resolve-address-stream.subscribe",
              std::make_unique<ResolveSubscribe>(Sockets));
}

TcpInstance::TcpInstance(SocketsHost &Sockets)
    : ComponentInstance("wasi:sockets/tcp@0.2.12") {
  exportType("input-stream",
             addSharedResourceType<InputStream>(Sockets.Io.InputType));
  exportType("output-stream",
             addSharedResourceType<OutputStream>(Sockets.Io.OutputType));
  exportType("pollable",
             addSharedResourceType<PollSource>(Sockets.Io.PollableType));
  const auto Mint = getTypeMinter();
  exportType("duration", Mint.primDefType(PrimValType::U64).getTypeIndex());
  exportType("network", addSharedResourceType<Network>(Sockets.NetworkType));
  exportType("error-code",
             Runtime::Component::Wit<NetworkError>::type(Mint).getTypeIndex());
  exportType(
      "ip-socket-address",
      Runtime::Component::Wit<IpSocketAddress>::type(Mint).getTypeIndex());
  exportType(
      "ip-address-family",
      Runtime::Component::Wit<IpAddressFamily>::type(Mint).getTypeIndex());
  exportType("shutdown-type",
             Runtime::Component::Wit<ShutdownType>::type(Mint).getTypeIndex());
  const uint32_t TcpIdx = addHostResourceType<TcpSocket>(
      [&Sockets](uint64_t Rep) { Sockets.TcpSockets.remove(Rep); });
  exportType("tcp-socket", TcpIdx);
  Sockets.TcpType = *getTypeResource(TcpIdx);
  addHostFunc("[method]tcp-socket.start-bind",
              std::make_unique<TcpStart>(Sockets, TcpStart::Op::Bind));
  addHostFunc("[method]tcp-socket.finish-bind",
              std::make_unique<TcpFinish>(Sockets, TcpFinish::Op::Bind));
  addHostFunc("[method]tcp-socket.start-connect",
              std::make_unique<TcpStart>(Sockets, TcpStart::Op::Connect));
  addHostFunc("[method]tcp-socket.finish-connect",
              std::make_unique<TcpFinishConnect>(Sockets));
  addHostFunc("[method]tcp-socket.start-listen",
              std::make_unique<TcpStartListen>(Sockets));
  addHostFunc("[method]tcp-socket.finish-listen",
              std::make_unique<TcpFinish>(Sockets, TcpFinish::Op::Listen));
  addHostFunc("[method]tcp-socket.accept",
              std::make_unique<TcpAccept>(Sockets));
  addHostFunc("[method]tcp-socket.local-address",
              std::make_unique<TcpAddress>(Sockets, false));
  addHostFunc("[method]tcp-socket.remote-address",
              std::make_unique<TcpAddress>(Sockets, true));
  addHostFunc("[method]tcp-socket.is-listening",
              std::make_unique<TcpIsListening>(Sockets));
  addHostFunc("[method]tcp-socket.address-family",
              std::make_unique<TcpAddressFamily>(Sockets));
  addHostFunc("[method]tcp-socket.set-listen-backlog-size",
              std::make_unique<TcpSetListenBacklogSize>(Sockets));
  addHostFunc("[method]tcp-socket.keep-alive-enabled",
              std::make_unique<TcpGetBoolOption>(
                  Sockets, SocketOption::KeepAliveEnabled));
  addHostFunc("[method]tcp-socket.set-keep-alive-enabled",
              std::make_unique<TcpSetBoolOption>(
                  Sockets, SocketOption::KeepAliveEnabled));
  addHostFunc("[method]tcp-socket.keep-alive-idle-time",
              std::make_unique<TcpGetU64Option>(
                  Sockets, SocketOption::KeepAliveIdleTime));
  addHostFunc("[method]tcp-socket.set-keep-alive-idle-time",
              std::make_unique<TcpSetU64Option>(
                  Sockets, SocketOption::KeepAliveIdleTime));
  addHostFunc("[method]tcp-socket.keep-alive-interval",
              std::make_unique<TcpGetU64Option>(
                  Sockets, SocketOption::KeepAliveInterval));
  addHostFunc("[method]tcp-socket.set-keep-alive-interval",
              std::make_unique<TcpSetU64Option>(
                  Sockets, SocketOption::KeepAliveInterval));
  addHostFunc(
      "[method]tcp-socket.keep-alive-count",
      std::make_unique<TcpGetU32Option>(Sockets, SocketOption::KeepAliveCount));
  addHostFunc(
      "[method]tcp-socket.set-keep-alive-count",
      std::make_unique<TcpSetU32Option>(Sockets, SocketOption::KeepAliveCount));
  addHostFunc(
      "[method]tcp-socket.hop-limit",
      std::make_unique<TcpGetU8Option>(Sockets, SocketOption::HopLimit));
  addHostFunc(
      "[method]tcp-socket.set-hop-limit",
      std::make_unique<TcpSetU8Option>(Sockets, SocketOption::HopLimit));
  addHostFunc("[method]tcp-socket.receive-buffer-size",
              std::make_unique<TcpGetU64Option>(
                  Sockets, SocketOption::ReceiveBufferSize));
  addHostFunc("[method]tcp-socket.set-receive-buffer-size",
              std::make_unique<TcpSetU64Option>(
                  Sockets, SocketOption::ReceiveBufferSize));
  addHostFunc(
      "[method]tcp-socket.send-buffer-size",
      std::make_unique<TcpGetU64Option>(Sockets, SocketOption::SendBufferSize));
  addHostFunc(
      "[method]tcp-socket.set-send-buffer-size",
      std::make_unique<TcpSetU64Option>(Sockets, SocketOption::SendBufferSize));
  addHostFunc("[method]tcp-socket.subscribe",
              std::make_unique<TcpSubscribe>(Sockets));
  addHostFunc("[method]tcp-socket.shutdown",
              std::make_unique<TcpShutdown>(Sockets));
}

TcpCreateSocketInstance::TcpCreateSocketInstance(SocketsHost &Sockets)
    : ComponentInstance("wasi:sockets/tcp-create-socket@0.2.12") {
  exportType("network", addSharedResourceType<Network>(Sockets.NetworkType));
  const auto Mint = getTypeMinter();
  exportType("error-code",
             Runtime::Component::Wit<NetworkError>::type(Mint).getTypeIndex());
  exportType(
      "ip-address-family",
      Runtime::Component::Wit<IpAddressFamily>::type(Mint).getTypeIndex());
  exportType("tcp-socket", addSharedResourceType<TcpSocket>(Sockets.TcpType));
  addHostFunc("create-tcp-socket", std::make_unique<CreateTcpSocket>(Sockets));
}

UdpInstance::UdpInstance(SocketsHost &Sockets)
    : ComponentInstance("wasi:sockets/udp@0.2.12") {
  exportType("pollable",
             addSharedResourceType<PollSource>(Sockets.Io.PollableType));
  exportType("network", addSharedResourceType<Network>(Sockets.NetworkType));
  const auto Mint = getTypeMinter();
  exportType("error-code",
             Runtime::Component::Wit<NetworkError>::type(Mint).getTypeIndex());
  exportType(
      "ip-socket-address",
      Runtime::Component::Wit<IpSocketAddress>::type(Mint).getTypeIndex());
  exportType(
      "ip-address-family",
      Runtime::Component::Wit<IpAddressFamily>::type(Mint).getTypeIndex());
  exportType(
      "incoming-datagram",
      Runtime::Component::Wit<IncomingDatagram>::type(Mint).getTypeIndex());
  exportType(
      "outgoing-datagram",
      Runtime::Component::Wit<OutgoingDatagram>::type(Mint).getTypeIndex());
  const uint32_t UdpIdx = addHostResourceType<UdpSocket>(
      [&Sockets](uint64_t Rep) { Sockets.UdpSockets.remove(Rep); });
  exportType("udp-socket", UdpIdx);
  Sockets.UdpType = *getTypeResource(UdpIdx);
  exportType("incoming-datagram-stream",
             addHostResourceType<IncomingDatagramStream>(
                 [&Sockets](uint64_t Rep) { Sockets.Incoming.remove(Rep); }));
  exportType("outgoing-datagram-stream",
             addHostResourceType<OutgoingDatagramStream>(
                 [&Sockets](uint64_t Rep) { Sockets.Outgoing.remove(Rep); }));
  addHostFunc("[method]udp-socket.start-bind",
              std::make_unique<UdpStartBind>(Sockets));
  addHostFunc("[method]udp-socket.finish-bind",
              std::make_unique<UdpFinishBind>(Sockets));
  addHostFunc("[method]udp-socket.stream",
              std::make_unique<UdpStream>(Sockets));
  addHostFunc("[method]udp-socket.local-address",
              std::make_unique<UdpAddress>(Sockets, false));
  addHostFunc("[method]udp-socket.remote-address",
              std::make_unique<UdpAddress>(Sockets, true));
  addHostFunc("[method]udp-socket.address-family",
              std::make_unique<UdpAddressFamily>(Sockets));
  addHostFunc(
      "[method]udp-socket.unicast-hop-limit",
      std::make_unique<UdpGetU8Option>(Sockets, SocketOption::HopLimit));
  addHostFunc(
      "[method]udp-socket.set-unicast-hop-limit",
      std::make_unique<UdpSetU8Option>(Sockets, SocketOption::HopLimit));
  addHostFunc("[method]udp-socket.receive-buffer-size",
              std::make_unique<UdpGetU64Option>(
                  Sockets, SocketOption::ReceiveBufferSize));
  addHostFunc("[method]udp-socket.set-receive-buffer-size",
              std::make_unique<UdpSetU64Option>(
                  Sockets, SocketOption::ReceiveBufferSize));
  addHostFunc(
      "[method]udp-socket.send-buffer-size",
      std::make_unique<UdpGetU64Option>(Sockets, SocketOption::SendBufferSize));
  addHostFunc(
      "[method]udp-socket.set-send-buffer-size",
      std::make_unique<UdpSetU64Option>(Sockets, SocketOption::SendBufferSize));
  addHostFunc("[method]udp-socket.subscribe",
              std::make_unique<UdpSubscribe>(Sockets));
  addHostFunc("[method]incoming-datagram-stream.receive",
              std::make_unique<IncomingReceive>(Sockets));
  addHostFunc("[method]incoming-datagram-stream.subscribe",
              std::make_unique<IncomingSubscribe>(Sockets));
  addHostFunc("[method]outgoing-datagram-stream.check-send",
              std::make_unique<OutgoingCheckSend>(Sockets));
  addHostFunc("[method]outgoing-datagram-stream.send",
              std::make_unique<OutgoingSend>(Sockets));
  addHostFunc("[method]outgoing-datagram-stream.subscribe",
              std::make_unique<OutgoingSubscribe>(Sockets));
}

UdpCreateSocketInstance::UdpCreateSocketInstance(SocketsHost &Sockets)
    : ComponentInstance("wasi:sockets/udp-create-socket@0.2.12") {
  exportType("network", addSharedResourceType<Network>(Sockets.NetworkType));
  const auto Mint = getTypeMinter();
  exportType("error-code",
             Runtime::Component::Wit<NetworkError>::type(Mint).getTypeIndex());
  exportType(
      "ip-address-family",
      Runtime::Component::Wit<IpAddressFamily>::type(Mint).getTypeIndex());
  exportType("udp-socket", addSharedResourceType<UdpSocket>(Sockets.UdpType));
  addHostFunc("create-udp-socket", std::make_unique<CreateUdpSocket>(Sockets));
}

std::vector<std::unique_ptr<Runtime::Instance::ComponentInstance>>
makeSocketsInstances(SocketsHost &Sockets) {
  std::vector<std::unique_ptr<Runtime::Instance::ComponentInstance>> Insts;
  Insts.push_back(std::make_unique<NetworkInstance>(Sockets));
  Insts.push_back(std::make_unique<InstanceNetworkInstance>(Sockets));
  Insts.push_back(std::make_unique<IpNameLookupInstance>(Sockets));
  Insts.push_back(std::make_unique<TcpInstance>(Sockets));
  Insts.push_back(std::make_unique<TcpCreateSocketInstance>(Sockets));
  Insts.push_back(std::make_unique<UdpInstance>(Sockets));
  Insts.push_back(std::make_unique<UdpCreateSocketInstance>(Sockets));
  return Insts;
}

} // namespace WasiP2
} // namespace Host
} // namespace WasmEdge
