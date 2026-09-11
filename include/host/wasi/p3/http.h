// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/host/wasi/p3/http.h - wasi:http host ---------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the host component instances of `wasi:http@0.3.1`:
/// `types` with the `fields`, `request`, `request-options`, and `response`
/// resources whose bodies are streams and whose trailers are futures, and
/// `client`, whose `send` reads the request body, exchanges it in HTTP/1.1
/// over a preview-1 socket, and answers with a response whose body the
/// guest consumes as a stream. There is no TLS.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/component_valtype.h"
#include "common/component_variant.h"
#include "common/errcode.h"
#include "common/expected.h"
#include "host/wasi/p2/http.h"
#include "runtime/component/callingframe.h"
#include "runtime/component/hostfunc.h"
#include "runtime/component/hosttransmit.h"
#include "runtime/component/resourcetable.h"
#include "runtime/component/wit.h"
#include "runtime/instance/component/component.h"
#include "runtime/instance/component/resource.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace WasiP3 {

using WasiP2::FieldEntry;
using WasiP2::Fields;
using WasiP2::HttpError;
using WasiP2::HttpResult;
using WasiP2::HttpStatus;
using WasiP2::Method;
using WasiP2::Scheme;

/// `header-error` of 0.3: the 0.2 cases, `size-exceeded`, and `other`.
struct HeaderError {
  enum Kind : uint32_t {
    InvalidSyntax,
    Forbidden,
    Immutable,
    SizeExceeded,
    Other
  };
  Kind Case = Other;
  std::optional<std::string> Text;
};

/// `request-options-error`
struct RequestOptionsError {
  enum Kind : uint32_t { NotSupported, Immutable, Other };
  Kind Case = Other;
  std::optional<std::string> Text;
};

using HeaderStatus = Expected<Runtime::Component::Unit, HeaderError>;
using OptionsStatus = Expected<Runtime::Component::Unit, RequestOptionsError>;
/// The value of a trailers future: the trailers, none, or the error.
using TrailersResult =
    HttpResult<std::optional<Runtime::Component::Own<Fields>>>;

/// `request-options`, immutable once attached to a request.
struct RequestOptions {
  std::optional<uint64_t> ConnectTimeout;
  std::optional<uint64_t> FirstByteTimeout;
  std::optional<uint64_t> BetweenBytesTimeout;
  bool Immutable = false;
};

/// A message body: the stream and the trailers future a guest gave, or the
/// bytes and trailers the host read; and the transmit-result future the
/// host resolves once the body went out.
struct Body {
  std::shared_ptr<void> Contents;
  std::shared_ptr<void> TrailersEnd;
  std::vector<uint8_t> Data;
  std::shared_ptr<Fields> Trailers;
  bool FromHost = false;
  bool Consumed = false;
  std::shared_ptr<Runtime::Component::HostTransmitEnd> Done;
};

struct Request {
  Method M;
  std::optional<std::string> PathWithQuery;
  std::optional<Scheme> S;
  std::optional<std::string> Authority;
  std::shared_ptr<RequestOptions> Options;
  std::shared_ptr<Fields> Headers;
  std::shared_ptr<Body> Content;
};

struct Response {
  uint16_t Status = 200;
  std::shared_ptr<Fields> Headers;
  std::shared_ptr<Body> Content;
};

using BodyEnds = std::tuple<Runtime::Component::Stream<uint8_t>,
                            Runtime::Component::Future<TrailersResult>>;

/// The tables and the element types of the http resources, shared by the
/// instances of the package.
struct HttpHost {
  Runtime::Component::ResourceTable<Fields> FieldsTable;
  Runtime::Component::ResourceTable<Request> Requests;
  Runtime::Component::ResourceTable<RequestOptions> Options;
  Runtime::Component::ResourceTable<Response> Responses;
  const Runtime::Instance::Component::ResourceTypeInstance *RequestType =
      nullptr;
  const Runtime::Instance::Component::ResourceTypeInstance *ResponseType =
      nullptr;
  /// `result<_, error-code>` and `result<option<trailers>, error-code>` in
  /// the types instance, the elements of the futures it hands out.
  ComponentValType StatusType;
  ComponentValType TrailersType;
  const Runtime::Instance::ComponentInstance *TypesInstance = nullptr;

