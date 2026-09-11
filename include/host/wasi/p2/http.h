// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/host/wasi/p2/http.h - wasi:http host ---------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the host component instances of `wasi:http@0.2.12`:
/// `types` with the request, response, body, and trailer resources over
/// buffered bodies and the `wasi:io` streams, and `outgoing-handler`, which
/// sends a request over a preview-1 socket in HTTP/1.1 and reads the whole
/// response when it is first asked for. There is no TLS. The incoming side
/// is what a host puts in front of an `incoming-handler` export.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/component_valtype.h"
#include "common/component_variant.h"
#include "common/errcode.h"
#include "common/expected.h"
#include "host/wasi/p2/io.h"
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
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace WasiP2 {

/// `method`: the standard methods and `other(string)`.
struct Method {
  enum Kind : uint32_t {
    Get,
    Head,
    Post,
    Put,
    Delete,
    Connect,
    Options,
    Trace,
    Patch,
    Other
  };
  Kind Case = Get;
  std::string Name;
  /// The request line form of the method.
  std::string text() const;
};

/// `scheme`: `HTTP`, `HTTPS`, and `other(string)`.
struct Scheme {
  enum Kind : uint32_t { Http, Https, Other };
  Kind Case = Http;
  std::string Name;
};

/// `header-error`
enum class HeaderError : uint32_t { InvalidSyntax, Forbidden, Immutable };

struct DnsErrorPayload {
  std::optional<std::string> Rcode;
  std::optional<uint16_t> InfoCode;
};

struct TlsAlertReceivedPayload {
  std::optional<uint8_t> AlertId;
  std::optional<std::string> AlertMessage;
};

struct FieldSizePayload {
  std::optional<std::string> FieldName;
  std::optional<uint32_t> FieldSize;
};

/// `error-code`: the case and the payload the case carries, if any.
struct HttpError {
  enum Kind : uint32_t {
    DnsTimeout,
    DnsError,
    DestinationNotFound,
    DestinationUnavailable,
    DestinationIpProhibited,
    DestinationIpUnroutable,
    ConnectionRefused,
    ConnectionTerminated,
    ConnectionTimeout,
    ConnectionReadTimeout,
    ConnectionWriteTimeout,
    ConnectionLimitReached,
    TlsProtocolError,
    TlsCertificateError,
    TlsAlertReceived,
    HttpRequestDenied,
    HttpRequestLengthRequired,
    HttpRequestBodySize,
    HttpRequestMethodInvalid,
    HttpRequestUriInvalid,
    HttpRequestUriTooLong,
    HttpRequestHeaderSectionSize,
    HttpRequestHeaderSize,
    HttpRequestTrailerSectionSize,
    HttpRequestTrailerSize,
    HttpResponseIncomplete,
    HttpResponseHeaderSectionSize,
    HttpResponseHeaderSize,
    HttpResponseBodySize,
    HttpResponseTrailerSectionSize,
    HttpResponseTrailerSize,
    HttpResponseTransferCoding,
    HttpResponseContentCoding,
    HttpResponseTimeout,
    HttpUpgradeFailed,
    HttpProtocolError,
    LoopDetected,
    ConfigurationError,
    InternalError
  };
  Kind Case = InternalError;
  /// `internal-error`, `HTTP-response-transfer-coding`, and
  /// `HTTP-response-content-coding`.
  std::optional<std::string> Text;
  /// The body size cases.
  std::optional<uint64_t> Size;
  /// The section size cases.
  std::optional<uint32_t> Count;
  /// The field size cases.
  std::optional<FieldSizePayload> Field;
  DnsErrorPayload Dns;
  TlsAlertReceivedPayload Tls;

  static HttpError of(Kind K) noexcept {
    HttpError E;
    E.Case = K;
    return E;
  }
  static HttpError internal(std::string Message) noexcept {
    HttpError E;
    E.Text = std::move(Message);
    return E;
  }
};

template <typename T> using HttpResult = Expected<T, HttpError>;
using HttpStatus = Expected<Runtime::Component::Unit, HttpError>;
using HeaderStatus = Expected<Runtime::Component::Unit, HeaderError>;

/// `fields`: ordered name and value pairs. Those of a request or response
/// are immutable once handed out.
struct Fields {
  std::vector<std::pair<std::string, std::vector<uint8_t>>> Entries;
  bool Immutable = false;
  std::vector<std::vector<uint8_t>> get(std::string_view Name) const;
  bool has(std::string_view Name) const noexcept;
  /// The first value of a header as text, if any.
  std::optional<std::string> first(std::string_view Name) const;

  /// An immutable copy of From, empty when From is null.
  static std::shared_ptr<Fields>
  immutableCopy(const std::shared_ptr<Fields> &From);
  /// Case-insensitive equality of two field names.
  static bool isSameName(std::string_view A, std::string_view B) noexcept;
  /// The syntax of a field name: an HTTP token.
  static bool isValidName(std::string_view Name) noexcept;
  /// A field value: no CR, LF, or NUL.
  static bool isValidValue(Span<const uint8_t> Value) noexcept;
  /// The headers a guest may not set: the connection is the host's.
  static bool isForbiddenName(std::string_view Name) noexcept;
};

/// `request-options`
struct RequestOptions {
  std::optional<uint64_t> ConnectTimeout;
  std::optional<uint64_t> FirstByteTimeout;
  std::optional<uint64_t> BetweenBytesTimeout;
};

/// A body the guest writes: the bytes so far, whether the stream and the
/// finish were taken, and the trailers given at the finish.
struct OutgoingBody {
  std::vector<uint8_t> Data;
  bool StreamTaken = false;
  bool Finished = false;
  std::shared_ptr<Fields> Trailers;
};

/// A body the guest reads: the bytes, the read position, and the trailers.
struct IncomingBody {
  std::vector<uint8_t> Data;
  size_t Offset = 0;
  bool StreamTaken = false;
  std::shared_ptr<Fields> Trailers;
};

struct OutgoingRequest {
  Method M;
  std::optional<std::string> PathWithQuery;
  std::optional<Scheme> S;
  std::optional<std::string> Authority;
  std::shared_ptr<Fields> Headers;
  std::shared_ptr<OutgoingBody> Body;
};

struct IncomingRequest {
  Method M;
  std::optional<std::string> PathWithQuery;
  std::optional<Scheme> S;
  std::optional<std::string> Authority;
  std::shared_ptr<Fields> Headers;
  std::shared_ptr<IncomingBody> Body;
  bool Consumed = false;
};

struct OutgoingResponse {
  uint16_t Status = 200;
  std::shared_ptr<Fields> Headers;
  std::shared_ptr<OutgoingBody> Body;
};

struct IncomingResponse {
  uint16_t Status = 200;
  std::shared_ptr<Fields> Headers;
  std::shared_ptr<IncomingBody> Body;
  bool Consumed = false;
};

/// What an `incoming-handler` sets: the response or the error.
struct ResponseOutparam {
  std::optional<HttpResult<std::shared_ptr<OutgoingResponse>>> Response;
};

struct FutureTrailers {
  std::shared_ptr<IncomingBody> Body;
  bool Taken = false;
};

/// The response of `outgoing-handler.handle`, fetched when first asked for.
struct FutureIncomingResponse {
  std::shared_ptr<OutgoingRequest> Request;
  RequestOptions Options;
  std::optional<HttpResult<std::shared_ptr<IncomingResponse>>> Result;
  bool Taken = false;
};

/// An output stream that appends to an outgoing body.
class BodyOutputStream : public OutputStream {
public:
  BodyOutputStream(std::shared_ptr<OutgoingBody> B) noexcept
      : Body(std::move(B)) {}
  bool ready() noexcept override { return true; }
  bool subscribe(WASI::VPoller &, __wasi_userdata_t) noexcept override {
    return false;
  }
  StreamResult<uint64_t> checkWrite() noexcept override;
  StreamResult<Runtime::Component::Unit>
  write(Span<const uint8_t> Data) noexcept override;
  StreamResult<Runtime::Component::Unit> flush() noexcept override {
    return Runtime::Component::Unit{};
  }

private:
  std::shared_ptr<OutgoingBody> Body;
};

/// An input stream over an incoming body, closed at its end.
class BodyInputStream : public InputStream {
public:
  BodyInputStream(std::shared_ptr<IncomingBody> B) noexcept
      : Body(std::move(B)) {}
  bool ready() noexcept override { return true; }
  bool subscribe(WASI::VPoller &, __wasi_userdata_t) noexcept override {
    return false;
  }
  StreamResult<std::vector<uint8_t>> read(uint64_t Len) noexcept override;
  StreamResult<std::vector<uint8_t>>
  blockingRead(uint64_t Len) noexcept override {
    return read(Len);
  }

private:
  std::shared_ptr<IncomingBody> Body;
};

/// The tables of the http resources, shared by the instances of the package.
struct HttpHost {
  HttpHost(IoHost &H) noexcept : Io(H) {}
  IoHost &Io;
  Runtime::Component::ResourceTable<Fields> FieldsTable;
  Runtime::Component::ResourceTable<IncomingRequest> IncomingRequests;
  Runtime::Component::ResourceTable<OutgoingRequest> OutgoingRequests;
  Runtime::Component::ResourceTable<RequestOptions> Options;
  Runtime::Component::ResourceTable<ResponseOutparam> Outparams;
  Runtime::Component::ResourceTable<IncomingResponse> IncomingResponses;
  Runtime::Component::ResourceTable<IncomingBody> IncomingBodies;
  Runtime::Component::ResourceTable<FutureTrailers> Trailers;
  Runtime::Component::ResourceTable<OutgoingResponse> OutgoingResponses;
  Runtime::Component::ResourceTable<OutgoingBody> OutgoingBodies;
  Runtime::Component::ResourceTable<FutureIncomingResponse> FutureResponses;
  const Runtime::Instance::Component::ResourceTypeInstance
      *OutgoingRequestType = nullptr;
  const Runtime::Instance::Component::ResourceTypeInstance *OptionsType =
      nullptr;
  const Runtime::Instance::Component::ResourceTypeInstance *FutureResponseType =
      nullptr;

  /// The result of a future, fetched now if it was not yet.
  const HttpResult<std::shared_ptr<IncomingResponse>> &
  resolve(FutureIncomingResponse &Future);
};

/// Sends Request in HTTP/1.1 over a new connection and reads the whole
/// response: a content length, chunks with their trailers, or up to the
/// close. A body with trailers goes out chunked. There is no TLS.
HttpResult<std::shared_ptr<IncomingResponse>>
exchangeHttp11(const OutgoingRequest &Request, const RequestOptions &Options);

using FieldsBorrow = Runtime::Component::Borrow<Fields>;
using IncomingRequestBorrow = Runtime::Component::Borrow<IncomingRequest>;
using OutgoingRequestBorrow = Runtime::Component::Borrow<OutgoingRequest>;
using OptionsBorrow = Runtime::Component::Borrow<RequestOptions>;
using IncomingResponseBorrow = Runtime::Component::Borrow<IncomingResponse>;
using IncomingBodyBorrow = Runtime::Component::Borrow<IncomingBody>;
using TrailersBorrow = Runtime::Component::Borrow<FutureTrailers>;
using OutgoingResponseBorrow = Runtime::Component::Borrow<OutgoingResponse>;
using OutgoingBodyBorrow = Runtime::Component::Borrow<OutgoingBody>;
using FutureResponseBorrow = Runtime::Component::Borrow<FutureIncomingResponse>;
using FieldEntry = std::tuple<std::string, std::vector<uint8_t>>;

} // namespace WasiP2
} // namespace Host

