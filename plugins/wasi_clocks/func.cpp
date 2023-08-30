// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2023 Second State INC

#include "func.h"
#include "common/defines.h"

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

} // namespace Host
} // namespace WasmEdge
