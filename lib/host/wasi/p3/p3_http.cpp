// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "common/component_valtype.h"
#include "common/component_variant.h"
#include "common/spdlog.h"
#include "host/wasi/component/node.h"
#include "host/wasi/component/result.h"
#include "host/wasi/p3/future.h"
#include "host/wasi/p3/http.h"

#include "executor/component/executor.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <memory>
#include <utility>

namespace WasmEdge {
namespace Host {
namespace WasiP3 {

namespace {

HeaderStatus headerFail(HeaderError::Kind K) noexcept {
  HeaderError E;
  E.Case = K;
  return Unexpected<HeaderError>(std::move(E));
}

bool validText(std::string_view Text) noexcept {
  for (char C : Text) {
    const auto U = static_cast<unsigned char>(C);
    if (U <= 0x20 || U >= 0x7F) {
      return false;
    }
  }
  return true;
}

} // namespace

Body HttpHost::newBody(
    Runtime::Component::CallingFrame &Frame,
    std::optional<Runtime::Component::Stream<uint8_t>> &Contents,
    Runtime::Component::Future<TrailersResult> &Trailers,
    Runtime::Component::Future<HttpStatus> &Done) {
  Body NewBody;
  if (Contents) {
    NewBody.Contents = Contents->HandleIdx;
  }
  NewBody.TrailersEnd = Trailers.HandleIdx;
  const auto DoneEnds =
      Frame.getExecutor().newHostStream(Frame, true, StatusType);
  NewBody.Done = DoneEnds.first;
  Done = Runtime::Component::Future<HttpStatus>{DoneEnds.second};
  return NewBody;
}

Expect<BodyEnds>
HttpHost::consume(Runtime::Component::CallingFrame &Frame, Body *Content,
                  Runtime::Component::Future<HttpStatus> &Res) {
  // A consumer of a host body reports to nobody: its result future goes.
  auto DropResult = [&Frame, &Res]() {
    Frame.getExecutor().dropStreamHandle(Frame, Res.HandleIdx);
  };
  if (!Content || Content->Consumed) {
    DropResult();
    auto Data = Frame.getExecutor().newHostStream(
        Frame, false, ComponentValType(ComponentTypeCode::U8));
    auto Trailers =
        Frame.getExecutor().newHostStream(Frame, true, TrailersType);
    BodyEnds Ends{Runtime::Component::Stream<uint8_t>{Data.second},
                  Runtime::Component::Future<TrailersResult>{Trailers.second}};
    Frame.getExecutor().dropStreamHandle(Frame, Data.first);
    resolveLater<TrailersResult>(
        Frame, Trailers.first,
        Unexpected<HttpError>(HttpError::internal("consumed")));
    return Ends;
  }
  Content->Consumed = true;
  if (!Content->FromHost) {
    // The result the consumer reports resolves the producer's future.
    if (Content->Done == 0) {
      DropResult();
    } else {
      const uint32_t Outcome = Res.HandleIdx;
      auto Done = Content->Done;
      Frame.spawn([Outcome, Done](Runtime::Component::CallingFrame &Spawned)
                      -> Expect<void> {
        std::vector<ComponentValVariant> Vals;
        auto Read = Spawned.getExecutor().readStream(Spawned, Outcome, Vals, 1);
        Spawned.getExecutor().dropStreamHandle(Spawned, Outcome);
        EXPECTED_TRY(Read);
        if (Vals.empty()) {
          Spawned.getExecutor().dropStreamHandle(Spawned, Done);
          return {};
        }
        Spawned.getExecutor().spawnStreamWrite(Spawned, Done,
                                               std::move(Vals[0]));
        return {};
      });
    }
    Runtime::Component::Stream<uint8_t> Data;
    if (Content->Contents != 0) {
      Data.HandleIdx = Content->Contents;
    } else {
      auto Empty = Frame.getExecutor().newHostStream(
          Frame, false, ComponentValType(ComponentTypeCode::U8));
      Data.HandleIdx = Empty.second;
      Frame.getExecutor().dropStreamHandle(Frame, Empty.first);
    }
    return BodyEnds{std::move(Data), Runtime::Component::Future<TrailersResult>{
                                         Content->TrailersEnd}};
  }
  DropResult();
  auto Data = Frame.getExecutor().newHostStream(
      Frame, false, ComponentValType(ComponentTypeCode::U8));
  auto Trailers = Frame.getExecutor().newHostStream(Frame, true, TrailersType);
  BodyEnds Ends{Runtime::Component::Stream<uint8_t>{Data.second},
                Runtime::Component::Future<TrailersResult>{Trailers.second}};
  TrailersResult Value{std::optional<Runtime::Component::Own<Fields>>{}};
  if (Content->Trailers) {
    Value = std::optional<Runtime::Component::Own<Fields>>{
        Runtime::Component::Own<Fields>{FieldsTable.add(
            std::make_unique<Fields>(std::move(*Content->Trailers)))}};
    Content->Trailers.reset();
  }
  Frame.spawn(
      [Bytes = std::move(Content->Data), Data, Trailers,
       Payload =
           Runtime::Component::Wit<TrailersResult>::into(std::move(Value))](
          Runtime::Component::CallingFrame &Spawned) mutable -> Expect<void> {
        if (!Bytes.empty()) {
          EXPECTED_TRY(
              Spawned.getExecutor().writeStream(Spawned, Data.first, Bytes));
        }
        Spawned.getExecutor().dropStreamHandle(Spawned, Data.first);
        Spawned.getExecutor().spawnStreamWrite(Spawned, Trailers.first,
                                               std::move(Payload));
        return {};
      });
  return Ends;
}

Expect<Runtime::Component::Own<Fields>>
FieldsNew::body(Runtime::Component::CallingFrame &) {
  return Runtime::Component::Own<Fields>{
      Http.FieldsTable.add(std::make_unique<Fields>())};
}

Expect<Expected<Runtime::Component::Own<Fields>, HeaderError>>
FieldsFromList::body(Runtime::Component::CallingFrame &,
                     std::vector<FieldEntry> Entries) {
  auto F = std::make_unique<Fields>();
  for (auto &[Name, Value] : Entries) {
    if (!Fields::isValidName(Name) || !Fields::isValidValue(Value)) {
      return Expected<Runtime::Component::Own<Fields>, HeaderError>(
          Unexpected<HeaderError>(
              headerFail(HeaderError::InvalidSyntax).error()));
    }
    if (Fields::isForbiddenName(Name)) {
      return Expected<Runtime::Component::Own<Fields>, HeaderError>(
          Unexpected<HeaderError>(headerFail(HeaderError::Forbidden).error()));
    }
    F->Entries.emplace_back(std::move(Name), std::move(Value));
  }
  return Expected<Runtime::Component::Own<Fields>, HeaderError>(
      Runtime::Component::Own<Fields>{Http.FieldsTable.add(std::move(F))});
}

Expect<std::vector<std::vector<uint8_t>>>
FieldsGet::body(Runtime::Component::CallingFrame &, FieldsBorrow Self,
                std::string Name) {
  auto F = Http.FieldsTable.get(Self.Rep);
  if (!F) {
    return std::vector<std::vector<uint8_t>>{};
  }
  return F->get(Name);
}

Expect<bool> FieldsHas::body(Runtime::Component::CallingFrame &,
                             FieldsBorrow Self, std::string Name) {
  auto F = Http.FieldsTable.get(Self.Rep);
  return F && F->has(Name);
}

Expect<HeaderStatus> FieldsSet::body(Runtime::Component::CallingFrame &,
                                     FieldsBorrow Self, std::string Name,
                                     std::vector<std::vector<uint8_t>> Values) {
  auto F = Http.FieldsTable.get(Self.Rep);
  if (!F || F->Immutable) {
    return headerFail(HeaderError::Immutable);
  }
  if (!Fields::isValidName(Name)) {
    return headerFail(HeaderError::InvalidSyntax);
  }
  for (const auto &V : Values) {
    if (!Fields::isValidValue(V)) {
      return headerFail(HeaderError::InvalidSyntax);
    }
  }
  if (Fields::isForbiddenName(Name)) {
    return headerFail(HeaderError::Forbidden);
  }
  F->Entries.erase(std::remove_if(F->Entries.begin(), F->Entries.end(),
                                  [&Name](const auto &E) {
                                    return Fields::isSameName(E.first, Name);
                                  }),
                   F->Entries.end());
  for (auto &V : Values) {
    F->Entries.emplace_back(Name, std::move(V));
  }
  return HeaderStatus(Runtime::Component::WitUnit{});
}

Expect<HeaderStatus> FieldsDelete::body(Runtime::Component::CallingFrame &,
                                        FieldsBorrow Self, std::string Name) {
  auto F = Http.FieldsTable.get(Self.Rep);
  if (!F || F->Immutable) {
    return headerFail(HeaderError::Immutable);
  }
  if (!Fields::isValidName(Name)) {
    return headerFail(HeaderError::InvalidSyntax);
  }
  if (Fields::isForbiddenName(Name)) {
    return headerFail(HeaderError::Forbidden);
  }
  F->Entries.erase(std::remove_if(F->Entries.begin(), F->Entries.end(),
                                  [&Name](const auto &E) {
                                    return Fields::isSameName(E.first, Name);
                                  }),
                   F->Entries.end());
  return HeaderStatus(Runtime::Component::WitUnit{});
}

Expect<Expected<std::vector<std::vector<uint8_t>>, HeaderError>>
FieldsGetAndDelete::body(Runtime::Component::CallingFrame &, FieldsBorrow Self,
                         std::string Name) {
  auto F = Http.FieldsTable.get(Self.Rep);
  if (!F || F->Immutable) {
    return Expected<std::vector<std::vector<uint8_t>>, HeaderError>(
        Unexpected<HeaderError>(headerFail(HeaderError::Immutable).error()));
  }
  if (!Fields::isValidName(Name)) {
    return Expected<std::vector<std::vector<uint8_t>>, HeaderError>(
        Unexpected<HeaderError>(
            headerFail(HeaderError::InvalidSyntax).error()));
  }
  if (Fields::isForbiddenName(Name)) {
    return Expected<std::vector<std::vector<uint8_t>>, HeaderError>(
        Unexpected<HeaderError>(headerFail(HeaderError::Forbidden).error()));
  }
  auto Values = F->get(Name);
  F->Entries.erase(std::remove_if(F->Entries.begin(), F->Entries.end(),
                                  [&Name](const auto &E) {
                                    return Fields::isSameName(E.first, Name);
                                  }),
                   F->Entries.end());
  return Expected<std::vector<std::vector<uint8_t>>, HeaderError>(
      std::move(Values));
}

Expect<HeaderStatus> FieldsAppend::body(Runtime::Component::CallingFrame &,
                                        FieldsBorrow Self, std::string Name,
                                        std::vector<uint8_t> Value) {
  auto F = Http.FieldsTable.get(Self.Rep);
  if (!F || F->Immutable) {
    return headerFail(HeaderError::Immutable);
  }
  if (!Fields::isValidName(Name) || !Fields::isValidValue(Value)) {
    return headerFail(HeaderError::InvalidSyntax);
  }
  if (Fields::isForbiddenName(Name)) {
    return headerFail(HeaderError::Forbidden);
  }
  F->Entries.emplace_back(std::move(Name), std::move(Value));
  return HeaderStatus(Runtime::Component::WitUnit{});
}

Expect<std::vector<FieldEntry>>
FieldsCopyAll::body(Runtime::Component::CallingFrame &, FieldsBorrow Self) {
  std::vector<FieldEntry> Out;
  if (auto F = Http.FieldsTable.get(Self.Rep)) {
    for (const auto &[Name, Value] : F->Entries) {
      Out.emplace_back(Name, Value);
    }
  }
  return Out;
}

Expect<Runtime::Component::Own<Fields>>
FieldsClone::body(Runtime::Component::CallingFrame &, FieldsBorrow Self) {
  auto Copy = std::make_unique<Fields>();
  if (auto F = Http.FieldsTable.get(Self.Rep)) {
    Copy->Entries = F->Entries;
  }
  return Runtime::Component::Own<Fields>{Http.FieldsTable.add(std::move(Copy))};
}

Expect<std::tuple<Runtime::Component::Own<Request>,
                  Runtime::Component::Future<HttpStatus>>>
RequestNew::body(
    Runtime::Component::CallingFrame &Frame,
    Runtime::Component::Own<Fields> Headers,
    std::optional<Runtime::Component::Stream<uint8_t>> Contents,
    Runtime::Component::Future<TrailersResult> Trailers,
    std::optional<Runtime::Component::Own<RequestOptions>> Options) {
  auto R = std::make_unique<Request>();
  if (auto Taken = Http.FieldsTable.remove(Headers.Rep)) {
    R->Headers = std::move(*Taken);
  }
  R->Headers.Immutable = true;
  if (Options) {
    if (auto Taken = Http.Options.remove(Options->Rep)) {
      R->Options = *Taken;
      R->Options->Immutable = true;
    }
  }
  Runtime::Component::Future<HttpStatus> Done;
  R->Content = Http.newBody(Frame, Contents, Trailers, Done);
  return std::tuple<Runtime::Component::Own<Request>,
                    Runtime::Component::Future<HttpStatus>>{
      Runtime::Component::Own<Request>{Http.Requests.add(std::move(R))},
      std::move(Done)};
}

Expect<Method> RequestGetMethod::body(Runtime::Component::CallingFrame &,
                                      RequestBorrow Self) {
  auto R = Http.Requests.get(Self.Rep);
  return R ? R->ReqMethod : Method{};
}

Expect<Expected<Runtime::Component::WitUnit, Runtime::Component::WitUnit>>
RequestSetMethod::body(Runtime::Component::CallingFrame &, RequestBorrow Self,
                       Method NewMethod) {
  auto R = Http.Requests.get(Self.Rep);
  if (!R || (NewMethod.Case == Method::Other &&
             !Fields::isValidName(NewMethod.Name))) {
    return WasiComponent::failUnit<Runtime::Component::WitUnit>();
  }
  R->ReqMethod = std::move(NewMethod);
  return Expected<Runtime::Component::WitUnit, Runtime::Component::WitUnit>(
      Runtime::Component::WitUnit{});
}

Expect<std::optional<std::string>>
RequestGetText::body(Runtime::Component::CallingFrame &, RequestBorrow Self) {
  auto R = Http.Requests.get(Self.Rep);
  if (!R) {
    return std::optional<std::string>{};
  }
  return Authority ? R->Authority : R->PathWithQuery;
}

Expect<Expected<Runtime::Component::WitUnit, Runtime::Component::WitUnit>>
RequestSetText::body(Runtime::Component::CallingFrame &, RequestBorrow Self,
                     std::optional<std::string> Value) {
  auto R = Http.Requests.get(Self.Rep);
  if (!R || (Value && !validText(*Value))) {
    return WasiComponent::failUnit<Runtime::Component::WitUnit>();
  }
  (Authority ? R->Authority : R->PathWithQuery) = std::move(Value);
  return Expected<Runtime::Component::WitUnit, Runtime::Component::WitUnit>(
      Runtime::Component::WitUnit{});
}

Expect<std::optional<Scheme>>
RequestGetScheme::body(Runtime::Component::CallingFrame &, RequestBorrow Self) {
  auto R = Http.Requests.get(Self.Rep);
  return R ? R->ReqScheme : std::optional<Scheme>{};
}

Expect<Expected<Runtime::Component::WitUnit, Runtime::Component::WitUnit>>
RequestSetScheme::body(Runtime::Component::CallingFrame &, RequestBorrow Self,
                       std::optional<Scheme> NewScheme) {
  auto R = Http.Requests.get(Self.Rep);
  if (!R) {
    return WasiComponent::failUnit<Runtime::Component::WitUnit>();
  }
  if (NewScheme && NewScheme->Case == Scheme::Other &&
      !Fields::isValidName(NewScheme->Name)) {
    return WasiComponent::failUnit<Runtime::Component::WitUnit>();
  }
  R->ReqScheme = std::move(NewScheme);
  return Expected<Runtime::Component::WitUnit, Runtime::Component::WitUnit>(
      Runtime::Component::WitUnit{});
}

Expect<std::optional<Runtime::Component::Own<RequestOptions>>>
RequestGetOptions::body(Runtime::Component::CallingFrame &,
                        RequestBorrow Self) {
  auto R = Http.Requests.get(Self.Rep);
  if (!R || !R->Options) {
    return std::optional<Runtime::Component::Own<RequestOptions>>{};
  }
  return std::optional<Runtime::Component::Own<RequestOptions>>{
      Runtime::Component::Own<RequestOptions>{
          Http.Options.add(std::make_unique<RequestOptions>(*R->Options))}};
}

Expect<Runtime::Component::Own<Fields>>
RequestGetHeaders::body(Runtime::Component::CallingFrame &,
                        RequestBorrow Self) {
  auto R = Http.Requests.get(Self.Rep);
  return Runtime::Component::Own<Fields>{
      Http.FieldsTable.add(Fields::immutableCopy(R ? &R->Headers : nullptr))};
}

Expect<BodyEnds>
RequestConsumeBody::body(Runtime::Component::CallingFrame &Frame,
                         Runtime::Component::Own<Request> This,
                         Runtime::Component::Future<HttpStatus> Res) {
  auto R = Http.Requests.remove(This.Rep);
  return Http.consume(Frame, R ? &R->Content : nullptr, Res);
}

Expect<Runtime::Component::Own<RequestOptions>>
OptionsNew::body(Runtime::Component::CallingFrame &) {
  return Runtime::Component::Own<RequestOptions>{
      Http.Options.add(std::make_unique<RequestOptions>())};
}

Expect<std::optional<uint64_t>>
OptionsGet::body(Runtime::Component::CallingFrame &, OptionsBorrow Self) {
  auto O = Http.Options.get(Self.Rep);
  if (!O) {
    return std::optional<uint64_t>{};
  }
  switch (W) {
  case Which::Connect:
    return O->ConnectTimeout;
  case Which::FirstByte:
    return O->FirstByteTimeout;
  case Which::BetweenBytes:
    return O->BetweenBytesTimeout;
  }
  return std::optional<uint64_t>{};
}

Expect<OptionsStatus> OptionsSet::body(Runtime::Component::CallingFrame &,
                                       OptionsBorrow Self,
                                       std::optional<uint64_t> Duration) {
  auto O = Http.Options.get(Self.Rep);
  RequestOptionsError E;
  if (!O) {
    E.Text = "no such options";
    return OptionsStatus(Unexpected<RequestOptionsError>(std::move(E)));
  }
  if (O->Immutable) {
    E.Case = RequestOptionsError::Immutable;
    return OptionsStatus(Unexpected<RequestOptionsError>(std::move(E)));
  }
  switch (W) {
  case Which::Connect:
    O->ConnectTimeout = Duration;
    break;
  case Which::FirstByte:
    O->FirstByteTimeout = Duration;
    break;
  case Which::BetweenBytes:
    O->BetweenBytesTimeout = Duration;
    break;
  }
  return OptionsStatus(Runtime::Component::WitUnit{});
}

Expect<Runtime::Component::Own<RequestOptions>>
OptionsClone::body(Runtime::Component::CallingFrame &, OptionsBorrow Self) {
  auto Copy = std::make_unique<RequestOptions>();
  if (auto O = Http.Options.get(Self.Rep)) {
    *Copy = *O;
    Copy->Immutable = false;
  }
  return Runtime::Component::Own<RequestOptions>{
      Http.Options.add(std::move(Copy))};
}

Expect<std::tuple<Runtime::Component::Own<Response>,
                  Runtime::Component::Future<HttpStatus>>>
ResponseNew::body(Runtime::Component::CallingFrame &Frame,
                  Runtime::Component::Own<Fields> Headers,
                  std::optional<Runtime::Component::Stream<uint8_t>> Contents,
                  Runtime::Component::Future<TrailersResult> Trailers) {
  auto R = std::make_unique<Response>();
  if (auto Taken = Http.FieldsTable.remove(Headers.Rep)) {
    R->Headers = std::move(*Taken);
  }
  R->Headers.Immutable = true;
  Runtime::Component::Future<HttpStatus> Done;
  R->Content = Http.newBody(Frame, Contents, Trailers, Done);
  return std::tuple<Runtime::Component::Own<Response>,
                    Runtime::Component::Future<HttpStatus>>{
      Runtime::Component::Own<Response>{Http.Responses.add(std::move(R))},
      std::move(Done)};
}

Expect<uint16_t> ResponseGetStatus::body(Runtime::Component::CallingFrame &,
                                         ResponseBorrow Self) {
  auto R = Http.Responses.get(Self.Rep);
  return R ? R->Status : uint16_t(0);
}

Expect<Expected<Runtime::Component::WitUnit, Runtime::Component::WitUnit>>
ResponseSetStatus::body(Runtime::Component::CallingFrame &, ResponseBorrow Self,
                        uint16_t Status) {
  auto R = Http.Responses.get(Self.Rep);
  if (!R || Status < 100 || Status > 999) {
    return WasiComponent::failUnit<Runtime::Component::WitUnit>();
  }
  R->Status = Status;
  return Expected<Runtime::Component::WitUnit, Runtime::Component::WitUnit>(
      Runtime::Component::WitUnit{});
}

Expect<Runtime::Component::Own<Fields>>
ResponseGetHeaders::body(Runtime::Component::CallingFrame &,
                         ResponseBorrow Self) {
  auto R = Http.Responses.get(Self.Rep);
  return Runtime::Component::Own<Fields>{
      Http.FieldsTable.add(Fields::immutableCopy(R ? &R->Headers : nullptr))};
}

Expect<BodyEnds>
ResponseConsumeBody::body(Runtime::Component::CallingFrame &Frame,
                          Runtime::Component::Own<Response> This,
                          Runtime::Component::Future<HttpStatus> Res) {
  auto R = Http.Responses.remove(This.Rep);
  return Http.consume(Frame, R ? &R->Content : nullptr, Res);
}

Expect<HttpResult<Runtime::Component::Own<Response>>>
ClientSend::body(Runtime::Component::CallingFrame &Frame,
                 Runtime::Component::Own<Request> Req) {
  auto ReqData = Http.Requests.remove(Req.Rep);
  if (!ReqData) {
    return HttpResult<Runtime::Component::Own<Response>>(
        Unexpected<HttpError>(HttpError::internal("no such request")));
  }
  Body &Content = ReqData->Content;
  // The result of the transmit, which also resolves the producer's future.
  auto WriteStatus = [&](HttpStatus Status) {
    if (Content.Done != 0 && !Content.Consumed) {
      Content.Consumed = true;
      resolveLater<HttpStatus>(Frame, Content.Done, std::move(Status));
    }
  };
  if (Content.Consumed) {
    return HttpResult<Runtime::Component::Own<Response>>(
        Unexpected<HttpError>(HttpError::internal("body consumed")));
  }

  WasiP2::OutgoingRequest Outgoing;
  Outgoing.M = ReqData->ReqMethod;
  Outgoing.PathWithQuery = ReqData->PathWithQuery;
  Outgoing.S = ReqData->ReqScheme;
  Outgoing.Authority = ReqData->Authority;
  Outgoing.Headers = std::move(ReqData->Headers);
  WasiP2::OutgoingBody OutBody;
  if (Content.FromHost) {
    OutBody.Data = std::move(Content.Data);
    OutBody.Trailers = std::move(Content.Trailers);
  } else {
    if (Content.Contents != 0) {
      const uint32_t In = Content.Contents;
      std::vector<uint8_t> Chunk;
      while (true) {
        EXPECTED_TRY(auto Got,
                     Frame.getExecutor().readStream(Frame, In, Chunk, 65536));
        OutBody.Data.insert(OutBody.Data.end(), Chunk.begin(), Chunk.end());
        if (Got.first != Runtime::Instance::Component::StreamInstance::
                             CopyResult::Completed ||
            Frame.isCancelled()) {
          break;
        }
      }
      Frame.getExecutor().dropStreamHandle(Frame, In);
      if (Frame.isCancelled()) {
        return HttpResult<Runtime::Component::Own<Response>>(
            Unexpected<HttpError>(HttpError::internal("cancelled")));
      }
    }
    if (Content.TrailersEnd != 0) {
      const uint32_t In = Content.TrailersEnd;
      std::vector<ComponentValVariant> Vals;
      auto Read = Frame.getExecutor().readStream(Frame, In, Vals, 1);
      Frame.getExecutor().dropStreamHandle(Frame, In);
      EXPECTED_TRY(Read);
      if (Frame.isCancelled()) {
        return HttpResult<Runtime::Component::Own<Response>>(
            Unexpected<HttpError>(HttpError::internal("cancelled")));
      }
      if (!Vals.empty()) {
        auto T = Runtime::Component::Wit<TrailersResult>::from(Vals[0]);
        if (!T) {
          WriteStatus(Unexpected<HttpError>(T.error()));
          return HttpResult<Runtime::Component::Own<Response>>(
              Unexpected<HttpError>(T.error()));
        }
        if (*T) {
          if (auto Taken = Http.FieldsTable.remove((*T)->Rep)) {
            OutBody.Trailers = std::move(*Taken);
          }
        }
      }
    }
  }
  EXPECTED_TRY(auto Reply, WasiP2::exchangeHttp11(Frame, Outgoing, &OutBody));
  if (Frame.isCancelled()) {
    return HttpResult<Runtime::Component::Own<Response>>(
        Unexpected<HttpError>(HttpError::internal("cancelled")));
  }
  if (!Reply) {
    WriteStatus(Unexpected<HttpError>(Reply.error()));
    return HttpResult<Runtime::Component::Own<Response>>(
        Unexpected<HttpError>(Reply.error()));
  }
  WriteStatus(HttpStatus(Runtime::Component::WitUnit{}));
  auto &Incoming = *Reply;
  auto Resp = std::make_unique<Response>();
  Resp->Status = Incoming->Status;
  Resp->Headers = std::move(Incoming->Headers);
  Resp->Content.FromHost = true;
  if (Incoming->Body) {
    Resp->Content.Data = std::move(Incoming->Body->Data);
    Resp->Content.Trailers = std::move(Incoming->Body->Trailers);
  }
  return HttpResult<Runtime::Component::Own<Response>>(
      Runtime::Component::Own<Response>{Http.Responses.add(std::move(Resp))});
}

HttpTypesInstance::HttpTypesInstance(HttpHost &Http)
    : ComponentInstance("wasi:http/types@0.3.1") {
  Http.TypesInstance = this;
  const auto Mint = getTypeMinter();
  exportType("duration", Mint.primDefType(PrimValType::U64).getTypeIndex());
  exportType("method",
             Runtime::Component::Wit<Method>::type(Mint).getTypeIndex());
  exportType("scheme",
             Runtime::Component::Wit<Scheme>::type(Mint).getTypeIndex());
  exportType("DNS-error-payload",
             Runtime::Component::Wit<WasiP2::DnsErrorPayload>::type(Mint)
                 .getTypeIndex());
  exportType(
      "TLS-alert-received-payload",
      Runtime::Component::Wit<WasiP2::TlsAlertReceivedPayload>::type(Mint)
          .getTypeIndex());
  exportType("field-size-payload",
             Runtime::Component::Wit<WasiP2::FieldSizePayload>::type(Mint)
                 .getTypeIndex());
  exportType("error-code",
             Runtime::Component::Wit<HttpError>::type(Mint).getTypeIndex());
  exportType("header-error",
             Runtime::Component::Wit<HeaderError>::type(Mint).getTypeIndex());
  exportType(
      "request-options-error",
      Runtime::Component::Wit<RequestOptionsError>::type(Mint).getTypeIndex());
  exportType("field-name",
             Mint.primDefType(PrimValType::String).getTypeIndex());
  exportType(
      "field-value",
      Runtime::Component::Wit<std::vector<uint8_t>>::type(Mint).getTypeIndex());
  const uint32_t FieldsIdx = addHostResourceType<Fields>(
      [&Http](uint64_t Rep) { Http.FieldsTable.remove(Rep); });
  exportType("fields", FieldsIdx);
  exportType("headers", FieldsIdx);
  exportType("trailers", FieldsIdx);
  const uint32_t ReqIdx = addHostResourceType<Request>(
      [&Http](uint64_t Rep) { Http.Requests.remove(Rep); });
  exportType("request", ReqIdx);
  Http.RequestType = *getTypeResource(ReqIdx);
  exportType("request-options",
             addHostResourceType<RequestOptions>(
                 [&Http](uint64_t Rep) { Http.Options.remove(Rep); }));
  exportType("status-code", Mint.primDefType(PrimValType::U16).getTypeIndex());
  const uint32_t RespIdx = addHostResourceType<Response>(
      [&Http](uint64_t Rep) { Http.Responses.remove(Rep); });
  exportType("response", RespIdx);
  Http.ResponseType = *getTypeResource(RespIdx);
  Http.StatusType = Runtime::Component::Wit<HttpStatus>::type(Mint);
  Http.TrailersType = Runtime::Component::Wit<TrailersResult>::type(Mint);

  addHostFunc("[constructor]fields", std::make_unique<FieldsNew>(Http));
  addHostFunc("[static]fields.from-list",
              std::make_unique<FieldsFromList>(Http));
  addHostFunc("[method]fields.get", std::make_unique<FieldsGet>(Http));
  addHostFunc("[method]fields.has", std::make_unique<FieldsHas>(Http));
  addHostFunc("[method]fields.set", std::make_unique<FieldsSet>(Http));
  addHostFunc("[method]fields.delete", std::make_unique<FieldsDelete>(Http));
  addHostFunc("[method]fields.get-and-delete",
              std::make_unique<FieldsGetAndDelete>(Http));
  addHostFunc("[method]fields.append", std::make_unique<FieldsAppend>(Http));
  addHostFunc("[method]fields.copy-all", std::make_unique<FieldsCopyAll>(Http));
  addHostFunc("[method]fields.clone", std::make_unique<FieldsClone>(Http));
  addHostFunc("[static]request.new", std::make_unique<RequestNew>(Http));
  addHostFunc("[method]request.get-method",
              std::make_unique<RequestGetMethod>(Http));
  addHostFunc("[method]request.set-method",
              std::make_unique<RequestSetMethod>(Http));
  addHostFunc("[method]request.get-path-with-query",
              std::make_unique<RequestGetText>(Http, false));
  addHostFunc("[method]request.set-path-with-query",
              std::make_unique<RequestSetText>(Http, false));
  addHostFunc("[method]request.get-scheme",
              std::make_unique<RequestGetScheme>(Http));
  addHostFunc("[method]request.set-scheme",
              std::make_unique<RequestSetScheme>(Http));
  addHostFunc("[method]request.get-authority",
              std::make_unique<RequestGetText>(Http, true));
  addHostFunc("[method]request.set-authority",
              std::make_unique<RequestSetText>(Http, true));
  addHostFunc("[method]request.get-options",
              std::make_unique<RequestGetOptions>(Http));
  addHostFunc("[method]request.get-headers",
              std::make_unique<RequestGetHeaders>(Http));
  addHostFunc("[static]request.consume-body",
              std::make_unique<RequestConsumeBody>(Http));
  addHostFunc("[constructor]request-options",
              std::make_unique<OptionsNew>(Http));
  addHostFunc("[method]request-options.get-connect-timeout",
              std::make_unique<OptionsGet>(Http, OptionsGet::Which::Connect));
  addHostFunc("[method]request-options.set-connect-timeout",
              std::make_unique<OptionsSet>(Http, OptionsGet::Which::Connect));
  addHostFunc("[method]request-options.get-first-byte-timeout",
              std::make_unique<OptionsGet>(Http, OptionsGet::Which::FirstByte));
  addHostFunc("[method]request-options.set-first-byte-timeout",
              std::make_unique<OptionsSet>(Http, OptionsGet::Which::FirstByte));
  addHostFunc(
      "[method]request-options.get-between-bytes-timeout",
      std::make_unique<OptionsGet>(Http, OptionsGet::Which::BetweenBytes));
  addHostFunc(
      "[method]request-options.set-between-bytes-timeout",
      std::make_unique<OptionsSet>(Http, OptionsGet::Which::BetweenBytes));
  addHostFunc("[method]request-options.clone",
              std::make_unique<OptionsClone>(Http));
  addHostFunc("[static]response.new", std::make_unique<ResponseNew>(Http));
  addHostFunc("[method]response.get-status-code",
              std::make_unique<ResponseGetStatus>(Http));
  addHostFunc("[method]response.set-status-code",
              std::make_unique<ResponseSetStatus>(Http));
  addHostFunc("[method]response.get-headers",
              std::make_unique<ResponseGetHeaders>(Http));
  addHostFunc("[static]response.consume-body",
              std::make_unique<ResponseConsumeBody>(Http));
}

ClientInstance::ClientInstance(HttpHost &Http)
    : ComponentInstance("wasi:http/client@0.3.1") {
  exportType("request", addSharedResourceType<Request>(Http.RequestType));
  exportType("response", addSharedResourceType<Response>(Http.ResponseType));
  exportType(
      "error-code",
      Runtime::Component::Wit<HttpError>::type(getTypeMinter()).getTypeIndex());
  addHostFunc("send", std::make_unique<ClientSend>(Http));
}

std::vector<std::unique_ptr<Runtime::Instance::ComponentInstance>>
newHttpInstances(HttpHost &Http) {
  std::vector<std::unique_ptr<Runtime::Instance::ComponentInstance>> Insts;
  Insts.push_back(std::make_unique<HttpTypesInstance>(Http));
  Insts.push_back(std::make_unique<ClientInstance>(Http));
  return Insts;
}

} // namespace WasiP3
} // namespace Host
} // namespace WasmEdge
