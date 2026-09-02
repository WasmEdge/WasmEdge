// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/host/wasi/p3/sockets.h - wasi:sockets host ---------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the host component instances of `wasi:sockets@0.3.1`:
/// `types` with the `tcp-socket` and `udp-socket` resources over the
/// preview-1 virtual nodes, and `ip-name-lookup`.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/component_valtype.h"
#include "common/component_variant.h"
#include "common/errcode.h"
#include "common/expected.h"
#include "host/wasi/vinode.h"
#include "runtime/component/callingframe.h"
#include "runtime/component/hostfunc.h"
#include "runtime/component/resourcetable.h"
#include "runtime/component/wit.h"
#include "runtime/instance/component/component.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace WasiP3 {

/// `wasi:sockets/types@0.3.1` `error-code`.
struct SocketError {
  enum Kind : uint32_t {
    AccessDenied,
    NotSupported,
    InvalidArgument,
    OutOfMemory,
    Timeout,
    InvalidState,
    AddressNotBindable,
    AddressInUse,
    RemoteUnreachable,
    ConnectionRefused,
    ConnectionBroken,
    ConnectionReset,
    ConnectionAborted,
    DatagramTooLarge,
    Other,
  };
  SocketError() = default;
  SocketError(Kind K) noexcept : Case(K) {}
  Kind Case = Other;
  std::optional<std::string> Detail;
};

/// `wasi:sockets/ip-name-lookup@0.3.1` `error-code`.
struct LookupError {
  enum Kind : uint32_t {
    AccessDenied,
    InvalidArgument,
    NameUnresolvable,
    TemporaryResolverFailure,
    PermanentResolverFailure,
    Other,
  };
  LookupError() = default;
  LookupError(Kind K) noexcept : Case(K) {}
  Kind Case = Other;
  std::optional<std::string> Detail;
};

enum class IpAddressFamily : uint32_t { Ipv4, Ipv6 };

using Ipv4Address = std::tuple<uint8_t, uint8_t, uint8_t, uint8_t>;
using Ipv6Address = std::tuple<uint16_t, uint16_t, uint16_t, uint16_t, uint16_t,
                               uint16_t, uint16_t, uint16_t>;

/// `ip-address`: one of the two families.
struct IpAddress {
  IpAddressFamily Family = IpAddressFamily::Ipv4;
  Ipv4Address V4;
  Ipv6Address V6;
};

struct Ipv4SocketAddress {
  uint16_t Port = 0;
  Ipv4Address Address;
};

struct Ipv6SocketAddress {
  uint16_t Port = 0;
  uint32_t FlowInfo = 0;
  Ipv6Address Address;
  uint32_t ScopeId = 0;
};

struct RawAddress;

/// `ip-socket-address`: one of the two families.
struct IpSocketAddress {
  IpAddressFamily Family = IpAddressFamily::Ipv4;
  Ipv4SocketAddress V4;
  Ipv6SocketAddress V6;

  /// The preview-1 form of the address.
  RawAddress raw() const noexcept;
};

/// The state behind a `tcp-socket` handle.
struct TcpSocket {
  std::shared_ptr<WASI::VINode> Node;
  IpAddressFamily Family = IpAddressFamily::Ipv4;
  bool Listening = false;
  uint64_t Backlog = 128;
};

/// The state behind a `udp-socket` handle; the remote address of a
/// connected socket filters what it sends and receives.
struct UdpSocket {
  std::shared_ptr<WASI::VINode> Node;
  IpAddressFamily Family = IpAddressFamily::Ipv4;
  std::optional<IpSocketAddress> Remote;
};

using TcpTable = Runtime::Component::ResourceTable<TcpSocket>;
using UdpTable = Runtime::Component::ResourceTable<UdpSocket>;
using TcpBorrow = Runtime::Component::Borrow<TcpSocket>;
using UdpBorrow = Runtime::Component::Borrow<UdpSocket>;

template <typename T> using SockResult = Expected<T, SocketError>;
using SockStatus = Expected<Runtime::Component::Unit, SocketError>;

} // namespace WasiP3
} // namespace Host

