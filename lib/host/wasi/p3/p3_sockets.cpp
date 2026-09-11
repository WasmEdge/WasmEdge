// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "common/component_valtype.h"
#include "common/component_variant.h"
#include "common/defines.h"
#include "host/wasi/p3/sockets.h"
#include "runtime/component/hosttransmit.h"

#include <array>
#include <cstring>
#include <memory>
#include <utility>

#if !WASMEDGE_OS_WINDOWS
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#endif

namespace WasmEdge {
namespace Host {
namespace WasiP3 {

namespace {

SocketError errorOf(__wasi_errno_t Errno) noexcept {
  switch (Errno) {
  case __WASI_ERRNO_ACCES:
  case __WASI_ERRNO_PERM:
  case __WASI_ERRNO_NOTCAPABLE:
    return SocketError::AccessDenied;
  case __WASI_ERRNO_NOTSUP:
  case __WASI_ERRNO_AFNOSUPPORT:
  case __WASI_ERRNO_PROTONOSUPPORT:
  case __WASI_ERRNO_PROTOTYPE:
    return SocketError::NotSupported;
  case __WASI_ERRNO_INVAL:
  case __WASI_ERRNO_DOM:
    return SocketError::InvalidArgument;
  case __WASI_ERRNO_NOMEM:
  case __WASI_ERRNO_NOBUFS:
    return SocketError::OutOfMemory;
  case __WASI_ERRNO_TIMEDOUT:
    return SocketError::Timeout;
  case __WASI_ERRNO_ISCONN:
  case __WASI_ERRNO_NOTCONN:
  case __WASI_ERRNO_ALREADY:
  case __WASI_ERRNO_INPROGRESS:
  case __WASI_ERRNO_DESTADDRREQ:
    return SocketError::InvalidState;
  case __WASI_ERRNO_ADDRNOTAVAIL:
    return SocketError::AddressNotBindable;
  case __WASI_ERRNO_ADDRINUSE:
    return SocketError::AddressInUse;
  case __WASI_ERRNO_NETUNREACH:
  case __WASI_ERRNO_HOSTUNREACH:
  case __WASI_ERRNO_NETDOWN:
    return SocketError::RemoteUnreachable;
  case __WASI_ERRNO_CONNREFUSED:
    return SocketError::ConnectionRefused;
  case __WASI_ERRNO_PIPE:
    return SocketError::ConnectionBroken;
  case __WASI_ERRNO_CONNRESET:
  case __WASI_ERRNO_NETRESET:
    return SocketError::ConnectionReset;
  case __WASI_ERRNO_CONNABORTED:
    return SocketError::ConnectionAborted;
  case __WASI_ERRNO_MSGSIZE:
    return SocketError::DatagramTooLarge;
  default:
    return SocketError::Other;
  }
}

LookupError lookupErrorOf(__wasi_errno_t Errno) noexcept {
  switch (Errno) {
  case __WASI_ERRNO_AIAGAIN:
    return LookupError::TemporaryResolverFailure;
  case __WASI_ERRNO_AINONAME:
  case __WASI_ERRNO_AINODATA:
    return LookupError::NameUnresolvable;
  case __WASI_ERRNO_AIBADFLAG:
  case __WASI_ERRNO_AIFAMILY:
  case __WASI_ERRNO_AISOCKTYPE:
  case __WASI_ERRNO_AISERVICE:
    return LookupError::InvalidArgument;
  case __WASI_ERRNO_ACCES:
  case __WASI_ERRNO_PERM:
    return LookupError::AccessDenied;
  default:
    return LookupError::PermanentResolverFailure;
  }
}

} // namespace

__wasi_address_family_t wasiFamilyOf(IpAddressFamily Family) noexcept {
  return Family == IpAddressFamily::Ipv4 ? __WASI_ADDRESS_FAMILY_INET4
                                         : __WASI_ADDRESS_FAMILY_INET6;
}

RawAddress IpSocketAddress::raw() const noexcept {
  RawAddress R;
  if (Family == IpAddressFamily::Ipv4) {
    R.Family = __WASI_ADDRESS_FAMILY_INET4;
    R.Length = 4;
    R.Port = V4.Port;
    R.Bytes[0] = std::get<0>(V4.Address);
    R.Bytes[1] = std::get<1>(V4.Address);
    R.Bytes[2] = std::get<2>(V4.Address);
    R.Bytes[3] = std::get<3>(V4.Address);
    return R;
  }
  R.Family = __WASI_ADDRESS_FAMILY_INET6;
  R.Length = 16;
  R.Port = V6.Port;
  const std::array<uint16_t, 8> Segments{
      std::get<0>(V6.Address), std::get<1>(V6.Address), std::get<2>(V6.Address),
      std::get<3>(V6.Address), std::get<4>(V6.Address), std::get<5>(V6.Address),
      std::get<6>(V6.Address), std::get<7>(V6.Address)};
  for (size_t I = 0; I < 8; ++I) {
    R.Bytes[2 * I] = static_cast<uint8_t>(Segments[I] >> 8);
    R.Bytes[2 * I + 1] = static_cast<uint8_t>(Segments[I] & 0xFF);
  }
  return R;
}

IpAddress addressOf(__wasi_address_family_t Family,
                    Span<const uint8_t> Bytes) noexcept {
  IpAddress A;
  if (Family == __WASI_ADDRESS_FAMILY_INET6 && Bytes.size() >= 16) {
    A.Family = IpAddressFamily::Ipv6;
    auto Seg = [&Bytes](size_t I) {
      return static_cast<uint16_t>((Bytes[2 * I] << 8) | Bytes[2 * I + 1]);
    };
    A.V6 = std::make_tuple(Seg(0), Seg(1), Seg(2), Seg(3), Seg(4), Seg(5),
                           Seg(6), Seg(7));
    return A;
  }
  A.Family = IpAddressFamily::Ipv4;
  if (Bytes.size() >= 4) {
    A.V4 = std::make_tuple(Bytes[0], Bytes[1], Bytes[2], Bytes[3]);
  }
  return A;
}

uint16_t portOf(uint16_t Wire) noexcept {
  uint8_t Bytes[2];
  std::memcpy(Bytes, &Wire, sizeof(Bytes));
  return static_cast<uint16_t>((Bytes[0] << 8) | Bytes[1]);
}

IpSocketAddress socketAddressOf(__wasi_address_family_t Family,
                                Span<const uint8_t> Bytes,
                                uint16_t Port) noexcept {
  const IpAddress Addr = addressOf(Family, Bytes);
  IpSocketAddress S;
  S.Family = Addr.Family;
  if (Addr.Family == IpAddressFamily::Ipv4) {
    S.V4 = Ipv4SocketAddress{Port, Addr.V4};
  } else {
    S.V6 = Ipv6SocketAddress{Port, 0, Addr.V6, 0};
  }
  return S;
}

WASI::WasiExpect<IpSocketAddress> nodeAddress(WASI::VINode &Node,
                                              bool Remote) noexcept {
  __wasi_address_family_t Family = __WASI_ADDRESS_FAMILY_UNSPEC;
  std::array<uint8_t, 16> Bytes{};
  uint16_t Port = 0;
  EXPECTED_TRY(Remote ? Node.sockGetPeerAddr(&Family, Bytes, &Port)
                      : Node.sockGetLocalAddr(&Family, Bytes, &Port));
  if (Family == __WASI_ADDRESS_FAMILY_UNSPEC) {
    return WASI::WasiUnexpect(__WASI_ERRNO_NOTCONN);
  }
  return socketAddressOf(Family, Bytes, Port);
}

WASI::WasiExpect<uint64_t> getOption(WASI::VINode &Node, IpAddressFamily Family,
                                     SocketOption Option) noexcept {
  int32_t Value = 0;
  switch (Option) {
  case SocketOption::KeepAliveEnabled:
  case SocketOption::ReceiveBufferSize:
  case SocketOption::SendBufferSize: {
    const auto Name =
        Option == SocketOption::KeepAliveEnabled ? __WASI_SOCK_OPT_SO_KEEPALIVE
        : Option == SocketOption::ReceiveBufferSize ? __WASI_SOCK_OPT_SO_RCVBUF
                                                    : __WASI_SOCK_OPT_SO_SNDBUF;
    Span<uint8_t> Flag(reinterpret_cast<uint8_t *>(&Value), sizeof(Value));
    EXPECTED_TRY(Node.sockGetOpt(__WASI_SOCK_OPT_LEVEL_SOL_SOCKET, Name, Flag));
    return static_cast<uint64_t>(Value);
  }
  default:
    break;
  }
#if WASMEDGE_OS_WINDOWS
  (void)Family;
  return WASI::WasiUnexpect(__WASI_ERRNO_NOTSUP);
#else
  EXPECTED_TRY(auto Handle, Node.getNativeHandler());
  const int Fd = static_cast<int>(Handle);
  int Level = IPPROTO_TCP;
  int Name = 0;
  switch (Option) {
  case SocketOption::KeepAliveIdleTime:
#if WASMEDGE_OS_MACOS
    Name = TCP_KEEPALIVE;
#else
    Name = TCP_KEEPIDLE;
#endif
    break;
  case SocketOption::KeepAliveInterval:
    Name = TCP_KEEPINTVL;
    break;
  case SocketOption::KeepAliveCount:
    Name = TCP_KEEPCNT;
    break;
  case SocketOption::HopLimit:
    Level = Family == IpAddressFamily::Ipv4 ? IPPROTO_IP : IPPROTO_IPV6;
    Name = Family == IpAddressFamily::Ipv4 ? IP_TTL : IPV6_UNICAST_HOPS;
    break;
  default:
    return WASI::WasiUnexpect(__WASI_ERRNO_NOTSUP);
  }
  socklen_t Size = sizeof(Value);
  if (::getsockopt(Fd, Level, Name, &Value, &Size) != 0) {
    return WASI::WasiUnexpect(__WASI_ERRNO_INVAL);
  }
  if (Option == SocketOption::KeepAliveIdleTime ||
      Option == SocketOption::KeepAliveInterval) {
    return static_cast<uint64_t>(Value) * 1000000000U;
  }
  return static_cast<uint64_t>(Value);
#endif
}

WASI::WasiExpect<void> setOption(WASI::VINode &Node, IpAddressFamily Family,
                                 SocketOption Option, uint64_t Value) noexcept {
  int32_t Native = static_cast<int32_t>(Value);
  switch (Option) {
  case SocketOption::KeepAliveEnabled:
  case SocketOption::ReceiveBufferSize:
  case SocketOption::SendBufferSize: {
    const auto Name =
        Option == SocketOption::KeepAliveEnabled ? __WASI_SOCK_OPT_SO_KEEPALIVE
        : Option == SocketOption::ReceiveBufferSize ? __WASI_SOCK_OPT_SO_RCVBUF
                                                    : __WASI_SOCK_OPT_SO_SNDBUF;
    return Node.sockSetOpt(
        __WASI_SOCK_OPT_LEVEL_SOL_SOCKET, Name,
        Span<const uint8_t>(reinterpret_cast<const uint8_t *>(&Native),
                            sizeof(Native)));
  }
  default:
    break;
  }
#if WASMEDGE_OS_WINDOWS
  (void)Family;
  return WASI::WasiUnexpect(__WASI_ERRNO_NOTSUP);
#else
  EXPECTED_TRY(auto Handle, Node.getNativeHandler());
  const int Fd = static_cast<int>(Handle);
  int Level = IPPROTO_TCP;
  int Name = 0;
  switch (Option) {
  case SocketOption::KeepAliveIdleTime:
  case SocketOption::KeepAliveInterval:
    // The kernel takes whole seconds; a shorter time rounds up to one.
    Native = static_cast<int32_t>((Value + 999999999U) / 1000000000U);
    if (Native < 1) {
      Native = 1;
    }
#if WASMEDGE_OS_MACOS
    Name = Option == SocketOption::KeepAliveIdleTime ? TCP_KEEPALIVE
                                                     : TCP_KEEPINTVL;
#else
    Name = Option == SocketOption::KeepAliveIdleTime ? TCP_KEEPIDLE
                                                     : TCP_KEEPINTVL;
#endif
    break;
  case SocketOption::KeepAliveCount:
    Name = TCP_KEEPCNT;
    break;
  case SocketOption::HopLimit:
    Level = Family == IpAddressFamily::Ipv4 ? IPPROTO_IP : IPPROTO_IPV6;
    Name = Family == IpAddressFamily::Ipv4 ? IP_TTL : IPV6_UNICAST_HOPS;
    break;
  default:
    return WASI::WasiUnexpect(__WASI_ERRNO_NOTSUP);
  }
  if (::setsockopt(Fd, Level, Name, &Native, sizeof(Native)) != 0) {
    return WASI::WasiUnexpect(__WASI_ERRNO_INVAL);
  }
  return {};
#endif
}

namespace {

// The local or peer address of a socket node.
SockResult<IpSocketAddress> queryAddress(WASI::VINode &Node,
                                         bool Remote) noexcept {
  auto Res = nodeAddress(Node, Remote);
  if (!Res) {
    return Unexpected<SocketError>(errorOf(Res.error()));
  }
  return *Res;
}

template <typename T>
SockResult<T> optionResult(WASI::WasiExpect<uint64_t> Res) noexcept {
  if (!Res) {
    return Unexpected<SocketError>(errorOf(Res.error()));
  }
  return static_cast<T>(*Res);
}

SockStatus statusOf(WASI::WasiExpect<void> Res) noexcept {
  if (!Res) {
    return Unexpected<SocketError>(errorOf(Res.error()));
  }
  return Runtime::Component::Unit{};
}

// Write all of Data to a connected socket.
WASI::WasiExpect<void> sendAll(WASI::VINode &Node,
                               Span<const uint8_t> Data) noexcept {
  while (!Data.empty()) {
    std::array<Span<const uint8_t>, 1> IOVs{Data};
    __wasi_size_t Sent = 0;
    EXPECTED_TRY(Node.sockSend(IOVs, static_cast<__wasi_siflags_t>(0), Sent));
    if (Sent == 0) {
      return WASI::WasiUnexpect(__WASI_ERRNO_PIPE);
    }
    Data = Data.subspan(Sent);
  }
  return {};
}

void resolveLater(
    const std::shared_ptr<Runtime::Component::HostTransmitEnd> &Done,
    SockStatus &&Status) noexcept {
  Done->writeDetached(
      Runtime::Component::Wit<SockStatus>::into(std::move(Status)));
}

} // namespace

Expect<SockResult<Runtime::Component::Own<TcpSocket>>>
TcpCreate::body(Runtime::Component::CallingFrame &, IpAddressFamily Family) {
  auto Node = WASI::VINode::sockOpen(wasiFamilyOf(Family),
                                     __WASI_SOCK_TYPE_SOCK_STREAM);
  if (!Node) {
    return SockResult<Runtime::Component::Own<TcpSocket>>(
        Unexpected<SocketError>(errorOf(Node.error())));
  }
  auto Sock = std::make_shared<TcpSocket>();
  Sock->Node = std::move(*Node);
  Sock->Family = Family;
  return SockResult<Runtime::Component::Own<TcpSocket>>(
      Runtime::Component::Own<TcpSocket>{Table.add(std::move(Sock))});
}

Expect<SockStatus> TcpBind::body(Runtime::Component::CallingFrame &,
                                 TcpBorrow Self, IpSocketAddress Local) {
  auto Sock = Table.get(Self.Rep);
  if (!Sock) {
    return SockStatus(Unexpected<SocketError>(SocketError::InvalidState));
  }
  if (Local.Family != Sock->Family) {
    return SockStatus(Unexpected<SocketError>(SocketError::InvalidArgument));
  }
  const auto Raw = Local.raw();
  return statusOf(Sock->Node->sockBind(Raw.Family, Raw.bytes(), Raw.Port));
}

Expect<SockStatus> TcpConnect::body(Runtime::Component::CallingFrame &Frame,
                                    TcpBorrow Self, IpSocketAddress Remote) {
  auto Sock = Table.get(Self.Rep);
  if (!Sock) {
    return SockStatus(Unexpected<SocketError>(SocketError::InvalidState));
  }
  if (Remote.Family != Sock->Family) {
    return SockStatus(Unexpected<SocketError>(SocketError::InvalidArgument));
  }
  auto Raw = std::make_shared<RawAddress>(Remote.raw());
  auto Node = Sock->Node;
  auto Res = std::make_shared<WASI::WasiExpect<void>>();
  EXPECTED_TRY(Frame.blocking([Node, Raw, Res]() {
    *Res = Node->sockConnect(Raw->Family, Raw->bytes(), Raw->Port);
  }));
  if (Frame.isCancelled()) {
    return SockStatus(Runtime::Component::Unit{});
  }
  return statusOf(*Res);
}

void TcpListen::declare(const Runtime::Component::TypeMinter &Mint) noexcept {
  HostFunction::declare(Mint);
  SocketType =
      Runtime::Component::Wit<Runtime::Component::Own<TcpSocket>>::type(Mint);
}

Expect<
    SockResult<Runtime::Component::Stream<Runtime::Component::Own<TcpSocket>>>>
TcpListen::body(Runtime::Component::CallingFrame &Frame, TcpBorrow Self) {
  auto Sock = Table.get(Self.Rep);
  if (!Sock) {
    return SockResult<
        Runtime::Component::Stream<Runtime::Component::Own<TcpSocket>>>(
        Unexpected<SocketError>(SocketError::InvalidState));
  }
  if (auto Res = Sock->Node->sockListen(static_cast<int32_t>(Sock->Backlog));
      !Res) {
    return SockResult<
        Runtime::Component::Stream<Runtime::Component::Own<TcpSocket>>>(
        Unexpected<SocketError>(errorOf(Res.error())));
  }
  Sock->Listening = true;
  auto Data =
      Runtime::Component::HostTransmitEnd::newWritable(Frame, true, SocketType);
  Runtime::Component::Stream<Runtime::Component::Own<TcpSocket>> Accepted{
      Data->getGuestValue()};
  auto Node = Sock->Node;
  const IpAddressFamily Family = Sock->Family;
  TcpTable *Sockets = &Table;
  Frame.spawn([Node, Data, Family,
               Sockets](Runtime::Component::CallingFrame &F) -> Expect<void> {
    while (true) {
      auto Res =
          std::make_shared<WASI::WasiExpect<std::shared_ptr<WASI::VINode>>>();
      EXPECTED_TRY(F.blocking([Node, Res]() {
        *Res = Node->sockAccept(static_cast<__wasi_fdflags_t>(0));
      }));
      if (!*Res) {
        break;
      }
      auto Conn = std::make_shared<TcpSocket>();
      Conn->Node = std::move(**Res);
      Conn->Family = Family;
      std::vector<ComponentValVariant> One{
          Runtime::Component::Wit<Runtime::Component::Own<TcpSocket>>::into(
              Runtime::Component::Own<TcpSocket>{
                  Sockets->add(std::move(Conn))})};
      EXPECTED_TRY(auto Outcome, Data->write(F, std::move(One)));
      if (Outcome.Result !=
          Runtime::Instance::Component::TransmitResult::Completed) {
        break;
      }
    }
    Data->drop();
    return {};
  });
  return SockResult<
      Runtime::Component::Stream<Runtime::Component::Own<TcpSocket>>>(
      std::move(Accepted));
}

void TcpSend::declare(const Runtime::Component::TypeMinter &Mint) noexcept {
  HostFunction::declare(Mint);
  ResultType = Runtime::Component::Wit<SockStatus>::type(Mint);
}

Expect<Runtime::Component::Future<SockStatus>>
TcpSend::body(Runtime::Component::CallingFrame &Frame, TcpBorrow Self,
              Runtime::Component::Stream<uint8_t> Data) {
  auto In =
      Runtime::Component::HostTransmitEnd::adoptReadable(Frame, Data.Shared);
  auto Done = Runtime::Component::HostTransmitEnd::newWritable(Frame, false,
                                                               ResultType);
  Runtime::Component::Future<SockStatus> Fut{Done->getGuestValue()};
  auto Sock = Table.get(Self.Rep);
  auto Failure = std::make_shared<SocketError>(SocketError::InvalidState);
  In->sink(
      [Sock, Failure](Span<const uint8_t> Bytes) {
        if (!Sock) {
          return false;
        }
        if (auto Res = sendAll(*Sock->Node, Bytes); !Res) {
          *Failure = errorOf(Res.error());
          return false;
        }
        return true;
      },
      [Done, Failure](bool Failed) {
        resolveLater(Done, Failed
                               ? SockStatus(Unexpected<SocketError>(*Failure))
                               : SockStatus(Runtime::Component::Unit{}));
      });
  return Fut;
}

void TcpReceive::declare(const Runtime::Component::TypeMinter &Mint) noexcept {
  HostFunction::declare(Mint);
  ResultType = Runtime::Component::Wit<SockStatus>::type(Mint);
}

Expect<std::tuple<Runtime::Component::Stream<uint8_t>,
                  Runtime::Component::Future<SockStatus>>>
TcpReceive::body(Runtime::Component::CallingFrame &Frame, TcpBorrow Self) {
  auto Data = Runtime::Component::HostTransmitEnd::newWritable(
      Frame, true, ComponentValType(ComponentTypeCode::U8));
  auto Done = Runtime::Component::HostTransmitEnd::newWritable(Frame, false,
                                                               ResultType);
  std::tuple<Runtime::Component::Stream<uint8_t>,
             Runtime::Component::Future<SockStatus>>
      Ends{Runtime::Component::Stream<uint8_t>{Data->getGuestValue()},
           Runtime::Component::Future<SockStatus>{Done->getGuestValue()}};
  auto Sock = Table.get(Self.Rep);
  if (!Sock) {
    Data->drop();
    resolveLater(
        Done, Unexpected<SocketError>(SocketError(SocketError::InvalidState)));
    return Ends;
  }
  auto Node = Sock->Node;
  Frame.spawn([Node, Data,
               Done](Runtime::Component::CallingFrame &F) -> Expect<void> {
    auto Buffer = std::make_shared<std::vector<uint8_t>>(65536);
    auto Read = std::make_shared<WASI::WasiExpect<__wasi_size_t>>(0);
    SockStatus Status{Runtime::Component::Unit{}};
    while (true) {
      EXPECTED_TRY(F.blocking([Node, Buffer, Read]() {
        std::array<Span<uint8_t>, 1> IOVs{Span<uint8_t>(*Buffer)};
        __wasi_size_t Count = 0;
        __wasi_roflags_t RoFlags = static_cast<__wasi_roflags_t>(0);
        if (auto Res = Node->sockRecv(IOVs, static_cast<__wasi_riflags_t>(0),
                                      Count, RoFlags);
            !Res) {
          *Read = WASI::WasiUnexpect(Res.error());
        } else {
          *Read = Count;
        }
      }));
      if (!*Read) {
        Status = Unexpected<SocketError>(errorOf(Read->error()));
        break;
      }
      if (**Read == 0) {
        break;
      }
      EXPECTED_TRY(auto Outcome,
                   Data->write(F, Span<const uint8_t>(Buffer->data(), **Read)));
      if (Outcome.Result !=
          Runtime::Instance::Component::TransmitResult::Completed) {
        break;
      }
    }
    Data->drop();
    resolveLater(Done, std::move(Status));
    return {};
  });
  return Ends;
}

Expect<SockResult<IpSocketAddress>>
TcpGetAddress::body(Runtime::Component::CallingFrame &, TcpBorrow Self) {
  auto Sock = Table.get(Self.Rep);
  if (!Sock) {
    return SockResult<IpSocketAddress>(
        Unexpected<SocketError>(SocketError::InvalidState));
  }
  return queryAddress(*Sock->Node, Remote);
}

Expect<bool> TcpGetIsListening::body(Runtime::Component::CallingFrame &,
                                     TcpBorrow Self) {
  auto Sock = Table.get(Self.Rep);
  return Sock && Sock->Listening;
}

Expect<IpAddressFamily>
TcpGetAddressFamily::body(Runtime::Component::CallingFrame &, TcpBorrow Self) {
  auto Sock = Table.get(Self.Rep);
  return Sock ? Sock->Family : IpAddressFamily::Ipv4;
}

Expect<SockStatus>
TcpSetListenBacklogSize::body(Runtime::Component::CallingFrame &,
                              TcpBorrow Self, uint64_t Value) {
  auto Sock = Table.get(Self.Rep);
  if (!Sock) {
    return SockStatus(Unexpected<SocketError>(SocketError::InvalidState));
  }
  if (Value == 0) {
    return SockStatus(Unexpected<SocketError>(SocketError::InvalidArgument));
  }
  Sock->Backlog = Value > 4096 ? 4096 : Value;
  if (Sock->Listening) {
    return statusOf(
        Sock->Node->sockListen(static_cast<int32_t>(Sock->Backlog)));
  }
  return SockStatus(Runtime::Component::Unit{});
}

Expect<SockResult<bool>>
TcpGetBoolOption::body(Runtime::Component::CallingFrame &, TcpBorrow Self) {
  auto Sock = Table.get(Self.Rep);
  if (!Sock) {
    return SockResult<bool>(Unexpected<SocketError>(SocketError::InvalidState));
  }
  return optionResult<bool>(getOption(*Sock->Node, Sock->Family, Option));
}

Expect<SockStatus> TcpSetBoolOption::body(Runtime::Component::CallingFrame &,
                                          TcpBorrow Self, bool Value) {
  auto Sock = Table.get(Self.Rep);
  if (!Sock) {
    return SockStatus(Unexpected<SocketError>(SocketError::InvalidState));
  }
  return statusOf(setOption(*Sock->Node, Sock->Family, Option, Value ? 1 : 0));
}

Expect<SockResult<uint64_t>>
TcpGetU64Option::body(Runtime::Component::CallingFrame &, TcpBorrow Self) {
  auto Sock = Table.get(Self.Rep);
  if (!Sock) {
    return SockResult<uint64_t>(
        Unexpected<SocketError>(SocketError::InvalidState));
  }
  return optionResult<uint64_t>(getOption(*Sock->Node, Sock->Family, Option));
}

Expect<SockStatus> TcpSetU64Option::body(Runtime::Component::CallingFrame &,
                                         TcpBorrow Self, uint64_t Value) {
  auto Sock = Table.get(Self.Rep);
  if (!Sock) {
    return SockStatus(Unexpected<SocketError>(SocketError::InvalidState));
  }
  if (Value == 0) {
    return SockStatus(Unexpected<SocketError>(SocketError::InvalidArgument));
  }
  return statusOf(setOption(*Sock->Node, Sock->Family, Option, Value));
}

Expect<SockResult<uint32_t>>
TcpGetU32Option::body(Runtime::Component::CallingFrame &, TcpBorrow Self) {
  auto Sock = Table.get(Self.Rep);
  if (!Sock) {
    return SockResult<uint32_t>(
        Unexpected<SocketError>(SocketError::InvalidState));
  }
  return optionResult<uint32_t>(getOption(*Sock->Node, Sock->Family, Option));
}

Expect<SockStatus> TcpSetU32Option::body(Runtime::Component::CallingFrame &,
                                         TcpBorrow Self, uint32_t Value) {
  auto Sock = Table.get(Self.Rep);
  if (!Sock) {
    return SockStatus(Unexpected<SocketError>(SocketError::InvalidState));
  }
  if (Value == 0) {
    return SockStatus(Unexpected<SocketError>(SocketError::InvalidArgument));
  }
  return statusOf(setOption(*Sock->Node, Sock->Family, Option, Value));
}

Expect<SockResult<uint8_t>>
TcpGetU8Option::body(Runtime::Component::CallingFrame &, TcpBorrow Self) {
  auto Sock = Table.get(Self.Rep);
  if (!Sock) {
    return SockResult<uint8_t>(
        Unexpected<SocketError>(SocketError::InvalidState));
  }
  return optionResult<uint8_t>(getOption(*Sock->Node, Sock->Family, Option));
}

Expect<SockStatus> TcpSetU8Option::body(Runtime::Component::CallingFrame &,
                                        TcpBorrow Self, uint8_t Value) {
  auto Sock = Table.get(Self.Rep);
  if (!Sock) {
    return SockStatus(Unexpected<SocketError>(SocketError::InvalidState));
  }
  if (Value == 0) {
    return SockStatus(Unexpected<SocketError>(SocketError::InvalidArgument));
  }
  return statusOf(setOption(*Sock->Node, Sock->Family, Option, Value));
}

Expect<SockResult<Runtime::Component::Own<UdpSocket>>>
UdpCreate::body(Runtime::Component::CallingFrame &, IpAddressFamily Family) {
  auto Node =
      WASI::VINode::sockOpen(wasiFamilyOf(Family), __WASI_SOCK_TYPE_SOCK_DGRAM);
  if (!Node) {
    return SockResult<Runtime::Component::Own<UdpSocket>>(
        Unexpected<SocketError>(errorOf(Node.error())));
  }
  auto Sock = std::make_shared<UdpSocket>();
  Sock->Node = std::move(*Node);
  Sock->Family = Family;
  return SockResult<Runtime::Component::Own<UdpSocket>>(
      Runtime::Component::Own<UdpSocket>{Table.add(std::move(Sock))});
}

Expect<SockStatus> UdpBind::body(Runtime::Component::CallingFrame &,
                                 UdpBorrow Self, IpSocketAddress Local) {
  auto Sock = Table.get(Self.Rep);
  if (!Sock) {
    return SockStatus(Unexpected<SocketError>(SocketError::InvalidState));
  }
  if (Local.Family != Sock->Family) {
    return SockStatus(Unexpected<SocketError>(SocketError::InvalidArgument));
  }
  const auto Raw = Local.raw();
  return statusOf(Sock->Node->sockBind(Raw.Family, Raw.bytes(), Raw.Port));
}

Expect<SockStatus> UdpConnect::body(Runtime::Component::CallingFrame &,
                                    UdpBorrow Self, IpSocketAddress Remote) {
  auto Sock = Table.get(Self.Rep);
  if (!Sock) {
    return SockStatus(Unexpected<SocketError>(SocketError::InvalidState));
  }
  if (Remote.Family != Sock->Family) {
    return SockStatus(Unexpected<SocketError>(SocketError::InvalidArgument));
  }
  Sock->Remote = Remote;
  return SockStatus(Runtime::Component::Unit{});
}

Expect<SockStatus> UdpDisconnect::body(Runtime::Component::CallingFrame &,
                                       UdpBorrow Self) {
  auto Sock = Table.get(Self.Rep);
  if (!Sock) {
    return SockStatus(Unexpected<SocketError>(SocketError::InvalidState));
  }
  if (!Sock->Remote.has_value()) {
    return SockStatus(Unexpected<SocketError>(SocketError::InvalidState));
  }
  Sock->Remote.reset();
  return SockStatus(Runtime::Component::Unit{});
}

Expect<SockStatus> UdpSend::body(Runtime::Component::CallingFrame &,
                                 UdpBorrow Self, std::vector<uint8_t> Data,
                                 std::optional<IpSocketAddress> Remote) {
  auto Sock = Table.get(Self.Rep);
  if (!Sock) {
    return SockStatus(Unexpected<SocketError>(SocketError::InvalidState));
  }
  if (!Remote.has_value()) {
    Remote = Sock->Remote;
  } else if (Sock->Remote.has_value()) {
    return SockStatus(Unexpected<SocketError>(SocketError::InvalidArgument));
  }
  if (!Remote.has_value()) {
    return SockStatus(Unexpected<SocketError>(SocketError::InvalidArgument));
  }
  if (Remote->Family != Sock->Family) {
    return SockStatus(Unexpected<SocketError>(SocketError::InvalidArgument));
  }
  const auto Raw = Remote->raw();
  std::array<Span<const uint8_t>, 1> IOVs{Span<const uint8_t>(Data)};
  __wasi_size_t Sent = 0;
  return statusOf(Sock->Node->sockSendTo(IOVs, static_cast<__wasi_siflags_t>(0),
                                         Raw.Family, Raw.bytes(), Raw.Port,
                                         Sent));
}

Expect<SockResult<std::tuple<std::vector<uint8_t>, IpSocketAddress>>>
UdpReceive::body(Runtime::Component::CallingFrame &Frame, UdpBorrow Self) {
  auto Sock = Table.get(Self.Rep);
  if (!Sock) {
    return SockResult<std::tuple<std::vector<uint8_t>, IpSocketAddress>>(
        Unexpected<SocketError>(SocketError::InvalidState));
  }
  auto Node = Sock->Node;
  // A datagram from another sender than the connected one is dropped.
  const auto Wanted = Sock->Remote;
  while (true) {
    auto Buffer = std::make_shared<std::vector<uint8_t>>(65536);
    struct Received {
      WASI::WasiExpect<__wasi_size_t> Count{0};
      __wasi_address_family_t Family = __WASI_ADDRESS_FAMILY_UNSPEC;
      std::array<uint8_t, 16> Bytes{};
      uint16_t Port = 0;
    };
    auto Got = std::make_shared<Received>();
    EXPECTED_TRY(Frame.blocking([Node, Buffer, Got]() {
      std::array<Span<uint8_t>, 1> IOVs{Span<uint8_t>(*Buffer)};
      __wasi_size_t Count = 0;
      __wasi_roflags_t RoFlags = static_cast<__wasi_roflags_t>(0);
      if (auto Res = Node->sockRecvFrom(IOVs, static_cast<__wasi_riflags_t>(0),
                                        &Got->Family, Got->Bytes, &Got->Port,
                                        Count, RoFlags);
          !Res) {
        Got->Count = WASI::WasiUnexpect(Res.error());
      } else {
        Got->Count = Count;
      }
    }));
    if (Frame.isCancelled()) {
      return SockResult<std::tuple<std::vector<uint8_t>, IpSocketAddress>>(
          Unexpected<SocketError>(SocketError::InvalidState));
    }
    if (!Got->Count) {
      return SockResult<std::tuple<std::vector<uint8_t>, IpSocketAddress>>(
          Unexpected<SocketError>(errorOf(Got->Count.error())));
    }
    IpSocketAddress From =
        socketAddressOf(Got->Family, Got->Bytes, portOf(Got->Port));
    if (Wanted.has_value()) {
      if (Wanted->raw() != From.raw()) {
        continue;
      }
    }
    Buffer->resize(*Got->Count);
    return SockResult<std::tuple<std::vector<uint8_t>, IpSocketAddress>>(
        std::make_tuple(std::move(*Buffer), From));
  }
}

Expect<SockResult<IpSocketAddress>>
UdpGetAddress::body(Runtime::Component::CallingFrame &, UdpBorrow Self) {
  auto Sock = Table.get(Self.Rep);
  if (!Sock) {
    return SockResult<IpSocketAddress>(
        Unexpected<SocketError>(SocketError::InvalidState));
  }
  if (Remote) {
    if (!Sock->Remote.has_value()) {
      return SockResult<IpSocketAddress>(
          Unexpected<SocketError>(SocketError::InvalidState));
    }
    return SockResult<IpSocketAddress>(*Sock->Remote);
  }
  return queryAddress(*Sock->Node, false);
}

Expect<IpAddressFamily>
UdpGetAddressFamily::body(Runtime::Component::CallingFrame &, UdpBorrow Self) {
  auto Sock = Table.get(Self.Rep);
  return Sock ? Sock->Family : IpAddressFamily::Ipv4;
}

Expect<SockResult<uint64_t>>
UdpGetU64Option::body(Runtime::Component::CallingFrame &, UdpBorrow Self) {
  auto Sock = Table.get(Self.Rep);
  if (!Sock) {
    return SockResult<uint64_t>(
        Unexpected<SocketError>(SocketError::InvalidState));
  }
  return optionResult<uint64_t>(getOption(*Sock->Node, Sock->Family, Option));
}

Expect<SockStatus> UdpSetU64Option::body(Runtime::Component::CallingFrame &,
                                         UdpBorrow Self, uint64_t Value) {
  auto Sock = Table.get(Self.Rep);
  if (!Sock) {
    return SockStatus(Unexpected<SocketError>(SocketError::InvalidState));
  }
  if (Value == 0) {
    return SockStatus(Unexpected<SocketError>(SocketError::InvalidArgument));
  }
  return statusOf(setOption(*Sock->Node, Sock->Family, Option, Value));
}

Expect<SockResult<uint8_t>>
UdpGetU8Option::body(Runtime::Component::CallingFrame &, UdpBorrow Self) {
  auto Sock = Table.get(Self.Rep);
  if (!Sock) {
    return SockResult<uint8_t>(
        Unexpected<SocketError>(SocketError::InvalidState));
  }
  return optionResult<uint8_t>(getOption(*Sock->Node, Sock->Family, Option));
}

Expect<SockStatus> UdpSetU8Option::body(Runtime::Component::CallingFrame &,
                                        UdpBorrow Self, uint8_t Value) {
  auto Sock = Table.get(Self.Rep);
  if (!Sock) {
    return SockStatus(Unexpected<SocketError>(SocketError::InvalidState));
  }
  if (Value == 0) {
    return SockStatus(Unexpected<SocketError>(SocketError::InvalidArgument));
  }
  return statusOf(setOption(*Sock->Node, Sock->Family, Option, Value));
}

WASI::WasiExpect<std::vector<IpAddress>>
resolveHostname(const std::string &Name) noexcept {
  // The preview-1 resolver fills caller-owned records.
  constexpr uint32_t Max = 16;
  std::array<__wasi_addrinfo_t, Max> Infos{};
  std::array<__wasi_sockaddr_t, Max> Addrs{};
  std::array<std::array<char, 32>, Max> Data{};
  std::array<std::array<char, 256>, Max> Names{};
  std::array<__wasi_addrinfo_t *, Max> InfoPtrs;
  std::array<__wasi_sockaddr_t *, Max> AddrPtrs;
  std::array<char *, Max> DataPtrs;
  std::array<char *, Max> NamePtrs;
  for (uint32_t I = 0; I < Max; ++I) {
    InfoPtrs[I] = &Infos[I];
    AddrPtrs[I] = &Addrs[I];
    DataPtrs[I] = Data[I].data();
    NamePtrs[I] = Names[I].data();
  }
  __wasi_addrinfo_t Hint;
  std::memset(&Hint, 0, sizeof(Hint));
  Hint.ai_family = __WASI_ADDRESS_FAMILY_UNSPEC;
  Hint.ai_socktype = __WASI_SOCK_TYPE_SOCK_ANY;
  __wasi_size_t Count = 0;
  EXPECTED_TRY(WASI::VINode::getAddrinfo(Name, "", Hint, Max, InfoPtrs,
                                         AddrPtrs, DataPtrs, NamePtrs, Count));
  std::vector<IpAddress> Addresses;
  for (uint32_t I = 0; I < Count; ++I) {
    const auto &Addr = Addrs[I];
    const auto *Bytes = reinterpret_cast<const uint8_t *>(Data[I].data());
    // sa_data starts with the port; the address follows it (after the flow
    // information for IPv6).
    IpAddress A;
    if (Addr.sa_family == __WASI_ADDRESS_FAMILY_INET4 &&
        Addr.sa_data_len >= 6) {
      A = addressOf(__WASI_ADDRESS_FAMILY_INET4,
                    Span<const uint8_t>(Bytes + 2, 4));
    } else if (Addr.sa_family == __WASI_ADDRESS_FAMILY_INET6 &&
               Addr.sa_data_len >= 22) {
      A = addressOf(__WASI_ADDRESS_FAMILY_INET6,
                    Span<const uint8_t>(Bytes + 6, 16));
    } else {
      continue;
    }
    bool Seen = false;
    for (const auto &Known : Addresses) {
      if (Known.Family == A.Family && Known.V4 == A.V4 && Known.V6 == A.V6) {
        Seen = true;
        break;
      }
    }
    if (!Seen) {
      Addresses.push_back(A);
    }
  }
  return Addresses;
}

Expect<Expected<std::vector<IpAddress>, LookupError>>
ResolveAddresses::body(Runtime::Component::CallingFrame &Frame,
                       std::string Name) {
  auto Result = std::make_shared<WASI::WasiExpect<std::vector<IpAddress>>>();
  auto Host = std::make_shared<std::string>(std::move(Name));
  EXPECTED_TRY(
      Frame.blocking([Result, Host]() { *Result = resolveHostname(*Host); }));
  if (Frame.isCancelled()) {
    return Expected<std::vector<IpAddress>, LookupError>(
        std::vector<IpAddress>{});
  }
  if (!*Result) {
    return Expected<std::vector<IpAddress>, LookupError>(
        Unexpected<LookupError>(lookupErrorOf(Result->error())));
  }
  return Expected<std::vector<IpAddress>, LookupError>(std::move(**Result));
}

SocketsTypesInstance::SocketsTypesInstance()
    : ComponentInstance("wasi:sockets/types@0.3.1") {
  exportType("tcp-socket", addHostResourceType<TcpSocket>([this](uint64_t Rep) {
               TcpSockets.remove(Rep);
             }));
  exportType("udp-socket", addHostResourceType<UdpSocket>([this](uint64_t Rep) {
               UdpSockets.remove(Rep);
             }));
  const auto Mint = getTypeMinter();
  exportType("duration", Mint.primDefType(PrimValType::U64).getTypeIndex());
  exportType("error-code",
             Runtime::Component::Wit<SocketError>::type(Mint).getTypeIndex());
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

  auto &Tcp = TcpSockets;
  addHostFunc("[static]tcp-socket.create", std::make_unique<TcpCreate>(Tcp));
  addHostFunc("[method]tcp-socket.bind", std::make_unique<TcpBind>(Tcp));
  addHostFunc("[method]tcp-socket.connect", std::make_unique<TcpConnect>(Tcp));
  addHostFunc("[method]tcp-socket.listen", std::make_unique<TcpListen>(Tcp));
  addHostFunc("[method]tcp-socket.send", std::make_unique<TcpSend>(Tcp));
  addHostFunc("[method]tcp-socket.receive", std::make_unique<TcpReceive>(Tcp));
  addHostFunc("[method]tcp-socket.get-local-address",
              std::make_unique<TcpGetAddress>(Tcp, false));
  addHostFunc("[method]tcp-socket.get-remote-address",
              std::make_unique<TcpGetAddress>(Tcp, true));
  addHostFunc("[method]tcp-socket.get-is-listening",
              std::make_unique<TcpGetIsListening>(Tcp));
  addHostFunc("[method]tcp-socket.get-address-family",
              std::make_unique<TcpGetAddressFamily>(Tcp));
  addHostFunc("[method]tcp-socket.set-listen-backlog-size",
              std::make_unique<TcpSetListenBacklogSize>(Tcp));
  addHostFunc(
      "[method]tcp-socket.get-keep-alive-enabled",
      std::make_unique<TcpGetBoolOption>(Tcp, SocketOption::KeepAliveEnabled));
  addHostFunc(
      "[method]tcp-socket.set-keep-alive-enabled",
      std::make_unique<TcpSetBoolOption>(Tcp, SocketOption::KeepAliveEnabled));
  addHostFunc(
      "[method]tcp-socket.get-keep-alive-idle-time",
      std::make_unique<TcpGetU64Option>(Tcp, SocketOption::KeepAliveIdleTime));
  addHostFunc(
      "[method]tcp-socket.set-keep-alive-idle-time",
      std::make_unique<TcpSetU64Option>(Tcp, SocketOption::KeepAliveIdleTime));
  addHostFunc(
      "[method]tcp-socket.get-keep-alive-interval",
      std::make_unique<TcpGetU64Option>(Tcp, SocketOption::KeepAliveInterval));
  addHostFunc(
      "[method]tcp-socket.set-keep-alive-interval",
      std::make_unique<TcpSetU64Option>(Tcp, SocketOption::KeepAliveInterval));
  addHostFunc(
      "[method]tcp-socket.get-keep-alive-count",
      std::make_unique<TcpGetU32Option>(Tcp, SocketOption::KeepAliveCount));
  addHostFunc(
      "[method]tcp-socket.set-keep-alive-count",
      std::make_unique<TcpSetU32Option>(Tcp, SocketOption::KeepAliveCount));
  addHostFunc("[method]tcp-socket.get-hop-limit",
              std::make_unique<TcpGetU8Option>(Tcp, SocketOption::HopLimit));
  addHostFunc("[method]tcp-socket.set-hop-limit",
              std::make_unique<TcpSetU8Option>(Tcp, SocketOption::HopLimit));
  addHostFunc(
      "[method]tcp-socket.get-receive-buffer-size",
      std::make_unique<TcpGetU64Option>(Tcp, SocketOption::ReceiveBufferSize));
  addHostFunc(
      "[method]tcp-socket.set-receive-buffer-size",
      std::make_unique<TcpSetU64Option>(Tcp, SocketOption::ReceiveBufferSize));
  addHostFunc(
      "[method]tcp-socket.get-send-buffer-size",
      std::make_unique<TcpGetU64Option>(Tcp, SocketOption::SendBufferSize));
  addHostFunc(
      "[method]tcp-socket.set-send-buffer-size",
      std::make_unique<TcpSetU64Option>(Tcp, SocketOption::SendBufferSize));

  auto &Udp = UdpSockets;
  addHostFunc("[static]udp-socket.create", std::make_unique<UdpCreate>(Udp));
  addHostFunc("[method]udp-socket.bind", std::make_unique<UdpBind>(Udp));
  addHostFunc("[method]udp-socket.connect", std::make_unique<UdpConnect>(Udp));
  addHostFunc("[method]udp-socket.disconnect",
              std::make_unique<UdpDisconnect>(Udp));
  addHostFunc("[method]udp-socket.send", std::make_unique<UdpSend>(Udp));
  addHostFunc("[method]udp-socket.receive", std::make_unique<UdpReceive>(Udp));
  addHostFunc("[method]udp-socket.get-local-address",
              std::make_unique<UdpGetAddress>(Udp, false));
  addHostFunc("[method]udp-socket.get-remote-address",
              std::make_unique<UdpGetAddress>(Udp, true));
  addHostFunc("[method]udp-socket.get-address-family",
              std::make_unique<UdpGetAddressFamily>(Udp));
  addHostFunc("[method]udp-socket.get-unicast-hop-limit",
              std::make_unique<UdpGetU8Option>(Udp, SocketOption::HopLimit));
  addHostFunc("[method]udp-socket.set-unicast-hop-limit",
              std::make_unique<UdpSetU8Option>(Udp, SocketOption::HopLimit));
  addHostFunc(
      "[method]udp-socket.get-receive-buffer-size",
      std::make_unique<UdpGetU64Option>(Udp, SocketOption::ReceiveBufferSize));
  addHostFunc(
      "[method]udp-socket.set-receive-buffer-size",
      std::make_unique<UdpSetU64Option>(Udp, SocketOption::ReceiveBufferSize));
  addHostFunc(
      "[method]udp-socket.get-send-buffer-size",
      std::make_unique<UdpGetU64Option>(Udp, SocketOption::SendBufferSize));
  addHostFunc(
      "[method]udp-socket.set-send-buffer-size",
      std::make_unique<UdpSetU64Option>(Udp, SocketOption::SendBufferSize));
}

IpNameLookupInstance::IpNameLookupInstance()
    : ComponentInstance("wasi:sockets/ip-name-lookup@0.3.1") {
  const auto Mint = getTypeMinter();
  exportType("ip-address",
             Runtime::Component::Wit<IpAddress>::type(Mint).getTypeIndex());
  exportType("error-code",
             Runtime::Component::Wit<LookupError>::type(Mint).getTypeIndex());
  addHostFunc("resolve-addresses", std::make_unique<ResolveAddresses>());
}

std::vector<std::unique_ptr<Runtime::Instance::ComponentInstance>>
makeSocketsInstances() {
  std::vector<std::unique_ptr<Runtime::Instance::ComponentInstance>> Out;
  Out.push_back(std::make_unique<SocketsTypesInstance>());
  Out.push_back(std::make_unique<IpNameLookupInstance>());
  return Out;
}

} // namespace WasiP3
} // namespace Host
} // namespace WasmEdge
