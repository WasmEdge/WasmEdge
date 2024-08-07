// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "common/defines.h"
#include "host/wasi/error.h"

namespace WasmEdge {
namespace Host {
namespace WASI {

class Clock {
public:
  /// Return the resolution of a clock.
  ///
  /// Implementations are required to provide a non-zero value for supported
  /// clocks. For unsupported clocks, return `errno::inval`.
  ///
  /// @param[in] Id The clock for which to return the resolution.
  /// @param[out] Resolution The resolution of the clock.
  /// @return Nothing or WASI error
  static WasiExpect<void> clockResGet(__wasi_clockid_t Id,
                                      __wasi_timestamp_t &Resolution) noexcept;

  /// Return the time value of a clock.
  ///
  /// Note: This is similar to `clock_gettime` in POSIX.
  ///
  /// @param[in] Id The clock for which to return the time.
  /// @param[in] Precision The maximum lag (exclusive) that the returned time
  /// value may have, compared to its actual value.
  /// @param[out] Time The time value of the clock.
  /// @return Nothing or WASI error
  static WasiExpect<void> clockTimeGet(__wasi_clockid_t Id,
                                       __wasi_timestamp_t Precision,
                                       __wasi_timestamp_t &Time) noexcept;
};

} // namespace WASI
} // namespace Host
} // namespace WasmEdge