namespace Runtime {
namespace Component {

template <> struct Wit<Host::WasiP3::SocketError> {
  static ComponentValType type(const TypeMinter &Mint) noexcept {
    WitVariant::Cases Cases;
    for (const char *Label :
         {"access-denied", "not-supported", "invalid-argument", "out-of-memory",
          "timeout", "invalid-state", "address-not-bindable", "address-in-use",
          "remote-unreachable", "connection-refused", "connection-broken",
          "connection-reset", "connection-aborted", "datagram-too-large"}) {
      Cases.emplace_back(Label, std::nullopt);
    }
    Cases.emplace_back("other", Wit<std::optional<std::string>>::type(Mint));
    return WitVariant::type(Mint, std::move(Cases));
  }
  static Host::WasiP3::SocketError from(const ComponentValVariant &V) {
    const auto &Val = WitVariant::value(V);
    Host::WasiP3::SocketError E;
    E.Case = static_cast<Host::WasiP3::SocketError::Kind>(Val.Case);
    if (E.Case == Host::WasiP3::SocketError::Other && Val.Payload) {
      E.Detail = Wit<std::optional<std::string>>::from(*Val.Payload);
    }
    return E;
  }
  static ComponentValVariant into(Host::WasiP3::SocketError &&E) noexcept {
    if (E.Case == Host::WasiP3::SocketError::Other) {
      return WitVariant::into(
          E.Case, Wit<std::optional<std::string>>::into(std::move(E.Detail)));
    }
    return WitVariant::into(E.Case);
  }
};

template <> struct Wit<Host::WasiP3::LookupError> {
  static ComponentValType type(const TypeMinter &Mint) noexcept {
    return WitVariant::type(
        Mint, {{"access-denied", std::nullopt},
               {"invalid-argument", std::nullopt},
               {"name-unresolvable", std::nullopt},
               {"temporary-resolver-failure", std::nullopt},
               {"permanent-resolver-failure", std::nullopt},
               {"other", Wit<std::optional<std::string>>::type(Mint)}});
  }
  static Host::WasiP3::LookupError from(const ComponentValVariant &V) {
    const auto &Val = WitVariant::value(V);
    Host::WasiP3::LookupError E;
    E.Case = static_cast<Host::WasiP3::LookupError::Kind>(Val.Case);
    if (E.Case == Host::WasiP3::LookupError::Other && Val.Payload) {
      E.Detail = Wit<std::optional<std::string>>::from(*Val.Payload);
    }
    return E;
  }
  static ComponentValVariant into(Host::WasiP3::LookupError &&E) noexcept {
    if (E.Case == Host::WasiP3::LookupError::Other) {
      return WitVariant::into(
          E.Case, Wit<std::optional<std::string>>::into(std::move(E.Detail)));
    }
    return WitVariant::into(E.Case);
  }
};

template <>
struct Wit<Host::WasiP3::IpAddressFamily>
    : WitEnum<Host::WasiP3::IpAddressFamily,
              Wit<Host::WasiP3::IpAddressFamily>> {
  static constexpr const char *Labels[] = {"ipv4", "ipv6"};
};

template <> struct Wit<Host::WasiP3::IpAddress> {
  static ComponentValType type(const TypeMinter &Mint) noexcept {
    return WitVariant::type(
        Mint, {{"ipv4", Wit<Host::WasiP3::Ipv4Address>::type(Mint)},
               {"ipv6", Wit<Host::WasiP3::Ipv6Address>::type(Mint)}});
  }
  static Host::WasiP3::IpAddress from(const ComponentValVariant &V) {
    const auto &Val = WitVariant::value(V);
    Host::WasiP3::IpAddress A;
    A.Family = static_cast<Host::WasiP3::IpAddressFamily>(Val.Case);
    if (A.Family == Host::WasiP3::IpAddressFamily::Ipv4) {
      A.V4 = Wit<Host::WasiP3::Ipv4Address>::from(*Val.Payload);
    } else {
      A.V6 = Wit<Host::WasiP3::Ipv6Address>::from(*Val.Payload);
    }
    return A;
  }
  static ComponentValVariant into(Host::WasiP3::IpAddress &&A) noexcept {
    if (A.Family == Host::WasiP3::IpAddressFamily::Ipv4) {
      return WitVariant::into(
          0, Wit<Host::WasiP3::Ipv4Address>::into(std::move(A.V4)));
    }
    return WitVariant::into(
        1, Wit<Host::WasiP3::Ipv6Address>::into(std::move(A.V6)));
  }
};

template <> struct Wit<Host::WasiP3::Ipv4SocketAddress> {
  static ComponentValType type(const TypeMinter &Mint) noexcept {
    return WitRecord::type(
        Mint, {{"port", Wit<uint16_t>::type(Mint)},
               {"address", Wit<Host::WasiP3::Ipv4Address>::type(Mint)}});
  }
  static Host::WasiP3::Ipv4SocketAddress from(const ComponentValVariant &V) {
    auto F = WitRecord::fields(V);
    return {Wit<uint16_t>::from(F[0].second),
            Wit<Host::WasiP3::Ipv4Address>::from(F[1].second)};
  }
  static ComponentValVariant
  into(Host::WasiP3::Ipv4SocketAddress &&A) noexcept {
    return WitRecord::into({{"port", Wit<uint16_t>::into(A.Port)},
                            {"address", Wit<Host::WasiP3::Ipv4Address>::into(
                                            std::move(A.Address))}});
  }
};

template <> struct Wit<Host::WasiP3::Ipv6SocketAddress> {
  static ComponentValType type(const TypeMinter &Mint) noexcept {
    return WitRecord::type(
        Mint, {{"port", Wit<uint16_t>::type(Mint)},
               {"flow-info", Wit<uint32_t>::type(Mint)},
               {"address", Wit<Host::WasiP3::Ipv6Address>::type(Mint)},
               {"scope-id", Wit<uint32_t>::type(Mint)}});
  }
  static Host::WasiP3::Ipv6SocketAddress from(const ComponentValVariant &V) {
    auto F = WitRecord::fields(V);
    return {Wit<uint16_t>::from(F[0].second), Wit<uint32_t>::from(F[1].second),
            Wit<Host::WasiP3::Ipv6Address>::from(F[2].second),
            Wit<uint32_t>::from(F[3].second)};
  }
  static ComponentValVariant
  into(Host::WasiP3::Ipv6SocketAddress &&A) noexcept {
    return WitRecord::into({{"port", Wit<uint16_t>::into(A.Port)},
                            {"flow-info", Wit<uint32_t>::into(A.FlowInfo)},
                            {"address", Wit<Host::WasiP3::Ipv6Address>::into(
                                            std::move(A.Address))},
                            {"scope-id", Wit<uint32_t>::into(A.ScopeId)}});
  }
};

template <> struct Wit<Host::WasiP3::IpSocketAddress> {
  static ComponentValType type(const TypeMinter &Mint) noexcept {
    return WitVariant::type(
        Mint, {{"ipv4", Wit<Host::WasiP3::Ipv4SocketAddress>::type(Mint)},
               {"ipv6", Wit<Host::WasiP3::Ipv6SocketAddress>::type(Mint)}});
  }
  static Host::WasiP3::IpSocketAddress from(const ComponentValVariant &V) {
    const auto &Val = WitVariant::value(V);
    Host::WasiP3::IpSocketAddress A;
    A.Family = static_cast<Host::WasiP3::IpAddressFamily>(Val.Case);
    if (A.Family == Host::WasiP3::IpAddressFamily::Ipv4) {
      A.V4 = Wit<Host::WasiP3::Ipv4SocketAddress>::from(*Val.Payload);
    } else {
      A.V6 = Wit<Host::WasiP3::Ipv6SocketAddress>::from(*Val.Payload);
    }
    return A;
  }
  static ComponentValVariant into(Host::WasiP3::IpSocketAddress &&A) noexcept {
    if (A.Family == Host::WasiP3::IpAddressFamily::Ipv4) {
      return WitVariant::into(
          0, Wit<Host::WasiP3::Ipv4SocketAddress>::into(std::move(A.V4)));
    }
    return WitVariant::into(
        1, Wit<Host::WasiP3::Ipv6SocketAddress>::into(std::move(A.V6)));
  }
};

} // namespace Component
} // namespace Runtime

