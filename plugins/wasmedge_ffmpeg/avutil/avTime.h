#pragma once
#include "avutil_base.h"
#include "runtime/callingframe.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVUtil {

class AVGetTime : public WasmEdgeFFmpegAVUtil<AVGetTime> {
public:
  AVGetTime(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int64_t> body(const Runtime::CallingFrame &Frame);
};

class AVGetTimeRelative : public WasmEdgeFFmpegAVUtil<AVGetTimeRelative> {
public:
  AVGetTimeRelative(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int64_t> body(const Runtime::CallingFrame &Frame);
};

class AVGetTimeRelativeIsMonotonic
    : public WasmEdgeFFmpegAVUtil<AVGetTimeRelativeIsMonotonic> {
public:
  AVGetTimeRelativeIsMonotonic(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int64_t> body(const Runtime::CallingFrame &Frame);
};

class AVUSleep : public WasmEdgeFFmpegAVUtil<AVUSleep> {
public:
  AVUSleep(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t USec);
};

} // namespace AVUtil
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
