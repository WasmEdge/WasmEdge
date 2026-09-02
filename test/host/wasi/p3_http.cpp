// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- test/host/wasi/p3_http.cpp - wasi:http 0.3.1 host -----------------===//

#include "common/component_variant.h"
#include "host/wasi/p3/http.h"
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
using namespace Host::WasiP3;
using HostTest::call;

std::vector<uint8_t> bytes(std::string_view S) {
  return std::vector<uint8_t>(S.begin(), S.end());
}

class WasiP3Http : public testing::Test {
protected:
  void SetUp() override {
    Http = std::make_unique<HttpHost>();
    Insts = makeHttpInstances(*Http);
    Types = Insts[0].get();
    Client = Insts[1].get();
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

  std::unique_ptr<HttpHost> Http;
  std::vector<std::unique_ptr<Runtime::Instance::ComponentInstance>> Insts;
  Runtime::Instance::ComponentInstance *Types = nullptr;
  Runtime::Instance::ComponentInstance *Client = nullptr;
};

TEST_F(WasiP3Http, InstancesAndTypes) {
  ASSERT_EQ(Insts.size(), 2u);
  EXPECT_EQ(Types->getComponentName(), "wasi:http/types@0.3.1");
  EXPECT_EQ(Client->getComponentName(), "wasi:http/client@0.3.1");
  EXPECT_EQ(Types->findTypeResource("headers"),
            Types->findTypeResource("fields"));
  EXPECT_EQ(Client->findTypeResource("request"), Http->RequestType);
  EXPECT_EQ(Client->findTypeResource("response"), Http->ResponseType);
  const auto *Err = Types->findType("header-error");
  ASSERT_NE(Err, nullptr);
  ASSERT_TRUE(Err->getDefValType().isVariantTy());
  const auto &Cases = Err->getDefValType().getVariant().Cases;
  ASSERT_EQ(Cases.size(), 5u);
  EXPECT_EQ(Cases[3].first, "size-exceeded");
  EXPECT_EQ(Cases[4].first, "other");
  const auto *Send = Client->findFunction("send");
  ASSERT_NE(Send, nullptr);
  EXPECT_TRUE(Send->getFuncType().isAsync());
  EXPECT_NE(Types->findFunction("[static]request.consume-body"), nullptr);
  EXPECT_NE(Types->findFunction("[method]fields.get-and-delete"), nullptr);
}

TEST_F(WasiP3Http, FieldsAndOptions) {
  const uint64_t F = newFields();
  ASSERT_TRUE(setField(F, "Accept", "text/plain"));
  auto Forbidden = setField(F, "Transfer-Encoding", "chunked");
  ASSERT_FALSE(Forbidden);
  EXPECT_EQ(Forbidden.error().Case, HeaderError::Forbidden);
  auto Taken =
      call(*Types, "[method]fields.get-and-delete", {fields(F), str("accept")});
  ASSERT_TRUE(Taken);
  auto Values =
      Runtime::Component::Wit<Expected<std::vector<std::vector<uint8_t>>,
                                       HeaderError>>::from((*Taken)[0]);
  ASSERT_TRUE(Values);
  ASSERT_EQ(Values->size(), 1u);
  EXPECT_EQ((*Values)[0], bytes("text/plain"));
  auto All = call(*Types, "[method]fields.copy-all", {fields(F)});
  ASSERT_TRUE(All);
  EXPECT_TRUE(Runtime::Component::Wit<std::vector<FieldEntry>>::from((*All)[0])
                  .empty());

  auto Opt = call(*Types, "[constructor]request-options", {});
  ASSERT_TRUE(Opt);
  const uint64_t O =
      Runtime::Component::Wit<Runtime::Component::Own<RequestOptions>>::from(
          (*Opt)[0])
          .Rep;
  auto OptSelf = Runtime::Component::Wit<OptionsBorrow>::into(OptionsBorrow{O});
  auto Set =
      call(*Types, "[method]request-options.set-connect-timeout",
           {OptSelf, Runtime::Component::Wit<std::optional<uint64_t>>::into(
                         std::optional<uint64_t>(5000))});
  ASSERT_TRUE(Set);
  EXPECT_TRUE(Runtime::Component::Wit<OptionsStatus>::from((*Set)[0]));
  auto Get =
      call(*Types, "[method]request-options.get-connect-timeout", {OptSelf});
  ASSERT_TRUE(Get);
  EXPECT_EQ(Runtime::Component::Wit<std::optional<uint64_t>>::from((*Get)[0]),
            5000u);
  auto Clone = call(*Types, "[method]request-options.clone", {OptSelf});
  ASSERT_TRUE(Clone);
  const uint64_t C =
      Runtime::Component::Wit<Runtime::Component::Own<RequestOptions>>::from(
          (*Clone)[0])
          .Rep;
  EXPECT_NE(C, O);
  auto Get2 =
      call(*Types, "[method]request-options.get-connect-timeout",
           {Runtime::Component::Wit<OptionsBorrow>::into(OptionsBorrow{C})});
  ASSERT_TRUE(Get2);
  EXPECT_EQ(Runtime::Component::Wit<std::optional<uint64_t>>::from((*Get2)[0]),
            5000u);

  // Options attached to a request are immutable.
  Http->Options.get(O)->Immutable = true;
  auto Frozen =
      call(*Types, "[method]request-options.set-first-byte-timeout",
           {OptSelf, Runtime::Component::Wit<std::optional<uint64_t>>::into(
                         std::optional<uint64_t>(1))});
  ASSERT_TRUE(Frozen);
  auto Status = Runtime::Component::Wit<OptionsStatus>::from((*Frozen)[0]);
  ASSERT_FALSE(Status);
  EXPECT_EQ(Status.error().Case, RequestOptionsError::Immutable);
}

TEST_F(WasiP3Http, RequestAndResponseFields) {
  auto R = std::make_shared<Request>();
  R->Headers = std::make_shared<Fields>();
  R->Headers->Entries.emplace_back("x-a", bytes("1"));
  R->Options = std::make_shared<RequestOptions>();
  R->Options->ConnectTimeout = 7;
  R->Options->Immutable = true;
  const uint64_t Rep = Http->Requests.add(R);
  auto Self = Runtime::Component::Wit<RequestBorrow>::into(RequestBorrow{Rep});
  Method Put;
  Put.Case = Method::Put;
  auto SetM =
      call(*Types, "[method]request.set-method",
           {Self, Runtime::Component::Wit<Method>::into(std::move(Put))});
  ASSERT_TRUE(SetM);
  auto GetM = call(*Types, "[method]request.get-method", {Self});
  ASSERT_TRUE(GetM);
  EXPECT_EQ(Runtime::Component::Wit<Method>::from((*GetM)[0]).Case,
            Method::Put);
  Method Bad;
  Bad.Case = Method::Other;
  Bad.Name = "no space";
  auto SetBad =
      call(*Types, "[method]request.set-method",
           {Self, Runtime::Component::Wit<Method>::into(std::move(Bad))});
  ASSERT_TRUE(SetBad);
  EXPECT_FALSE((Runtime::Component::Wit<
                Expected<Runtime::Component::Unit,
                         Runtime::Component::Unit>>::from((*SetBad)[0])));
  auto SetA =
      call(*Types, "[method]request.set-authority",
           {Self, Runtime::Component::Wit<std::optional<std::string>>::into(
                      std::string("example.com:8080"))});
  ASSERT_TRUE(SetA);
  auto GetA = call(*Types, "[method]request.get-authority", {Self});
  ASSERT_TRUE(GetA);
  EXPECT_EQ(
      Runtime::Component::Wit<std::optional<std::string>>::from((*GetA)[0]),
      "example.com:8080");
  Scheme Https;
  Https.Case = Scheme::Https;
  auto SetS = call(*Types, "[method]request.set-scheme",
                   {Self, Runtime::Component::Wit<std::optional<Scheme>>::into(
                              std::optional<Scheme>(Https))});
  ASSERT_TRUE(SetS);
  auto GetS = call(*Types, "[method]request.get-scheme", {Self});
  ASSERT_TRUE(GetS);
  auto S = Runtime::Component::Wit<std::optional<Scheme>>::from((*GetS)[0]);
  ASSERT_TRUE(S);
  EXPECT_EQ(S->Case, Scheme::Https);
  auto Opts = call(*Types, "[method]request.get-options", {Self});
  ASSERT_TRUE(Opts);
  auto O = Runtime::Component::Wit<
      std::optional<Runtime::Component::Own<RequestOptions>>>::from((*Opts)[0]);
  ASSERT_TRUE(O);
  auto Timeout = call(
      *Types, "[method]request-options.get-connect-timeout",
      {Runtime::Component::Wit<OptionsBorrow>::into(OptionsBorrow{O->Rep})});
  ASSERT_TRUE(Timeout);
  EXPECT_EQ(
      Runtime::Component::Wit<std::optional<uint64_t>>::from((*Timeout)[0]),
      7u);
  auto H = call(*Types, "[method]request.get-headers", {Self});
  ASSERT_TRUE(H);
  const uint64_t HRep =
      Runtime::Component::Wit<Runtime::Component::Own<Fields>>::from((*H)[0])
          .Rep;
  auto Frozen = setField(HRep, "x-b", "2");
  ASSERT_FALSE(Frozen);
  EXPECT_EQ(Frozen.error().Case, HeaderError::Immutable);

  auto Resp = std::make_shared<Response>();
  Resp->Headers = std::make_shared<Fields>();
  const uint64_t RespRep = Http->Responses.add(Resp);
  auto RespSelf =
      Runtime::Component::Wit<ResponseBorrow>::into(ResponseBorrow{RespRep});
  auto SetStatus = call(*Types, "[method]response.set-status-code",
                        {RespSelf, ComponentValVariant{uint16_t(404)}});
  ASSERT_TRUE(SetStatus);
  auto GetStatus = call(*Types, "[method]response.get-status-code", {RespSelf});
  ASSERT_TRUE(GetStatus);
  EXPECT_EQ(std::get<uint16_t>((*GetStatus)[0]), 404u);
  auto BadStatus = call(*Types, "[method]response.set-status-code",
                        {RespSelf, ComponentValVariant{uint16_t(42)}});
  ASSERT_TRUE(BadStatus);
  EXPECT_FALSE((Runtime::Component::Wit<
                Expected<Runtime::Component::Unit,
                         Runtime::Component::Unit>>::from((*BadStatus)[0])));
}

} // namespace