namespace Host {
namespace WasiP3 {

/// The socket options the get and set methods pair up on.
enum class SocketOption {
  KeepAliveEnabled,
  KeepAliveIdleTime,
  KeepAliveInterval,
  KeepAliveCount,
  HopLimit,
  ReceiveBufferSize,
  SendBufferSize,
};

/// A socket address as the preview-1 layer takes it: the family, the raw
/// address bytes in network order, and the port in host order.
struct RawAddress {
  __wasi_address_family_t Family = __WASI_ADDRESS_FAMILY_INET4;
  std::array<uint8_t, 16> Bytes{};
  size_t Length = 4;
  uint16_t Port = 0;
  Span<const uint8_t> bytes() const noexcept {
    return Span<const uint8_t>(Bytes.data(), Length);
  }
  friend bool operator==(const RawAddress &A, const RawAddress &B) noexcept {
    return A.Family == B.Family && A.Port == B.Port && A.Length == B.Length &&
           std::equal(A.Bytes.begin(), A.Bytes.begin() + A.Length,
                      B.Bytes.begin());
  }
  friend bool operator!=(const RawAddress &A, const RawAddress &B) noexcept {
    return !(A == B);
  }
};

/// The conversions between the WIT addresses and the preview-1 forms, shared
/// with the 0.2 sockets.
__wasi_address_family_t wasiFamilyOf(IpAddressFamily Family) noexcept;
IpAddress addressOf(__wasi_address_family_t Family,
                    Span<const uint8_t> Bytes) noexcept;
/// The preview-1 receive hands the sender's port back as the wire bytes.
uint16_t portOf(uint16_t Wire) noexcept;
IpSocketAddress socketAddressOf(__wasi_address_family_t Family,
                                Span<const uint8_t> Bytes,
                                uint16_t Port) noexcept;
/// The local or peer address of a socket node.
WASI::WasiExpect<IpSocketAddress> nodeAddress(WASI::VINode &Node,
                                              bool Remote) noexcept;
/// Socket options through the preview-1 layer where it has them, and the
/// native socket otherwise.
WASI::WasiExpect<uint64_t> getOption(WASI::VINode &Node, IpAddressFamily Family,
                                     SocketOption Option) noexcept;
WASI::WasiExpect<void> setOption(WASI::VINode &Node, IpAddressFamily Family,
                                 SocketOption Option, uint64_t Value) noexcept;
/// The addresses a host name resolves to, through the preview-1 resolver.
WASI::WasiExpect<std::vector<IpAddress>>
resolveHostname(const std::string &Name) noexcept;

/// The base of the `tcp-socket` methods: the table their `self` resolves in.
template <typename T>
class TcpMethod : public Runtime::Component::HostFunction<T> {
public:
  TcpMethod(TcpTable &Tab, bool Async = false) noexcept
      : Runtime::Component::HostFunction<T>(Async), Table(Tab) {}

protected:
  TcpTable &Table;
};

/// `[static]tcp-socket.create: func(address-family: ip-address-family) ->
/// result<tcp-socket, error-code>`
class TcpCreate : public TcpMethod<TcpCreate> {
public:
  static constexpr const char *ParamNames[] = {"address-family"};
  using TcpMethod::TcpMethod;
  Expect<SockResult<Runtime::Component::Own<TcpSocket>>>
  body(Runtime::Component::CallingFrame &, IpAddressFamily Family);
};

/// `[method]tcp-socket.bind: func(local-address: ip-socket-address) ->
/// result<_, error-code>`
class TcpBind : public TcpMethod<TcpBind> {
public:
  static constexpr const char *ParamNames[] = {"self", "local-address"};
  using TcpMethod::TcpMethod;
  Expect<SockStatus> body(Runtime::Component::CallingFrame &, TcpBorrow Self,
                          IpSocketAddress Local);
};

/// `[method]tcp-socket.connect: async func(remote-address:
/// ip-socket-address) -> result<_, error-code>`
class TcpConnect : public TcpMethod<TcpConnect> {
public:
  static constexpr const char *ParamNames[] = {"self", "remote-address"};
  TcpConnect(TcpTable &Tab) noexcept : TcpMethod(Tab, true) {}
  Expect<SockStatus> body(Runtime::Component::CallingFrame &Frame,
                          TcpBorrow Self, IpSocketAddress Remote);
};

/// `[method]tcp-socket.listen: func() -> result<stream<tcp-socket>,
/// error-code>`: a host task accepts connections into the stream.
class TcpListen : public TcpMethod<TcpListen> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  using TcpMethod::TcpMethod;
  void declare(const Runtime::Component::TypeMinter &Mint) noexcept override;
  Expect<SockResult<
      Runtime::Component::Stream<Runtime::Component::Own<TcpSocket>>>>
  body(Runtime::Component::CallingFrame &Frame, TcpBorrow Self);

private:
  ComponentValType SocketType;
};

/// `[method]tcp-socket.send: func(data: stream<u8>) -> future<result<_,
/// error-code>>`: the stream sinks into the connection.
class TcpSend : public TcpMethod<TcpSend> {
public:
  static constexpr const char *ParamNames[] = {"self", "data"};
  using TcpMethod::TcpMethod;
  void declare(const Runtime::Component::TypeMinter &Mint) noexcept override;
  Expect<Runtime::Component::Future<SockStatus>>
  body(Runtime::Component::CallingFrame &Frame, TcpBorrow Self,
       Runtime::Component::Stream<uint8_t> Data);

private:
  ComponentValType ResultType;
};

/// `[method]tcp-socket.receive: func() -> tuple<stream<u8>, future<result<_,
/// error-code>>>`: a host task feeds the stream from the connection.
class TcpReceive : public TcpMethod<TcpReceive> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  using TcpMethod::TcpMethod;
  void declare(const Runtime::Component::TypeMinter &Mint) noexcept override;
  Expect<std::tuple<Runtime::Component::Stream<uint8_t>,
                    Runtime::Component::Future<SockStatus>>>
  body(Runtime::Component::CallingFrame &Frame, TcpBorrow Self);

private:
  ComponentValType ResultType;
};

