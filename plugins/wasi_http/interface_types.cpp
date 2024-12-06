// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "func.h"
#include "interface.h"

#include <memory>

namespace WasmEdge {
namespace Host {

WasiHttp_Types::WasiHttp_Types() : ComponentInstance("wasi:http/types@0.2.0") {
  using namespace Types;
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
}

} // namespace Host
} // namespace WasmEdge
