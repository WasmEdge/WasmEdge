// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "ffmpeg_base.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVUtil {

class AVGetTime : public HostFunction<AVGetTime> {
public:
  using HostFunction::HostFunction;
  Expect<int64_t> body(const Runtime::CallingFrame &Frame);
};

class AVGetTimeRelative : public HostFunction<AVGetTimeRelative> {
public:
  using HostFunction::HostFunction;
  Expect<int64_t> body(const Runtime::CallingFrame &Frame);
};

class AVGetTimeRelativeIsMonotonic
    : public HostFunction<AVGetTimeRelativeIsMonotonic> {
public:
  using HostFunction::HostFunction;
  Expect<int64_t> body(const Runtime::CallingFrame &Frame);
};

class AVUSleep : public HostFunction<AVUSleep> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t USec);
};

} // namespace AVUtil
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