/// `[method]tcp-socket.get-local-address` and `get-remote-address`
class TcpGetAddress : public TcpMethod<TcpGetAddress> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  TcpGetAddress(TcpTable &Tab, bool Remote) noexcept
      : TcpMethod(Tab), Remote(Remote) {}
  Expect<SockResult<IpSocketAddress>> body(Runtime::Component::CallingFrame &,
                                           TcpBorrow Self);

private:
  bool Remote;
};

/// `[method]tcp-socket.get-is-listening: func() -> bool`
class TcpGetIsListening : public TcpMethod<TcpGetIsListening> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  using TcpMethod::TcpMethod;
  Expect<bool> body(Runtime::Component::CallingFrame &, TcpBorrow Self);
};

/// `[method]tcp-socket.get-address-family: func() -> ip-address-family`
class TcpGetAddressFamily : public TcpMethod<TcpGetAddressFamily> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  using TcpMethod::TcpMethod;
  Expect<IpAddressFamily> body(Runtime::Component::CallingFrame &,
                               TcpBorrow Self);
};

/// `[method]tcp-socket.set-listen-backlog-size: func(value: u64) ->
/// result<_, error-code>`
class TcpSetListenBacklogSize : public TcpMethod<TcpSetListenBacklogSize> {
public:
  static constexpr const char *ParamNames[] = {"self", "value"};
  using TcpMethod::TcpMethod;
  Expect<SockStatus> body(Runtime::Component::CallingFrame &, TcpBorrow Self,
                          uint64_t Value);
};