namespace Runtime {
namespace Component {

template <> struct Wit<Host::WasiP2::Method> {
  static ComponentValType type(const TypeMinter &Mint) noexcept {
    WitVariant::Cases Cases;
    for (const char *Label : {"get", "head", "post", "put", "delete", "connect",
                              "options", "trace", "patch"}) {
      Cases.emplace_back(Label, std::nullopt);
    }
    Cases.emplace_back("other", Wit<std::string>::type(Mint));
    return WitVariant::type(Mint, std::move(Cases));
  }
  static Host::WasiP2::Method from(const ComponentValVariant &V) {
    const auto &Val = WitVariant::value(V);
    Host::WasiP2::Method M;
    M.Case = static_cast<Host::WasiP2::Method::Kind>(Val.Case);
    if (M.Case == Host::WasiP2::Method::Other && Val.Payload) {
      M.Name = Wit<std::string>::from(*Val.Payload);
    }
    return M;
  }
  static ComponentValVariant into(Host::WasiP2::Method &&M) noexcept {
    if (M.Case == Host::WasiP2::Method::Other) {
      return WitVariant::into(M.Case,
                              Wit<std::string>::into(std::move(M.Name)));
    }
    return WitVariant::into(M.Case);
  }
};

template <> struct Wit<Host::WasiP2::Scheme> {
  static ComponentValType type(const TypeMinter &Mint) noexcept {
    return WitVariant::type(Mint, {{"HTTP", std::nullopt},
                                   {"HTTPS", std::nullopt},
                                   {"other", Wit<std::string>::type(Mint)}});
  }
  static Host::WasiP2::Scheme from(const ComponentValVariant &V) {
    const auto &Val = WitVariant::value(V);
    Host::WasiP2::Scheme S;
    S.Case = static_cast<Host::WasiP2::Scheme::Kind>(Val.Case);
    if (S.Case == Host::WasiP2::Scheme::Other && Val.Payload) {
      S.Name = Wit<std::string>::from(*Val.Payload);
    }
    return S;
  }
  static ComponentValVariant into(Host::WasiP2::Scheme &&S) noexcept {
    if (S.Case == Host::WasiP2::Scheme::Other) {
      return WitVariant::into(S.Case,
                              Wit<std::string>::into(std::move(S.Name)));
    }
    return WitVariant::into(S.Case);
  }
};

template <> struct Wit<Host::WasiP2::HeaderError> {
  static ComponentValType type(const TypeMinter &Mint) noexcept {
    return WitVariant::type(Mint, {{"invalid-syntax", std::nullopt},
                                   {"forbidden", std::nullopt},
                                   {"immutable", std::nullopt}});
  }
  static Host::WasiP2::HeaderError from(const ComponentValVariant &V) {
    return static_cast<Host::WasiP2::HeaderError>(WitVariant::value(V).Case);
  }
  static ComponentValVariant into(Host::WasiP2::HeaderError E) noexcept {
    return WitVariant::into(static_cast<uint32_t>(E));
  }
};

template <> struct Wit<Host::WasiP2::DnsErrorPayload> {
  static ComponentValType type(const TypeMinter &Mint) noexcept {
    return WitRecord::type(
        Mint, {{"rcode", Wit<std::optional<std::string>>::type(Mint)},
               {"info-code", Wit<std::optional<uint16_t>>::type(Mint)}});
  }
  static Host::WasiP2::DnsErrorPayload from(const ComponentValVariant &V) {
    auto F = WitRecord::fields(V);
    return {Wit<std::optional<std::string>>::from(F[0].second),
            Wit<std::optional<uint16_t>>::from(F[1].second)};
  }
  static ComponentValVariant into(Host::WasiP2::DnsErrorPayload &&P) noexcept {
    return WitRecord::into(
        {{"rcode", Wit<std::optional<std::string>>::into(std::move(P.Rcode))},
         {"info-code",
          Wit<std::optional<uint16_t>>::into(std::move(P.InfoCode))}});
  }
};

template <> struct Wit<Host::WasiP2::TlsAlertReceivedPayload> {
  static ComponentValType type(const TypeMinter &Mint) noexcept {
    return WitRecord::type(
        Mint, {{"alert-id", Wit<std::optional<uint8_t>>::type(Mint)},
               {"alert-message", Wit<std::optional<std::string>>::type(Mint)}});
  }
  static Host::WasiP2::TlsAlertReceivedPayload
  from(const ComponentValVariant &V) {
    auto F = WitRecord::fields(V);
    return {Wit<std::optional<uint8_t>>::from(F[0].second),
            Wit<std::optional<std::string>>::from(F[1].second)};
  }
  static ComponentValVariant
  into(Host::WasiP2::TlsAlertReceivedPayload &&P) noexcept {
    return WitRecord::into(
        {{"alert-id", Wit<std::optional<uint8_t>>::into(std::move(P.AlertId))},
         {"alert-message",
          Wit<std::optional<std::string>>::into(std::move(P.AlertMessage))}});
  }
};

template <> struct Wit<Host::WasiP2::FieldSizePayload> {
  static ComponentValType type(const TypeMinter &Mint) noexcept {
    return WitRecord::type(
        Mint, {{"field-name", Wit<std::optional<std::string>>::type(Mint)},
               {"field-size", Wit<std::optional<uint32_t>>::type(Mint)}});
  }
  static Host::WasiP2::FieldSizePayload from(const ComponentValVariant &V) {
    auto F = WitRecord::fields(V);
    return {Wit<std::optional<std::string>>::from(F[0].second),
            Wit<std::optional<uint32_t>>::from(F[1].second)};
  }
  static ComponentValVariant into(Host::WasiP2::FieldSizePayload &&P) noexcept {
    return WitRecord::into(
        {{"field-name",
          Wit<std::optional<std::string>>::into(std::move(P.FieldName))},
         {"field-size",
          Wit<std::optional<uint32_t>>::into(std::move(P.FieldSize))}});
  }
};

template <> struct Wit<Host::WasiP2::HttpError> {
  /// The payload shape of each case.
  enum class Payload {
    None,
    Dns,
    Tls,
    OptU64,
    OptU32,
    OptField,
    Field,
    OptStr
  };
  static constexpr Payload Shapes[] = {
      Payload::None,   Payload::Dns,    Payload::None,     Payload::None,
      Payload::None,   Payload::None,   Payload::None,     Payload::None,
      Payload::None,   Payload::None,   Payload::None,     Payload::None,
      Payload::None,   Payload::None,   Payload::Tls,      Payload::None,
      Payload::None,   Payload::OptU64, Payload::None,     Payload::None,
      Payload::None,   Payload::OptU32, Payload::OptField, Payload::OptU32,
      Payload::Field,  Payload::None,   Payload::OptU32,   Payload::Field,
      Payload::OptU64, Payload::OptU32, Payload::Field,    Payload::OptStr,
      Payload::OptStr, Payload::None,   Payload::None,     Payload::None,
      Payload::None,   Payload::None,   Payload::OptStr};
  static constexpr const char *Labels[] = {"DNS-timeout",
                                           "DNS-error",
                                           "destination-not-found",
                                           "destination-unavailable",
                                           "destination-IP-prohibited",
                                           "destination-IP-unroutable",
                                           "connection-refused",
                                           "connection-terminated",
                                           "connection-timeout",
                                           "connection-read-timeout",
                                           "connection-write-timeout",
                                           "connection-limit-reached",
                                           "TLS-protocol-error",
                                           "TLS-certificate-error",
                                           "TLS-alert-received",
                                           "HTTP-request-denied",
                                           "HTTP-request-length-required",
                                           "HTTP-request-body-size",
                                           "HTTP-request-method-invalid",
                                           "HTTP-request-URI-invalid",
                                           "HTTP-request-URI-too-long",
                                           "HTTP-request-header-section-size",
                                           "HTTP-request-header-size",
                                           "HTTP-request-trailer-section-size",
                                           "HTTP-request-trailer-size",
                                           "HTTP-response-incomplete",
                                           "HTTP-response-header-section-size",
                                           "HTTP-response-header-size",
                                           "HTTP-response-body-size",
                                           "HTTP-response-trailer-section-size",
                                           "HTTP-response-trailer-size",
                                           "HTTP-response-transfer-coding",
                                           "HTTP-response-content-coding",
                                           "HTTP-response-timeout",
                                           "HTTP-upgrade-failed",
                                           "HTTP-protocol-error",
                                           "loop-detected",
                                           "configuration-error",
                                           "internal-error"};
  static ComponentValType type(const TypeMinter &Mint) noexcept {
    WitVariant::Cases Cases;
    for (size_t I = 0; I < 39; ++I) {
      std::optional<ComponentValType> P;
      switch (Shapes[I]) {
      case Payload::None:
        break;
      case Payload::Dns:
        P = Wit<Host::WasiP2::DnsErrorPayload>::type(Mint);
        break;
      case Payload::Tls:
        P = Wit<Host::WasiP2::TlsAlertReceivedPayload>::type(Mint);
        break;
      case Payload::OptU64:
        P = Wit<std::optional<uint64_t>>::type(Mint);
        break;
      case Payload::OptU32:
        P = Wit<std::optional<uint32_t>>::type(Mint);
        break;
      case Payload::OptField:
        P = Wit<std::optional<Host::WasiP2::FieldSizePayload>>::type(Mint);
        break;
      case Payload::Field:
        P = Wit<Host::WasiP2::FieldSizePayload>::type(Mint);
        break;
      case Payload::OptStr:
        P = Wit<std::optional<std::string>>::type(Mint);
        break;
      }
      Cases.emplace_back(Labels[I], P);
    }
    return WitVariant::type(Mint, std::move(Cases));
  }
  static Host::WasiP2::HttpError from(const ComponentValVariant &V) {
    const auto &Val = WitVariant::value(V);
    Host::WasiP2::HttpError Err;
    Err.Case = static_cast<Host::WasiP2::HttpError::Kind>(Val.Case);
    if (!Val.Payload) {
      return Err;
    }
    switch (Shapes[Val.Case]) {
    case Payload::None:
      break;
    case Payload::Dns:
      Err.Dns = Wit<Host::WasiP2::DnsErrorPayload>::from(*Val.Payload);
      break;
    case Payload::Tls:
      Err.Tls = Wit<Host::WasiP2::TlsAlertReceivedPayload>::from(*Val.Payload);
      break;
    case Payload::OptU64:
      Err.Size = Wit<std::optional<uint64_t>>::from(*Val.Payload);
      break;
    case Payload::OptU32:
      Err.Count = Wit<std::optional<uint32_t>>::from(*Val.Payload);
      break;
    case Payload::OptField:
      Err.Field = Wit<std::optional<Host::WasiP2::FieldSizePayload>>::from(
          *Val.Payload);
      break;
    case Payload::Field:
      Err.Field = Wit<Host::WasiP2::FieldSizePayload>::from(*Val.Payload);
      break;
    case Payload::OptStr:
      Err.Text = Wit<std::optional<std::string>>::from(*Val.Payload);
      break;
    }
    return Err;
  }
  static ComponentValVariant into(Host::WasiP2::HttpError &&Err) noexcept {
    switch (Shapes[Err.Case]) {
    case Payload::None:
      return WitVariant::into(Err.Case);
    case Payload::Dns:
      return WitVariant::into(
          Err.Case,
          Wit<Host::WasiP2::DnsErrorPayload>::into(std::move(Err.Dns)));
    case Payload::Tls:
      return WitVariant::into(
          Err.Case,
          Wit<Host::WasiP2::TlsAlertReceivedPayload>::into(std::move(Err.Tls)));
    case Payload::OptU64:
      return WitVariant::into(
          Err.Case, Wit<std::optional<uint64_t>>::into(std::move(Err.Size)));
    case Payload::OptU32:
      return WitVariant::into(
          Err.Case, Wit<std::optional<uint32_t>>::into(std::move(Err.Count)));
    case Payload::OptField:
      return WitVariant::into(
          Err.Case, Wit<std::optional<Host::WasiP2::FieldSizePayload>>::into(
                        std::move(Err.Field)));
    case Payload::Field:
      return WitVariant::into(
          Err.Case, Wit<Host::WasiP2::FieldSizePayload>::into(
                        Err.Field ? std::move(*Err.Field)
                                  : Host::WasiP2::FieldSizePayload{}));
    case Payload::OptStr:
      return WitVariant::into(
          Err.Case, Wit<std::optional<std::string>>::into(std::move(Err.Text)));
    }
    return WitVariant::into(Err.Case);
  }
};

} // namespace Component
} // namespace Runtime

