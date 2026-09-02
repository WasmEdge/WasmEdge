// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/host/wasi/p2/sockets.h - wasi:sockets host ---------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the host component instances of `wasi:sockets@0.2.12`:
/// `network`, `instance-network`, `ip-name-lookup`, `tcp`,
/// `tcp-create-socket`, `udp`, and `udp-create-socket`, over the preview-1
/// socket nodes and the `wasi:io` streams and pollables. The address types
/// are those of 0.3.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/component_valtype.h"
#include "common/component_variant.h"
#include "common/errcode.h"
#include "common/expected.h"
#include "host/wasi/p2/io.h"
#include "host/wasi/p3/sockets.h"
#include "host/wasi/vinode.h"
#include "runtime/component/callingframe.h"
#include "runtime/component/hostfunc.h"
#include "runtime/component/resourcetable.h"
#include "runtime/component/wit.h"
#include "runtime/instance/component/component.h"
#include "runtime/instance/component/resource.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace WasiP2 {

using WasiP3::IpAddress;
using WasiP3::IpAddressFamily;
using WasiP3::IpSocketAddress;
using WasiP3::Ipv4Address;
using WasiP3::Ipv4SocketAddress;
using WasiP3::Ipv6Address;
using WasiP3::Ipv6SocketAddress;
using WasiP3::SocketOption;

/// `wasi:sockets/network@0.2.12` `error-code`.
enum class NetworkError : uint32_t {
  Unknown,
  AccessDenied,
  NotSupported,
  InvalidArgument,
  OutOfMemory,
  Timeout,
  ConcurrencyConflict,
  NotInProgress,
  WouldBlock,
  InvalidState,
  NewSocketLimit,
  AddressNotBindable,
  AddressInUse,
  RemoteUnreachable,
  ConnectionRefused,
  ConnectionReset,
  ConnectionAborted,
  DatagramTooLarge,
  NameUnresolvable,
  TemporaryResolverFailure,
  PermanentResolverFailure,
};

enum class ShutdownType : uint32_t { Receive, Send, Both };

/// The `network` resource carries no state.
struct Network {};

/// The addresses a name resolved to, handed out one at a time.
struct ResolveAddressStream {
  std::vector<IpAddress> Addresses;
  size_t Next = 0;
};

/// The state behind a `tcp-socket` handle. Binding, connecting, and
/// listening complete in their `start-` call; the `finish-` call reports.
struct TcpSocket {
  enum class State { Unbound, Bound, Connected, Listening, Failed };
  std::shared_ptr<WASI::VINode> Node;
  IpAddressFamily Family = IpAddressFamily::Ipv4;
  State St = State::Unbound;
  /// A `start-` operation whose `finish-` is still due, with its outcome.
  std::optional<NetworkError> Pending;
  bool PendingBind = false;
  bool PendingConnect = false;
  bool PendingListen = false;
  uint64_t Backlog = 128;
};

/// The state behind a `udp-socket` handle and its datagram streams.
struct UdpSocket {
  std::shared_ptr<WASI::VINode> Node;
  IpAddressFamily Family = IpAddressFamily::Ipv4;
  bool Bound = false;
  bool PendingBind = false;
  std::optional<NetworkError> Pending;
  std::optional<IpSocketAddress> Remote;
};

struct IncomingDatagram {
  std::vector<uint8_t> Data;
  IpSocketAddress Remote;
};

struct OutgoingDatagram {
  std::vector<uint8_t> Data;
  std::optional<IpSocketAddress> Remote;
};

/// The datagram streams of a socket share its state.
struct IncomingDatagramStream {
  std::shared_ptr<UdpSocket> Socket;
};
struct OutgoingDatagramStream {
  std::shared_ptr<UdpSocket> Socket;
};

