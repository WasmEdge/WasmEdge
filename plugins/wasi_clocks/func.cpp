// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2023 Second State INC

#include "func.h"
#include "common/defines.h"
#include <string>
#include <vector>

namespace WasmEdge {
namespace Host {

namespace MonotonicClock {

Expect<Instant> Now::body(const Runtime::CallingFrame &) {
  timespec Ts;
  auto Err = clock_gettime(CLOCK_MONOTONIC, &Ts);
  if (Err) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }
  return static_cast<Instant>(Ts.tv_nsec);
}

Expect<Instant> Resolution::body(const Runtime::CallingFrame &) {
  timespec Ts;
  auto Err = clock_getres(CLOCK_MONOTONIC, &Ts);
  if (Err) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }
  return static_cast<Instant>(Ts.tv_nsec);
}

} // namespace MonotonicClock

namespace WallClock {

Expect<Datetime> Now::body(const Runtime::CallingFrame &) {
  timespec Ts;
  auto Err = clock_gettime(CLOCK_REALTIME, &Ts);
  if (Err) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }
  return Datetime(Ts.tv_sec, Ts.tv_nsec);
}

Expect<Datetime> Resolution::body(const Runtime::CallingFrame &) {
  timespec Ts;
  auto Err = clock_getres(CLOCK_REALTIME, &Ts);
  if (Err) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }
  return Datetime(Ts.tv_sec, Ts.tv_nsec);
}

} // namespace WallClock

namespace Timezone {

Expect<uint32_t> GetDisplayNameLen::body(const Runtime::CallingFrame &,
                                         Timezone, uint64_t Secs, uint32_t) {
  time_t S = static_cast<time_t>(Secs);
  tm *T = localtime(&S);
  return strlen(T->tm_zone);
}

Expect<TimezoneDisplay> Display::body(const Runtime::CallingFrame &Frame,
                                      Timezone, uint64_t Secs, uint32_t,
                                      uint32_t NamePtr, uint32_t NameLen) {
  time_t S = static_cast<time_t>(Secs);
  tm *T = localtime(&S);

  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }
  auto Output = MemInst->getSpan<char>(NamePtr, NameLen);
  if (unlikely(Output.size() != NameLen)) {
    spdlog::error("[WASI-Clocks] "sv
                  "Failed when accessing the name's buffer memory."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }
  std::vector<char> TZ{T->tm_zone, T->tm_zone + NameLen};
  std::copy_n(TZ.begin(), TZ.size(), Output.begin());

  return TimezoneDisplay(T->tm_gmtoff, NamePtr, NameLen, T->tm_isdst);
}

Expect<int32_t> UtcOffset::body(const Runtime::CallingFrame &, Timezone,
                                uint64_t Secs, uint32_t) {
  time_t S = static_cast<time_t>(Secs);
  tm *T = localtime(&S);
  return static_cast<int32_t>(T->tm_gmtoff);
}

} // namespace Timezone

} // namespace Host
} // namespace WasmEdge
