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

class ErrorCode : public Component::Variant,
                  public AST::Component::VariantTy {};

// http-error-code: func(err: borrow<io-error>) -> option<error-code>;
class HttpErrorCode : public WasiHttp<HttpErrorCode> {
public:
  HttpErrorCode(WasiHttpEnvironment &HostEnv) : WasiHttp(HostEnv) {}
  Expect<Option<ErrorCode>> body(uint32_t Err);
};

} // namespace Types

} // namespace Host
} // namespace WasmEdge
