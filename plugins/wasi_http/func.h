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
using T = Record<Option<std::string>, Option<u16>>;

AST::Component::RecordTy ast() noexcept;
} // namespace DNSErrorPayload

namespace TLSAlertReceivedPayload {
using T = Record<Option<u8>, Option<std::string>>;

AST::Component::RecordTy ast() noexcept;
}; // namespace TLSAlertReceivedPayload

namespace FieldSizePayload {
using T = Record<Option<std::string>, Option<u32>>;

AST::Component::RecordTy ast() noexcept;
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

AST::Component::VariantTy ast() noexcept;
} // namespace ErrorCode

// http-error-code: func(err: borrow<io-error>) -> option<error-code>;
class HttpErrorCode : public WasiHttp<HttpErrorCode> {
public:
  HttpErrorCode(WasiHttpEnvironment &HostEnv) : WasiHttp(HostEnv) {}
  // TODO: http-error-code: func(err: borrow<io-error>) -> option<error-code>;
  // The input is borrow<io-error> and io-error is imported from wasi:io/error
  Expect<Option<ErrorCode>> body(uint32_t Err);
};

namespace HeaderError {
using T = Component::Variant<Tuple<>, Tuple<>, Tuple<>>;

AST::Component::VariantTy ast() noexcept;
} // namespace HeaderError

} // namespace Types

} // namespace Host
} // namespace WasmEdge
