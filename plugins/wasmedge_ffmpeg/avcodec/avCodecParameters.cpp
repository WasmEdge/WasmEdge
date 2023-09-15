#include "avCodecParameters.h"

extern "C" {
#include "libavcodec/avcodec.h"
}

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVcodec {

Expect<uint32_t> AVCodecParamCodecId::body(const Runtime::CallingFrame &,uint32_t AvCodecParamId){

  FFMPEG_PTR_FETCH(AvCodecParams,AvCodecParamId,AVCodecParameters ,"",true);
  return FFmpegUtils::CodecID::fromAVCodecID(AvCodecParams->codec_id);
}

Expect<int32_t> AVCodecParamCodecType::body(const Runtime::CallingFrame &,uint32_t AvCodecParamId){

  FFMPEG_PTR_FETCH(AvCodecParams,AvCodecParamId,AVCodecParameters ,"",true);
  return FFmpegUtils::MediaType::fromMediaType(AvCodecParams->codec_type);
}

} // namespace AVcodec
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