using ResolveTable = Runtime::Component::ResourceTable<ResolveAddressStream>;
using TcpTable = Runtime::Component::ResourceTable<TcpSocket>;
using UdpTable = Runtime::Component::ResourceTable<UdpSocket>;
using IncomingTable = Runtime::Component::ResourceTable<IncomingDatagramStream>;
using OutgoingTable = Runtime::Component::ResourceTable<OutgoingDatagramStream>;
using NetworkBorrow = Runtime::Component::Borrow<Network>;
using ResolveBorrow = Runtime::Component::Borrow<ResolveAddressStream>;
using TcpBorrow = Runtime::Component::Borrow<TcpSocket>;
using UdpBorrow = Runtime::Component::Borrow<UdpSocket>;
using IncomingBorrow = Runtime::Component::Borrow<IncomingDatagramStream>;
using OutgoingBorrow = Runtime::Component::Borrow<OutgoingDatagramStream>;
template <typename T> using NetResult = Expected<T, NetworkError>;
using NetStatus = Expected<Runtime::Component::Unit, NetworkError>;

/// The tables of the sockets, shared by the instances of the package.
struct SocketsHost {
  SocketsHost(IoHost &H) noexcept : Io(H) {}
  IoHost &Io;
  ResolveTable Resolves;
  TcpTable TcpSockets;
  UdpTable UdpSockets;
  IncomingTable Incoming;
  OutgoingTable Outgoing;
  const Runtime::Instance::Component::ResourceTypeInstance *NetworkType =
      nullptr;
  const Runtime::Instance::Component::ResourceTypeInstance *TcpType = nullptr;
  const Runtime::Instance::Component::ResourceTypeInstance *UdpType = nullptr;

  /// The socket behind a borrow, if the handle is live.
  std::shared_ptr<TcpSocket> socket(TcpBorrow Self) noexcept {
    return TcpSockets.get(Self.Rep);
  }
  std::shared_ptr<UdpSocket> socket(UdpBorrow Self) noexcept {
    return UdpSockets.get(Self.Rep);
  }
};

} // namespace WasiP2
} // namespace Host

namespace Runtime {
namespace Component {

template <>
struct Wit<Host::WasiP2::NetworkError>
    : WitEnum<Host::WasiP2::NetworkError, Wit<Host::WasiP2::NetworkError>> {
  static constexpr const char *Labels[] = {"unknown",
                                           "access-denied",
                                           "not-supported",
                                           "invalid-argument",
                                           "out-of-memory",
                                           "timeout",
                                           "concurrency-conflict",
                                           "not-in-progress",
                                           "would-block",
                                           "invalid-state",
                                           "new-socket-limit",
                                           "address-not-bindable",
                                           "address-in-use",
                                           "remote-unreachable",
                                           "connection-refused",
                                           "connection-reset",
                                           "connection-aborted",
                                           "datagram-too-large",
                                           "name-unresolvable",
                                           "temporary-resolver-failure",
                                           "permanent-resolver-failure"};
};

template <>
struct Wit<Host::WasiP2::ShutdownType>
    : WitEnum<Host::WasiP2::ShutdownType, Wit<Host::WasiP2::ShutdownType>> {
  static constexpr const char *Labels[] = {"receive", "send", "both"};
};

template <> struct Wit<Host::WasiP2::IncomingDatagram> {
  static ComponentValType type(const TypeMinter &Mint) noexcept {
    return WitRecord::type(
        Mint,
        {{"data", Wit<std::vector<uint8_t>>::type(Mint)},
         {"remote-address", Wit<Host::WasiP2::IpSocketAddress>::type(Mint)}});
  }
  static Host::WasiP2::IncomingDatagram from(const ComponentValVariant &V) {
    auto F = WitRecord::fields(V);
    return {Wit<std::vector<uint8_t>>::from(F[0].second),
            Wit<Host::WasiP2::IpSocketAddress>::from(F[1].second)};
  }
  static ComponentValVariant into(Host::WasiP2::IncomingDatagram &&D) noexcept {
    return WitRecord::into(
        {{"data", Wit<std::vector<uint8_t>>::into(std::move(D.Data))},
         {"remote-address",
          Wit<Host::WasiP2::IpSocketAddress>::into(std::move(D.Remote))}});
  }
};

template <> struct Wit<Host::WasiP2::OutgoingDatagram> {
  static ComponentValType type(const TypeMinter &Mint) noexcept {
    return WitRecord::type(
        Mint,
        {{"data", Wit<std::vector<uint8_t>>::type(Mint)},
         {"remote-address",
          Wit<std::optional<Host::WasiP2::IpSocketAddress>>::type(Mint)}});
  }
  static Host::WasiP2::OutgoingDatagram from(const ComponentValVariant &V) {
    auto F = WitRecord::fields(V);
    return {
        Wit<std::vector<uint8_t>>::from(F[0].second),
        Wit<std::optional<Host::WasiP2::IpSocketAddress>>::from(F[1].second)};
  }
  static ComponentValVariant into(Host::WasiP2::OutgoingDatagram &&D) noexcept {
    return WitRecord::into(
        {{"data", Wit<std::vector<uint8_t>>::into(std::move(D.Data))},
         {"remote-address",
          Wit<std::optional<Host::WasiP2::IpSocketAddress>>::into(
              std::move(D.Remote))}});
  }
};

} // namespace Component
} // namespace Runtime

