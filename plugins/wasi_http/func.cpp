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

Expect<void> WasiHttpPrint::body(const Runtime::CallingFrame &,
                                 StrVariant Str) {
  spdlog::info("[WASI-HTTP] print: {}", Str.getString());
  return {};
}

Expect<StrVariant> WasiHttpGet::body(const Runtime::CallingFrame &,
                                     StrVariant URI) {
  const auto &S = URI.getString();
  spdlog::info("[WASI-HTTP] URI: {}", S);
  cpr::Response Res = cpr::Get(
      cpr::Url{S}, cpr::Authentication{"user", "pass", cpr::AuthMode::BASIC});
  spdlog::info("[WASI-HTTP] status: {}", Res.status_code);

  return StrVariant(std::move(Res.text));
}

} // namespace Host
} // namespace WasmEdge
