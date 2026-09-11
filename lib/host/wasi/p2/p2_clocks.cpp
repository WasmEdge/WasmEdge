// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "host/wasi/component/clocks.h"
#include "host/wasi/p2/clocks.h"

#include <memory>

namespace WasmEdge {
namespace Host {
namespace WasiP2 {

Expect<Datetime> WallNow::body(Runtime::Component::CallingFrame &) {
  EXPECTED_TRY(const uint64_t Now,
               WasiComponent::readClock(__WASI_CLOCKID_REALTIME));
  return Datetime::fromNanos(Now);
}

Expect<Datetime> WallResolution::body(Runtime::Component::CallingFrame &) {
  EXPECTED_TRY(const uint64_t Res,
               WasiComponent::readResolution(__WASI_CLOCKID_REALTIME));
  return Datetime::fromNanos(Res);
}

Expect<uint64_t> MonotonicNow::body(Runtime::Component::CallingFrame &) {
  return WasiComponent::readClock(__WASI_CLOCKID_MONOTONIC);
}

Expect<uint64_t> MonotonicResolution::body(Runtime::Component::CallingFrame &) {
  return WasiComponent::readResolution(__WASI_CLOCKID_MONOTONIC);
}

Expect<Runtime::Component::Own<PollSource>>
SubscribeClock::body(Runtime::Component::CallingFrame &, uint64_t When) {
  uint64_t Deadline = When;
  if (Relative) {
    EXPECTED_TRY(const uint64_t Now,
                 WasiComponent::readClock(__WASI_CLOCKID_MONOTONIC));
    Deadline = Now + When;
  }
  return Runtime::Component::Own<PollSource>{
      Host.pollables().add(std::make_shared<DeadlineSource>(Deadline))};
}

WallClockInstance::WallClockInstance()
    : ComponentInstance("wasi:clocks/wall-clock@0.2.12") {
  exportType(
      "datetime",
      Runtime::Component::Wit<Datetime>::type(getTypeMinter()).getTypeIndex());
  addHostFunc("now", std::make_unique<WallNow>());
  addHostFunc("resolution", std::make_unique<WallResolution>());
}

MonotonicClockInstance::MonotonicClockInstance(IoHost &Host)
    : ComponentInstance("wasi:clocks/monotonic-clock@0.2.12") {
  exportType("pollable", addSharedResourceType<PollSource>(Host.PollableType));
  const auto Mint = getTypeMinter();
  exportType("instant", Mint.primDefType(PrimValType::U64).getTypeIndex());
  exportType("duration", Mint.primDefType(PrimValType::U64).getTypeIndex());
  addHostFunc("now", std::make_unique<MonotonicNow>());
  addHostFunc("resolution", std::make_unique<MonotonicResolution>());
  addHostFunc("subscribe-instant",
              std::make_unique<SubscribeClock>(Host, false));
  addHostFunc("subscribe-duration",
              std::make_unique<SubscribeClock>(Host, true));
}

std::vector<std::unique_ptr<Runtime::Instance::ComponentInstance>>
makeClocksInstances(IoHost &Host) {
  std::vector<std::unique_ptr<Runtime::Instance::ComponentInstance>> Out;
  Out.push_back(std::make_unique<WallClockInstance>());
  Out.push_back(std::make_unique<MonotonicClockInstance>(Host));
  return Out;
}

} // namespace WasiP2
} // namespace Host
} // namespace WasmEdge