namespace Host {
namespace WasiP2 {

/// The base of the socket functions: the tables they resolve in.
template <typename T>
class SocketFunction : public Runtime::Component::HostFunction<T> {
public:
  SocketFunction(SocketsHost &S) noexcept : Sockets(S) {}

protected:
  SocketsHost &Sockets;
};

/// `instance-network.instance-network: func() -> network`
class InstanceNetwork : public SocketFunction<InstanceNetwork> {
public:
  using SocketFunction::SocketFunction;
  Expect<Runtime::Component::Own<Network>>
  body(Runtime::Component::CallingFrame &);
};

/// `ip-name-lookup.resolve-addresses: func(network: borrow<network>, name:
/// string) -> result<resolve-address-stream, error-code>`
class ResolveAddresses : public SocketFunction<ResolveAddresses> {
public:
  static constexpr const char *ParamNames[] = {"network", "name"};
  using SocketFunction::SocketFunction;
  Expect<NetResult<Runtime::Component::Own<ResolveAddressStream>>>
  body(Runtime::Component::CallingFrame &, NetworkBorrow Net, std::string Name);
};

/// `[method]resolve-address-stream.resolve-next-address: func() ->
/// result<option<ip-address>, error-code>`
class ResolveNextAddress : public SocketFunction<ResolveNextAddress> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  using SocketFunction::SocketFunction;
  Expect<NetResult<std::optional<IpAddress>>>
  body(Runtime::Component::CallingFrame &, ResolveBorrow Self);
};

/// `[method]resolve-address-stream.subscribe: func() -> pollable`
class ResolveSubscribe : public SocketFunction<ResolveSubscribe> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  using SocketFunction::SocketFunction;
  Expect<Runtime::Component::Own<PollSource>>
  body(Runtime::Component::CallingFrame &, ResolveBorrow Self);
};

/// `tcp-create-socket.create-tcp-socket: func(address-family:
/// ip-address-family) -> result<tcp-socket, error-code>`
class CreateTcpSocket : public SocketFunction<CreateTcpSocket> {
public:
  static constexpr const char *ParamNames[] = {"address-family"};
  using SocketFunction::SocketFunction;
  Expect<NetResult<Runtime::Component::Own<TcpSocket>>>
  body(Runtime::Component::CallingFrame &, IpAddressFamily Family);
};

/// `[method]tcp-socket.start-bind` and `start-connect`: the operation runs
/// to completion here, and its `finish-` reports the outcome.
class TcpStart : public SocketFunction<TcpStart> {
public:
  enum class Op { Bind, Connect };
  static constexpr const char *ParamNames[] = {"self", "network", "address"};
  TcpStart(SocketsHost &S, Op Which) noexcept
      : SocketFunction(S), Which(Which) {}
  Expect<NetStatus> body(Runtime::Component::CallingFrame &, TcpBorrow Self,
                         NetworkBorrow Net, IpSocketAddress Address);

private:
  Op Which;
};

