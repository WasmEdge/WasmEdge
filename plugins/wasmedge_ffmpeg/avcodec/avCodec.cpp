#include "avCodec.h"

extern "C" {
#include "libavcodec/avcodec.h"
}

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVcodec {

Expect<uint32_t> AVCodecID::body(const Runtime::CallingFrame &,
                                 uint32_t AvCodecId) {

  FFMPEG_PTR_FETCH(AvCodec, AvCodecId, const AVCodec);
  return FFmpegUtils::CodecID::fromAVCodecID(AvCodec->id);
}

Expect<int32_t> AVCodecType::body(const Runtime::CallingFrame &,
                                  uint32_t AvCodecId) {

  FFMPEG_PTR_FETCH(AvCodec, AvCodecId, const AVCodec);
  return FFmpegUtils::MediaType::fromMediaType(AvCodec->type);
}

} // namespace AVcodec
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
