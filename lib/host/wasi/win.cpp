// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "common/defines.h"
#if WASMEDGE_OS_WINDOWS

#include "win.h"
#include <boost/winapi/dll.hpp>
#include <boost/winapi/get_proc_address.hpp>
#include <boost/winapi/time.hpp>
#include <boost/winapi/timers.hpp>
#include <chrono>
#include <mutex>

namespace WasmEdge::Host::WASI {
inline namespace detail {

namespace winapi = boost::winapi;
using namespace std::literals::chrono_literals;

long(__stdcall *NtQueryTimerResolution)(
    unsigned long *LowestResolution, unsigned long *HighestResolution,
    unsigned long *CurrentResolution) = nullptr;

namespace {

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
void ensureInit() noexcept {
  static std::once_flag Initialized;
  std::call_once(Initialized, [&]() {
    auto NTDll = winapi::GetModuleHandleA("ntdll.dll");
    NtQueryTimerResolution = reinterpret_cast<decltype(NtQueryTimerResolution)>(
        winapi::GetProcAddress(NTDll, "NtQueryTimerResolution"));
  });
}
#pragma GCC diagnostic pop

} // namespace

std::chrono::nanoseconds getResolution() noexcept {
  ensureInit();
  if (NtQueryTimerResolution) {
    unsigned long LowestResolution;
    unsigned long HighestResolution;
    unsigned long CurrentResolution;
    NtQueryTimerResolution(&LowestResolution, &HighestResolution,
                           &CurrentResolution);
    return CurrentResolution * 100ns;
  } else {
    winapi::LARGE_INTEGER_ Frequency;
    winapi::QueryPerformanceFrequency(&Frequency);
    return 1s / Frequency.QuadPart;
  }
}

} // namespace detail
} // namespace WasmEdge::Host::WASI

#endif