/// `[method]tcp-socket.finish-bind` and `finish-listen`
class TcpFinish : public SocketFunction<TcpFinish> {
public:
  enum class Op { Bind, Listen };
  static constexpr const char *ParamNames[] = {"self"};
  TcpFinish(SocketsHost &S, Op Which) noexcept
      : SocketFunction(S), Which(Which) {}
  Expect<NetStatus> body(Runtime::Component::CallingFrame &, TcpBorrow Self);

private:
  Op Which;
};

/// `[method]tcp-socket.finish-connect: func() -> result<tuple<input-stream,
/// output-stream>, error-code>`
class TcpFinishConnect : public SocketFunction<TcpFinishConnect> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  using SocketFunction::SocketFunction;
  Expect<NetResult<std::tuple<Runtime::Component::Own<InputStream>,
                              Runtime::Component::Own<OutputStream>>>>
  body(Runtime::Component::CallingFrame &, TcpBorrow Self);
};

/// `[method]tcp-socket.start-listen: func() -> result<_, error-code>`
class TcpStartListen : public SocketFunction<TcpStartListen> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  using SocketFunction::SocketFunction;
  Expect<NetStatus> body(Runtime::Component::CallingFrame &, TcpBorrow Self);
};

/// `[method]tcp-socket.accept: func() -> result<tuple<tcp-socket,
/// input-stream, output-stream>, error-code>`
class TcpAccept : public SocketFunction<TcpAccept> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  using SocketFunction::SocketFunction;
  Expect<NetResult<std::tuple<Runtime::Component::Own<TcpSocket>,
                              Runtime::Component::Own<InputStream>,
                              Runtime::Component::Own<OutputStream>>>>
  body(Runtime::Component::CallingFrame &, TcpBorrow Self);
};

/// `[method]tcp-socket.local-address` and `remote-address`
class TcpAddress : public SocketFunction<TcpAddress> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  TcpAddress(SocketsHost &S, bool Remote) noexcept
      : SocketFunction(S), Remote(Remote) {}
  Expect<NetResult<IpSocketAddress>> body(Runtime::Component::CallingFrame &,
                                          TcpBorrow Self);

private:
  bool Remote;
};

/// `[method]tcp-socket.is-listening: func() -> bool`
class TcpIsListening : public SocketFunction<TcpIsListening> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  using SocketFunction::SocketFunction;
  Expect<bool> body(Runtime::Component::CallingFrame &, TcpBorrow Self);
};

/// `[method]tcp-socket.address-family: func() -> ip-address-family`
class TcpAddressFamily : public SocketFunction<TcpAddressFamily> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  using SocketFunction::SocketFunction;
  Expect<IpAddressFamily> body(Runtime::Component::CallingFrame &,
                               TcpBorrow Self);
};

/// `[method]tcp-socket.set-listen-backlog-size`
class TcpSetListenBacklogSize : public SocketFunction<TcpSetListenBacklogSize> {
public:
  static constexpr const char *ParamNames[] = {"self", "value"};
  using SocketFunction::SocketFunction;
  Expect<NetStatus> body(Runtime::Component::CallingFrame &, TcpBorrow Self,
                         uint64_t Value);
};

/// The option getters and setters of `tcp-socket`, one class per value type.
class TcpGetBoolOption : public SocketFunction<TcpGetBoolOption> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  TcpGetBoolOption(SocketsHost &S, SocketOption Opt) noexcept
      : SocketFunction(S), Option(Opt) {}
  Expect<NetResult<bool>> body(Runtime::Component::CallingFrame &,
                               TcpBorrow Self);

private:
  SocketOption Option;
};