namespace Host {
namespace WasiP2 {

/// The base of the http functions: the tables they resolve in.
template <typename T>
class HttpFunction : public Runtime::Component::HostFunction<T> {
public:
  HttpFunction(HttpHost &H) noexcept : Http(H) {}

protected:
  HttpHost &Http;
};

/// `[constructor]fields`
class FieldsNew : public HttpFunction<FieldsNew> {
public:
  using HttpFunction::HttpFunction;
  Expect<Runtime::Component::Own<Fields>>
  body(Runtime::Component::CallingFrame &);
};

/// `[static]fields.from-list`
class FieldsFromList : public HttpFunction<FieldsFromList> {
public:
  static constexpr const char *ParamNames[] = {"entries"};
  using HttpFunction::HttpFunction;
  Expect<Expected<Runtime::Component::Own<Fields>, HeaderError>>
  body(Runtime::Component::CallingFrame &, std::vector<FieldEntry> Entries);
};

/// `[method]fields.get`
class FieldsGet : public HttpFunction<FieldsGet> {
public:
  static constexpr const char *ParamNames[] = {"self", "name"};
  using HttpFunction::HttpFunction;
  Expect<std::vector<std::vector<uint8_t>>>
  body(Runtime::Component::CallingFrame &, FieldsBorrow Self, std::string Name);
};

/// `[method]fields.has`
class FieldsHas : public HttpFunction<FieldsHas> {
public:
  static constexpr const char *ParamNames[] = {"self", "name"};
  using HttpFunction::HttpFunction;
  Expect<bool> body(Runtime::Component::CallingFrame &, FieldsBorrow Self,
                    std::string Name);
};

/// `[method]fields.set`
class FieldsSet : public HttpFunction<FieldsSet> {
public:
  static constexpr const char *ParamNames[] = {"self", "name", "value"};
  using HttpFunction::HttpFunction;
  Expect<HeaderStatus> body(Runtime::Component::CallingFrame &,
                            FieldsBorrow Self, std::string Name,
                            std::vector<std::vector<uint8_t>> Values);
};

/// `[method]fields.delete`
class FieldsDelete : public HttpFunction<FieldsDelete> {
public:
  static constexpr const char *ParamNames[] = {"self", "name"};
  using HttpFunction::HttpFunction;
  Expect<HeaderStatus> body(Runtime::Component::CallingFrame &,
                            FieldsBorrow Self, std::string Name);
};

/// `[method]fields.append`
class FieldsAppend : public HttpFunction<FieldsAppend> {
public:
  static constexpr const char *ParamNames[] = {"self", "name", "value"};
  using HttpFunction::HttpFunction;
  Expect<HeaderStatus> body(Runtime::Component::CallingFrame &,
                            FieldsBorrow Self, std::string Name,
                            std::vector<uint8_t> Value);
};

/// `[method]fields.entries`
class FieldsEntries : public HttpFunction<FieldsEntries> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  using HttpFunction::HttpFunction;
  Expect<std::vector<FieldEntry>> body(Runtime::Component::CallingFrame &,
                                       FieldsBorrow Self);
};

/// `[method]fields.clone`
class FieldsClone : public HttpFunction<FieldsClone> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  using HttpFunction::HttpFunction;
  Expect<Runtime::Component::Own<Fields>>
  body(Runtime::Component::CallingFrame &, FieldsBorrow Self);
};

