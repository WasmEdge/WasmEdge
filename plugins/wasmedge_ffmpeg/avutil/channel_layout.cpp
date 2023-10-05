#include "channel_layout.h"
extern "C" {
#include "libavutil/channel_layout.h"
}

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVUtil {

Expect<int32_t>
AVGetChannelLayoutNbChannels::body(const Runtime::CallingFrame &,
                                   uint64_t ChannelLayoutId) {
  uint64_t const ChannelLay =
      FFmpegUtils::ChannelLayout::fromChannelLayoutID(ChannelLayoutId);
  return av_get_channel_layout_nb_channels(ChannelLay);
}

Expect<uint64_t> AVGetDefaultChannelLayout::body(const Runtime::CallingFrame &,
                                                 int32_t Number) {
  uint64_t const ChannelLayout = av_get_default_channel_layout(Number);
  return FFmpegUtils::ChannelLayout::intoAVChannelID(ChannelLayout);
}

} // namespace AVUtil
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
