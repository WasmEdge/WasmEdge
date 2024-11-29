// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "func.h"
#include "interface.h"

#include <memory>

namespace WasmEdge {
namespace Host {

WasiHttp_Types::WasiHttp_Types() : ComponentInstance("wasi:http/types@0.2.0") {
  addHostType("method", Types::Method());
  addHostType("scheme", Types::Scheme());
  addHostType("DNS-error-payload", DNSErrorPayload());
  addHostType("TLS-alert-received-payload", TLSAlertReceivedPayload());
  addHostType("field-size-payload", FieldSizePayload());
  addHostType("error-code", Types::ErrorCode());
  addHostFunc("http-error-code", std::make_unique<Types::HttpErrorCode>(Env));
}

} // namespace Host
} // namespace WasmEdge