class TcpSetBoolOption : public SocketFunction<TcpSetBoolOption> {
public:
  static constexpr const char *ParamNames[] = {"self", "value"};
  TcpSetBoolOption(SocketsHost &S, SocketOption Opt) noexcept
      : SocketFunction(S), Option(Opt) {}
  Expect<NetStatus> body(Runtime::Component::CallingFrame &, TcpBorrow Self,
                         bool Value);

private:
  SocketOption Option;
};

class TcpGetU64Option : public SocketFunction<TcpGetU64Option> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  TcpGetU64Option(SocketsHost &S, SocketOption Opt) noexcept
      : SocketFunction(S), Option(Opt) {}
  Expect<NetResult<uint64_t>> body(Runtime::Component::CallingFrame &,
                                   TcpBorrow Self);

private:
  SocketOption Option;
};

class TcpSetU64Option : public SocketFunction<TcpSetU64Option> {
public:
  static constexpr const char *ParamNames[] = {"self", "value"};
  TcpSetU64Option(SocketsHost &S, SocketOption Opt) noexcept
      : SocketFunction(S), Option(Opt) {}
  Expect<NetStatus> body(Runtime::Component::CallingFrame &, TcpBorrow Self,
                         uint64_t Value);

private:
  SocketOption Option;
};

class TcpGetU32Option : public SocketFunction<TcpGetU32Option> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  TcpGetU32Option(SocketsHost &S, SocketOption Opt) noexcept
      : SocketFunction(S), Option(Opt) {}
  Expect<NetResult<uint32_t>> body(Runtime::Component::CallingFrame &,
                                   TcpBorrow Self);

private:
  SocketOption Option;
};

class TcpSetU32Option : public SocketFunction<TcpSetU32Option> {
public:
  static constexpr const char *ParamNames[] = {"self", "value"};
  TcpSetU32Option(SocketsHost &S, SocketOption Opt) noexcept
      : SocketFunction(S), Option(Opt) {}
  Expect<NetStatus> body(Runtime::Component::CallingFrame &, TcpBorrow Self,
                         uint32_t Value);

private:
  SocketOption Option;
};

class TcpGetU8Option : public SocketFunction<TcpGetU8Option> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  TcpGetU8Option(SocketsHost &S, SocketOption Opt) noexcept
      : SocketFunction(S), Option(Opt) {}
  Expect<NetResult<uint8_t>> body(Runtime::Component::CallingFrame &,
                                  TcpBorrow Self);

private:
  SocketOption Option;
};

class TcpSetU8Option : public SocketFunction<TcpSetU8Option> {
public:
  static constexpr const char *ParamNames[] = {"self", "value"};
  TcpSetU8Option(SocketsHost &S, SocketOption Opt) noexcept
      : SocketFunction(S), Option(Opt) {}
  Expect<NetStatus> body(Runtime::Component::CallingFrame &, TcpBorrow Self,
                         uint8_t Value);

private:
  SocketOption Option;
};

/// `[method]tcp-socket.subscribe: func() -> pollable`
class TcpSubscribe : public SocketFunction<TcpSubscribe> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  using SocketFunction::SocketFunction;
  Expect<Runtime::Component::Own<PollSource>>
  body(Runtime::Component::CallingFrame &, TcpBorrow Self);
};

/// `[method]tcp-socket.shutdown: func(shutdown-type: shutdown-type) ->
/// result<_, error-code>`
class TcpShutdown : public SocketFunction<TcpShutdown> {
public:
  static constexpr const char *ParamNames[] = {"self", "shutdown-type"};
  using SocketFunction::SocketFunction;
  Expect<NetStatus> body(Runtime::Component::CallingFrame &, TcpBorrow Self,
                         ShutdownType Type);
};

/// `udp-create-socket.create-udp-socket`
class CreateUdpSocket : public SocketFunction<CreateUdpSocket> {
public:
  static constexpr const char *ParamNames[] = {"address-family"};
  using SocketFunction::SocketFunction;
  Expect<NetResult<Runtime::Component::Own<UdpSocket>>>
  body(Runtime::Component::CallingFrame &, IpAddressFamily Family);
};

