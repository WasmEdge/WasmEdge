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
class Now : public WasmEdgeWasiClocks<class Now> {
public:
  Now(WasmEdgeWasiClocksEnvironment &HostEnv) : WasmEdgeWasiClocks(HostEnv) {}

  Expect<Instant> body(const Runtime::CallingFrame &);
};

// Query the resolution of the clock.
class Resolution : public WasmEdgeWasiClocks<class Resolution> {
public:
  Resolution(WasmEdgeWasiClocksEnvironment &HostEnv)
      : WasmEdgeWasiClocks(HostEnv) {}

  Expect<Instant> body(const Runtime::CallingFrame &);
};

} // namespace MonotonicClock

namespace WallClock {

using Datetime = cxx20::tuple<uint64_t, uint32_t>;

/// Read the current value of the clock.
///
/// This clock is not monotonic, therefore calling this function repeatedly
/// will not necessarily produce a sequence of non-decreasing values.
///
/// The returned timestamps represent the number of seconds since
/// 1970-01-01T00:00:00Z, also known as [POSIX's Seconds Since the Epoch],
/// also known as [Unix Time].
///
/// The nanoseconds field of the output is always less than 1000000000.
///
/// [POSIX's Seconds Since the Epoch]:
/// https://pubs.opengroup.org/onlinepubs/9699919799/xrat/V4_xbd_chap04.html#tag_21_04_16
/// [Unix Time]: https://en.wikipedia.org/wiki/Unix_time
class Now : public WasmEdgeWasiClocks<class Now> {
public:
  Now(WasmEdgeWasiClocksEnvironment &HostEnv) : WasmEdgeWasiClocks(HostEnv) {}

  Expect<Datetime> body(const Runtime::CallingFrame &);
};

/// Query the resolution of the clock.
///
/// The nanoseconds field of the output is always less than 1000000000.
class Resolution : public WasmEdgeWasiClocks<class Resolution> {
public:
  Resolution(WasmEdgeWasiClocksEnvironment &HostEnv)
      : WasmEdgeWasiClocks(HostEnv) {}

  Expect<Datetime> body(const Runtime::CallingFrame &);
};

} // namespace WallClock

} // namespace Host
} // namespace WasmEdge
