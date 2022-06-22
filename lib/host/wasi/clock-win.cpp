// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "common/defines.h"
#if WASMEDGE_OS_WINDOWS

#include "host/wasi/clock.h"
#include "win.h"
#include <boost/winapi/time.hpp>
#include <boost/winapi/timers.hpp>

#if BOOST_USE_WINAPI_VERSION >= BOOST_WINAPI_VERSION_WIN8 &&                   \
    !defined(BOOST_USE_WINDOWS_H)
extern "C" {
BOOST_WINAPI_IMPORT void BOOST_WINAPI_WINAPI_CC
GetSystemTimePreciseAsFileTime(::_FILETIME *lpSystemTimeAsFileTime);
}
#endif

namespace boost::winapi {
BOOST_FORCEINLINE BOOL_
GetSystemTimePreciseAsFileTime(FILETIME_ *lpSystemTimeAsFileTime) {
#if BOOST_USE_WINAPI_VERSION >= BOOST_WINAPI_VERSION_WIN8
  return ::GetSystemTimePreciseAsFileTime(
      reinterpret_cast<::_FILETIME *>(lpSystemTimeAsFileTime));
#else
  GetSystemTimeAsFileTime(lpSystemTimeAsFileTime);
  return true;
#endif
}
} // namespace boost::winapi

namespace WasmEdge::Host::WASI {

namespace winapi = boost::winapi;

WasiExpect<void> Clock::clockResGet(__wasi_clockid_t Id,
                                    __wasi_timestamp_t &Resolution) noexcept {
  switch (Id) {
  case __WASI_CLOCKID_REALTIME:
  case __WASI_CLOCKID_MONOTONIC:
    Resolution = static_cast<__wasi_timestamp_t>(getResolution().count());
    return {};
  default:
    return WasiUnexpect(__WASI_ERRNO_NOSYS);
  }
}

WasiExpect<void> Clock::clockTimeGet(__wasi_clockid_t Id,
                                     __wasi_timestamp_t Precision
                                     [[maybe_unused]],
                                     __wasi_timestamp_t &Time) noexcept {
  switch (Id) {
  case __WASI_CLOCKID_REALTIME:
  case __WASI_CLOCKID_MONOTONIC: {
    winapi::LARGE_INTEGER_ SysNow;
    winapi::GetSystemTimePreciseAsFileTime(
        reinterpret_cast<winapi::FILETIME_ *>(&SysNow));
    Time = fromFileTyime(static_cast<uint64_t>(SysNow.QuadPart));
    return {};
  }
  default:
    return WasiUnexpect(__WASI_ERRNO_NOSYS);
  }
}

} // namespace WasmEdge::Host::WASI

#endif