/// The boolean, u64, u32, and u8 option getters and setters of `tcp-socket`,
/// one class per value type.
class TcpGetBoolOption : public TcpMethod<TcpGetBoolOption> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  TcpGetBoolOption(TcpTable &Tab, SocketOption Opt) noexcept
      : TcpMethod(Tab), Option(Opt) {}
  Expect<SockResult<bool>> body(Runtime::Component::CallingFrame &,
                                TcpBorrow Self);

private:
  SocketOption Option;
};

class TcpSetBoolOption : public TcpMethod<TcpSetBoolOption> {
public:
  static constexpr const char *ParamNames[] = {"self", "value"};
  TcpSetBoolOption(TcpTable &Tab, SocketOption Opt) noexcept
      : TcpMethod(Tab), Option(Opt) {}
  Expect<SockStatus> body(Runtime::Component::CallingFrame &, TcpBorrow Self,
                          bool Value);

private:
  SocketOption Option;
};

class TcpGetU64Option : public TcpMethod<TcpGetU64Option> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  TcpGetU64Option(TcpTable &Tab, SocketOption Opt) noexcept
      : TcpMethod(Tab), Option(Opt) {}
  Expect<SockResult<uint64_t>> body(Runtime::Component::CallingFrame &,
                                    TcpBorrow Self);

private:
  SocketOption Option;
};

class TcpSetU64Option : public TcpMethod<TcpSetU64Option> {
public:
  static constexpr const char *ParamNames[] = {"self", "value"};
  TcpSetU64Option(TcpTable &Tab, SocketOption Opt) noexcept
      : TcpMethod(Tab), Option(Opt) {}
  Expect<SockStatus> body(Runtime::Component::CallingFrame &, TcpBorrow Self,
                          uint64_t Value);

private:
  SocketOption Option;
};

