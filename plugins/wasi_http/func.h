// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "base.h"
#include "runtime/callingframe.h"

namespace WasmEdge {
namespace Host {

class WasiHttpPrint : public WasiHttp<WasiHttpPrint> {
public:
  WasiHttpPrint(WasiHttpEnvironment &HostEnv) : WasiHttp(HostEnv) {}
  Expect<void> body(std::string Str);
};

class WasiHttpGet : public WasiHttp<WasiHttpGet> {
public:
  WasiHttpGet(WasiHttpEnvironment &HostEnv) : WasiHttp(HostEnv) {}
  Expect<std::string> body(std::string URI);
};

namespace Types {

namespace Method {
using T = Component::Variant<Tuple<>,    // get
                             Tuple<>,    // head
                             Tuple<>,    // post
                             Tuple<>,    // put
                             Tuple<>,    // delete
                             Tuple<>,    // connect
                             Tuple<>,    // options
                             Tuple<>,    // trace
                             Tuple<>,    // patch
                             std::string // other(string)
                             >;

AST::Component::VariantTy ast() noexcept;
} // namespace Method

namespace Scheme {
using T = Component::Variant<Tuple<>,    // HTTP
                             Tuple<>,    // HTTPS
                             std::string // other(string)
                             >;
AST::Component::VariantTy ast() noexcept;
} // namespace Scheme

namespace DNSErrorPayload {
using T = Record<Option<std::string>, Option<uint16_t>>;

AST::Component::RecordTy
ast(Runtime::Instance::ComponentInstance *Comp) noexcept;
} // namespace DNSErrorPayload

namespace TLSAlertReceivedPayload {
using T = Record<Option<uint8_t>, Option<std::string>>;

AST::Component::RecordTy
ast(Runtime::Instance::ComponentInstance *Comp) noexcept;
}; // namespace TLSAlertReceivedPayload

namespace FieldSizePayload {
using T = Record<Option<std::string>, Option<uint32_t>>;

AST::Component::RecordTy
ast(Runtime::Instance::ComponentInstance *Comp) noexcept;
}; // namespace FieldSizePayload

namespace ErrorCode {
using T =
    Component::Variant<Tuple<>, DNSErrorPayload::T, Tuple<>, Tuple<>, Tuple<>,
                       Tuple<>, Tuple<>, Tuple<>, Tuple<>, Tuple<>, Tuple<>,
                       Tuple<>, Tuple<>, Tuple<>, TLSAlertReceivedPayload::T,
                       Tuple<>, Tuple<>, Option<uint64_t>, Tuple<>, Tuple<>,
                       Tuple<>, Option<uint32_t>, Option<FieldSizePayload::T>,
                       Tuple<>, Option<uint32_t>, Option<FieldSizePayload::T>,
                       Option<std::string>, Option<std::string>, Tuple<>,
                       Tuple<>, Tuple<>, Tuple<>, Tuple<>, Option<std::string>>;

AST::Component::VariantTy
ast(Runtime::Instance::ComponentInstance *Comp,
    AST::Component::TypeIndex DNSErrorPayload,
    AST::Component::TypeIndex TLSAlterReceivedPayload,
    AST::Component::TypeIndex FieldSizePayload) noexcept;
} // namespace ErrorCode

// http-error-code: func(err: borrow<io-error>) -> option<error-code>;
class HttpErrorCode : public WasiHttp<HttpErrorCode> {
public:
  HttpErrorCode(WasiHttpEnvironment &HostEnv) : WasiHttp(HostEnv) {}
  // TODO: http-error-code: func(err: borrow<io-error>) -> option<error-code>;
  // The input is borrow<io-error> and io-error is imported from wasi:io/error
  Expect<Option<ErrorCode::T>> body(uint32_t Err);
};

namespace HeaderError {
using T = Component::Variant<Tuple<>, Tuple<>, Tuple<>>;

AST::Component::VariantTy ast() noexcept;
} // namespace HeaderError

namespace FieldKey {
using T = std::string;
AST::Component::PrimValType ast() noexcept;
} // namespace FieldKey

namespace FieldValue {
using T = List<uint8_t>;
AST::Component::ListTy ast() noexcept;
} // namespace FieldValue

namespace Fields {
using T = uint32_t;
AST::Component::ResourceType ast() noexcept;
} // namespace Fields

namespace Headers {
using T = Fields::T;
AST::Component::ResourceType ast() noexcept;
} // namespace Headers

namespace Trailers {
using T = Fields::T;
AST::Component::ResourceType ast() noexcept;
} // namespace Trailers

namespace IncomingRequest {
using T = uint32_t;
AST::Component::ResourceType ast() noexcept;
} // namespace IncomingRequest

namespace OutgoingRequest {
using T = uint32_t;
AST::Component::ResourceType ast() noexcept;
} // namespace OutgoingRequest

namespace RequestOptions {
using T = uint32_t;
AST::Component::ResourceType ast() noexcept;
} // namespace RequestOptions

namespace ResponseOutparam {
using T = uint32_t;
AST::Component::ResourceType ast() noexcept;
} // namespace ResponseOutparam

using StatusCode = uint16_t;

namespace IncomingResponse {
using T = uint32_t;
AST::Component::ResourceType ast() noexcept;
} // namespace IncomingResponse

namespace IncomingBody {
using T = uint32_t;
AST::Component::ResourceType ast() noexcept;
} // namespace IncomingBody

namespace FutureTrailers {
using T = uint32_t;
AST::Component::ResourceType ast() noexcept;
} // namespace FutureTrailers

namespace OutgoingResponse {
using T = uint32_t;
AST::Component::ResourceType ast() noexcept;
} // namespace OutgoingResponse

namespace OutgoingBody {
using T = uint32_t;
AST::Component::ResourceType ast() noexcept;
} // namespace OutgoingBody

namespace FutureIncomingResponse {
using T = uint32_t;
AST::Component::ResourceType ast() noexcept;
} // namespace FutureIncomingResponse

} // namespace Types

} // namespace Host
} // namespace WasmEdge