  /// The body a guest builds a request or response from.
  std::shared_ptr<Body>
  newBody(Runtime::Component::CallingFrame &Frame,
          std::optional<Runtime::Component::Stream<uint8_t>> &Contents,
          Runtime::Component::Future<TrailersResult> &Trailers,
          Runtime::Component::Future<HttpStatus> &Done);
  /// The ends a consumer reads a body through. A guest body passes
  /// through; a host body is written by a detached task.
  Expect<BodyEnds> consume(Runtime::Component::CallingFrame &Frame,
                           const std::shared_ptr<Body> &B,
                           Runtime::Component::Future<HttpStatus> &Res);
};

using FieldsBorrow = Runtime::Component::Borrow<Fields>;
using RequestBorrow = Runtime::Component::Borrow<Request>;
using OptionsBorrow = Runtime::Component::Borrow<RequestOptions>;
using ResponseBorrow = Runtime::Component::Borrow<Response>;
} // namespace WasiP3
} // namespace Host

namespace Runtime {
namespace Component {

template <> struct Wit<Host::WasiP3::HeaderError> {
  static ComponentValType type(const TypeMinter &Mint) noexcept {
    return WitVariant::type(
        Mint, {{"invalid-syntax", std::nullopt},
               {"forbidden", std::nullopt},
               {"immutable", std::nullopt},
               {"size-exceeded", std::nullopt},
               {"other", Wit<std::optional<std::string>>::type(Mint)}});
  }
  static Host::WasiP3::HeaderError from(const ComponentValVariant &V) {
    const auto &Val = WitVariant::value(V);
    Host::WasiP3::HeaderError E;
    E.Case = static_cast<Host::WasiP3::HeaderError::Kind>(Val.Case);
    if (E.Case == Host::WasiP3::HeaderError::Other && Val.Payload) {
      E.Text = Wit<std::optional<std::string>>::from(*Val.Payload);
    }
    return E;
  }
  static ComponentValVariant into(Host::WasiP3::HeaderError &&E) noexcept {
    if (E.Case == Host::WasiP3::HeaderError::Other) {
      return WitVariant::into(
          E.Case, Wit<std::optional<std::string>>::into(std::move(E.Text)));
    }
    return WitVariant::into(E.Case);
  }
};

template <> struct Wit<Host::WasiP3::RequestOptionsError> {
  static ComponentValType type(const TypeMinter &Mint) noexcept {
    return WitVariant::type(
        Mint, {{"not-supported", std::nullopt},
               {"immutable", std::nullopt},
               {"other", Wit<std::optional<std::string>>::type(Mint)}});
  }
  static Host::WasiP3::RequestOptionsError from(const ComponentValVariant &V) {
    const auto &Val = WitVariant::value(V);
    Host::WasiP3::RequestOptionsError E;
    E.Case = static_cast<Host::WasiP3::RequestOptionsError::Kind>(Val.Case);
    if (E.Case == Host::WasiP3::RequestOptionsError::Other && Val.Payload) {
      E.Text = Wit<std::optional<std::string>>::from(*Val.Payload);
    }
    return E;
  }
  static ComponentValVariant
  into(Host::WasiP3::RequestOptionsError &&E) noexcept {
    if (E.Case == Host::WasiP3::RequestOptionsError::Other) {
      return WitVariant::into(
          E.Case, Wit<std::optional<std::string>>::into(std::move(E.Text)));
    }
    return WitVariant::into(E.Case);
  }
};

} // namespace Component
} // namespace Runtime

