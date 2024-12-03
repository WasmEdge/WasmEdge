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

RecordTy DNSErrorPayload::ast() noexcept {
  return RecordTy{LabelValType("rcode", OptionTy(PrimValType::String)),
                  LabelValType("info-code", OptionTy(PrimValType::U16))};
}

RecordTy TLSAlertReceivedPayload::ast() noexcept {
  return RecordTy{LabelValType("alert-id", OptionTy(PrimValType::U8)),
                  LabelValType("alert-message", OptionTy(PrimValType::String))};
}

RecordTy FieldSizePayload::ast() noexcept {
  return RecordTy{LabelValType("field-name", OptionTy(PrimValType::String)),
                  LabelValType("field-size", OptionTy(PrimValType::U32))};
}

VariantTy ErrorCode::ast() noexcept {
  return VariantTy{
      Case("DNS-timeout"), Case("DNS-error", DNSErrorPayload()),
      Case("destination-not-found"), Case("destination-unavailable"),
      Case("destination-IP-prohibited"), Case("destination-IP-unroutable"),
      Case("connection-refused"), Case("connection-terminated"),
      Case("connection-timeout"), Case("connection-read-timeout"),
      Case("connection-write-timeout"), Case("connection-limit-reached"),
      Case("TLS-protocol-error"), Case("TLS-certificate-error"),
      // Case("TLS-alert-received", TLSAlertReceivedPayload::ast()),
      Case("HTTP-request-denied"), Case("HTTP-request-length-required"),
      // Case("HTTP-request-body-size", OptionTy(PrimValType::U64)),
      Case("HTTP-request-method-invalid"), Case("HTTP-request-URI-invalid"),
      Case("HTTP-request-URI-too-long"),
      // Case("HTTP-request-header-section-size", OptionTy(PrimValType::U32)),
      // Case("HTTP-request-header-size", OptionTy(FieldSizePayload())),
      // Case("HTTP-request-trailer-section-size", OptionTy(PrimValType::U32)),
      // Case("HTTP-request-trailer-size", FieldSizePayload::ast()),
      Case("HTTP-response-incomplete"),
      // Case("HTTP-response-header-section-size", OptionTy(PrimValType::U32)),
      // Case("HTTP-response-header-size",
      //      FieldSizePayload(OptionTy(PrimValType::U32))),
      // Case("HTTP-response-body-size", OptionTy(PrimValType::U64)),
      // Case("HTTP-response-trailer-section-size", OptionTy(PrimValType::U32)),
      // Case("HTTP-response-trailer-size", FieldSizePayload::ast()),
      // Case("HTTP-response-transfer-coding", OptionTy(PrimValType::String)),
      // Case("HTTP-response-content-coding", OptionTy(PrimValType::String)),
      Case("HTTP-response-timeout"), Case("HTTP-upgrade-failed"),
      Case("HTTP-protocol-error"), Case("loop-detected"),
      Case("configuration-error"),
      /// This is a catch-all error for anything that doesn't fit cleanly
      /// into a more specific case. It also includes an optional string
      /// for an unstructured description of the error. Users should not
      /// depend on the string for diagnosing errors, as it's not required
      /// to be consistent between implementations.
      Case("internal-error", OptionTy(PrimValType::String))};
}

Expect<Option<ErrorCode>> HttpErrorCode::body(uint32_t Err) {
  return std::nullopt;
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

} // namespace Types

} // namespace Host
} // namespace WasmEdge