class TcpGetU32Option : public TcpMethod<TcpGetU32Option> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  TcpGetU32Option(TcpTable &Tab, SocketOption Opt) noexcept
      : TcpMethod(Tab), Option(Opt) {}
  Expect<SockResult<uint32_t>> body(Runtime::Component::CallingFrame &,
                                    TcpBorrow Self);

private:
  SocketOption Option;
};

class TcpSetU32Option : public TcpMethod<TcpSetU32Option> {
public:
  static constexpr const char *ParamNames[] = {"self", "value"};
  TcpSetU32Option(TcpTable &Tab, SocketOption Opt) noexcept
      : TcpMethod(Tab), Option(Opt) {}
  Expect<SockStatus> body(Runtime::Component::CallingFrame &, TcpBorrow Self,
                          uint32_t Value);

private:
  SocketOption Option;
};

class TcpGetU8Option : public TcpMethod<TcpGetU8Option> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  TcpGetU8Option(TcpTable &Tab, SocketOption Opt) noexcept
      : TcpMethod(Tab), Option(Opt) {}
  Expect<SockResult<uint8_t>> body(Runtime::Component::CallingFrame &,
                                   TcpBorrow Self);

private:
  SocketOption Option;
};

class TcpSetU8Option : public TcpMethod<TcpSetU8Option> {
public:
  static constexpr const char *ParamNames[] = {"self", "value"};
  TcpSetU8Option(TcpTable &Tab, SocketOption Opt) noexcept
      : TcpMethod(Tab), Option(Opt) {}
  Expect<SockStatus> body(Runtime::Component::CallingFrame &, TcpBorrow Self,
                          uint8_t Value);

private:
  SocketOption Option;
};

/// The base of the `udp-socket` methods: the table their `self` resolves in.
template <typename T>
class UdpMethod : public Runtime::Component::HostFunction<T> {
public:
  UdpMethod(UdpTable &Tab, bool Async = false) noexcept
      : Runtime::Component::HostFunction<T>(Async), Table(Tab) {}

protected:
  UdpTable &Table;
};

/// `[static]udp-socket.create: func(address-family: ip-address-family) ->
/// result<udp-socket, error-code>`
class UdpCreate : public UdpMethod<UdpCreate> {
public:
  static constexpr const char *ParamNames[] = {"address-family"};
  using UdpMethod::UdpMethod;
  Expect<SockResult<Runtime::Component::Own<UdpSocket>>>
  body(Runtime::Component::CallingFrame &, IpAddressFamily Family);
};

/// `[method]udp-socket.bind: func(local-address: ip-socket-address) ->
/// result<_, error-code>`
class UdpBind : public UdpMethod<UdpBind> {
public:
  static constexpr const char *ParamNames[] = {"self", "local-address"};
  using UdpMethod::UdpMethod;
  Expect<SockStatus> body(Runtime::Component::CallingFrame &, UdpBorrow Self,
                          IpSocketAddress Local);
};

/// `[method]udp-socket.connect: func(remote-address: ip-socket-address) ->
/// result<_, error-code>`
class UdpConnect : public UdpMethod<UdpConnect> {
public:
  static constexpr const char *ParamNames[] = {"self", "remote-address"};
  using UdpMethod::UdpMethod;
  Expect<SockStatus> body(Runtime::Component::CallingFrame &, UdpBorrow Self,
                          IpSocketAddress Remote);
};

/// `[method]udp-socket.disconnect: func() -> result<_, error-code>`
class UdpDisconnect : public UdpMethod<UdpDisconnect> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  using UdpMethod::UdpMethod;
  Expect<SockStatus> body(Runtime::Component::CallingFrame &, UdpBorrow Self);
};

/// `[method]udp-socket.send: async func(data: list<u8>, remote-address:
/// option<ip-socket-address>) -> result<_, error-code>`
class UdpSend : public UdpMethod<UdpSend> {
public:
  static constexpr const char *ParamNames[] = {"self", "data",
                                               "remote-address"};
  UdpSend(UdpTable &Tab) noexcept : UdpMethod(Tab, true) {}
  Expect<SockStatus> body(Runtime::Component::CallingFrame &, UdpBorrow Self,
                          std::vector<uint8_t> Data,
                          std::optional<IpSocketAddress> Remote);
};