namespace Host {
namespace WasiP3 {

/// The base of the http functions: the tables they resolve in.
template <typename T>
class HttpFunction : public Runtime::Component::HostFunction<T> {
public:
  HttpFunction(HttpHost &H, bool Async = false) noexcept
      : Runtime::Component::HostFunction<T>(Async), Http(H) {}

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

/// `[method]fields.get-and-delete`
class FieldsGetAndDelete : public HttpFunction<FieldsGetAndDelete> {
public:
  static constexpr const char *ParamNames[] = {"self", "name"};
  using HttpFunction::HttpFunction;
  Expect<Expected<std::vector<std::vector<uint8_t>>, HeaderError>>
  body(Runtime::Component::CallingFrame &, FieldsBorrow Self, std::string Name);
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

/// `[method]fields.copy-all`
class FieldsCopyAll : public HttpFunction<FieldsCopyAll> {
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

/// `[static]request.new: func(headers: headers, contents:
/// option<stream<u8>>, trailers: future<result<option<trailers>,
/// error-code>>, options: option<request-options>) -> tuple<request,
/// future<result<_, error-code>>>`
class RequestNew : public HttpFunction<RequestNew> {
public:
  static constexpr const char *ParamNames[] = {"headers", "contents",
                                               "trailers", "options"};
  using HttpFunction::HttpFunction;
  Expect<std::tuple<Runtime::Component::Own<Request>,
                    Runtime::Component::Future<HttpStatus>>>
  body(Runtime::Component::CallingFrame &Frame,
       Runtime::Component::Own<Fields> Headers,
       std::optional<Runtime::Component::Stream<uint8_t>> Contents,
       Runtime::Component::Future<TrailersResult> Trailers,
       std::optional<Runtime::Component::Own<RequestOptions>> Options);
};

/// `[method]request.get-method`
class RequestGetMethod : public HttpFunction<RequestGetMethod> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  using HttpFunction::HttpFunction;
  Expect<Method> body(Runtime::Component::CallingFrame &, RequestBorrow Self);
};

/// `[method]request.set-method`
class RequestSetMethod : public HttpFunction<RequestSetMethod> {
public:
  static constexpr const char *ParamNames[] = {"self", "method"};
  using HttpFunction::HttpFunction;
  Expect<Expected<Runtime::Component::Unit, Runtime::Component::Unit>>
  body(Runtime::Component::CallingFrame &, RequestBorrow Self, Method M);
};

/// `[method]request.get-path-with-query` and `get-authority`
class RequestGetText : public HttpFunction<RequestGetText> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  RequestGetText(HttpHost &H, bool Authority) noexcept
      : HttpFunction(H), Authority(Authority) {}
  Expect<std::optional<std::string>> body(Runtime::Component::CallingFrame &,
                                          RequestBorrow Self);

private:
  bool Authority;
};

/// `[method]request.set-path-with-query` and `set-authority`
class RequestSetText : public HttpFunction<RequestSetText> {
public:
  static constexpr const char *ParamNames[] = {"self", "value"};
  RequestSetText(HttpHost &H, bool Authority) noexcept
      : HttpFunction(H), Authority(Authority) {}
  Expect<Expected<Runtime::Component::Unit, Runtime::Component::Unit>>
  body(Runtime::Component::CallingFrame &, RequestBorrow Self,
       std::optional<std::string> Value);

private:
  bool Authority;
};

/// `[method]request.get-scheme`
class RequestGetScheme : public HttpFunction<RequestGetScheme> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  using HttpFunction::HttpFunction;
  Expect<std::optional<Scheme>> body(Runtime::Component::CallingFrame &,
                                     RequestBorrow Self);
};

/// `[method]request.set-scheme`
class RequestSetScheme : public HttpFunction<RequestSetScheme> {
public:
  static constexpr const char *ParamNames[] = {"self", "scheme"};
  using HttpFunction::HttpFunction;
  Expect<Expected<Runtime::Component::Unit, Runtime::Component::Unit>>
  body(Runtime::Component::CallingFrame &, RequestBorrow Self,
       std::optional<Scheme> S);
};

/// `[method]request.get-options`
class RequestGetOptions : public HttpFunction<RequestGetOptions> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  using HttpFunction::HttpFunction;
  Expect<std::optional<Runtime::Component::Own<RequestOptions>>>
  body(Runtime::Component::CallingFrame &, RequestBorrow Self);
};

/// `[method]request.get-headers`
class RequestGetHeaders : public HttpFunction<RequestGetHeaders> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  using HttpFunction::HttpFunction;
  Expect<Runtime::Component::Own<Fields>>
  body(Runtime::Component::CallingFrame &, RequestBorrow Self);
};

/// `[static]request.consume-body: func(this: request, res:
/// future<result<_, error-code>>) -> tuple<stream<u8>,
/// future<result<option<trailers>, error-code>>>`
class RequestConsumeBody : public HttpFunction<RequestConsumeBody> {
public:
  static constexpr const char *ParamNames[] = {"this", "res"};
  using HttpFunction::HttpFunction;
  Expect<BodyEnds> body(Runtime::Component::CallingFrame &Frame,
                        Runtime::Component::Own<Request> This,
                        Runtime::Component::Future<HttpStatus> Res);
};

/// `[constructor]request-options`
class OptionsNew : public HttpFunction<OptionsNew> {
public:
  using HttpFunction::HttpFunction;
  Expect<Runtime::Component::Own<RequestOptions>>
  body(Runtime::Component::CallingFrame &);
};

