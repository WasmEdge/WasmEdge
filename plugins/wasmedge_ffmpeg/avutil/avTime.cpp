// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "avTime.h"

extern "C" {
#include "libavutil/time.h"
}

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVUtil {

Expect<int64_t> AVGetTime::body(const Runtime::CallingFrame &) {
  return av_gettime();
}

Expect<int64_t> AVGetTimeRelative::body(const Runtime::CallingFrame &) {
  return av_gettime_relative();
}

Expect<int64_t>
AVGetTimeRelativeIsMonotonic::body(const Runtime::CallingFrame &) {
  return av_gettime_relative_is_monotonic();
}

Expect<int32_t> AVUSleep::body(const Runtime::CallingFrame &, uint32_t USec) {
  return av_usleep(USec);
}

} // namespace AVUtil
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
