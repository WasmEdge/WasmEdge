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

Expect<void> WasiHttpPrint::body(const Runtime::CallingFrame &, uint64_t Ptr,
                                 uint64_t Len) {
  std::string S{reinterpret_cast<const char *>(Ptr), Len};
  spdlog::info("[WASI-HTTP] print: {}", S);
  return {};
}

Expect<Str> WasiHttpGet::body(const Runtime::CallingFrame &, uint64_t Idx) {
  auto URI = Env.loadURI(Idx);

  cpr::Response Res = cpr::Get(
      cpr::Url{URI}, cpr::Authentication{"user", "pass", cpr::AuthMode::BASIC});
  spdlog::info("[WASI-HTTP] status: {}", Res.status_code);
  Env.Bodies.push_back(Res.text);

  return Str((uint64_t)Env.Bodies.back().data(), Env.Bodies.back().size());
}

} // namespace Host
} // namespace WasmEdge
