// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "func.h"
#include "interface.h"

#include <memory>

namespace WasmEdge {
namespace Host {

WasiHttp_Types::WasiHttp_Types() : ComponentInstance("wasi:http/types@0.2.0") {
  Env.setComponentInstance(this);

  using namespace Types;
  using namespace AST::Component;
  addHostType("method", Method::ast());
  addHostType("scheme", Scheme::ast());
  addHostType("DNS-error-payload", DNSErrorPayload::ast(this));
  auto DNSErrorPayload = getLastTypeIndex();
  addHostType("TLS-alert-received-payload", TLSAlertReceivedPayload::ast(this));
  auto TLSAlterReceivedPayload = getLastTypeIndex();
  addHostType("field-size-payload", FieldSizePayload::ast(this));
  auto FieldSizePayload = getLastTypeIndex();
  addHostType("error-code",
              ErrorCode::ast(this, DNSErrorPayload, TLSAlterReceivedPayload,
                             FieldSizePayload));
  addHostFunc("http-error-code", std::make_unique<HttpErrorCode>(Env));
  addHostType("header-error", HeaderError::ast());
  addHostType("field-key", FieldKey::ast());
  addHostType("field-value", FieldValue::ast());

  addHostType("fields", Fields::ast());
  addHostFunc("[constructor]fields",
              std::make_unique<Fields::Constructor>(Env));

  addHostType("headers", Headers::ast());
  addHostType("trailers", Trailers::ast());
  addHostType("incoming-request", IncomingRequest::ast());
  addHostType("outgoing-request", OutgoingRequest::ast());
  addHostType("request-options", RequestOptions::ast());

  addHostType("response-outparam", ResponseOutparam::ast());
  addHostFunc("[static]response-outparam.set",
              std::make_unique<ResponseOutparam::Set>(Env));

  addHostType("status-code", PrimValType::U16);
  addHostType("incoming-response", IncomingResponse::ast());
  addHostFunc("[method]incoming-response.status",
              std::make_unique<IncomingResponse::Status>(Env));
  addHostFunc("[method]incoming-response.headers",
              std::make_unique<IncomingResponse::MethodHeaders>(Env));

  addHostType("incoming-body", IncomingBody::ast());
  addHostType("future-trailers", FutureTrailers::ast());

  addHostType("outgoing-response", OutgoingResponse::ast());
  addHostFunc("[constructor]outgoing-response",
              std::make_unique<OutgoingResponse::Constructor>(Env));
  addHostFunc("[method]outgoing-response.body",
              std::make_unique<OutgoingResponse::Body>(Env));

  addHostType("outgoing-body", OutgoingBody::ast());
  addHostFunc("[method]outgoing-body.write",
              std::make_unique<OutgoingBody::Write>(Env));
  addHostFunc("[static]outgoing-body.finish",
              std::make_unique<OutgoingBody::Finish>(Env));

  addHostType("future-incoming-response", FutureIncomingResponse::ast());
}

} // namespace Host
} // namespace WasmEdge