/// `[method]incoming-request.method`
class IncomingRequestMethod : public HttpFunction<IncomingRequestMethod> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  using HttpFunction::HttpFunction;
  Expect<Method> body(Runtime::Component::CallingFrame &,
                      IncomingRequestBorrow Self);
};

/// `[method]incoming-request.path-with-query` and `authority`
class IncomingRequestText : public HttpFunction<IncomingRequestText> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  IncomingRequestText(HttpHost &H, bool Authority) noexcept
      : HttpFunction(H), Authority(Authority) {}
  Expect<std::optional<std::string>> body(Runtime::Component::CallingFrame &,
                                          IncomingRequestBorrow Self);

private:
  bool Authority;
};

/// `[method]incoming-request.scheme`
class IncomingRequestScheme : public HttpFunction<IncomingRequestScheme> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  using HttpFunction::HttpFunction;
  Expect<std::optional<Scheme>> body(Runtime::Component::CallingFrame &,
                                     IncomingRequestBorrow Self);
};

/// `[method]incoming-request.headers`
class IncomingRequestHeaders : public HttpFunction<IncomingRequestHeaders> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  using HttpFunction::HttpFunction;
  Expect<Runtime::Component::Own<Fields>>
  body(Runtime::Component::CallingFrame &, IncomingRequestBorrow Self);
};

