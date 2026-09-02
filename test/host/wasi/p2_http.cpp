// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- test/host/wasi/p2_http.cpp - wasi:http 0.2.12 host ----------------===//

#include "common/component_variant.h"
#include "common/defines.h"
#include "host/wasi/component/env.h"
#include "host/wasi/p2/http.h"
#include "host/wasi/p2/io.h"
#include "hostcall.h"
#include "runtime/component/wit.h"

#include <gtest/gtest.h>

#include <atomic>
#include <cstdint>
#include <cstring>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <tuple>
#include <vector>

#if !WASMEDGE_OS_WINDOWS
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace {

using namespace WasmEdge;
using namespace Host::WasiP2;
using HostTest::call;

std::vector<uint8_t> bytes(std::string_view S) {
  return std::vector<uint8_t>(S.begin(), S.end());
}

std::string text(const std::vector<uint8_t> &B) {
  return std::string(B.begin(), B.end());
}

class WasiP2Http : public testing::Test {
protected:
  void SetUp() override {
    Env.init({}, "prog", {}, {});
    Io = std::make_unique<IoHost>(Env);
    IoInsts = makeIoInstances(*Io);
    Http = std::make_unique<HttpHost>(*Io);
    Insts = makeHttpInstances(*Http);
    Types = Insts[0].get();
    Handler = Insts[1].get();
    Streams = IoInsts[2].get();
    Poll = IoInsts[1].get();
  }

  ComponentValVariant fields(uint64_t Rep) {
    return Runtime::Component::Wit<FieldsBorrow>::into(FieldsBorrow{Rep});
  }
  ComponentValVariant str(std::string_view S) {
    return Runtime::Component::Wit<std::string>::into(std::string(S));
  }
  uint64_t newFields() {
    auto Res = call(*Types, "[constructor]fields", {});
    EXPECT_TRUE(Res);
    return Runtime::Component::Wit<Runtime::Component::Own<Fields>>::from(
               (*Res)[0])
        .Rep;
  }
  HeaderStatus setField(uint64_t Rep, std::string_view Name,
                        std::string_view Value) {
    auto Res =
        call(*Types, "[method]fields.set",
             {fields(Rep), str(Name),
              Runtime::Component::Wit<std::vector<std::vector<uint8_t>>>::into(
                  {bytes(Value)})});
    EXPECT_TRUE(Res);
    return Runtime::Component::Wit<HeaderStatus>::from((*Res)[0]);
  }
  std::vector<std::string> getField(uint64_t Rep, std::string_view Name) {
    auto Res = call(*Types, "[method]fields.get", {fields(Rep), str(Name)});
    EXPECT_TRUE(Res);
    std::vector<std::string> Out;
    for (auto &V :
         Runtime::Component::Wit<std::vector<std::vector<uint8_t>>>::from(
             (*Res)[0])) {
      Out.push_back(text(V));
    }
    return Out;
  }
  std::string readAll(uint64_t InRep) {
    std::string Out;
    while (true) {
      auto Res =
          call(*Streams, "[method]input-stream.blocking-read",
               {Runtime::Component::Wit<InputBorrow>::into(InputBorrow{InRep}),
                ComponentValVariant{uint64_t(4)}});
      EXPECT_TRUE(Res);
      auto Chunk =
          Runtime::Component::Wit<IoResult<std::vector<uint8_t>>>::from(
              (*Res)[0]);
      if (!Chunk) {
        EXPECT_TRUE(Chunk.error().Closed);
        return Out;
      }
      Out += text(*Chunk);
    }
  }
  void writeAll(uint64_t OutRep, std::string_view Data) {
    auto Res = call(
        *Streams, "[method]output-stream.blocking-write-and-flush",
        {Runtime::Component::Wit<OutputBorrow>::into(OutputBorrow{OutRep}),
         Runtime::Component::Wit<std::vector<uint8_t>>::into(bytes(Data))});
    ASSERT_TRUE(Res);
    EXPECT_TRUE(
        Runtime::Component::Wit<IoResult<Runtime::Component::Unit>>::from(
            (*Res)[0]));
  }

  Host::WasiComponent::Env Env;
  std::unique_ptr<IoHost> Io;
  std::unique_ptr<HttpHost> Http;
  std::vector<std::unique_ptr<Runtime::Instance::ComponentInstance>> IoInsts;
  std::vector<std::unique_ptr<Runtime::Instance::ComponentInstance>> Insts;
  Runtime::Instance::ComponentInstance *Types = nullptr;
  Runtime::Instance::ComponentInstance *Handler = nullptr;
  Runtime::Instance::ComponentInstance *Streams = nullptr;
  Runtime::Instance::ComponentInstance *Poll = nullptr;
};

TEST_F(WasiP2Http, InstancesAndTypes) {
  ASSERT_EQ(Insts.size(), 2u);
  EXPECT_EQ(Types->getComponentName(), "wasi:http/types@0.2.12");
  EXPECT_EQ(Handler->getComponentName(), "wasi:http/outgoing-handler@0.2.12");
  EXPECT_NE(Types->findTypeResource("fields"), nullptr);
  EXPECT_EQ(Types->findTypeResource("headers"),
            Types->findTypeResource("fields"));
  EXPECT_EQ(Types->findTypeResource("trailers"),
            Types->findTypeResource("fields"));
  EXPECT_EQ(Handler->findTypeResource("outgoing-request"),
            Types->findTypeResource("outgoing-request"));
  EXPECT_EQ(Types->findTypeResource("input-stream"), Io->InputType);
  const auto *Err = Types->findType("error-code");
  ASSERT_NE(Err, nullptr);
  ASSERT_TRUE(Err->getDefValType().isVariantTy());
  const auto &Cases = Err->getDefValType().getVariant().Cases;
  ASSERT_EQ(Cases.size(), 39u);
  EXPECT_EQ(Cases[0].first, "DNS-timeout");
  EXPECT_TRUE(Cases[1].second.has_value());
  EXPECT_EQ(Cases[6].first, "connection-refused");
  EXPECT_FALSE(Cases[6].second.has_value());
  EXPECT_EQ(Cases[38].first, "internal-error");
  EXPECT_TRUE(Cases[38].second.has_value());
  const auto *M = Types->findType("method");
  ASSERT_NE(M, nullptr);
  ASSERT_TRUE(M->getDefValType().isVariantTy());
  EXPECT_EQ(M->getDefValType().getVariant().Cases.size(), 10u);
  EXPECT_NE(Types->findFunction("[static]response-outparam.set"), nullptr);
  EXPECT_NE(Handler->findFunction("handle"), nullptr);
}

TEST_F(WasiP2Http, FieldsFollowTheHeaderRules) {
  const uint64_t F = newFields();
  ASSERT_TRUE(setField(F, "Content-Type", "text/plain"));
  auto Appended =
      call(*Types, "[method]fields.append",
           {fields(F), str("Accept"),
            Runtime::Component::Wit<std::vector<uint8_t>>::into(bytes("*/*"))});
  ASSERT_TRUE(Appended);
  EXPECT_TRUE(Runtime::Component::Wit<HeaderStatus>::from((*Appended)[0]));
  EXPECT_EQ(getField(F, "content-type"),
            (std::vector<std::string>{"text/plain"}));
  auto Has = call(*Types, "[method]fields.has", {fields(F), str("ACCEPT")});
  ASSERT_TRUE(Has);
  EXPECT_TRUE(std::get<bool>((*Has)[0]));

  auto Forbidden = setField(F, "Host", "x");
  ASSERT_FALSE(Forbidden);
  EXPECT_EQ(Forbidden.error(), HeaderError::Forbidden);
  auto Invalid = setField(F, "bad name", "x");
  ASSERT_FALSE(Invalid);
  EXPECT_EQ(Invalid.error(), HeaderError::InvalidSyntax);
  auto BadValue = setField(F, "X-Line", "a\r\nb");
  ASSERT_FALSE(BadValue);
  EXPECT_EQ(BadValue.error(), HeaderError::InvalidSyntax);

  auto Entries = call(*Types, "[method]fields.entries", {fields(F)});
  ASSERT_TRUE(Entries);
  auto List =
      Runtime::Component::Wit<std::vector<FieldEntry>>::from((*Entries)[0]);
  ASSERT_EQ(List.size(), 2u);
  EXPECT_EQ(std::get<0>(List[0]), "Content-Type");
  EXPECT_EQ(text(std::get<1>(List[1])), "*/*");

  auto Deleted =
      call(*Types, "[method]fields.delete", {fields(F), str("accept")});
  ASSERT_TRUE(Deleted);
  EXPECT_TRUE(getField(F, "accept").empty());

  auto FromList =
      call(*Types, "[static]fields.from-list",
           {Runtime::Component::Wit<std::vector<FieldEntry>>::into(
               {FieldEntry{"a", bytes("1")}, FieldEntry{"b", bytes("2")}})});
  ASSERT_TRUE(FromList);
  auto Made =
      Runtime::Component::Wit<Expected<Runtime::Component::Own<Fields>,
                                       HeaderError>>::from((*FromList)[0]);
  ASSERT_TRUE(Made);
  EXPECT_EQ(getField(Made->Rep, "b"), (std::vector<std::string>{"2"}));
  auto BadList = call(*Types, "[static]fields.from-list",
                      {Runtime::Component::Wit<std::vector<FieldEntry>>::into(
                          {FieldEntry{"connection", bytes("close")}})});
  ASSERT_TRUE(BadList);
  EXPECT_FALSE(
      (Runtime::Component::Wit<Expected<Runtime::Component::Own<Fields>,
                                        HeaderError>>::from((*BadList)[0])));

  // Headers attached to a request are immutable; the clone is not.
  auto Req =
      call(*Types, "[constructor]outgoing-request",
           {Runtime::Component::Wit<Runtime::Component::Own<Fields>>::into(
               Runtime::Component::Own<Fields>{F})});
  ASSERT_TRUE(Req);
  const uint64_t ReqRep =
      Runtime::Component::Wit<Runtime::Component::Own<OutgoingRequest>>::from(
          (*Req)[0])
          .Rep;
  auto Headers = call(*Types, "[method]outgoing-request.headers",
                      {Runtime::Component::Wit<OutgoingRequestBorrow>::into(
                          OutgoingRequestBorrow{ReqRep})});
  ASSERT_TRUE(Headers);
  const uint64_t H =
      Runtime::Component::Wit<Runtime::Component::Own<Fields>>::from(
          (*Headers)[0])
          .Rep;
  EXPECT_EQ(getField(H, "content-type"),
            (std::vector<std::string>{"text/plain"}));
  auto Frozen = setField(H, "X-New", "1");
  ASSERT_FALSE(Frozen);
  EXPECT_EQ(Frozen.error(), HeaderError::Immutable);
  auto Clone = call(*Types, "[method]fields.clone", {fields(H)});
  ASSERT_TRUE(Clone);
  const uint64_t C =
      Runtime::Component::Wit<Runtime::Component::Own<Fields>>::from(
          (*Clone)[0])
          .Rep;
  EXPECT_TRUE(setField(C, "X-New", "1"));
}

#if !WASMEDGE_OS_WINDOWS
// A one-shot HTTP/1.1 server on the loopback that answers with chunks and a
// trailer, keeping what it received.
class OneShotServer {
public:
  bool start() {
    Fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (Fd < 0) {
      return false;
    }
    sockaddr_in Addr;
    std::memset(&Addr, 0, sizeof(Addr));
    Addr.sin_family = AF_INET;
    Addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    Addr.sin_port = 0;
    if (::bind(Fd, reinterpret_cast<sockaddr *>(&Addr), sizeof(Addr)) != 0 ||
        ::listen(Fd, 1) != 0) {
      return false;
    }
    socklen_t Len = sizeof(Addr);
    if (::getsockname(Fd, reinterpret_cast<sockaddr *>(&Addr), &Len) != 0) {
      return false;
    }
    Port = ntohs(Addr.sin_port);
    Thread = std::thread([this]() { serve(); });
    return true;
  }
  void join() {
    if (Thread.joinable()) {
      if (!Accepted) {
        // Wake the accept so a failed client does not hang the test.
        const int S = ::socket(AF_INET, SOCK_STREAM, 0);
        sockaddr_in Addr;
        std::memset(&Addr, 0, sizeof(Addr));
        Addr.sin_family = AF_INET;
        Addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        Addr.sin_port = htons(Port);
        ::connect(S, reinterpret_cast<sockaddr *>(&Addr), sizeof(Addr));
        ::close(S);
      }
      Thread.join();
    }
    if (Fd >= 0) {
      ::close(Fd);
      Fd = -1;
    }
  }
  ~OneShotServer() { join(); }

  uint16_t Port = 0;
  std::string Received;
  std::string Reply =
      "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n"
      "Transfer-Encoding: chunked\r\n\r\n"
      "5\r\nhello\r\n6\r\n world\r\n0\r\nX-Trailer: yes\r\n\r\n";

private:
  void serve() {
    const int Client = ::accept(Fd, nullptr, nullptr);
    Accepted = true;
    if (Client < 0) {
      return;
    }
    char Buffer[4096];
    size_t Expected = std::string::npos;
    while (true) {
      const auto N = ::read(Client, Buffer, sizeof(Buffer));
      if (N <= 0) {
        break;
      }
      Received.append(Buffer, static_cast<size_t>(N));
      const auto End = Received.find("\r\n\r\n");
      if (End == std::string::npos) {
        continue;
      }
      if (Expected == std::string::npos) {
        size_t Length = 0;
        const auto CL = Received.find("Content-Length: ");
        if (CL != std::string::npos && CL < End) {
          Length = static_cast<size_t>(std::atoi(Received.c_str() + CL + 16));
        }
        Expected = End + 4 + Length;
      }
      if (Received.size() >= Expected) {
        break;
      }
    }
    size_t Off = 0;
    while (Off < Reply.size()) {
      const auto N = ::write(Client, Reply.data() + Off, Reply.size() - Off);
      if (N <= 0) {
        break;
      }
      Off += static_cast<size_t>(N);
    }
    ::close(Client);
  }
  int Fd = -1;
  std::atomic<bool> Accepted{false};
  std::thread Thread;
};

TEST_F(WasiP2Http, PostsOverTheLoopbackAndReadsChunks) {
  OneShotServer Server;
  ASSERT_TRUE(Server.start());

  const uint64_t F = newFields();
  ASSERT_TRUE(setField(F, "Content-Type", "text/plain"));
  auto Req =
      call(*Types, "[constructor]outgoing-request",
           {Runtime::Component::Wit<Runtime::Component::Own<Fields>>::into(
               Runtime::Component::Own<Fields>{F})});
  ASSERT_TRUE(Req);
  const uint64_t ReqRep =
      Runtime::Component::Wit<Runtime::Component::Own<OutgoingRequest>>::from(
          (*Req)[0])
          .Rep;
  auto Self = Runtime::Component::Wit<OutgoingRequestBorrow>::into(
      OutgoingRequestBorrow{ReqRep});
  Method Post;
  Post.Case = Method::Post;
  auto SetMethod =
      call(*Types, "[method]outgoing-request.set-method",
           {Self, Runtime::Component::Wit<Method>::into(std::move(Post))});
  ASSERT_TRUE(SetMethod);
  EXPECT_TRUE((Runtime::Component::Wit<
               Expected<Runtime::Component::Unit,
                        Runtime::Component::Unit>>::from((*SetMethod)[0])));
  Scheme Http;
  auto SetScheme =
      call(*Types, "[method]outgoing-request.set-scheme",
           {Self, Runtime::Component::Wit<std::optional<Scheme>>::into(
                      std::optional<Scheme>(Http))});
  ASSERT_TRUE(SetScheme);
  auto SetAuthority =
      call(*Types, "[method]outgoing-request.set-authority",
           {Self, Runtime::Component::Wit<std::optional<std::string>>::into(
                      "127.0.0.1:" + std::to_string(Server.Port))});
  ASSERT_TRUE(SetAuthority);
  EXPECT_TRUE((Runtime::Component::Wit<
               Expected<Runtime::Component::Unit,
                        Runtime::Component::Unit>>::from((*SetAuthority)[0])));
  auto SetPath =
      call(*Types, "[method]outgoing-request.set-path-with-query",
           {Self, Runtime::Component::Wit<std::optional<std::string>>::into(
                      std::string("/echo?x=1"))});
  ASSERT_TRUE(SetPath);
  auto BadAuthority =
      call(*Types, "[method]outgoing-request.set-authority",
           {Self, Runtime::Component::Wit<std::optional<std::string>>::into(
                      std::string("host:port"))});
  ASSERT_TRUE(BadAuthority);
  EXPECT_FALSE((Runtime::Component::Wit<
                Expected<Runtime::Component::Unit,
                         Runtime::Component::Unit>>::from((*BadAuthority)[0])));

  // The body is written after the request is handed to the handler.
  auto Body = call(*Types, "[method]outgoing-request.body", {Self});
  ASSERT_TRUE(Body);
  auto BodyRep = Runtime::Component::Wit<
      Expected<Runtime::Component::Own<OutgoingBody>,
               Runtime::Component::Unit>>::from((*Body)[0]);
  ASSERT_TRUE(BodyRep);
  auto Again = call(*Types, "[method]outgoing-request.body", {Self});
  ASSERT_TRUE(Again);
  EXPECT_FALSE((Runtime::Component::Wit<
                Expected<Runtime::Component::Own<OutgoingBody>,
                         Runtime::Component::Unit>>::from((*Again)[0])));
  auto Handled = call(
      *Handler, "handle",
      {Runtime::Component::Wit<Runtime::Component::Own<OutgoingRequest>>::into(
           Runtime::Component::Own<OutgoingRequest>{ReqRep}),
       Runtime::Component::Wit<std::optional<
           Runtime::Component::Own<RequestOptions>>>::into(std::nullopt)});
  ASSERT_TRUE(Handled);
  auto Future = Runtime::Component::
      Wit<HttpResult<Runtime::Component::Own<FutureIncomingResponse>>>::from(
          (*Handled)[0]);
  ASSERT_TRUE(Future);
  auto Write = call(*Types, "[method]outgoing-body.write",
                    {Runtime::Component::Wit<OutgoingBodyBorrow>::into(
                        OutgoingBodyBorrow{BodyRep->Rep})});
  ASSERT_TRUE(Write);
  auto Out = Runtime::Component::Wit<
      Expected<Runtime::Component::Own<OutputStream>,
               Runtime::Component::Unit>>::from((*Write)[0]);
  ASSERT_TRUE(Out);
  writeAll(Out->Rep, "ping");
  auto Finish = call(
      *Types, "[static]outgoing-body.finish",
      {Runtime::Component::Wit<Runtime::Component::Own<OutgoingBody>>::into(
           Runtime::Component::Own<OutgoingBody>{BodyRep->Rep}),
       Runtime::Component::Wit<std::optional<Runtime::Component::Own<Fields>>>::
           into(std::nullopt)});
  ASSERT_TRUE(Finish);
  EXPECT_TRUE(Runtime::Component::Wit<HttpStatus>::from((*Finish)[0]));

  auto FutureSelf = Runtime::Component::Wit<FutureResponseBorrow>::into(
      FutureResponseBorrow{Future->Rep});
  auto Sub =
      call(*Types, "[method]future-incoming-response.subscribe", {FutureSelf});
  ASSERT_TRUE(Sub);
  auto Block = call(
      *Poll, "[method]pollable.block",
      {Runtime::Component::Wit<PollableBorrow>::into(PollableBorrow{
          Runtime::Component::Wit<Runtime::Component::Own<PollSource>>::from(
              (*Sub)[0])
              .Rep})});
  ASSERT_TRUE(Block);
  auto Got = call(*Types, "[method]future-incoming-response.get", {FutureSelf});
  ASSERT_TRUE(Got);
  auto Result =
      Runtime::Component::Wit<FutureResponseGet::Result>::from((*Got)[0]);
  ASSERT_TRUE(Result);
  ASSERT_TRUE(*Result);
  ASSERT_TRUE(**Result);
  const uint64_t RespRep = (***Result).Rep;
  auto Twice =
      call(*Types, "[method]future-incoming-response.get", {FutureSelf});
  ASSERT_TRUE(Twice);
  auto Second =
      Runtime::Component::Wit<FutureResponseGet::Result>::from((*Twice)[0]);
  ASSERT_TRUE(Second);
  EXPECT_FALSE(*Second);

  Server.join();
  EXPECT_NE(Server.Received.find("POST /echo?x=1 HTTP/1.1\r\n"),
            std::string::npos);
  EXPECT_NE(Server.Received.find("Host: 127.0.0.1:"), std::string::npos);
  EXPECT_NE(Server.Received.find("Content-Type: text/plain\r\n"),
            std::string::npos);
  EXPECT_NE(Server.Received.find("Content-Length: 4\r\n"), std::string::npos);
  EXPECT_EQ(Server.Received.substr(Server.Received.size() - 4), "ping");

  auto RespSelf = Runtime::Component::Wit<IncomingResponseBorrow>::into(
      IncomingResponseBorrow{RespRep});
  auto Status = call(*Types, "[method]incoming-response.status", {RespSelf});
  ASSERT_TRUE(Status);
  EXPECT_EQ(std::get<uint16_t>((*Status)[0]), 200u);
  auto Headers = call(*Types, "[method]incoming-response.headers", {RespSelf});
  ASSERT_TRUE(Headers);
  EXPECT_EQ(
      getField(Runtime::Component::Wit<Runtime::Component::Own<Fields>>::from(
                   (*Headers)[0])
                   .Rep,
               "content-type"),
      (std::vector<std::string>{"text/plain"}));
  auto Consumed = call(*Types, "[method]incoming-response.consume", {RespSelf});
  ASSERT_TRUE(Consumed);
  auto InBody = Runtime::Component::Wit<
      Expected<Runtime::Component::Own<IncomingBody>,
               Runtime::Component::Unit>>::from((*Consumed)[0]);
  ASSERT_TRUE(InBody);
  auto Stream = call(*Types, "[method]incoming-body.stream",
                     {Runtime::Component::Wit<IncomingBodyBorrow>::into(
                         IncomingBodyBorrow{InBody->Rep})});
  ASSERT_TRUE(Stream);
  auto In = Runtime::Component::Wit<
      Expected<Runtime::Component::Own<InputStream>,
               Runtime::Component::Unit>>::from((*Stream)[0]);
  ASSERT_TRUE(In);
  EXPECT_EQ(readAll(In->Rep), "hello world");
  auto Trailers = call(
      *Types, "[static]incoming-body.finish",
      {Runtime::Component::Wit<Runtime::Component::Own<IncomingBody>>::into(
          Runtime::Component::Own<IncomingBody>{InBody->Rep})});
  ASSERT_TRUE(Trailers);
  auto TrailersSelf =
      Runtime::Component::Wit<TrailersBorrow>::into(TrailersBorrow{
          Runtime::Component::Wit<
              Runtime::Component::Own<FutureTrailers>>::from((*Trailers)[0])
              .Rep});
  auto GotTrailers =
      call(*Types, "[method]future-trailers.get", {TrailersSelf});
  ASSERT_TRUE(GotTrailers);
  auto T = Runtime::Component::Wit<FutureTrailersGet::Result>::from(
      (*GotTrailers)[0]);
  ASSERT_TRUE(T);
  ASSERT_TRUE(*T);
  ASSERT_TRUE(**T);
  ASSERT_TRUE(***T);
  EXPECT_EQ(getField((***T)->Rep, "x-trailer"),
            (std::vector<std::string>{"yes"}));
}

TEST_F(WasiP2Http, ReportsRefusedAndUnsupported) {
  auto Req =
      call(*Types, "[constructor]outgoing-request",
           {Runtime::Component::Wit<Runtime::Component::Own<Fields>>::into(
               Runtime::Component::Own<Fields>{newFields()})});
  ASSERT_TRUE(Req);
  const uint64_t ReqRep =
      Runtime::Component::Wit<Runtime::Component::Own<OutgoingRequest>>::from(
          (*Req)[0])
          .Rep;
  auto Self = Runtime::Component::Wit<OutgoingRequestBorrow>::into(
      OutgoingRequestBorrow{ReqRep});
  ASSERT_TRUE(
      call(*Types, "[method]outgoing-request.set-authority",
           {Self, Runtime::Component::Wit<std::optional<std::string>>::into(
                      std::string("127.0.0.1:1"))}));
  auto Handled = call(
      *Handler, "handle",
      {Runtime::Component::Wit<Runtime::Component::Own<OutgoingRequest>>::into(
           Runtime::Component::Own<OutgoingRequest>{ReqRep}),
       Runtime::Component::Wit<std::optional<
           Runtime::Component::Own<RequestOptions>>>::into(std::nullopt)});
  ASSERT_TRUE(Handled);
  auto Future = Runtime::Component::
      Wit<HttpResult<Runtime::Component::Own<FutureIncomingResponse>>>::from(
          (*Handled)[0]);
  ASSERT_TRUE(Future);
  auto Got = call(*Types, "[method]future-incoming-response.get",
                  {Runtime::Component::Wit<FutureResponseBorrow>::into(
                      FutureResponseBorrow{Future->Rep})});
  ASSERT_TRUE(Got);
  auto Result =
      Runtime::Component::Wit<FutureResponseGet::Result>::from((*Got)[0]);
  ASSERT_TRUE(Result);
  ASSERT_TRUE(*Result);
  ASSERT_FALSE(**Result);
  EXPECT_EQ((**Result).error().Case, HttpError::ConnectionRefused);

  auto Req2 =
      call(*Types, "[constructor]outgoing-request",
           {Runtime::Component::Wit<Runtime::Component::Own<Fields>>::into(
               Runtime::Component::Own<Fields>{newFields()})});
  ASSERT_TRUE(Req2);
  const uint64_t Rep2 =
      Runtime::Component::Wit<Runtime::Component::Own<OutgoingRequest>>::from(
          (*Req2)[0])
          .Rep;
  auto Self2 = Runtime::Component::Wit<OutgoingRequestBorrow>::into(
      OutgoingRequestBorrow{Rep2});
  Scheme Https;
  Https.Case = Scheme::Https;
  ASSERT_TRUE(call(*Types, "[method]outgoing-request.set-scheme",
                   {Self2, Runtime::Component::Wit<std::optional<Scheme>>::into(
                               std::optional<Scheme>(Https))}));
  ASSERT_TRUE(
      call(*Types, "[method]outgoing-request.set-authority",
           {Self2, Runtime::Component::Wit<std::optional<std::string>>::into(
                       std::string("example.com"))}));
  auto Handled2 = call(
      *Handler, "handle",
      {Runtime::Component::Wit<Runtime::Component::Own<OutgoingRequest>>::into(
           Runtime::Component::Own<OutgoingRequest>{Rep2}),
       Runtime::Component::Wit<std::optional<
           Runtime::Component::Own<RequestOptions>>>::into(std::nullopt)});
  ASSERT_TRUE(Handled2);
  auto Future2 = Runtime::Component::
      Wit<HttpResult<Runtime::Component::Own<FutureIncomingResponse>>>::from(
          (*Handled2)[0]);
  ASSERT_FALSE(Future2);
  EXPECT_EQ(Future2.error().Case, HttpError::InternalError);
  ASSERT_TRUE(Future2.error().Text);
  EXPECT_EQ(*Future2.error().Text, "TLS is not supported");
}
#endif

TEST_F(WasiP2Http, IncomingSideServesAHandler) {
  // The host puts a request in front of a handler and reads its response.
  auto R = std::make_shared<IncomingRequest>();
  R->M.Case = Method::Put;
  R->PathWithQuery = "/items/1";
  R->Authority = "localhost";
  Scheme S;
  R->S = S;
  R->Headers = std::make_shared<Fields>();
  R->Headers->Entries.emplace_back("content-type", bytes("text/plain"));
  R->Body = std::make_shared<IncomingBody>();
  R->Body->Data = bytes("payload");
  const uint64_t ReqRep = Http->IncomingRequests.add(R);
  auto Param = std::make_shared<ResponseOutparam>();
  const uint64_t ParamRep = Http->Outparams.add(Param);

  auto Self = Runtime::Component::Wit<IncomingRequestBorrow>::into(
      IncomingRequestBorrow{ReqRep});
  auto M = call(*Types, "[method]incoming-request.method", {Self});
  ASSERT_TRUE(M);
  EXPECT_EQ(Runtime::Component::Wit<Method>::from((*M)[0]).Case, Method::Put);
  auto Path = call(*Types, "[method]incoming-request.path-with-query", {Self});
  ASSERT_TRUE(Path);
  EXPECT_EQ(
      Runtime::Component::Wit<std::optional<std::string>>::from((*Path)[0]),
      "/items/1");
  auto Auth = call(*Types, "[method]incoming-request.authority", {Self});
  ASSERT_TRUE(Auth);
  EXPECT_EQ(
      Runtime::Component::Wit<std::optional<std::string>>::from((*Auth)[0]),
      "localhost");
  auto Sch = call(*Types, "[method]incoming-request.scheme", {Self});
  ASSERT_TRUE(Sch);
  ASSERT_TRUE(Runtime::Component::Wit<std::optional<Scheme>>::from((*Sch)[0]));
  auto Headers = call(*Types, "[method]incoming-request.headers", {Self});
  ASSERT_TRUE(Headers);
  EXPECT_EQ(
      getField(Runtime::Component::Wit<Runtime::Component::Own<Fields>>::from(
                   (*Headers)[0])
                   .Rep,
               "Content-Type"),
      (std::vector<std::string>{"text/plain"}));
  auto Consumed = call(*Types, "[method]incoming-request.consume", {Self});
  ASSERT_TRUE(Consumed);
  auto InBody = Runtime::Component::Wit<
      Expected<Runtime::Component::Own<IncomingBody>,
               Runtime::Component::Unit>>::from((*Consumed)[0]);
  ASSERT_TRUE(InBody);
  auto Twice = call(*Types, "[method]incoming-request.consume", {Self});
  ASSERT_TRUE(Twice);
  EXPECT_FALSE((Runtime::Component::Wit<
                Expected<Runtime::Component::Own<IncomingBody>,
                         Runtime::Component::Unit>>::from((*Twice)[0])));
  auto Stream = call(*Types, "[method]incoming-body.stream",
                     {Runtime::Component::Wit<IncomingBodyBorrow>::into(
                         IncomingBodyBorrow{InBody->Rep})});
  ASSERT_TRUE(Stream);
  auto In = Runtime::Component::Wit<
      Expected<Runtime::Component::Own<InputStream>,
               Runtime::Component::Unit>>::from((*Stream)[0]);
  ASSERT_TRUE(In);
  EXPECT_EQ(readAll(In->Rep), "payload");

  const uint64_t F = newFields();
  ASSERT_TRUE(setField(F, "X-Answer", "42"));
  auto Resp =
      call(*Types, "[constructor]outgoing-response",
           {Runtime::Component::Wit<Runtime::Component::Own<Fields>>::into(
               Runtime::Component::Own<Fields>{F})});
  ASSERT_TRUE(Resp);
  const uint64_t RespRep =
      Runtime::Component::Wit<Runtime::Component::Own<OutgoingResponse>>::from(
          (*Resp)[0])
          .Rep;
  auto RespSelf = Runtime::Component::Wit<OutgoingResponseBorrow>::into(
      OutgoingResponseBorrow{RespRep});
  auto SetStatus = call(*Types, "[method]outgoing-response.set-status-code",
                        {RespSelf, ComponentValVariant{uint16_t(201)}});
  ASSERT_TRUE(SetStatus);
  EXPECT_TRUE((Runtime::Component::Wit<
               Expected<Runtime::Component::Unit,
                        Runtime::Component::Unit>>::from((*SetStatus)[0])));
  auto Body = call(*Types, "[method]outgoing-response.body", {RespSelf});
  ASSERT_TRUE(Body);
  auto OutBody = Runtime::Component::Wit<
      Expected<Runtime::Component::Own<OutgoingBody>,
               Runtime::Component::Unit>>::from((*Body)[0]);
  ASSERT_TRUE(OutBody);
  auto Write = call(*Types, "[method]outgoing-body.write",
                    {Runtime::Component::Wit<OutgoingBodyBorrow>::into(
                        OutgoingBodyBorrow{OutBody->Rep})});
  ASSERT_TRUE(Write);
  auto Out = Runtime::Component::Wit<
      Expected<Runtime::Component::Own<OutputStream>,
               Runtime::Component::Unit>>::from((*Write)[0]);
  ASSERT_TRUE(Out);
  writeAll(Out->Rep, "pong");
  const uint64_t T = newFields();
  ASSERT_TRUE(setField(T, "X-Checksum", "abc"));
  auto Finish = call(
      *Types, "[static]outgoing-body.finish",
      {Runtime::Component::Wit<Runtime::Component::Own<OutgoingBody>>::into(
           Runtime::Component::Own<OutgoingBody>{OutBody->Rep}),
       Runtime::Component::Wit<std::optional<Runtime::Component::Own<Fields>>>::
           into(std::optional<Runtime::Component::Own<Fields>>(
               Runtime::Component::Own<Fields>{T}))});
  ASSERT_TRUE(Finish);
  EXPECT_TRUE(Runtime::Component::Wit<HttpStatus>::from((*Finish)[0]));
  auto Set = call(
      *Types, "[static]response-outparam.set",
      {Runtime::Component::Wit<Runtime::Component::Own<ResponseOutparam>>::into(
           Runtime::Component::Own<ResponseOutparam>{ParamRep}),
       Runtime::Component::
           Wit<HttpResult<Runtime::Component::Own<OutgoingResponse>>>::into(
               HttpResult<Runtime::Component::Own<OutgoingResponse>>(
                   Runtime::Component::Own<OutgoingResponse>{RespRep}))});
  ASSERT_TRUE(Set);
  ASSERT_TRUE(Param->Response);
  ASSERT_TRUE(*Param->Response);
  const auto &Response = ***Param->Response;
  EXPECT_EQ(Response.Status, 201u);
  EXPECT_EQ(Response.Headers->first("x-answer"), "42");
  ASSERT_TRUE(Response.Body);
  EXPECT_EQ(text(Response.Body->Data), "pong");
  EXPECT_TRUE(Response.Body->Finished);
  ASSERT_TRUE(Response.Body->Trailers);
  EXPECT_EQ(Response.Body->Trailers->first("x-checksum"), "abc");
  EXPECT_EQ(Http->Outparams.size(), 0u);
}

} // namespace
