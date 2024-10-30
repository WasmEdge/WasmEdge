// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "base.h"
#include "runtime/callingframe.h"
#include "type.h"

namespace WasmEdge {
namespace Host {

class DropError : public WasiIO<DropError> {
public:
  DropError(WasiIOEnvironment &HostEnv) : WasiIO(HostEnv) {}
  Expect<void> body(uint32_t ThisIOError);
};

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
  Expect<void> body() { return {}; }
};
class DropOutputStream : public WasiIO<DropOutputStream> {
public:
  DropOutputStream(WasiIOEnvironment &HostEnv) : WasiIO(HostEnv) {}
  Expect<void> body(uint32_t ThisOutputStream);
};

class DropInputStream : public WasiIO<DropInputStream> {
public:
  DropInputStream(WasiIOEnvironment &HostEnv) : WasiIO(HostEnv) {}
  Expect<void> body(uint32_t ThisInputStream);
};

class DropStreamError : public WasiIO<DropStreamError> {
public:
  DropStreamError(WasiIOEnvironment &HostEnv) : WasiIO(HostEnv) {}
  Expect<void> body(uint32_t ThisStreamError);
};

} // namespace Host
} // namespace WasmEdge
