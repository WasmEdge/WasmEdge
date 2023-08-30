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

// The clock is monotonic, therefore calling this function repeatedly
// will produce a sequence of non-decreasing values.
//
// now: func() -> instant
class Now : public WasmEdgeWasiClocks<class Now> {
public:
  Now(WasmEdgeWasiClocksEnvironment &HostEnv) : WasmEdgeWasiClocks(HostEnv) {}

  Expect<Instant> body(const Runtime::CallingFrame &);
};

// Query the resolution of the clock.
//
// resolution: func() -> instant
class Resolution : public WasmEdgeWasiClocks<class Resolution> {
public:
  Resolution(WasmEdgeWasiClocksEnvironment &HostEnv)
      : WasmEdgeWasiClocks(HostEnv) {}

  Expect<Instant> body(const Runtime::CallingFrame &);
};

} // namespace MonotonicClock

} // namespace Host
} // namespace WasmEdge
