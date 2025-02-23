// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "base.h"
#include "runtime/callingframe.h"
#include "type.h"

namespace WasmEdge {
namespace Host {

class OutputStream_CheckWrite : public WasiIO<OutputStream_CheckWrite> {
public:
  OutputStream_CheckWrite(WasiIOEnvironment &HostEnv) : WasiIO(HostEnv) {}
  Expect<void> body() { return {}; }
};
class OutputStream_Write : public WasiIO<OutputStream_Write> {
public:
  OutputStream_Write(WasiIOEnvironment &HostEnv) : WasiIO(HostEnv) {}
  Expect<void> body() { return {}; }
};
class OutputStream_BlockingFlush : public WasiIO<OutputStream_BlockingFlush> {
public:
  OutputStream_BlockingFlush(WasiIOEnvironment &HostEnv) : WasiIO(HostEnv) {}
  Expect<void> body() { return {}; }
};
class OutputStream_BlockingWriteAndFlush
    : public WasiIO<OutputStream_BlockingWriteAndFlush> {
public:
  OutputStream_BlockingWriteAndFlush(WasiIOEnvironment &HostEnv)
      : WasiIO(HostEnv) {}
  Expect<Result<Tuple<>, StreamError::T>> body(int32_t Self,
                                               List<uint8_t> Contents);
};

} // namespace Host
} // namespace WasmEdge
