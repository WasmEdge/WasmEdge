#include "avCodecParameters.h"

extern "C" {
#include "libavcodec/avcodec.h"
}

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVcodec {

Expect<uint32_t> AVCodecParam_codecId::body(const Runtime::CallingFrame &,uint32_t avCodecParamId){
  auto* ffmpegMemory = Env.get();
  const AVCodecParameters* avCodecParams = static_cast<AVCodecParameters*>(ffmpegMemory->fetchData(avCodecParamId));

  return FFmpegUtils::CodecID::fromAVCodecID(avCodecParams->codec_id);
}

Expect<int32_t> AVCodecParam_codecType::body(const Runtime::CallingFrame &,uint32_t avCodecParamId){

  auto* ffmpegMemory = Env.get();
  const AVCodecParameters* avCodecParams = static_cast<AVCodecParameters*>(ffmpegMemory->fetchData(avCodecParamId));

  return FFmpegUtils::MediaType::fromMediaType(avCodecParams->codec_type);
}

}
}
}
}
