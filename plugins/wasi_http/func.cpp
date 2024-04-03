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
  spdlog::info("[WASI-HTTP] print: {}", Str.getPtr()->getString());
  return {};
}

Expect<StrVariant> WasiHttpGet::body(const Runtime::CallingFrame &,
                                     uint64_t Idx) {
  auto URI = Env.loadURI(Idx);

  cpr::Response Res = cpr::Get(
      cpr::Url{URI}, cpr::Authentication{"user", "pass", cpr::AuthMode::BASIC});
  spdlog::info("[WASI-HTTP] status: {}", Res.status_code);

  const auto *R = new Runtime::Instance::StringInstance(std::move(Res.text));
  return StrVariant(R);
}

} // namespace Host
} // namespace WasmEdge
