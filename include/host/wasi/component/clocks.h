// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/host/wasi/component/clocks.h - Shared clock reads --------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// The clock reads the 0.2 and 0.3 clocks share, through the preview-1
/// clock layer.
//===----------------------------------------------------------------------===//
#pragma once

#include "common/errcode.h"
#include "common/expected.h"
#include "host/wasi/clock.h"

#include <cstdint>

namespace WasmEdge {
namespace Host {
namespace WasiComponent {

/// The time of the clock Id in nanoseconds.
inline Expect<uint64_t> readClock(__wasi_clockid_t Id) noexcept {
  __wasi_timestamp_t Now = 0;
  if (auto Res = WASI::Clock::clockTimeGet(Id, 1, Now); !Res) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }
  return static_cast<uint64_t>(Now);
}

/// The resolution of the clock Id in nanoseconds.
inline Expect<uint64_t> readResolution(__wasi_clockid_t Id) noexcept {
  __wasi_timestamp_t Resolution = 0;
  if (auto Res = WASI::Clock::clockResGet(Id, Resolution); !Res) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }
  return static_cast<uint64_t>(Resolution);
}

} // namespace WasiComponent
} // namespace Host
} // namespace WasmEdge
