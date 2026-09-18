// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/host/wasi/p3/clocks.h - wasi:clocks host -----------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the host component instances of `wasi:clocks@0.3.1`:
/// the type-only `types` interface, `monotonic-clock`, and `system-clock`.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/component_valtype.h"
#include "common/component_variant.h"
#include "common/errcode.h"
#include "runtime/component/callingframe.h"
#include "runtime/component/hostfunc.h"
#include "runtime/component/wit.h"
#include "runtime/instance/component/component.h"

#include <cstdint>
#include <utility>

namespace WasmEdge {
namespace Host {
namespace WasiP3 {

/// `wasi:clocks/system-clock@0.3.1` `instant`: seconds since the epoch and
/// the nanosecond remainder.
struct Instant {
  int64_t Seconds = 0;
  uint32_t Nanoseconds = 0;

  /// The nanoseconds since the epoch.
  uint64_t nanos() const noexcept {
    return static_cast<uint64_t>(Seconds) * 1000000000U + Nanoseconds;
  }
  /// The instant Nanos nanoseconds after the epoch.
  static Instant fromNanos(uint64_t Nanos) noexcept {
    return Instant{static_cast<int64_t>(Nanos / 1000000000U),
                   static_cast<uint32_t>(Nanos % 1000000000U)};
  }
};

} // namespace WasiP3
} // namespace Host

namespace Runtime {
namespace Component {

template <> struct Wit<Host::WasiP3::Instant> {
  static ComponentValType type(const TypeMinter &Mint) noexcept {
    return WitRecord::type(Mint, {{"seconds", Wit<int64_t>::type(Mint)},
                                  {"nanoseconds", Wit<uint32_t>::type(Mint)}});
  }
  static Host::WasiP3::Instant from(const ComponentValVariant &V) {
    auto Fields = WitRecord::fields(V);
    return {Wit<int64_t>::from(Fields[0].second),
            Wit<uint32_t>::from(Fields[1].second)};
  }
  static ComponentValVariant into(Host::WasiP3::Instant &&I) noexcept {
    return WitRecord::into(
        {{"seconds", Wit<int64_t>::into(I.Seconds)},
         {"nanoseconds", Wit<uint32_t>::into(I.Nanoseconds)}});
  }
};

} // namespace Component
} // namespace Runtime

namespace Host {
namespace WasiP3 {

/// `monotonic-clock.now: func() -> mark`
class MonotonicNow : public Runtime::Component::HostFunction<MonotonicNow> {
public:
  Expect<uint64_t> body(Runtime::Component::CallingFrame &);
};

/// `monotonic-clock.get-resolution: func() -> duration`
class MonotonicResolution
    : public Runtime::Component::HostFunction<MonotonicResolution> {
public:
  Expect<uint64_t> body(Runtime::Component::CallingFrame &);
};

/// `monotonic-clock.wait-until: async func(when: mark)`
class MonotonicWaitUntil
    : public Runtime::Component::HostFunction<MonotonicWaitUntil> {
public:
  static constexpr const char *ParamNames[] = {"when"};
  MonotonicWaitUntil() : HostFunction(/*Async=*/true) {}
  Expect<void> body(Runtime::Component::CallingFrame &Frame, uint64_t When);
};

/// `monotonic-clock.wait-for: async func(how-long: duration)`
class MonotonicWaitFor
    : public Runtime::Component::HostFunction<MonotonicWaitFor> {
public:
  static constexpr const char *ParamNames[] = {"how-long"};
  MonotonicWaitFor() : HostFunction(/*Async=*/true) {}
  Expect<void> body(Runtime::Component::CallingFrame &Frame, uint64_t HowLong);
};

/// `system-clock.now: func() -> instant`
class SystemNow : public Runtime::Component::HostFunction<SystemNow> {
public:
  Expect<Instant> body(Runtime::Component::CallingFrame &);
};

/// `system-clock.get-resolution: func() -> duration`
class SystemResolution
    : public Runtime::Component::HostFunction<SystemResolution> {
public:
  Expect<uint64_t> body(Runtime::Component::CallingFrame &);
};

/// `wasi:clocks/types@0.3.1`: type-only, `duration` is u64 nanoseconds.
class ClocksTypesInstance : public Runtime::Instance::ComponentInstance {
public:
  ClocksTypesInstance();
};

/// `wasi:clocks/monotonic-clock@0.3.1`
class MonotonicClockInstance : public Runtime::Instance::ComponentInstance {
public:
  MonotonicClockInstance();
};

/// `wasi:clocks/system-clock@0.3.1`
class SystemClockInstance : public Runtime::Instance::ComponentInstance {
public:
  SystemClockInstance();
};

} // namespace WasiP3
} // namespace Host
} // namespace WasmEdge
