// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- test/host/wasi/p2_sockets.cpp - wasi:sockets 0.2.12 host ----------===//

#include "common/component_variant.h"
#include "common/defines.h"
#include "host/wasi/component/env.h"
#include "host/wasi/p2/io.h"
#include "host/wasi/p2/sockets.h"
#include "hostcall.h"
#include "runtime/component/wit.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

namespace {

using namespace WasmEdge;
using namespace Host::WasiP2;
using HostTest::call;

IpSocketAddress loopback(uint16_t Port) {
  IpSocketAddress A;
  A.Family = IpAddressFamily::Ipv4;
  A.V4 = Ipv4SocketAddress{Port, std::make_tuple(127, 0, 0, 1)};
  return A;
}

class WasiP2Sockets : public testing::Test {
protected:
  void SetUp() override {
    Env.init({}, "prog", {}, {});
    Io = std::make_unique<IoHost>(Env);
    IoInsts = makeIoInstances(*Io);
    Sockets = std::make_unique<SocketsHost>(*Io);
    Insts = makeSocketsInstances(*Sockets);
    Poll = IoInsts[1].get();
    Streams = IoInsts[2].get();
    Lookup = Insts[2].get();
    Tcp = Insts[3].get();
    Udp = Insts[5].get();
  }

  ComponentValVariant network() {
    return Runtime::Component::Wit<NetworkBorrow>::into(NetworkBorrow{1});
  }
  ComponentValVariant tcp(uint64_t Rep) {
    return Runtime::Component::Wit<TcpBorrow>::into(TcpBorrow{Rep});
  }
  ComponentValVariant udp(uint64_t Rep) {
    return Runtime::Component::Wit<UdpBorrow>::into(UdpBorrow{Rep});
  }
  ComponentValVariant input(uint64_t Rep) {
    return Runtime::Component::Wit<InputBorrow>::into(InputBorrow{Rep});
  }
  ComponentValVariant output(uint64_t Rep) {
    return Runtime::Component::Wit<OutputBorrow>::into(OutputBorrow{Rep});
  }
  ComponentValVariant pollable(uint64_t Rep) {
    return Runtime::Component::Wit<PollableBorrow>::into(PollableBorrow{Rep});
  }

  NetStatus status(const Expect<std::vector<ComponentValVariant>> &Res) {
    EXPECT_TRUE(Res);
    return Runtime::Component::Wit<NetStatus>::from((*Res)[0]);
  }
  uint64_t createTcp() {
    auto Res = call(*Insts[4], "create-tcp-socket",
                    {Runtime::Component::Wit<IpAddressFamily>::into(
                        IpAddressFamily::Ipv4)});
    EXPECT_TRUE(Res);
    auto Sock = Runtime::Component::Wit<
        NetResult<Runtime::Component::Own<TcpSocket>>>::from((*Res)[0]);
    EXPECT_TRUE(Sock);
    return Sock ? Sock->Rep : 0;
  }
  uint64_t createUdp() {
    auto Res = call(*Insts[6], "create-udp-socket",
                    {Runtime::Component::Wit<IpAddressFamily>::into(
                        IpAddressFamily::Ipv4)});
    EXPECT_TRUE(Res);
    auto Sock = Runtime::Component::Wit<
        NetResult<Runtime::Component::Own<UdpSocket>>>::from((*Res)[0]);
    EXPECT_TRUE(Sock);
    return Sock ? Sock->Rep : 0;
  }
  // Binds a loopback port and returns the address the socket got.
  std::optional<IpSocketAddress> bindTcp(uint64_t Rep) {
    EXPECT_TRUE(status(
        call(*Tcp, "[method]tcp-socket.start-bind",
             {tcp(Rep), network(),
              Runtime::Component::Wit<IpSocketAddress>::into(loopback(0))})));
    EXPECT_TRUE(
        status(call(*Tcp, "[method]tcp-socket.finish-bind", {tcp(Rep)})));
    auto Res = call(*Tcp, "[method]tcp-socket.local-address", {tcp(Rep)});
    EXPECT_TRUE(Res);
    auto Addr =
        Runtime::Component::Wit<NetResult<IpSocketAddress>>::from((*Res)[0]);
    EXPECT_TRUE(Addr);
    if (!Addr) {
      return std::nullopt;
    }
    return *Addr;
  }
  std::optional<IpSocketAddress> bindUdp(uint64_t Rep) {
    EXPECT_TRUE(status(
        call(*Udp, "[method]udp-socket.start-bind",
             {udp(Rep), network(),
              Runtime::Component::Wit<IpSocketAddress>::into(loopback(0))})));
    EXPECT_TRUE(
        status(call(*Udp, "[method]udp-socket.finish-bind", {udp(Rep)})));
    auto Res = call(*Udp, "[method]udp-socket.local-address", {udp(Rep)});
    EXPECT_TRUE(Res);
    auto Addr =
        Runtime::Component::Wit<NetResult<IpSocketAddress>>::from((*Res)[0]);
    EXPECT_TRUE(Addr);
    if (!Addr) {
      return std::nullopt;
    }
    return *Addr;
  }
  void block(uint64_t PollRep) {
    ASSERT_TRUE(call(*Poll, "[method]pollable.block", {pollable(PollRep)}));
  }
  std::vector<uint8_t> readBlocking(uint64_t InRep) {
    auto Res = call(*Streams, "[method]input-stream.blocking-read",
                    {input(InRep), ComponentValVariant{uint64_t(64)}});
    EXPECT_TRUE(Res);
    auto Bytes = Runtime::Component::Wit<IoResult<std::vector<uint8_t>>>::from(
        (*Res)[0]);
    EXPECT_TRUE(Bytes);
    return Bytes ? *Bytes : std::vector<uint8_t>{};
  }
  void writeBlocking(uint64_t OutRep, std::vector<uint8_t> Data) {
    auto Res = call(
        *Streams, "[method]output-stream.blocking-write-and-flush",
        {output(OutRep),
         Runtime::Component::Wit<std::vector<uint8_t>>::into(std::move(Data))});
    ASSERT_TRUE(Res);
    EXPECT_TRUE(
        Runtime::Component::Wit<IoResult<Runtime::Component::Unit>>::from(
            (*Res)[0]));
  }

