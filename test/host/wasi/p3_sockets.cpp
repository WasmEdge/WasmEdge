// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- test/host/wasi/p3_sockets.cpp - wasi:sockets@0.3.1 host -----------===//

#include "common/component_variant.h"
#include "host/wasi/p3/sockets.h"
#include "hostcall.h"
#include "runtime/component/wit.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

namespace {

using namespace WasmEdge;
using namespace Host::WasiP3;
using HostTest::call;

IpSocketAddress loopback(uint16_t Port) {
  IpSocketAddress A;
  A.Family = IpAddressFamily::Ipv4;
  A.V4 = Ipv4SocketAddress{Port, std::make_tuple(127, 0, 0, 1)};
  return A;
}

class WasiP3Sockets : public testing::Test {
protected:
  void SetUp() override {
    Insts = makeSocketsInstances();
    Types = static_cast<SocketsTypesInstance *>(Insts[0].get());
  }
  ComponentValVariant tcp(uint64_t Rep) {
    return Runtime::Component::Wit<TcpBorrow>::into(TcpBorrow{Rep});
  }
  ComponentValVariant udp(uint64_t Rep) {
    return Runtime::Component::Wit<UdpBorrow>::into(UdpBorrow{Rep});
  }
  uint64_t createTcp() {
    auto Res = call(*Types, "[static]tcp-socket.create",
                    {Runtime::Component::Wit<IpAddressFamily>::into(
                        IpAddressFamily::Ipv4)});
    EXPECT_TRUE(Res);
    auto Sock = Runtime::Component::Wit<
        SockResult<Runtime::Component::Own<TcpSocket>>>::from((*Res)[0]);
    EXPECT_TRUE(Sock);
    return Sock ? Sock->Rep : 0;
  }
  uint64_t createUdp() {
    auto Res = call(*Types, "[static]udp-socket.create",
                    {Runtime::Component::Wit<IpAddressFamily>::into(
                        IpAddressFamily::Ipv4)});
    EXPECT_TRUE(Res);
    auto Sock = Runtime::Component::Wit<
        SockResult<Runtime::Component::Own<UdpSocket>>>::from((*Res)[0]);
    EXPECT_TRUE(Sock);
    return Sock ? Sock->Rep : 0;
  }
  SockStatus status(std::string_view Name,
                    std::vector<ComponentValVariant> Args) {
    auto Res = call(*Types, Name, std::move(Args));
    if (!Res) {
      return Unexpected<SocketError>(SocketError());
    }
    return Runtime::Component::Wit<SockStatus>::from((*Res)[0]);
  }
  SockResult<IpSocketAddress> localAddress(std::string_view Name,
                                           ComponentValVariant Self) {
    auto Res = call(*Types, Name, {std::move(Self)});
    if (!Res) {
      return Unexpected<SocketError>(SocketError());
    }
    return Runtime::Component::Wit<SockResult<IpSocketAddress>>::from(
        (*Res)[0]);
  }

