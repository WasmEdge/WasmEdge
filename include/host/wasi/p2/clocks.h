// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/host/wasi/p2/clocks.h - wasi:clocks host -----------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the host component instances of `wasi:clocks@0.2.12`:
/// `wall-clock` and `monotonic-clock`, whose subscriptions are pollables.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/component_valtype.h"
#include "common/component_variant.h"
#include "common/errcode.h"
#include "host/wasi/p2/io.h"
#include "runtime/component/callingframe.h"
#include "runtime/component/hostfunc.h"
#include "runtime/component/wit.h"
#include "runtime/instance/component/component.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace WasiP2 {

/// `wall-clock` `datetime`: seconds since the epoch and the nanosecond
/// remainder.
struct Datetime {
  uint64_t Seconds = 0;
  uint32_t Nanoseconds = 0;

  /// The nanoseconds since the epoch.
  uint64_t nanos() const noexcept {
    return Seconds * 1000000000U + Nanoseconds;
  }
  /// The datetime Nanos nanoseconds after the epoch.
  static Datetime fromNanos(uint64_t Nanos) noexcept {
    return Datetime{Nanos / 1000000000U,
                    static_cast<uint32_t>(Nanos % 1000000000U)};
  }
};

} // namespace WasiP2
} // namespace Host

namespace Runtime {
namespace Component {

template <> struct Wit<Host::WasiP2::Datetime> {
  static ComponentValType type(const TypeMinter &Mint) noexcept {
    return WitRecord::type(Mint, {{"seconds", Wit<uint64_t>::type(Mint)},
                                  {"nanoseconds", Wit<uint32_t>::type(Mint)}});
  }
  static Host::WasiP2::Datetime from(const ComponentValVariant &V) {
    auto Fields = WitRecord::fields(V);
    return {Wit<uint64_t>::from(Fields[0].second),
            Wit<uint32_t>::from(Fields[1].second)};
  }
  static ComponentValVariant into(Host::WasiP2::Datetime &&D) noexcept {
    return WitRecord::into(
        {{"seconds", Wit<uint64_t>::into(D.Seconds)},
         {"nanoseconds", Wit<uint32_t>::into(D.Nanoseconds)}});
  }
};

} // namespace Component
} // namespace Runtime

namespace Host {
namespace WasiP2 {

/// `wall-clock.now: func() -> datetime`
class WallNow : public Runtime::Component::HostFunction<WallNow> {
public:
  Expect<Datetime> body(Runtime::Component::CallingFrame &);
};

/// `wall-clock.resolution: func() -> datetime`
class WallResolution : public Runtime::Component::HostFunction<WallResolution> {
public:
  Expect<Datetime> body(Runtime::Component::CallingFrame &);
};

/// `monotonic-clock.now: func() -> instant`
class MonotonicNow : public Runtime::Component::HostFunction<MonotonicNow> {
public:
  Expect<uint64_t> body(Runtime::Component::CallingFrame &);
};

/// `monotonic-clock.resolution: func() -> duration`
class MonotonicResolution
    : public Runtime::Component::HostFunction<MonotonicResolution> {
public:
  Expect<uint64_t> body(Runtime::Component::CallingFrame &);
};

/// `monotonic-clock.subscribe-instant: func(when: instant) -> pollable`
/// and `subscribe-duration: func(when: duration) -> pollable`
class SubscribeClock : public Runtime::Component::HostFunction<SubscribeClock> {
public:
  static constexpr const char *ParamNames[] = {"when"};
  SubscribeClock(IoHost &H, bool Relative) noexcept
      : Host(H), Relative(Relative) {}
  Expect<Runtime::Component::Own<PollSource>>
  body(Runtime::Component::CallingFrame &, uint64_t When);

private:
  IoHost &Host;
  bool Relative;
};

/// `wasi:clocks/wall-clock@0.2.12`
class WallClockInstance : public Runtime::Instance::ComponentInstance {
public:
  WallClockInstance();
};

/// `wasi:clocks/monotonic-clock@0.2.12`, over the pollables of `wasi:io`.
class MonotonicClockInstance : public Runtime::Instance::ComponentInstance {
public:
  MonotonicClockInstance(IoHost &Host);
};

/// The two instances of `wasi:clocks@0.2.12`.
std::vector<std::unique_ptr<Runtime::Instance::ComponentInstance>>
makeClocksInstances(IoHost &Host);

} // namespace WasiP2
} // namespace Host
} // namespace WasmEdge
