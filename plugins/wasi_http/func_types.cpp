// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "common/defines.h"
#include "common/errcode.h"
#include "func.h"

namespace WasmEdge {
namespace Host {

namespace Types {

using namespace AST::Component;

VariantTy Method::ast() noexcept {
  return VariantTy{
      Case("get"),     Case("head"),
      Case("post"),    Case("put"),
      Case("delete"),  Case("connect"),
      Case("options"), Case("trace"),
      Case("patch"),   Case("other", PrimValType::String),
  };
}

VariantTy Scheme::ast() noexcept {
  return VariantTy{
      Case("HTTP"),
      Case("HTTPS"),
      Case("other", AST::Component::PrimValType::String),
  };
}

RecordTy
DNSErrorPayload::ast(Runtime::Instance::ComponentInstance *Comp) noexcept {
  return RecordTy{
      LabelValType("rcode", Comp->typeToIndex(OptionTy(PrimValType::String))),
      LabelValType("info-code", Comp->typeToIndex(OptionTy(PrimValType::U16)))};
}

RecordTy TLSAlertReceivedPayload::ast(
    Runtime::Instance::ComponentInstance *Comp) noexcept {
  return RecordTy{
      LabelValType("alert-id", Comp->typeToIndex(OptionTy(PrimValType::U8))),
      LabelValType("alert-message",
                   Comp->typeToIndex(OptionTy(PrimValType::String)))};
}

RecordTy
FieldSizePayload::ast(Runtime::Instance::ComponentInstance *Comp) noexcept {
  return RecordTy{LabelValType("field-name", Comp->typeToIndex(OptionTy(
                                                 PrimValType::String))),
                  LabelValType("field-size",
                               Comp->typeToIndex(OptionTy(PrimValType::U32)))};
}

VariantTy ErrorCode::ast(Runtime::Instance::ComponentInstance *Comp,
                         TypeIndex DNSErrorPayload,
                         TypeIndex TLSAlterReceivedPayload,
                         TypeIndex FieldSizePayload) noexcept {
  return VariantTy{
      Case("DNS-timeout"), Case("DNS-error", DNSErrorPayload),
      Case("destination-not-found"), Case("destination-unavailable"),
      Case("destination-IP-prohibited"), Case("destination-IP-unroutable"),
      Case("connection-refused"), Case("connection-terminated"),
      Case("connection-timeout"), Case("connection-read-timeout"),
      Case("connection-write-timeout"), Case("connection-limit-reached"),
      Case("TLS-protocol-error"), Case("TLS-certificate-error"),
      Case("TLS-alert-received", TLSAlterReceivedPayload),
      Case("HTTP-request-denied"), Case("HTTP-request-length-required"),
      Case("HTTP-request-body-size",
           Comp->typeToIndex(OptionTy(PrimValType::U64))),
      Case("HTTP-request-method-invalid"), Case("HTTP-request-URI-invalid"),
      Case("HTTP-request-URI-too-long"),
      Case("HTTP-request-header-section-size",
           Comp->typeToIndex(OptionTy(PrimValType::U32))),
      Case("HTTP-request-header-size",
           Comp->typeToIndex(OptionTy(FieldSizePayload))),
      Case("HTTP-request-trailer-section-size",
           Comp->typeToIndex(OptionTy(PrimValType::U32))),
      Case("HTTP-request-trailer-size", FieldSizePayload),
      Case("HTTP-response-incomplete"),
      Case("HTTP-response-header-section-size",
           Comp->typeToIndex(OptionTy(PrimValType::U32))),
      Case("HTTP-response-header-size", FieldSizePayload),
      Case("HTTP-response-body-size",
           Comp->typeToIndex(OptionTy(PrimValType::U64))),
      Case("HTTP-response-trailer-section-size",
           Comp->typeToIndex(OptionTy(PrimValType::U32))),
      Case("HTTP-response-trailer-size", FieldSizePayload),
      Case("HTTP-response-transfer-coding",
           Comp->typeToIndex(OptionTy(PrimValType::String))),
      Case("HTTP-response-content-coding",
           Comp->typeToIndex(OptionTy(PrimValType::String))),
      Case("HTTP-response-timeout"), Case("HTTP-upgrade-failed"),
      Case("HTTP-protocol-error"), Case("loop-detected"),
      Case("configuration-error"),
      /// This is a catch-all error for anything that doesn't fit cleanly
      /// into a more specific case. It also includes an optional string
      /// for an unstructured description of the error. Users should not
      /// depend on the string for diagnosing errors, as it's not required
      /// to be consistent between implementations.
      Case("internal-error", Comp->typeToIndex(OptionTy(PrimValType::String)))};
}

Expect<Option<ErrorCode::T>> HttpErrorCode::body(uint32_t) {
  return Option<ErrorCode::T>();
}

VariantTy HeaderError::ast() noexcept {
  return VariantTy{
      Case("invalid-syntax"),
      Case("forbidden"),
      Case("immutable"),
  };
}

PrimValType FieldKey::ast() noexcept { return PrimValType::String; }

ListTy FieldValue::ast() noexcept { return ListTy(PrimValType::U8); }

namespace Fields {

AST::Component::ResourceType ast() noexcept {
  return ResourceType(new Runtime::Instance::ComponentInstance("fields"));
}

Expect<T> Constructor::body() { return 0; }

} // namespace Fields

AST::Component::ResourceType Headers::ast() noexcept { return Fields::ast(); }
AST::Component::ResourceType Trailers::ast() noexcept { return Fields::ast(); }

AST::Component::ResourceType IncomingRequest::ast() noexcept {
  return ResourceType(
      new Runtime::Instance::ComponentInstance("incoming-request"));
}

AST::Component::ResourceType OutgoingRequest::ast() noexcept {
  return ResourceType(
      new Runtime::Instance::ComponentInstance("outgoing-request"));
}

AST::Component::ResourceType RequestOptions::ast() noexcept {
  return ResourceType(
      new Runtime::Instance::ComponentInstance("request-options"));
}

namespace ResponseOutparam {

AST::Component::ResourceType ast() noexcept {
  return ResourceType(
      new Runtime::Instance::ComponentInstance("response-outparam"));
}

/// Set the value of the `response-outparam` to either send a response,
/// or indicate an error.
///
/// This method consumes the `response-outparam` to ensure that it is
/// called at most once. If it is never called, the implementation
/// will respond with an error.
///
/// The user may provide an `error` to `response` to allow the
/// implementation determine how to respond with an HTTP error response.
Expect<void> Set::body(T Param, Result<OutgoingResponse::T, ErrorCode::T>) {
  auto Inst = Env.getComponentInstance();
  /* auto ParamHandle = */ Inst->getResource(Param);
  // TODO:
  // 1. set `Response` as its value
  return {};
}

} // namespace ResponseOutparam
namespace IncomingResponse {

AST::Component::ResourceType ast() noexcept {
  return ResourceType(
      new Runtime::Instance::ComponentInstance("incoming-response"));
}

Expect<StatusCode> Status::body() {
  // TODO: find out how to use `uint32_t` to find out current status
  return 200;
}
Expect<Headers::T> MethodHeaders::body() {
  // TODO: find out how to return a proper value
  return 0;
}

} // namespace IncomingResponse

AST::Component::ResourceType IncomingBody::ast() noexcept {
  return ResourceType(
      new Runtime::Instance::ComponentInstance("incoming-body"));
}

AST::Component::ResourceType FutureTrailers::ast() noexcept {
  return ResourceType(
      new Runtime::Instance::ComponentInstance("future-trailers"));
}

namespace OutgoingResponse {

AST::Component::ResourceType ast() noexcept {
  return ResourceType(
      new Runtime::Instance::ComponentInstance("outgoing-response"));
}

Expect<T> Constructor::body(int32_t) { return 0; }

Expect<Result<OutgoingBody::T, Tuple<>>> Body::body(T) { return 0; }

} // namespace OutgoingResponse

namespace OutgoingBody {

AST::Component::ResourceType ast() noexcept {
  return ResourceType(
      new Runtime::Instance::ComponentInstance("outgoing-body"));
}

Expect<Result<OutputStream::T, Tuple<>>> Write::body(T) { return 0; }

Expect<Result<Tuple<>, ErrorCode::T>> Finish::body(T, Option<Trailers::T>) {
  return Tuple();
}

} // namespace OutgoingBody

AST::Component::ResourceType FutureIncomingResponse::ast() noexcept {
  return ResourceType(
      new Runtime::Instance::ComponentInstance("future-incoming-response"));
}

} // namespace Types

} // namespace Host
} // namespace WasmEdge