/// `[method]udp-socket.receive: async func() -> result<tuple<list<u8>,
/// ip-socket-address>, error-code>`
class UdpReceive : public UdpMethod<UdpReceive> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  UdpReceive(UdpTable &Tab) noexcept : UdpMethod(Tab, true) {}
  Expect<SockResult<std::tuple<std::vector<uint8_t>, IpSocketAddress>>>
  body(Runtime::Component::CallingFrame &Frame, UdpBorrow Self);
};

/// `[method]udp-socket.get-local-address` and `get-remote-address`
class UdpGetAddress : public UdpMethod<UdpGetAddress> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  UdpGetAddress(UdpTable &Tab, bool Remote) noexcept
      : UdpMethod(Tab), Remote(Remote) {}
  Expect<SockResult<IpSocketAddress>> body(Runtime::Component::CallingFrame &,
                                           UdpBorrow Self);

private:
  bool Remote;
};

/// `[method]udp-socket.get-address-family: func() -> ip-address-family`
class UdpGetAddressFamily : public UdpMethod<UdpGetAddressFamily> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  using UdpMethod::UdpMethod;
  Expect<IpAddressFamily> body(Runtime::Component::CallingFrame &,
                               UdpBorrow Self);
};

/// The u64 and u8 option getters and setters of `udp-socket`.
class UdpGetU64Option : public UdpMethod<UdpGetU64Option> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  UdpGetU64Option(UdpTable &Tab, SocketOption Opt) noexcept
      : UdpMethod(Tab), Option(Opt) {}
  Expect<SockResult<uint64_t>> body(Runtime::Component::CallingFrame &,
                                    UdpBorrow Self);

private:
  SocketOption Option;
};

class UdpSetU64Option : public UdpMethod<UdpSetU64Option> {
public:
  static constexpr const char *ParamNames[] = {"self", "value"};
  UdpSetU64Option(UdpTable &Tab, SocketOption Opt) noexcept
      : UdpMethod(Tab), Option(Opt) {}
  Expect<SockStatus> body(Runtime::Component::CallingFrame &, UdpBorrow Self,
                          uint64_t Value);

private:
  SocketOption Option;
};

class UdpGetU8Option : public UdpMethod<UdpGetU8Option> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  UdpGetU8Option(UdpTable &Tab, SocketOption Opt) noexcept
      : UdpMethod(Tab), Option(Opt) {}
  Expect<SockResult<uint8_t>> body(Runtime::Component::CallingFrame &,
                                   UdpBorrow Self);

private:
  SocketOption Option;
};

class UdpSetU8Option : public UdpMethod<UdpSetU8Option> {
public:
  static constexpr const char *ParamNames[] = {"self", "value"};
  UdpSetU8Option(UdpTable &Tab, SocketOption Opt) noexcept
      : UdpMethod(Tab), Option(Opt) {}
  Expect<SockStatus> body(Runtime::Component::CallingFrame &, UdpBorrow Self,
                          uint8_t Value);

private:
  SocketOption Option;
};

/// `ip-name-lookup.resolve-addresses: async func(name: string) ->
/// result<list<ip-address>, error-code>`
class ResolveAddresses
    : public Runtime::Component::HostFunction<ResolveAddresses> {
public:
  static constexpr const char *ParamNames[] = {"name"};
  ResolveAddresses() noexcept : HostFunction(/*Async=*/true) {}
  Expect<Expected<std::vector<IpAddress>, LookupError>>
  body(Runtime::Component::CallingFrame &Frame, std::string Name);
};

/// `wasi:sockets/types@0.3.1`: the socket resources, their methods, and the
/// named types. It owns the socket tables.
class SocketsTypesInstance : public Runtime::Instance::ComponentInstance {
public:
  SocketsTypesInstance();
  TcpTable &tcpSockets() noexcept { return TcpSockets; }
  UdpTable &udpSockets() noexcept { return UdpSockets; }

private:
  TcpTable TcpSockets;
  UdpTable UdpSockets;
};

/// `wasi:sockets/ip-name-lookup@0.3.1`
class IpNameLookupInstance : public Runtime::Instance::ComponentInstance {
public:
  IpNameLookupInstance();
};

/// The two instances of `wasi:sockets@0.3.1`, `types` first.
std::vector<std::unique_ptr<Runtime::Instance::ComponentInstance>>
makeSocketsInstances();

} // namespace WasiP3
} // namespace Host
} // namespace WasmEdge
