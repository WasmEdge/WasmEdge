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

} // namespace Host
} // namespace WasmEdge
