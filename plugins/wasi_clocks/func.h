// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2023 Second State INC

#pragma once

#include "base.h"
#include "runtime/callingframe.h"
#include <time.h>

namespace WasmEdge {
namespace Host {

namespace MonotonicClock {

// A timestamp in nanoseconds.
using Instant = uint64_t;

class Now : public WasmEdgeWasiClocks<class Now> {
public:
  Now(WasmEdgeWasiClocksEnvironment &HostEnv) : WasmEdgeWasiClocks(HostEnv) {}

  Expect<Instant> body(const Runtime::CallingFrame &);
};

class Resolution : public WasmEdgeWasiClocks<class Resolution> {
public:
  Resolution(WasmEdgeWasiClocksEnvironment &HostEnv)
      : WasmEdgeWasiClocks(HostEnv) {}

  Expect<Instant> body(const Runtime::CallingFrame &);
};

} // namespace MonotonicClock

namespace WallClock {

using Datetime = std::tuple</* seconds */ uint64_t, /* nanoseconds */ uint32_t>;

class Now : public WasmEdgeWasiClocks<class Now> {
public:
  Now(WasmEdgeWasiClocksEnvironment &HostEnv) : WasmEdgeWasiClocks(HostEnv) {}

  Expect<Datetime> body(const Runtime::CallingFrame &);
};

class Resolution : public WasmEdgeWasiClocks<class Resolution> {
public:
  Resolution(WasmEdgeWasiClocksEnvironment &HostEnv)
      : WasmEdgeWasiClocks(HostEnv) {}

  Expect<Datetime> body(const Runtime::CallingFrame &);
};

} // namespace WallClock

namespace Timezone {

using Timezone = uint32_t;
using TimezoneDisplay = std::tuple</* utc-offset */ int32_t,
                                   /* name_ptr */ uint32_t,
                                   /* name_len */ uint32_t,
                                   /* in-daylight-saving-time */ int32_t>;

class GetDisplayNameLen : public WasmEdgeWasiClocks<class GetDisplayNameLen> {
public:
  GetDisplayNameLen(WasmEdgeWasiClocksEnvironment &HostEnv)
      : WasmEdgeWasiClocks(HostEnv) {}

  Expect<uint32_t> body(const Runtime::CallingFrame &, Timezone _This,
                        uint64_t Secs, uint32_t NanoSecs);
};

class Display : public WasmEdgeWasiClocks<class Display> {
public:
  Display(WasmEdgeWasiClocksEnvironment &HostEnv)
      : WasmEdgeWasiClocks(HostEnv) {}

  Expect<TimezoneDisplay> body(const Runtime::CallingFrame &, Timezone _This,
                               uint64_t Secs, uint32_t NanoSecs,
                               uint32_t NamePtr, uint32_t NameLen);
};

class UtcOffset : public WasmEdgeWasiClocks<class UtcOffset> {
public:
  UtcOffset(WasmEdgeWasiClocksEnvironment &HostEnv)
      : WasmEdgeWasiClocks(HostEnv) {}

  Expect<int32_t> body(const Runtime::CallingFrame &, Timezone This,
                       uint64_t Secs, uint32_t NanoSecs);
};

} // namespace Timezone

} // namespace Host
} // namespace WasmEdge
