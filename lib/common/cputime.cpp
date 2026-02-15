// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "common/cputime.h"
#include "common/defines.h"

#if WASMEDGE_OS_LINUX || WASMEDGE_OS_MACOS
#include <ctime>
#elif WASMEDGE_OS_WINDOWS
#include "system/winapi.h"
#endif

namespace WasmEdge {
namespace CpuTime {

uint64_t getProcessCpuTimeNs() noexcept {
#if WASMEDGE_OS_LINUX || WASMEDGE_OS_MACOS
  struct timespec Ts;
  if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &Ts) == 0) {
    return static_cast<uint64_t>(Ts.tv_sec) * UINT64_C(1000000000) +
           static_cast<uint64_t>(Ts.tv_nsec);
  }
  return 0;
#elif WASMEDGE_OS_WINDOWS
  FILETIME Creation, Exit, Kernel, User;
  if (GetProcessTimes(GetCurrentProcess(), &Creation, &Exit, &Kernel, &User)) {
    // FILETIME is in 100-nanosecond intervals.
    auto ToNs = [](const FILETIME &Ft) -> uint64_t {
      uint64_t Val =
          (static_cast<uint64_t>(Ft.dwHighDateTime) << 32) | Ft.dwLowDateTime;
      return Val * 100;
    };
    return ToNs(Kernel) + ToNs(User);
  }
  return 0;
#else
  return 0;
#endif
}

} // namespace CpuTime
} // namespace WasmEdge
