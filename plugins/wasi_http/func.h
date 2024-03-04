// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2023 Second State INC

#pragma once

#include "base.h"
#include "runtime/callingframe.h"

namespace WasmEdge {
namespace Host {

using Str = std::tuple<uint64_t, uint64_t>;

class WasiHttpPrint : public WasiHttp<WasiHttpPrint> {
public:
  WasiHttpPrint(WasiHttpEnvironment &HostEnv) : WasiHttp(HostEnv) {}
  Expect<void> body(const Runtime::CallingFrame &Frame, uint64_t StrPtr,
                    uint64_t StrLen);
};

class WasiHttpGet : public WasiHttp<WasiHttpGet> {
public:
  WasiHttpGet(WasiHttpEnvironment &HostEnv) : WasiHttp(HostEnv) {}
  Expect<Str> body(const Runtime::CallingFrame &Frame, uint64_t URIIndex);
};

} // namespace Host
} // namespace WasmEdge
