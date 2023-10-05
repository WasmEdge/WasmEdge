#pragma once
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

#include "avutil_base.h"
#include "ffmpeg_env.h"
#include "runtime/hostfunc.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVUtil {

class AVGetChannelLayoutNbChannels
    : public WasmEdgeFFmpegAVUtil<AVGetChannelLayoutNbChannels> {
public:
  AVGetChannelLayoutNbChannels(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint64_t ChannelLayoutId);
};

class AVGetDefaultChannelLayout
    : public WasmEdgeFFmpegAVUtil<AVGetDefaultChannelLayout> {
public:
  AVGetDefaultChannelLayout(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<uint64_t> body(const Runtime::CallingFrame &Frame,
                        int32_t ChannelLayoutId);
};

} // namespace AVUtil
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
