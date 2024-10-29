// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "func.h"
#include "common/defines.h"
#include "common/errcode.h"

#include <cpr/cpr.h>
#include <cstdint>
#include <string>
#include <vector>

namespace WasmEdge {
namespace Host {

Expect<void> WasiHttpPrint::body(std::string S) {
  spdlog::info("[WASI-HTTP] print: {}", S);
  return {};
}

Expect<std::string> WasiHttpGet::body(std::string URI) {
  spdlog::info("[WASI-HTTP] URI: {}", URI);
  cpr::Response Res = cpr::Get(
      cpr::Url{URI}, cpr::Authentication{"user", "pass", cpr::AuthMode::BASIC});
  spdlog::info("[WASI-HTTP] status: {}", Res.status_code);

  return std::move(Res.text);
}

} // namespace Host
} // namespace WasmEdge
