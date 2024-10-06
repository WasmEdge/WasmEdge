// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "common/defines.h"
#if WASMEDGE_OS_LINUX

#include "host/wasi/clock.h"
#include "linux.h"

namespace WasmEdge {
namespace Host {
namespace WASI {

WasiExpect<void> Clock::clockResGet(__wasi_clockid_t Id,
                                    __wasi_timestamp_t &Resolution) noexcept {
  timespec SysTimespec;
  if (auto Res = ::clock_getres(toClockId(Id), &SysTimespec);
      unlikely(Res != 0)) {
    return WasiUnexpect(fromErrNo(errno));
  }

  Resolution = fromTimespec(SysTimespec);
  return {};
}

WasiExpect<void> Clock::clockTimeGet(__wasi_clockid_t Id, __wasi_timestamp_t,
                                     __wasi_timestamp_t &Time) noexcept {
  timespec SysTimespec;
  if (auto Res = ::clock_gettime(toClockId(Id), &SysTimespec);
      unlikely(Res != 0)) {
    return WasiUnexpect(fromErrNo(errno));
  }

  Time = fromTimespec(SysTimespec);
  return {};
}

} // namespace WASI
} // namespace Host
} // namespace WasmEdge

#endif
