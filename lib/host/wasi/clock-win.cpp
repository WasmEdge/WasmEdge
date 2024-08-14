// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "common/defines.h"
#if WASMEDGE_OS_WINDOWS
#include "host/wasi/clock.h"
#include "win.h"

using namespace WasmEdge::winapi;

namespace WasmEdge::Host::WASI {

namespace {
static const uint64_t kFrequency = []() noexcept {
  LARGE_INTEGER_ Frequency;
  QueryPerformanceFrequency(&Frequency);
  return static_cast<uint64_t>(Frequency.QuadPart);
}();
uint64_t counter() noexcept {
  LARGE_INTEGER_ Counter;
  QueryPerformanceCounter(&Counter);
  return static_cast<uint64_t>(Counter.QuadPart);
}
} // namespace

WasiExpect<void> Clock::clockResGet(__wasi_clockid_t Id,
                                    __wasi_timestamp_t &Resolution) noexcept {
  switch (Id) {
  case __WASI_CLOCKID_MONOTONIC: {
    const std::chrono::nanoseconds Result =
        std::chrono::nanoseconds(std::chrono::seconds{1}) / kFrequency;
    Resolution = static_cast<__wasi_timestamp_t>(Result.count());
    return {};
  }
  case __WASI_CLOCKID_REALTIME:
  case __WASI_CLOCKID_PROCESS_CPUTIME_ID:
  case __WASI_CLOCKID_THREAD_CPUTIME_ID: {
    ULONG_ MinimumResolution;
    ULONG_ MaximumResolution;
    ULONG_ CurrentResolution;
    if (auto Res = NtQueryTimerResolution(
            &MinimumResolution, &MaximumResolution, &CurrentResolution);
        unlikely(!NT_SUCCESS_(Res))) {
      return WasiUnexpect(detail::fromLastError(RtlNtStatusToDosError(Res)));
    }
    const std::chrono::nanoseconds Result = FiletimeDuration{CurrentResolution};
    Resolution = static_cast<__wasi_timestamp_t>(Result.count());
    return {};
  }
  default:
    return WasiUnexpect(__WASI_ERRNO_NOSYS);
  }
}

WasiExpect<void> Clock::clockTimeGet(__wasi_clockid_t Id,
                                     __wasi_timestamp_t Precision
                                     [[maybe_unused]],
                                     __wasi_timestamp_t &Time) noexcept {
  switch (Id) {
  case __WASI_CLOCKID_REALTIME: {
    FILETIME_ SysNow;
#if NTDDI_VERSION >= NTDDI_WIN8
    GetSystemTimePreciseAsFileTime(&SysNow);
#else
    GetSystemTimeAsFileTime(&SysNow);
#endif
    Time = detail::fromFiletime(SysNow);
    return {};
  }
  case __WASI_CLOCKID_MONOTONIC: {
    uint64_t Nanoseconds;
    const auto Counter = counter();
    if (likely(std::nano::den % kFrequency == 0)) {
      Nanoseconds = Counter * (std::nano::den / kFrequency);
    } else {
      const auto Seconds = Counter / kFrequency;
      const auto Fractions = Counter % kFrequency;
      Nanoseconds =
          Seconds * std::nano::den + (Fractions * std::nano::den) / kFrequency;
    }
    Time = static_cast<__wasi_timestamp_t>(Nanoseconds);
    return {};
  }
  case __WASI_CLOCKID_PROCESS_CPUTIME_ID: {
    FILETIME_ CreationTime;
    FILETIME_ ExitTime;
    FILETIME_ KernelTime;
    FILETIME_ UserTime;
    if (unlikely(!GetProcessTimes(GetCurrentProcess(), &CreationTime, &ExitTime,
                                  &KernelTime, &UserTime))) {
      return WasiUnexpect(detail::fromLastError(GetLastError()));
    }

    Time = detail::fromFiletime(KernelTime) + detail::fromFiletime(UserTime);

    return {};
  }
  case __WASI_CLOCKID_THREAD_CPUTIME_ID: {
    FILETIME_ CreationTime;
    FILETIME_ ExitTime;
    FILETIME_ KernelTime;
    FILETIME_ UserTime;
    if (unlikely(!GetThreadTimes(GetCurrentThread(), &CreationTime, &ExitTime,
                                 &KernelTime, &UserTime))) {
      return WasiUnexpect(detail::fromLastError(GetLastError()));
    }

    Time = detail::fromFiletime(KernelTime) + detail::fromFiletime(UserTime);

    return {};
  }
  default:
    return WasiUnexpect(__WASI_ERRNO_NOSYS);
  }
}

} // namespace WasmEdge::Host::WASI

#endif
