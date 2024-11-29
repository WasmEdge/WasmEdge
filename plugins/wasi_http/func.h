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

class Method : public Component::Variant<Tuple<>,    // get
                                         Tuple<>,    // head
                                         Tuple<>,    // post
                                         Tuple<>,    // put
                                         Tuple<>,    // delete
                                         Tuple<>,    // connect
                                         Tuple<>,    // options
                                         Tuple<>,    // trace
                                         Tuple<>,    // patch
                                         std::string // other(string)
                                         >,
               public AST::Component::VariantTy {
  Method()
      : AST::Component::VariantTy{
            Case("get"),     Case("head"),
            Case("post"),    Case("put"),
            Case("delete"),  Case("connect"),
            Case("options"), Case("trace"),
            Case("patch"),   Case("other", PrimValType::String),
        } {}
};

class Scheme : public Component::Variant<Tuple<>,    // HTTP
                                         Tuple<>,    // HTTPS
                                         std::string // other(string)
                                         >,
               public AST::Component::VariantTy {
  Scheme()
      : AST::Component::VariantTy{
            Case("HTTP"),
            Case("HTTPS"),
            Case("other", PrimValType::String),
        } {}
};

class DNSErrorPayload : public Record<Option<std::string>, Option<u16>>,
                        RecordTy {
  DNSErrorPayload()
      : RecordTy{LabelValType("rcode", Option<std::string>),
                 LabelValType("info-code", Option<u16>)} {}
};

class TLSAlertReceivedPayload : public Record<Option<u8>, Option<std::string>>,
                                RecordTy {
  TLSAlertReceivedPayload()
      : RecordTy{LabelValType("alert-id", Option<u8>),
                 LabelValType("alert-message", Option<std::string>)} {}
};

class FieldSizePayload : public Record<Option<std::string>, Option<u32>>,
                         RecordTy {
  FieldSizePayload()
      : RecordTy{LabelValType("field-name", Option<std::string>),
                 LabelValType("field-size", Option<u32>)} {}
};

class ErrorCode : public Component::Variant, public AST::Component::VariantTy {
  Scheme()
      : AST::Component::VariantTy{
            Case("DNS-timeout"), Case("DNS-error", DNSErrorPayload()),
            Case("destination-not-found"), Case("destination-unavailable"),
            Case("destination-IP-prohibited"),
            Case("destination-IP-unroutable"), Case("connection-refused"),
            Case("connection-terminated"), Case("connection-timeout"),
            Case("connection-read-timeout"), Case("connection-write-timeout"),
            Case("connection-limit-reached"), Case("TLS-protocol-error"),
            Case("TLS-certificate-error"),
            Case("TLS-alert-received", TLSAlertReceivedPayload()),
            Case("HTTP-request-denied"), Case("HTTP-request-length-required"),
            Case("HTTP-request-body-size", OptionTy(PrimValType::U64)),
            Case("HTTP-request-method-invalid"),
            Case("HTTP-request-URI-invalid"), Case("HTTP-request-URI-too-long"),
            Case("HTTP-request-header-section-size",
                 OptionTy(PrimValType::U32)),
            Case("HTTP-request-header-size", OptionTy(FieldSizePayload())),
            Case("HTTP-request-trailer-section-size",
                 OptionTy(PrimValType::U32)),
            Case("HTTP-request-trailer-size", FieldSizePayload),
            Case("HTTP-response-incomplete"),
            Case("HTTP-response-header-section-size",
                 OptionTy(PrimValType::U32)),
            Case("HTTP-response-header-size",
                 FieldSizePayload(OptionTy(PrimValType::U32))),
            Case("HTTP-response-body-size", OptionTy(PrimValType::U64)),
            Case("HTTP-response-trailer-section-size",
                 OptionTy(PrimValType::U32)),
            Case("HTTP-response-trailer-size", FieldSizePayload()),
            Case("HTTP-response-transfer-coding",
                 OptionTy(PrimValType::String)),
            Case("HTTP-response-content-coding", OptionTy(PrimValType::String)),
            Case("HTTP-response-timeout"), Case("HTTP-upgrade-failed"),
            Case("HTTP-protocol-error"), Case("loop-detected"),
            Case("configuration-error"),
            /// This is a catch-all error for anything that doesn't fit cleanly
            /// into a more specific case. It also includes an optional string
            /// for an unstructured description of the error. Users should not
            /// depend on the string for diagnosing errors, as it's not required
            /// to be consistent between implementations.
            Case("internal-error", OptionTy(PrimValType::String))} {}
};

// http-error-code: func(err: borrow<io-error>) -> option<error-code>;
class HttpErrorCode : public WasiHttp<HttpErrorCode> {
public:
  HttpErrorCode(WasiHttpEnvironment &HostEnv) : WasiHttp(HostEnv) {}
  Expect<Option<ErrorCode>> body(uint32_t Err);
};

} // namespace Types

} // namespace Host
} // namespace WasmEdge
