#include "avCodec.h"

extern "C" {
  #include "libavcodec/avcodec.h"
}

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVcodec {

Expect<uint32_t> AVCodec_ID::body(const Runtime::CallingFrame &,uint32_t avCodecId){

  auto* ffmpegMemory = Env.get();
  const AVCodec* avCodecCtx = static_cast<const AVCodec*>(ffmpegMemory->fetchData(avCodecId));

  return FFmpegUtils::CodecID::fromAVCodecID(avCodecCtx->id);
}

Expect<int32_t> AVCodec_Type::body(const Runtime::CallingFrame &,uint32_t avCodecId){

  auto* ffmpegMemory = Env.get();
  const AVCodec* avCodecCtx = static_cast<const AVCodec*>(ffmpegMemory->fetchData(avCodecId));

  return FFmpegUtils::MediaType::fromMediaType(avCodecCtx->type);
}

}
}
}
}