/// `[method]incoming-request.consume`
class IncomingRequestConsume : public HttpFunction<IncomingRequestConsume> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  using HttpFunction::HttpFunction;
  Expect<
      Expected<Runtime::Component::Own<IncomingBody>, Runtime::Component::Unit>>
  body(Runtime::Component::CallingFrame &, IncomingRequestBorrow Self);
};

/// `[constructor]outgoing-request`
class OutgoingRequestNew : public HttpFunction<OutgoingRequestNew> {
public:
  static constexpr const char *ParamNames[] = {"headers"};
  using HttpFunction::HttpFunction;
  Expect<Runtime::Component::Own<OutgoingRequest>>
  body(Runtime::Component::CallingFrame &,
       Runtime::Component::Own<Fields> Headers);
};

/// `[method]outgoing-request.body`
class OutgoingRequestBody : public HttpFunction<OutgoingRequestBody> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  using HttpFunction::HttpFunction;
  Expect<
      Expected<Runtime::Component::Own<OutgoingBody>, Runtime::Component::Unit>>
  body(Runtime::Component::CallingFrame &, OutgoingRequestBorrow Self);
};

/// `[method]outgoing-request.method`
class OutgoingRequestMethod : public HttpFunction<OutgoingRequestMethod> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  using HttpFunction::HttpFunction;
  Expect<Method> body(Runtime::Component::CallingFrame &,
                      OutgoingRequestBorrow Self);
};