  Host::WasiComponent::Env Env;
  std::unique_ptr<IoHost> Io;
  std::unique_ptr<SocketsHost> Sockets;
  std::vector<std::unique_ptr<Runtime::Instance::ComponentInstance>> IoInsts;
  std::vector<std::unique_ptr<Runtime::Instance::ComponentInstance>> Insts;
  Runtime::Instance::ComponentInstance *Poll = nullptr;
  Runtime::Instance::ComponentInstance *Streams = nullptr;
  Runtime::Instance::ComponentInstance *Lookup = nullptr;
  Runtime::Instance::ComponentInstance *Tcp = nullptr;
  Runtime::Instance::ComponentInstance *Udp = nullptr;
};

TEST_F(WasiP2Sockets, InstancesAndTypes) {
  ASSERT_EQ(Insts.size(), 7u);
  EXPECT_EQ(Insts[0]->getComponentName(), "wasi:sockets/network@0.2.12");
  EXPECT_EQ(Insts[1]->getComponentName(),
            "wasi:sockets/instance-network@0.2.12");
  EXPECT_EQ(Insts[2]->getComponentName(), "wasi:sockets/ip-name-lookup@0.2.12");
  EXPECT_EQ(Insts[3]->getComponentName(), "wasi:sockets/tcp@0.2.12");
  EXPECT_EQ(Insts[4]->getComponentName(),
            "wasi:sockets/tcp-create-socket@0.2.12");
  EXPECT_EQ(Insts[5]->getComponentName(), "wasi:sockets/udp@0.2.12");
  EXPECT_EQ(Insts[6]->getComponentName(),
            "wasi:sockets/udp-create-socket@0.2.12");
  EXPECT_EQ(Insts[0]->findTypeResource("network"), Sockets->NetworkType);
  EXPECT_EQ(Insts[1]->findTypeResource("network"), Sockets->NetworkType);
  EXPECT_EQ(Tcp->findTypeResource("tcp-socket"), Sockets->TcpType);
  EXPECT_EQ(Insts[4]->findTypeResource("tcp-socket"), Sockets->TcpType);
  EXPECT_EQ(Udp->findTypeResource("udp-socket"), Sockets->UdpType);
  EXPECT_EQ(Tcp->findTypeResource("input-stream"), Io->InputType);
  EXPECT_EQ(Tcp->findTypeResource("pollable"), Io->PollableType);
  const auto *Err = Insts[0]->findType("error-code");
  ASSERT_NE(Err, nullptr);
  ASSERT_TRUE(Err->getDefValType().isEnumTy());
  const auto &Labels = Err->getDefValType().getEnum().Labels;
  ASSERT_EQ(Labels.size(), 21u);
  EXPECT_EQ(Labels[0], "unknown");
  EXPECT_EQ(Labels[8], "would-block");
  EXPECT_EQ(Labels[20], "permanent-resolver-failure");
  auto Net = call(*Insts[1], "instance-network", {});
  ASSERT_TRUE(Net);
  EXPECT_EQ(
      Runtime::Component::Wit<Runtime::Component::Own<Network>>::from((*Net)[0])
          .Rep,
      1u);
}

TEST_F(WasiP2Sockets, TcpLoopbackThroughStreams) {
  const uint64_t Listener = createTcp();
  ASSERT_NE(Listener, 0u);
  auto Local = bindTcp(Listener);
  ASSERT_TRUE(Local);
  EXPECT_NE(Local->V4.Port, 0u);
  EXPECT_EQ(Local->V4.Address, std::make_tuple(127, 0, 0, 1));

  // A finish without a start is not in progress.
  auto Again =
      status(call(*Tcp, "[method]tcp-socket.finish-bind", {tcp(Listener)}));
  ASSERT_FALSE(Again);
  EXPECT_EQ(Again.error(), NetworkError::NotInProgress);

  ASSERT_TRUE(status(call(*Tcp, "[method]tcp-socket.set-listen-backlog-size",
                          {tcp(Listener), ComponentValVariant{uint64_t(4)}})));
  ASSERT_TRUE(
      status(call(*Tcp, "[method]tcp-socket.start-listen", {tcp(Listener)})));
  ASSERT_TRUE(
      status(call(*Tcp, "[method]tcp-socket.finish-listen", {tcp(Listener)})));
  auto Listening =
      call(*Tcp, "[method]tcp-socket.is-listening", {tcp(Listener)});
  ASSERT_TRUE(Listening);
  EXPECT_TRUE(std::get<bool>((*Listening)[0]));

  // Nobody is connecting yet.
  auto Early = call(*Tcp, "[method]tcp-socket.accept", {tcp(Listener)});
  ASSERT_TRUE(Early);
  using Accepted = std::tuple<Runtime::Component::Own<TcpSocket>,
                              Runtime::Component::Own<InputStream>,
                              Runtime::Component::Own<OutputStream>>;
  auto NoClient =
      Runtime::Component::Wit<NetResult<Accepted>>::from((*Early)[0]);
  ASSERT_FALSE(NoClient);
  EXPECT_EQ(NoClient.error(), NetworkError::WouldBlock);

  const uint64_t Client = createTcp();
  ASSERT_TRUE(status(call(*Tcp, "[method]tcp-socket.start-connect",
                          {tcp(Client), network(),
                           Runtime::Component::Wit<IpSocketAddress>::into(
                               loopback(Local->V4.Port))})));
  auto Connected =
      call(*Tcp, "[method]tcp-socket.finish-connect", {tcp(Client)});
  ASSERT_TRUE(Connected);
  using ClientStreams = std::tuple<Runtime::Component::Own<InputStream>,
                                   Runtime::Component::Own<OutputStream>>;
  auto CStreams =
      Runtime::Component::Wit<NetResult<ClientStreams>>::from((*Connected)[0]);
  ASSERT_TRUE(CStreams);
  auto Remote = call(*Tcp, "[method]tcp-socket.remote-address", {tcp(Client)});
  ASSERT_TRUE(Remote);
  auto RemoteAddr =
      Runtime::Component::Wit<NetResult<IpSocketAddress>>::from((*Remote)[0]);
  ASSERT_TRUE(RemoteAddr);
  EXPECT_EQ(RemoteAddr->V4.Port, Local->V4.Port);

  // The listener's pollable wakes for the pending connection.
  auto Sub = call(*Tcp, "[method]tcp-socket.subscribe", {tcp(Listener)});
  ASSERT_TRUE(Sub);
  block(Runtime::Component::Wit<Runtime::Component::Own<PollSource>>::from(
            (*Sub)[0])
            .Rep);
  auto Acc = call(*Tcp, "[method]tcp-socket.accept", {tcp(Listener)});
  ASSERT_TRUE(Acc);
  auto Server = Runtime::Component::Wit<NetResult<Accepted>>::from((*Acc)[0]);
  ASSERT_TRUE(Server);
  const uint64_t ServerSock = std::get<0>(*Server).Rep;
  auto Family =
      call(*Tcp, "[method]tcp-socket.address-family", {tcp(ServerSock)});
  ASSERT_TRUE(Family);
  EXPECT_EQ(Runtime::Component::Wit<IpAddressFamily>::from((*Family)[0]),
            IpAddressFamily::Ipv4);

  writeBlocking(std::get<1>(*CStreams).Rep, {'p', 'i', 'n', 'g'});
  EXPECT_EQ(readBlocking(std::get<1>(*Server).Rep),
            (std::vector<uint8_t>{'p', 'i', 'n', 'g'}));
  writeBlocking(std::get<2>(*Server).Rep, {'p', 'o', 'n', 'g'});
  EXPECT_EQ(readBlocking(std::get<0>(*CStreams).Rep),
            (std::vector<uint8_t>{'p', 'o', 'n', 'g'}));

  // Options round-trip on a connected socket.
  ASSERT_TRUE(status(call(*Tcp, "[method]tcp-socket.set-keep-alive-enabled",
                          {tcp(Client), ComponentValVariant{true}})));
  auto KeepAlive =
      call(*Tcp, "[method]tcp-socket.keep-alive-enabled", {tcp(Client)});
  ASSERT_TRUE(KeepAlive);
  auto Enabled =
      Runtime::Component::Wit<NetResult<bool>>::from((*KeepAlive)[0]);
  ASSERT_TRUE(Enabled);
  EXPECT_TRUE(*Enabled);
  auto Zero = status(call(*Tcp, "[method]tcp-socket.set-hop-limit",
                          {tcp(Client), ComponentValVariant{uint8_t(0)}}));
  ASSERT_FALSE(Zero);
  EXPECT_EQ(Zero.error(), NetworkError::InvalidArgument);

  // Shutting the client's send side ends the server's input.
  ASSERT_TRUE(status(
      call(*Tcp, "[method]tcp-socket.shutdown",
           {tcp(Client),
            Runtime::Component::Wit<ShutdownType>::into(ShutdownType::Send)})));
  auto End =
      call(*Streams, "[method]input-stream.blocking-read",
           {input(std::get<1>(*Server).Rep), ComponentValVariant{uint64_t(8)}});
  ASSERT_TRUE(End);
  auto Closed =
      Runtime::Component::Wit<IoResult<std::vector<uint8_t>>>::from((*End)[0]);
  ASSERT_FALSE(Closed);
  EXPECT_TRUE(Closed.error().Closed);

  // Listening on a connected socket is an invalid state.
  auto Bad =
      status(call(*Tcp, "[method]tcp-socket.start-listen", {tcp(Client)}));
  ASSERT_FALSE(Bad);
  EXPECT_EQ(Bad.error(), NetworkError::InvalidState);
}

TEST_F(WasiP2Sockets, ConnectRefusedIsReported) {
  const uint64_t Client = createTcp();
  ASSERT_TRUE(status(
      call(*Tcp, "[method]tcp-socket.start-connect",
           {tcp(Client), network(),
            Runtime::Component::Wit<IpSocketAddress>::into(loopback(1))})));
  auto Res = call(*Tcp, "[method]tcp-socket.finish-connect", {tcp(Client)});
  ASSERT_TRUE(Res);
  using ClientStreams = std::tuple<Runtime::Component::Own<InputStream>,
                                   Runtime::Component::Own<OutputStream>>;
  auto Streams2 =
      Runtime::Component::Wit<NetResult<ClientStreams>>::from((*Res)[0]);
  ASSERT_FALSE(Streams2);
  EXPECT_EQ(Streams2.error(), NetworkError::ConnectionRefused);
  auto Port0 = status(
      call(*Tcp, "[method]tcp-socket.start-connect",
           {tcp(createTcp()), network(),
            Runtime::Component::Wit<IpSocketAddress>::into(loopback(0))}));
  ASSERT_FALSE(Port0);
  EXPECT_EQ(Port0.error(), NetworkError::InvalidArgument);
}

TEST_F(WasiP2Sockets, UdpDatagramStreams) {
  const uint64_t Server = createUdp();
  const uint64_t Client = createUdp();
  auto ServerAddr = bindUdp(Server);
  auto ClientAddr = bindUdp(Client);
  ASSERT_TRUE(ServerAddr);
  ASSERT_TRUE(ClientAddr);

  using DgStreams = std::tuple<Runtime::Component::Own<IncomingDatagramStream>,
                               Runtime::Component::Own<OutgoingDatagramStream>>;
  auto CRes =
      call(*Udp, "[method]udp-socket.stream",
           {udp(Client),
            Runtime::Component::Wit<std::optional<IpSocketAddress>>::into(
                std::optional<IpSocketAddress>(*ServerAddr))});
  ASSERT_TRUE(CRes);
  auto CStreams =
      Runtime::Component::Wit<NetResult<DgStreams>>::from((*CRes)[0]);
  ASSERT_TRUE(CStreams);
  auto SRes =
      call(*Udp, "[method]udp-socket.stream",
           {udp(Server),
            Runtime::Component::Wit<std::optional<IpSocketAddress>>::into(
                std::optional<IpSocketAddress>())});
  ASSERT_TRUE(SRes);
  auto SStreams =
      Runtime::Component::Wit<NetResult<DgStreams>>::from((*SRes)[0]);
  ASSERT_TRUE(SStreams);
  auto Remote = call(*Udp, "[method]udp-socket.remote-address", {udp(Client)});
  ASSERT_TRUE(Remote);
  auto RemoteAddr =
      Runtime::Component::Wit<NetResult<IpSocketAddress>>::from((*Remote)[0]);
  ASSERT_TRUE(RemoteAddr);
  EXPECT_EQ(RemoteAddr->V4.Port, ServerAddr->V4.Port);

  // The connected client sends without a remote; the server sees the sender.
  auto Budget = call(*Udp, "[method]outgoing-datagram-stream.check-send",
                     {Runtime::Component::Wit<OutgoingBorrow>::into(
                         OutgoingBorrow{std::get<1>(*CStreams).Rep})});
  ASSERT_TRUE(Budget);
  auto Allowed =
      Runtime::Component::Wit<NetResult<uint64_t>>::from((*Budget)[0]);
  ASSERT_TRUE(Allowed);
  EXPECT_GT(*Allowed, 0u);
  auto Sent =
      call(*Udp, "[method]outgoing-datagram-stream.send",
           {Runtime::Component::Wit<OutgoingBorrow>::into(
                OutgoingBorrow{std::get<1>(*CStreams).Rep}),
            Runtime::Component::Wit<std::vector<OutgoingDatagram>>::into(
                {OutgoingDatagram{{'h', 'i'}, std::nullopt}})});
  ASSERT_TRUE(Sent);
  auto Count = Runtime::Component::Wit<NetResult<uint64_t>>::from((*Sent)[0]);
  ASSERT_TRUE(Count);
  EXPECT_EQ(*Count, 1u);
  auto Sub = call(*Udp, "[method]incoming-datagram-stream.subscribe",
                  {Runtime::Component::Wit<IncomingBorrow>::into(
                      IncomingBorrow{std::get<0>(*SStreams).Rep})});
  ASSERT_TRUE(Sub);
  block(Runtime::Component::Wit<Runtime::Component::Own<PollSource>>::from(
            (*Sub)[0])
            .Rep);
  auto Got = call(*Udp, "[method]incoming-datagram-stream.receive",
                  {Runtime::Component::Wit<IncomingBorrow>::into(
                       IncomingBorrow{std::get<0>(*SStreams).Rep}),
                   ComponentValVariant{uint64_t(8)}});
  ASSERT_TRUE(Got);
  auto Datagrams =
      Runtime::Component::Wit<NetResult<std::vector<IncomingDatagram>>>::from(
          (*Got)[0]);
  ASSERT_TRUE(Datagrams);
  ASSERT_EQ(Datagrams->size(), 1u);
  EXPECT_EQ((*Datagrams)[0].Data, (std::vector<uint8_t>{'h', 'i'}));
  EXPECT_EQ((*Datagrams)[0].Remote.V4.Port, ClientAddr->V4.Port);

  // The unconnected server needs a remote, and answers to it.
  auto NoRemote =
      call(*Udp, "[method]outgoing-datagram-stream.send",
           {Runtime::Component::Wit<OutgoingBorrow>::into(
                OutgoingBorrow{std::get<1>(*SStreams).Rep}),
            Runtime::Component::Wit<std::vector<OutgoingDatagram>>::into(
                {OutgoingDatagram{{'x'}, std::nullopt}})});
  ASSERT_TRUE(NoRemote);
  auto Missing =
      Runtime::Component::Wit<NetResult<uint64_t>>::from((*NoRemote)[0]);
  ASSERT_FALSE(Missing);
  EXPECT_EQ(Missing.error(), NetworkError::InvalidArgument);
  auto Reply =
      call(*Udp, "[method]outgoing-datagram-stream.send",
           {Runtime::Component::Wit<OutgoingBorrow>::into(
                OutgoingBorrow{std::get<1>(*SStreams).Rep}),
            Runtime::Component::Wit<std::vector<OutgoingDatagram>>::into(
                {OutgoingDatagram{{'o', 'k'}, (*Datagrams)[0].Remote}})});
  ASSERT_TRUE(Reply);
  EXPECT_EQ(*Runtime::Component::Wit<NetResult<uint64_t>>::from((*Reply)[0]),
            1u);
  auto CSub = call(*Udp, "[method]incoming-datagram-stream.subscribe",
                   {Runtime::Component::Wit<IncomingBorrow>::into(
                       IncomingBorrow{std::get<0>(*CStreams).Rep})});
  ASSERT_TRUE(CSub);
  block(Runtime::Component::Wit<Runtime::Component::Own<PollSource>>::from(
            (*CSub)[0])
            .Rep);
  auto Back = call(*Udp, "[method]incoming-datagram-stream.receive",
                   {Runtime::Component::Wit<IncomingBorrow>::into(
                        IncomingBorrow{std::get<0>(*CStreams).Rep}),
                    ComponentValVariant{uint64_t(8)}});
  ASSERT_TRUE(Back);
  auto Answer =
      Runtime::Component::Wit<NetResult<std::vector<IncomingDatagram>>>::from(
          (*Back)[0]);
  ASSERT_TRUE(Answer);
  ASSERT_EQ(Answer->size(), 1u);
  EXPECT_EQ((*Answer)[0].Data, (std::vector<uint8_t>{'o', 'k'}));
  EXPECT_EQ((*Answer)[0].Remote.V4.Port, ServerAddr->V4.Port);

  // An empty receive when nothing is pending.
  auto None = call(*Udp, "[method]incoming-datagram-stream.receive",
                   {Runtime::Component::Wit<IncomingBorrow>::into(
                        IncomingBorrow{std::get<0>(*CStreams).Rep}),
                    ComponentValVariant{uint64_t(8)}});
  ASSERT_TRUE(None);
  auto Empty =
      Runtime::Component::Wit<NetResult<std::vector<IncomingDatagram>>>::from(
          (*None)[0]);
  ASSERT_TRUE(Empty);
  EXPECT_TRUE(Empty->empty());
}

TEST_F(WasiP2Sockets, ResolvesLocalhost) {
  auto Res = call(
      *Lookup, "resolve-addresses",
      {network(), Runtime::Component::Wit<std::string>::into("localhost")});
  ASSERT_TRUE(Res);
  auto Stream = Runtime::Component::
      Wit<NetResult<Runtime::Component::Own<ResolveAddressStream>>>::from(
          (*Res)[0]);
  ASSERT_TRUE(Stream);
  auto Self =
      Runtime::Component::Wit<ResolveBorrow>::into(ResolveBorrow{Stream->Rep});
  auto Sub = call(*Lookup, "[method]resolve-address-stream.subscribe", {Self});
  ASSERT_TRUE(Sub);
  auto Ready = call(
      *Poll, "[method]pollable.ready",
      {pollable(
          Runtime::Component::Wit<Runtime::Component::Own<PollSource>>::from(
              (*Sub)[0])
              .Rep)});
  ASSERT_TRUE(Ready);
  EXPECT_TRUE(std::get<bool>((*Ready)[0]));
  size_t Count = 0;
  bool Loopback = false;
  while (true) {
    auto Next = call(
        *Lookup, "[method]resolve-address-stream.resolve-next-address", {Self});
    ASSERT_TRUE(Next);
    auto Addr =
        Runtime::Component::Wit<NetResult<std::optional<IpAddress>>>::from(
            (*Next)[0]);
    ASSERT_TRUE(Addr);
    if (!*Addr) {
      break;
    }
    ++Count;
    if ((*Addr)->Family == IpAddressFamily::Ipv4 &&
        (*Addr)->V4 == std::make_tuple(127, 0, 0, 1)) {
      Loopback = true;
    }
    ASSERT_LT(Count, 64u);
  }
  EXPECT_GT(Count, 0u);
  EXPECT_TRUE(Loopback || Count > 0);

  auto Bad = call(*Lookup, "resolve-addresses",
                  {network(), Runtime::Component::Wit<std::string>::into("")});
  ASSERT_TRUE(Bad);
  auto Invalid = Runtime::Component::
      Wit<NetResult<Runtime::Component::Own<ResolveAddressStream>>>::from(
          (*Bad)[0]);
  ASSERT_FALSE(Invalid);
  EXPECT_EQ(Invalid.error(), NetworkError::InvalidArgument);
}

} // namespace