/// `[method]udp-socket.start-bind`
class UdpStartBind : public SocketFunction<UdpStartBind> {
public:
  static constexpr const char *ParamNames[] = {"self", "network",
                                               "local-address"};
  using SocketFunction::SocketFunction;
  Expect<NetStatus> body(Runtime::Component::CallingFrame &, UdpBorrow Self,
                         NetworkBorrow Net, IpSocketAddress Local);
};

/// `[method]udp-socket.finish-bind`
class UdpFinishBind : public SocketFunction<UdpFinishBind> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  using SocketFunction::SocketFunction;
  Expect<NetStatus> body(Runtime::Component::CallingFrame &, UdpBorrow Self);
};

/// `[method]udp-socket.stream: func(remote-address: option<ip-socket-address>)
/// -> result<tuple<incoming-datagram-stream, outgoing-datagram-stream>,
/// error-code>`
class UdpStream : public SocketFunction<UdpStream> {
public:
  static constexpr const char *ParamNames[] = {"self", "remote-address"};
  using SocketFunction::SocketFunction;
  Expect<NetResult<std::tuple<Runtime::Component::Own<IncomingDatagramStream>,
                              Runtime::Component::Own<OutgoingDatagramStream>>>>
  body(Runtime::Component::CallingFrame &, UdpBorrow Self,
       std::optional<IpSocketAddress> Remote);
};

/// `[method]udp-socket.local-address` and `remote-address`
class UdpAddress : public SocketFunction<UdpAddress> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  UdpAddress(SocketsHost &S, bool Remote) noexcept
      : SocketFunction(S), Remote(Remote) {}
  Expect<NetResult<IpSocketAddress>> body(Runtime::Component::CallingFrame &,
                                          UdpBorrow Self);

private:
  bool Remote;
};

/// `[method]udp-socket.address-family`
class UdpAddressFamily : public SocketFunction<UdpAddressFamily> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  using SocketFunction::SocketFunction;
  Expect<IpAddressFamily> body(Runtime::Component::CallingFrame &,
                               UdpBorrow Self);
};

/// The option getters and setters of `udp-socket`.
class UdpGetU64Option : public SocketFunction<UdpGetU64Option> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  UdpGetU64Option(SocketsHost &S, SocketOption Opt) noexcept
      : SocketFunction(S), Option(Opt) {}
  Expect<NetResult<uint64_t>> body(Runtime::Component::CallingFrame &,
                                   UdpBorrow Self);

private:
  SocketOption Option;
};

class UdpSetU64Option : public SocketFunction<UdpSetU64Option> {
public:
  static constexpr const char *ParamNames[] = {"self", "value"};
  UdpSetU64Option(SocketsHost &S, SocketOption Opt) noexcept
      : SocketFunction(S), Option(Opt) {}
  Expect<NetStatus> body(Runtime::Component::CallingFrame &, UdpBorrow Self,
                         uint64_t Value);

private:
  SocketOption Option;
};

class UdpGetU8Option : public SocketFunction<UdpGetU8Option> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  UdpGetU8Option(SocketsHost &S, SocketOption Opt) noexcept
      : SocketFunction(S), Option(Opt) {}
  Expect<NetResult<uint8_t>> body(Runtime::Component::CallingFrame &,
                                  UdpBorrow Self);

private:
  SocketOption Option;
};

class UdpSetU8Option : public SocketFunction<UdpSetU8Option> {
public:
  static constexpr const char *ParamNames[] = {"self", "value"};
  UdpSetU8Option(SocketsHost &S, SocketOption Opt) noexcept
      : SocketFunction(S), Option(Opt) {}
  Expect<NetStatus> body(Runtime::Component::CallingFrame &, UdpBorrow Self,
                         uint8_t Value);

private:
  SocketOption Option;
};

/// `[method]udp-socket.subscribe: func() -> pollable`
class UdpSubscribe : public SocketFunction<UdpSubscribe> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  using SocketFunction::SocketFunction;
  Expect<Runtime::Component::Own<PollSource>>
  body(Runtime::Component::CallingFrame &, UdpBorrow Self);
};