/// `[method]outgoing-request.set-method`
class OutgoingRequestSetMethod : public HttpFunction<OutgoingRequestSetMethod> {
public:
  static constexpr const char *ParamNames[] = {"self", "method"};
  using HttpFunction::HttpFunction;
  Expect<Expected<Runtime::Component::Unit, Runtime::Component::Unit>>
  body(Runtime::Component::CallingFrame &, OutgoingRequestBorrow Self,
       Method M);
};

/// `[method]outgoing-request.path-with-query` and `authority`
class OutgoingRequestText : public HttpFunction<OutgoingRequestText> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  OutgoingRequestText(HttpHost &H, bool Authority) noexcept
      : HttpFunction(H), Authority(Authority) {}
  Expect<std::optional<std::string>> body(Runtime::Component::CallingFrame &,
                                          OutgoingRequestBorrow Self);

private:
  bool Authority;
};

/// `[method]outgoing-request.set-path-with-query` and `set-authority`
class OutgoingRequestSetText : public HttpFunction<OutgoingRequestSetText> {
public:
  static constexpr const char *ParamNames[] = {"self", "value"};
  OutgoingRequestSetText(HttpHost &H, bool Authority) noexcept
      : HttpFunction(H), Authority(Authority) {}
  Expect<Expected<Runtime::Component::Unit, Runtime::Component::Unit>>
  body(Runtime::Component::CallingFrame &, OutgoingRequestBorrow Self,
       std::optional<std::string> Value);

private:
  bool Authority;
};

