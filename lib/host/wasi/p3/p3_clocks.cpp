// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "host/wasi/component/clocks.h"
#include "host/wasi/p3/clocks.h"

#include <chrono>
#include <memory>
#include <utility>

namespace WasmEdge {
namespace Host {
namespace WasiP3 {

Expect<uint64_t> MonotonicNow::body(Runtime::Component::CallingFrame &) {
  return WasiComponent::readClock(__WASI_CLOCKID_MONOTONIC);
}

Expect<uint64_t> MonotonicResolution::body(Runtime::Component::CallingFrame &) {
  return WasiComponent::readResolution(__WASI_CLOCKID_MONOTONIC);
}

Expect<void> MonotonicWaitUntil::body(Runtime::Component::CallingFrame &Frame,
                                      uint64_t When) {
  EXPECTED_TRY(const uint64_t Now,
               WasiComponent::readClock(__WASI_CLOCKID_MONOTONIC));
  if (When <= Now) {
    return {};
  }
  return Frame.sleepFor(std::chrono::nanoseconds(When - Now));
}

Expect<void> MonotonicWaitFor::body(Runtime::Component::CallingFrame &Frame,
                                    uint64_t HowLong) {
  return Frame.sleepFor(std::chrono::nanoseconds(HowLong));
}

Expect<Instant> SystemNow::body(Runtime::Component::CallingFrame &) {
  EXPECTED_TRY(const uint64_t Now,
               WasiComponent::readClock(__WASI_CLOCKID_REALTIME));
  return Instant::fromNanos(Now);
}

Expect<uint64_t> SystemResolution::body(Runtime::Component::CallingFrame &) {
  return WasiComponent::readResolution(__WASI_CLOCKID_REALTIME);
}

ClocksTypesInstance::ClocksTypesInstance()
    : ComponentInstance("wasi:clocks/types@0.3.1") {
  exportType("duration",
             getTypeMinter().primDefType(PrimValType::U64).getTypeIndex());
}

MonotonicClockInstance::MonotonicClockInstance()
    : ComponentInstance("wasi:clocks/monotonic-clock@0.3.1") {
  exportType("mark",
             getTypeMinter().primDefType(PrimValType::U64).getTypeIndex());
  addHostFunc("now", std::make_unique<MonotonicNow>());
  addHostFunc("get-resolution", std::make_unique<MonotonicResolution>());
  addHostFunc("wait-until", std::make_unique<MonotonicWaitUntil>());
  addHostFunc("wait-for", std::make_unique<MonotonicWaitFor>());
}

SystemClockInstance::SystemClockInstance()
    : ComponentInstance("wasi:clocks/system-clock@0.3.1") {
  const auto InstantTy =
      Runtime::Component::Wit<Instant>::type(getTypeMinter());
  exportType("instant", InstantTy.getTypeIndex());
  addHostFunc("now", std::make_unique<SystemNow>());
  addHostFunc("get-resolution", std::make_unique<SystemResolution>());
}

} // namespace WasiP3
} // namespace Host
} // namespace WasmEdge