/// `[method]incoming-datagram-stream.receive: func(max-results: u64) ->
/// result<list<incoming-datagram>, error-code>`
class IncomingReceive : public SocketFunction<IncomingReceive> {
public:
  static constexpr const char *ParamNames[] = {"self", "max-results"};
  using SocketFunction::SocketFunction;
  Expect<NetResult<std::vector<IncomingDatagram>>>
  body(Runtime::Component::CallingFrame &, IncomingBorrow Self,
       uint64_t MaxResults);
};

/// `[method]incoming-datagram-stream.subscribe: func() -> pollable`
class IncomingSubscribe : public SocketFunction<IncomingSubscribe> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  using SocketFunction::SocketFunction;
  Expect<Runtime::Component::Own<PollSource>>
  body(Runtime::Component::CallingFrame &, IncomingBorrow Self);
};

/// `[method]outgoing-datagram-stream.check-send: func() -> result<u64,
/// error-code>`
class OutgoingCheckSend : public SocketFunction<OutgoingCheckSend> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  using SocketFunction::SocketFunction;
  Expect<NetResult<uint64_t>> body(Runtime::Component::CallingFrame &,
                                   OutgoingBorrow Self);
};

/// `[method]outgoing-datagram-stream.send: func(datagrams:
/// list<outgoing-datagram>) -> result<u64, error-code>`
class OutgoingSend : public SocketFunction<OutgoingSend> {
public:
  static constexpr const char *ParamNames[] = {"self", "datagrams"};
  using SocketFunction::SocketFunction;
  Expect<NetResult<uint64_t>> body(Runtime::Component::CallingFrame &,
                                   OutgoingBorrow Self,
                                   std::vector<OutgoingDatagram> Datagrams);
};

/// `[method]outgoing-datagram-stream.subscribe: func() -> pollable`
class OutgoingSubscribe : public SocketFunction<OutgoingSubscribe> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  using SocketFunction::SocketFunction;
  Expect<Runtime::Component::Own<PollSource>>
  body(Runtime::Component::CallingFrame &, OutgoingBorrow Self);
};

/// `wasi:sockets/network@0.2.12`: the `network` resource and the address
/// and error types.
class NetworkInstance : public Runtime::Instance::ComponentInstance {
public:
  NetworkInstance(SocketsHost &Sockets);
};

/// `wasi:sockets/instance-network@0.2.12`
class InstanceNetworkInstance : public Runtime::Instance::ComponentInstance {
public:
  InstanceNetworkInstance(SocketsHost &Sockets);
};

/// `wasi:sockets/ip-name-lookup@0.2.12`
class IpNameLookupInstance : public Runtime::Instance::ComponentInstance {
public:
  IpNameLookupInstance(SocketsHost &Sockets);
};

/// `wasi:sockets/tcp@0.2.12`
class TcpInstance : public Runtime::Instance::ComponentInstance {
public:
  TcpInstance(SocketsHost &Sockets);
};

/// `wasi:sockets/tcp-create-socket@0.2.12`
class TcpCreateSocketInstance : public Runtime::Instance::ComponentInstance {
public:
  TcpCreateSocketInstance(SocketsHost &Sockets);
};

/// `wasi:sockets/udp@0.2.12`
class UdpInstance : public Runtime::Instance::ComponentInstance {
public:
  UdpInstance(SocketsHost &Sockets);
};

/// `wasi:sockets/udp-create-socket@0.2.12`
class UdpCreateSocketInstance : public Runtime::Instance::ComponentInstance {
public:
  UdpCreateSocketInstance(SocketsHost &Sockets);
};

/// The seven instances of `wasi:sockets@0.2.12`, which define the resource
/// types of Sockets in order.
std::vector<std::unique_ptr<Runtime::Instance::ComponentInstance>>
makeSocketsInstances(SocketsHost &Sockets);

} // namespace WasiP2
} // namespace Host
} // namespace WasmEdge