  std::vector<std::unique_ptr<Runtime::Instance::ComponentInstance>> Insts;
  SocketsTypesInstance *Types = nullptr;
};

TEST_F(WasiP3Sockets, InstancesAndTypes) {
  EXPECT_EQ(Insts[0]->getComponentName(), "wasi:sockets/types@0.3.1");
  EXPECT_EQ(Insts[1]->getComponentName(), "wasi:sockets/ip-name-lookup@0.3.1");
  EXPECT_NE(Types->findTypeResource("tcp-socket"), nullptr);
  EXPECT_NE(Types->findTypeResource("udp-socket"), nullptr);
  const auto *Err = Types->findType("error-code");
  ASSERT_NE(Err, nullptr);
  ASSERT_TRUE(Err->getDefValType().isVariantTy());
  EXPECT_EQ(Err->getDefValType().getVariant().Cases.size(), 15u);
  const auto *Addr = Types->findType("ip-socket-address");
  ASSERT_NE(Addr, nullptr);
  ASSERT_TRUE(Addr->getDefValType().isVariantTy());
  auto *Connect = Types->findFunction("[method]tcp-socket.connect");
  ASSERT_NE(Connect, nullptr);
  EXPECT_TRUE(Connect->getFuncType().isAsync());
  auto *Listen = Types->findFunction("[method]tcp-socket.listen");
  ASSERT_NE(Listen, nullptr);
  EXPECT_FALSE(Listen->getFuncType().isAsync());
  auto *Resolve = Insts[1]->findFunction("resolve-addresses");
  ASSERT_NE(Resolve, nullptr);
  EXPECT_TRUE(Resolve->getFuncType().isAsync());
}

TEST_F(WasiP3Sockets, TcpBindConnectAndOptions) {
  const uint64_t Listener = createTcp();
  ASSERT_NE(Listener, 0u);
  ASSERT_TRUE(
      status("[method]tcp-socket.bind",
             {tcp(Listener),
              Runtime::Component::Wit<IpSocketAddress>::into(loopback(0))}));
  auto Local =
      localAddress("[method]tcp-socket.get-local-address", tcp(Listener));
  ASSERT_TRUE(Local);
  EXPECT_EQ(Local->Family, IpAddressFamily::Ipv4);
  EXPECT_NE(Local->V4.Port, 0u);
  EXPECT_EQ(Local->V4.Address, std::make_tuple(127, 0, 0, 1));
  auto Listening =
      call(*Types, "[method]tcp-socket.get-is-listening", {tcp(Listener)});
  ASSERT_TRUE(Listening);
  EXPECT_FALSE(std::get<bool>((*Listening)[0]));
  auto Family =
      call(*Types, "[method]tcp-socket.get-address-family", {tcp(Listener)});
  ASSERT_TRUE(Family);
  EXPECT_EQ(Runtime::Component::Wit<IpAddressFamily>::from((*Family)[0]),
            IpAddressFamily::Ipv4);
  ASSERT_TRUE(status("[method]tcp-socket.set-listen-backlog-size",
                     {tcp(Listener), ComponentValVariant{uint64_t(4)}}));
  // Listen through the node itself: the stream form needs an executor.
  ASSERT_TRUE(Types->tcpSockets().get(Listener)->Node->sockListen(4));

  const uint64_t Client = createTcp();
  ASSERT_NE(Client, 0u);
  ASSERT_TRUE(
      status("[method]tcp-socket.connect",
             {tcp(Client), Runtime::Component::Wit<IpSocketAddress>::into(
                               loopback(Local->V4.Port))}));
  auto Remote =
      localAddress("[method]tcp-socket.get-remote-address", tcp(Client));
  ASSERT_TRUE(Remote);
  EXPECT_EQ(Remote->V4.Port, Local->V4.Port);

  ASSERT_TRUE(status("[method]tcp-socket.set-keep-alive-enabled",
                     {tcp(Client), ComponentValVariant{true}}));
  auto KeepAlive =
      call(*Types, "[method]tcp-socket.get-keep-alive-enabled", {tcp(Client)});
  ASSERT_TRUE(KeepAlive);
  auto Enabled =
      Runtime::Component::Wit<SockResult<bool>>::from((*KeepAlive)[0]);
  ASSERT_TRUE(Enabled);
  EXPECT_TRUE(*Enabled);
  ASSERT_TRUE(status("[method]tcp-socket.set-receive-buffer-size",
                     {tcp(Client), ComponentValVariant{uint64_t(65536)}}));
  auto Buffer =
      call(*Types, "[method]tcp-socket.get-receive-buffer-size", {tcp(Client)});
  ASSERT_TRUE(Buffer);
  auto Size = Runtime::Component::Wit<SockResult<uint64_t>>::from((*Buffer)[0]);
  ASSERT_TRUE(Size);
  EXPECT_GE(*Size, 65536u);
  auto Zero = status("[method]tcp-socket.set-hop-limit",
                     {tcp(Client), ComponentValVariant{uint8_t(0)}});
  ASSERT_FALSE(Zero);
  EXPECT_EQ(Zero.error().Case, SocketError::InvalidArgument);
  ASSERT_TRUE(status("[method]tcp-socket.set-hop-limit",
                     {tcp(Client), ComponentValVariant{uint8_t(33)}}));
  auto Hops = call(*Types, "[method]tcp-socket.get-hop-limit", {tcp(Client)});
  ASSERT_TRUE(Hops);
  auto Limit = Runtime::Component::Wit<SockResult<uint8_t>>::from((*Hops)[0]);
  ASSERT_TRUE(Limit);
  EXPECT_EQ(*Limit, 33u);
  ASSERT_TRUE(status("[method]tcp-socket.set-keep-alive-count",
                     {tcp(Client), ComponentValVariant{uint32_t(3)}}));
  ASSERT_TRUE(status("[method]tcp-socket.set-keep-alive-idle-time",
                     {tcp(Client), ComponentValVariant{uint64_t(2500000000)}}));
  auto Idle = call(*Types, "[method]tcp-socket.get-keep-alive-idle-time",
                   {tcp(Client)});
  ASSERT_TRUE(Idle);
  auto IdleTime =
      Runtime::Component::Wit<SockResult<uint64_t>>::from((*Idle)[0]);
  ASSERT_TRUE(IdleTime);
  EXPECT_EQ(*IdleTime, 3000000000u);

  auto Refused =
      status("[method]tcp-socket.connect",
             {tcp(createTcp()),
              Runtime::Component::Wit<IpSocketAddress>::into(loopback(1))});
  ASSERT_FALSE(Refused);
  EXPECT_EQ(Refused.error().Case, SocketError::ConnectionRefused);
  auto Bad = status(
      "[method]tcp-socket.bind",
      {tcp(999), Runtime::Component::Wit<IpSocketAddress>::into(loopback(0))});
  ASSERT_FALSE(Bad);
  EXPECT_EQ(Bad.error().Case, SocketError::InvalidState);
  EXPECT_EQ(Types->tcpSockets().size(), 3u);
}

TEST_F(WasiP3Sockets, UdpDatagramsRoundTrip) {
  const uint64_t Server = createUdp();
  const uint64_t Client = createUdp();
  ASSERT_TRUE(
      status("[method]udp-socket.bind",
             {udp(Server),
              Runtime::Component::Wit<IpSocketAddress>::into(loopback(0))}));
  auto ServerAddr =
      localAddress("[method]udp-socket.get-local-address", udp(Server));
  ASSERT_TRUE(ServerAddr);
  ASSERT_TRUE(
      status("[method]udp-socket.bind",
             {udp(Client),
              Runtime::Component::Wit<IpSocketAddress>::into(loopback(0))}));
  auto NoRemote =
      localAddress("[method]udp-socket.get-remote-address", udp(Client));
  ASSERT_FALSE(NoRemote);
  EXPECT_EQ(NoRemote.error().Case, SocketError::InvalidState);

  std::vector<uint8_t> Payload{'p', 'i', 'n', 'g'};
  ASSERT_TRUE(status(
      "[method]udp-socket.send",
      {udp(Client),
       Runtime::Component::Wit<std::vector<uint8_t>>::into(std::move(Payload)),
       Runtime::Component::Wit<std::optional<IpSocketAddress>>::into(
           std::optional<IpSocketAddress>(*ServerAddr))}));
  auto Got = call(*Types, "[method]udp-socket.receive", {udp(Server)});
  ASSERT_TRUE(Got);
  using Datagram = std::tuple<std::vector<uint8_t>, IpSocketAddress>;
  auto Received =
      Runtime::Component::Wit<SockResult<Datagram>>::from((*Got)[0]);
  ASSERT_TRUE(Received);
  EXPECT_EQ(std::get<0>(*Received), (std::vector<uint8_t>{'p', 'i', 'n', 'g'}));
  auto ClientAddr =
      localAddress("[method]udp-socket.get-local-address", udp(Client));
  ASSERT_TRUE(ClientAddr);
  EXPECT_EQ(std::get<1>(*Received).V4.Port, ClientAddr->V4.Port);

  // A connected socket answers without an explicit address.
  ASSERT_TRUE(
      status("[method]udp-socket.connect",
             {udp(Server), Runtime::Component::Wit<IpSocketAddress>::into(
                               IpSocketAddress(std::get<1>(*Received)))}));
  std::vector<uint8_t> Reply{'p', 'o', 'n', 'g'};
  ASSERT_TRUE(status(
      "[method]udp-socket.send",
      {udp(Server),
       Runtime::Component::Wit<std::vector<uint8_t>>::into(std::move(Reply)),
       Runtime::Component::Wit<std::optional<IpSocketAddress>>::into(
           std::optional<IpSocketAddress>())}));
  auto Back = call(*Types, "[method]udp-socket.receive", {udp(Client)});
  ASSERT_TRUE(Back);
  auto Answer = Runtime::Component::Wit<SockResult<Datagram>>::from((*Back)[0]);
  ASSERT_TRUE(Answer);
  EXPECT_EQ(std::get<0>(*Answer), (std::vector<uint8_t>{'p', 'o', 'n', 'g'}));
  auto Remote =
      localAddress("[method]udp-socket.get-remote-address", udp(Server));
  ASSERT_TRUE(Remote);
  EXPECT_EQ(Remote->V4.Port, ClientAddr->V4.Port);
  ASSERT_TRUE(status("[method]udp-socket.disconnect", {udp(Server)}));
  auto Twice = status("[method]udp-socket.disconnect", {udp(Server)});
  ASSERT_FALSE(Twice);
  EXPECT_EQ(Twice.error().Case, SocketError::InvalidState);
  ASSERT_TRUE(status("[method]udp-socket.set-unicast-hop-limit",
                     {udp(Server), ComponentValVariant{uint8_t(7)}}));
  auto Hops =
      call(*Types, "[method]udp-socket.get-unicast-hop-limit", {udp(Server)});
  ASSERT_TRUE(Hops);
  auto Limit = Runtime::Component::Wit<SockResult<uint8_t>>::from((*Hops)[0]);
  ASSERT_TRUE(Limit);
  EXPECT_EQ(*Limit, 7u);
}

TEST_F(WasiP3Sockets, ResolvesLocalhost) {
  auto Res = call(*Insts[1], "resolve-addresses",
                  {Runtime::Component::Wit<std::string>::into("localhost")});
  ASSERT_TRUE(Res);
  auto Addrs = Runtime::Component::Wit<
      Expected<std::vector<IpAddress>, LookupError>>::from((*Res)[0]);
  ASSERT_TRUE(Addrs);
  ASSERT_FALSE(Addrs->empty());
  bool Loopback = false;
  for (const auto &A : *Addrs) {
    if ((A.Family == IpAddressFamily::Ipv4 &&
         A.V4 == std::make_tuple(127, 0, 0, 1)) ||
        (A.Family == IpAddressFamily::Ipv6 &&
         A.V6 == std::make_tuple(0, 0, 0, 0, 0, 0, 0, 1))) {
      Loopback = true;
    }
  }
  EXPECT_TRUE(Loopback);
  auto Bad = call(
      *Insts[1], "resolve-addresses",
      {Runtime::Component::Wit<std::string>::into("no-such-host.invalid")});
  ASSERT_TRUE(Bad);
  auto None = Runtime::Component::Wit<
      Expected<std::vector<IpAddress>, LookupError>>::from((*Bad)[0]);
  ASSERT_FALSE(None);
  EXPECT_NE(None.error().Case, LookupError::Other);
}

} // namespace