/// `[method]outgoing-request.scheme`
class OutgoingRequestScheme : public HttpFunction<OutgoingRequestScheme> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  using HttpFunction::HttpFunction;
  Expect<std::optional<Scheme>> body(Runtime::Component::CallingFrame &,
                                     OutgoingRequestBorrow Self);
};

/// `[method]outgoing-request.set-scheme`
class OutgoingRequestSetScheme : public HttpFunction<OutgoingRequestSetScheme> {
public:
  static constexpr const char *ParamNames[] = {"self", "scheme"};
  using HttpFunction::HttpFunction;
  Expect<Expected<Runtime::Component::Unit, Runtime::Component::Unit>>
  body(Runtime::Component::CallingFrame &, OutgoingRequestBorrow Self,
       std::optional<Scheme> S);
};

/// `[method]outgoing-request.headers`
class OutgoingRequestHeaders : public HttpFunction<OutgoingRequestHeaders> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  using HttpFunction::HttpFunction;
  Expect<Runtime::Component::Own<Fields>>
  body(Runtime::Component::CallingFrame &, OutgoingRequestBorrow Self);
};

/// `[constructor]request-options`
class RequestOptionsNew : public HttpFunction<RequestOptionsNew> {
public:
  using HttpFunction::HttpFunction;
  Expect<Runtime::Component::Own<RequestOptions>>
  body(Runtime::Component::CallingFrame &);
};

/// The three timeout getters of `request-options`.
class RequestOptionsGet : public HttpFunction<RequestOptionsGet> {
public:
  enum class Which { Connect, FirstByte, BetweenBytes };
  static constexpr const char *ParamNames[] = {"self"};
  RequestOptionsGet(HttpHost &H, Which W) noexcept : HttpFunction(H), W(W) {}
  Expect<std::optional<uint64_t>> body(Runtime::Component::CallingFrame &,
                                       OptionsBorrow Self);

private:
  Which W;
};

/// The three timeout setters of `request-options`.
class RequestOptionsSet : public HttpFunction<RequestOptionsSet> {
public:
  using Which = RequestOptionsGet::Which;
  static constexpr const char *ParamNames[] = {"self", "duration"};
  RequestOptionsSet(HttpHost &H, Which W) noexcept : HttpFunction(H), W(W) {}
  Expect<Expected<Runtime::Component::Unit, Runtime::Component::Unit>>
  body(Runtime::Component::CallingFrame &, OptionsBorrow Self,
       std::optional<uint64_t> Duration);

private:
  Which W;
};

/// `[static]response-outparam.set`
class ResponseOutparamSet : public HttpFunction<ResponseOutparamSet> {
public:
  static constexpr const char *ParamNames[] = {"param", "response"};
  using HttpFunction::HttpFunction;
  Expect<void>
  body(Runtime::Component::CallingFrame &,
       Runtime::Component::Own<ResponseOutparam> Param,
       HttpResult<Runtime::Component::Own<OutgoingResponse>> Response);
};

/// `[method]incoming-response.status`
class IncomingResponseStatus : public HttpFunction<IncomingResponseStatus> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  using HttpFunction::HttpFunction;
  Expect<uint16_t> body(Runtime::Component::CallingFrame &,
                        IncomingResponseBorrow Self);
};

/// `[method]incoming-response.headers`
class IncomingResponseHeaders : public HttpFunction<IncomingResponseHeaders> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  using HttpFunction::HttpFunction;
  Expect<Runtime::Component::Own<Fields>>
  body(Runtime::Component::CallingFrame &, IncomingResponseBorrow Self);
};

/// `[method]incoming-response.consume`
class IncomingResponseConsume : public HttpFunction<IncomingResponseConsume> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  using HttpFunction::HttpFunction;
  Expect<
      Expected<Runtime::Component::Own<IncomingBody>, Runtime::Component::Unit>>
  body(Runtime::Component::CallingFrame &, IncomingResponseBorrow Self);
};

/// `[method]incoming-body.stream`
class IncomingBodyStream : public HttpFunction<IncomingBodyStream> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  using HttpFunction::HttpFunction;
  Expect<
      Expected<Runtime::Component::Own<InputStream>, Runtime::Component::Unit>>
  body(Runtime::Component::CallingFrame &, IncomingBodyBorrow Self);
};

/// `[static]incoming-body.finish`
class IncomingBodyFinish : public HttpFunction<IncomingBodyFinish> {
public:
  static constexpr const char *ParamNames[] = {"this"};
  using HttpFunction::HttpFunction;
  Expect<Runtime::Component::Own<FutureTrailers>>
  body(Runtime::Component::CallingFrame &,
       Runtime::Component::Own<IncomingBody> This);
};

/// `[method]future-trailers.subscribe`
class FutureTrailersSubscribe : public HttpFunction<FutureTrailersSubscribe> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  using HttpFunction::HttpFunction;
  Expect<Runtime::Component::Own<PollSource>>
  body(Runtime::Component::CallingFrame &, TrailersBorrow Self);
};