/// The three timeout getters of `request-options`.
class OptionsGet : public HttpFunction<OptionsGet> {
public:
  enum class Which { Connect, FirstByte, BetweenBytes };
  static constexpr const char *ParamNames[] = {"self"};
  OptionsGet(HttpHost &H, Which W) noexcept : HttpFunction(H), W(W) {}
  Expect<std::optional<uint64_t>> body(Runtime::Component::CallingFrame &,
                                       OptionsBorrow Self);

private:
  Which W;
};

/// The three timeout setters of `request-options`.
class OptionsSet : public HttpFunction<OptionsSet> {
public:
  using Which = OptionsGet::Which;
  static constexpr const char *ParamNames[] = {"self", "duration"};
  OptionsSet(HttpHost &H, Which W) noexcept : HttpFunction(H), W(W) {}
  Expect<OptionsStatus> body(Runtime::Component::CallingFrame &,
                             OptionsBorrow Self,
                             std::optional<uint64_t> Duration);

private:
  Which W;
};

/// `[method]request-options.clone`
class OptionsClone : public HttpFunction<OptionsClone> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  using HttpFunction::HttpFunction;
  Expect<Runtime::Component::Own<RequestOptions>>
  body(Runtime::Component::CallingFrame &, OptionsBorrow Self);
};

/// `[static]response.new: func(headers: headers, contents:
/// option<stream<u8>>, trailers: future<result<option<trailers>,
/// error-code>>) -> tuple<response, future<result<_, error-code>>>`
class ResponseNew : public HttpFunction<ResponseNew> {
public:
  static constexpr const char *ParamNames[] = {"headers", "contents",
                                               "trailers"};
  using HttpFunction::HttpFunction;
  Expect<std::tuple<Runtime::Component::Own<Response>,
                    Runtime::Component::Future<HttpStatus>>>
  body(Runtime::Component::CallingFrame &Frame,
       Runtime::Component::Own<Fields> Headers,
       std::optional<Runtime::Component::Stream<uint8_t>> Contents,
       Runtime::Component::Future<TrailersResult> Trailers);
};

/// `[method]response.get-status-code`
class ResponseGetStatus : public HttpFunction<ResponseGetStatus> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  using HttpFunction::HttpFunction;
  Expect<uint16_t> body(Runtime::Component::CallingFrame &,
                        ResponseBorrow Self);
};

/// `[method]response.set-status-code`
class ResponseSetStatus : public HttpFunction<ResponseSetStatus> {
public:
  static constexpr const char *ParamNames[] = {"self", "status-code"};
  using HttpFunction::HttpFunction;
  Expect<Expected<Runtime::Component::Unit, Runtime::Component::Unit>>
  body(Runtime::Component::CallingFrame &, ResponseBorrow Self,
       uint16_t Status);
};

/// `[method]response.get-headers`
class ResponseGetHeaders : public HttpFunction<ResponseGetHeaders> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  using HttpFunction::HttpFunction;
  Expect<Runtime::Component::Own<Fields>>
  body(Runtime::Component::CallingFrame &, ResponseBorrow Self);
};

/// `[static]response.consume-body`
class ResponseConsumeBody : public HttpFunction<ResponseConsumeBody> {
public:
  static constexpr const char *ParamNames[] = {"this", "res"};
  using HttpFunction::HttpFunction;
  Expect<BodyEnds> body(Runtime::Component::CallingFrame &Frame,
                        Runtime::Component::Own<Response> This,
                        Runtime::Component::Future<HttpStatus> Res);
};

/// `client.send: async func(request: request) -> result<response,
/// error-code>`
class ClientSend : public HttpFunction<ClientSend> {
public:
  static constexpr const char *ParamNames[] = {"request"};
  ClientSend(HttpHost &H) noexcept : HttpFunction(H, true) {}
  Expect<HttpResult<Runtime::Component::Own<Response>>>
  body(Runtime::Component::CallingFrame &Frame,
       Runtime::Component::Own<Request> Req);
};

/// `wasi:http/types@0.3.1`
class HttpTypesInstance : public Runtime::Instance::ComponentInstance {
public:
  HttpTypesInstance(HttpHost &Http);
};

/// `wasi:http/client@0.3.1`
class ClientInstance : public Runtime::Instance::ComponentInstance {
public:
  ClientInstance(HttpHost &Http);
};

/// The two instances of `wasi:http@0.3.1`, in order.
std::vector<std::unique_ptr<Runtime::Instance::ComponentInstance>>
makeHttpInstances(HttpHost &Http);

} // namespace WasiP3
} // namespace Host
} // namespace WasmEdge
