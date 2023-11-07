// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#pragma once

#include "host/preview2/wasi-sockets/api.h"
#include "host/preview2/wasi-sockets/base.h"
#include "host/preview2/wasi-sockets/variant_helper.h"
#include "host/wasi/wasibase.h"

#include <cstdint>

namespace WasmEdge {
namespace Host {
namespace WasiSocket {
namespace IpNameLookup {

class ResolveAddresses : public WasiSockets<ResolveAddresses> {
public:
  ResolveAddresses(WasiSocketsEnvironment &HostEnv) : WasiSockets(HostEnv) {}

  Expect<void> body(const Runtime::CallingFrame &Frame, uint32_t Handle,
                    uint32_t StrPtr, uint32_t StrLen, uint32_t RetBufPtr);
};

class ResolveNextAddress : public WasiSockets<ResolveNextAddress> {
public:
  ResolveNextAddress(WasiSocketsEnvironment &HostEnv) : WasiSockets(HostEnv) {}

  Expect<void> body(const Runtime::CallingFrame &Frame, uint32_t Handle,
                    uint32_t RetBufPtr);
};

class ResolveDrop : public WasiSockets<ResolveDrop> {
public:
  ResolveDrop(WasiSocketsEnvironment &HostEnv) : WasiSockets(HostEnv) {}

  Expect<void> body(const Runtime::CallingFrame &Frame, uint32_t Handle);
};

} // namespace IpNameLookup
} // namespace WasiSocket
} // namespace Host
} // namespace WasmEdge
