#include "avCodecContext.h"

extern "C" {
  #include "libavcodec/avcodec.h"
}

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVcodec {

Expect<uint32_t> AVCodecCtxCodecID::body(const Runtime::CallingFrame &,uint32_t avCodecCtxId){

  auto* ffmpegMemory = Env.get();
  AVCodecContext* avCodecCtx = static_cast<AVCodecContext*>(ffmpegMemory->fetchData(avCodecCtxId));
  AVCodecID avCodecId = avCodecCtx->codec_id;

  const char* name = avcodec_get_name(avCodecId);
  printf("CodecID %s\n",name);

  return FFmpegUtils::CodecID::fromAVCodecID(avCodecId);
}

Expect<uint32_t> AVCodecCtxCodecType::body(const Runtime::CallingFrame &,uint32_t avCodecCtxId){

  auto* ffmpegMemory = Env.get();
  AVCodecContext* avCodecCtx = static_cast<AVCodecContext*>(ffmpegMemory->fetchData(avCodecCtxId));
  AVMediaType avMediaType = avCodecCtx->codec_type;

  return FFmpegUtils::MediaType::fromMediaType(avMediaType);
}

}
}
}
}