/// `[method]future-trailers.get`
class FutureTrailersGet : public HttpFunction<FutureTrailersGet> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  using HttpFunction::HttpFunction;
  using Result = std::optional<
      Expected<HttpResult<std::optional<Runtime::Component::Own<Fields>>>,
               Runtime::Component::Unit>>;
  Expect<Result> body(Runtime::Component::CallingFrame &, TrailersBorrow Self);
};

/// `[constructor]outgoing-response`
class OutgoingResponseNew : public HttpFunction<OutgoingResponseNew> {
public:
  static constexpr const char *ParamNames[] = {"headers"};
  using HttpFunction::HttpFunction;
  Expect<Runtime::Component::Own<OutgoingResponse>>
  body(Runtime::Component::CallingFrame &,
       Runtime::Component::Own<Fields> Headers);
};

/// `[method]outgoing-response.status-code`
class OutgoingResponseStatus : public HttpFunction<OutgoingResponseStatus> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  using HttpFunction::HttpFunction;
  Expect<uint16_t> body(Runtime::Component::CallingFrame &,
                        OutgoingResponseBorrow Self);
};

/// `[method]outgoing-response.set-status-code`
class OutgoingResponseSetStatus
    : public HttpFunction<OutgoingResponseSetStatus> {
public:
  static constexpr const char *ParamNames[] = {"self", "status-code"};
  using HttpFunction::HttpFunction;
  Expect<Expected<Runtime::Component::Unit, Runtime::Component::Unit>>
  body(Runtime::Component::CallingFrame &, OutgoingResponseBorrow Self,
       uint16_t Status);
};

/// `[method]outgoing-response.headers`
class OutgoingResponseHeaders : public HttpFunction<OutgoingResponseHeaders> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  using HttpFunction::HttpFunction;
  Expect<Runtime::Component::Own<Fields>>
  body(Runtime::Component::CallingFrame &, OutgoingResponseBorrow Self);
};

/// `[method]outgoing-response.body`
class OutgoingResponseBody : public HttpFunction<OutgoingResponseBody> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  using HttpFunction::HttpFunction;
  Expect<
      Expected<Runtime::Component::Own<OutgoingBody>, Runtime::Component::Unit>>
  body(Runtime::Component::CallingFrame &, OutgoingResponseBorrow Self);
};

/// `[method]outgoing-body.write`
class OutgoingBodyWrite : public HttpFunction<OutgoingBodyWrite> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  using HttpFunction::HttpFunction;
  Expect<
      Expected<Runtime::Component::Own<OutputStream>, Runtime::Component::Unit>>
  body(Runtime::Component::CallingFrame &, OutgoingBodyBorrow Self);
};

/// `[static]outgoing-body.finish`
class OutgoingBodyFinish : public HttpFunction<OutgoingBodyFinish> {
public:
  static constexpr const char *ParamNames[] = {"this", "trailers"};
  using HttpFunction::HttpFunction;
  Expect<HttpStatus>
  body(Runtime::Component::CallingFrame &,
       Runtime::Component::Own<OutgoingBody> This,
       std::optional<Runtime::Component::Own<Fields>> Trailers);
};

/// `[method]future-incoming-response.subscribe`
class FutureResponseSubscribe : public HttpFunction<FutureResponseSubscribe> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  using HttpFunction::HttpFunction;
  Expect<Runtime::Component::Own<PollSource>>
  body(Runtime::Component::CallingFrame &, FutureResponseBorrow Self);
};

/// `[method]future-incoming-response.get`
class FutureResponseGet : public HttpFunction<FutureResponseGet> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  using HttpFunction::HttpFunction;
  using Result = std::optional<
      Expected<HttpResult<Runtime::Component::Own<IncomingResponse>>,
               Runtime::Component::Unit>>;
  Expect<Result> body(Runtime::Component::CallingFrame &,
                      FutureResponseBorrow Self);
};

/// `http-error-code: func(err: borrow<io-error>) -> option<error-code>`
class HttpErrorCode : public HttpFunction<HttpErrorCode> {
public:
  static constexpr const char *ParamNames[] = {"err"};
  using HttpFunction::HttpFunction;
  Expect<std::optional<HttpError>> body(Runtime::Component::CallingFrame &,
                                        ErrorBorrow Err);
};

/// `outgoing-handler.handle`
class OutgoingHandle : public HttpFunction<OutgoingHandle> {
public:
  static constexpr const char *ParamNames[] = {"request", "options"};
  using HttpFunction::HttpFunction;
  Expect<HttpResult<Runtime::Component::Own<FutureIncomingResponse>>>
  body(Runtime::Component::CallingFrame &,
       Runtime::Component::Own<OutgoingRequest> Request,
       std::optional<Runtime::Component::Own<RequestOptions>> Options);
};

/// `wasi:http/types@0.2.12`
class HttpTypesInstance : public Runtime::Instance::ComponentInstance {
public:
  HttpTypesInstance(HttpHost &Http);
};

/// `wasi:http/outgoing-handler@0.2.12`
class OutgoingHandlerInstance : public Runtime::Instance::ComponentInstance {
public:
  OutgoingHandlerInstance(HttpHost &Http);
};

/// The two instances of `wasi:http@0.2.12`, in order.
std::vector<std::unique_ptr<Runtime::Instance::ComponentInstance>>
makeHttpInstances(HttpHost &Http);

} // namespace WasiP2
} // namespace Host
} // namespace WasmEdge
