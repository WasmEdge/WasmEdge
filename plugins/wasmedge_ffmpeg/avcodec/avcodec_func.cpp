#include "avcodec_func.h"

extern "C" {
    #include "libavcodec/avcodec.h"
}

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVcodec {

Expect<int32_t> AVCodecAllocContext3::body(const Runtime::CallingFrame &Frame, uint32_t avCodecId,uint32_t avCodecCtxPtr){
    MEMINST_CHECK(MemInst,Frame,0);
    MEM_PTR_CHECK(avCodecCtxId,MemInst,uint32_t,avCodecCtxPtr,"Failed when accessing the return AVCodecContext Memory",true);

    auto* ffmpegMemory = Env.get();

    AVCodec* avCodec = NULL;
    if(avCodecId)
      avCodec = static_cast<AVCodec*>(ffmpegMemory->fetchData(avCodecId));

    AVCodecContext* avCodecCtx = avcodec_alloc_context3(avCodec);
    ffmpegMemory->alloc(avCodecCtx,avCodecCtxId);
    return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVCodecParametersFromContext::body(const Runtime::CallingFrame &, uint32_t avCodecCtxId,uint32_t avCodecParamId){

  auto* ffmpegMemory = Env.get();

  AVCodecContext* avCodecCtx = static_cast<AVCodecContext*>(ffmpegMemory->fetchData(avCodecCtxId));
  AVCodecParameters* avCodecParam = static_cast<AVCodecParameters*>(ffmpegMemory->fetchData(avCodecParamId));

//  printf("I am here %p, %p\n",avCodecCtx,avCodecParam);
//  return 1;
  return avcodec_parameters_from_context(avCodecParam,avCodecCtx);
}


Expect<int32_t> AVCodecParametersFree::body(const Runtime::CallingFrame &,uint32_t avCodecParamId){

  auto* ffmpegMemory = Env.get();
  AVCodecParameters* avCodecParam = static_cast<AVCodecParameters*>(ffmpegMemory->fetchData(avCodecParamId));

  avcodec_parameters_free(&avCodecParam);
  ffmpegMemory->dealloc(avCodecParamId);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVCodecFreeContext::body(const Runtime::CallingFrame &,uint32_t avCodecCtxId){

  auto* ffmpegMemory = Env.get();

  AVCodecContext* avCodecCtx = static_cast<AVCodecContext*>(ffmpegMemory->fetchData(avCodecCtxId));

  avcodec_free_context(&avCodecCtx);
  ffmpegMemory->dealloc(avCodecCtxId);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVCodecParametersAlloc::body(const Runtime::CallingFrame &Frame,uint32_t avCodecParamPtr){
  MEMINST_CHECK(MemInst,Frame,0);
  MEM_PTR_CHECK(avCodecParamId,MemInst,uint32_t,avCodecParamPtr,"Failed when accessing the return AVCodecParameters Memory",false);

  auto* ffmpegMemory = Env.get();

  AVCodecParameters* avCodecParam = avcodec_parameters_alloc();
  ffmpegMemory->alloc(avCodecParam,avCodecParamId);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVCodecGetType::body(const Runtime::CallingFrame &,uint32_t avCodecIDIndex){
  AVCodecID avCodecID = FFmpegUtils::CodecID::intoAVCodecID(avCodecIDIndex);
  AVMediaType mediaType = avcodec_get_type(avCodecID);

  return FFmpegUtils::MediaType::fromMediaType(mediaType);
}


Expect<int32_t> AVCodecOpen2::body(const Runtime::CallingFrame &,uint32_t avCodecCtxId,uint32_t avCodecId,uint32_t avDictionaryId){

  auto* ffmpegMemory = Env.get();

  AVCodecContext* avCodecCtx = NULL;
  AVCodec* avCodec = NULL;
  AVDictionary** avDictionary = NULL;

  if(avCodecCtxId)
    avCodecCtx = static_cast<AVCodecContext*>(ffmpegMemory->fetchData(avCodecCtxId));
  if(avDictionaryId)
    avDictionary = static_cast<AVDictionary**>(ffmpegMemory->fetchData(avDictionaryId));
  if(avCodecId)
    avCodec = static_cast<AVCodec*>(ffmpegMemory->fetchData(avCodecId));

  return avcodec_open2(avCodecCtx,avCodec,avDictionary);
}

Expect<int32_t> AVCodecFindDecoder::body(const Runtime::CallingFrame &Frame,uint32_t avCodecIDIndex,uint32_t avCodecPtr){

  MEMINST_CHECK(MemInst,Frame,0);
  MEM_PTR_CHECK(avCodecId,MemInst,uint32_t,avCodecPtr,"Failed when accessing the return AVCodec Memory",true);

  AVCodecID avCodecID = FFmpegUtils::CodecID::intoAVCodecID(avCodecIDIndex);

  auto* ffmpegMemory = Env.get();
  const AVCodec* avCodec = avcodec_find_decoder(avCodecID);

  if(avCodec == NULL) {
    *avCodecId = 0;
    return static_cast<int32_t>(ErrNo::Success);
  }

  ffmpegMemory->alloc(const_cast<AVCodec*>(avCodec),avCodecId);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVCodecIsEncoder::body(const Runtime::CallingFrame &,uint32_t avCodecId){

  auto* ffmpegMemory = Env.get();
  const AVCodec* avCodec = static_cast<const AVCodec*>(ffmpegMemory->fetchData(avCodecId));

  return av_codec_is_encoder(avCodec);
}

Expect<int32_t> AVCodecIsDecoder::body(const Runtime::CallingFrame &,uint32_t avCodecId){
  auto* ffmpegMemory = Env.get();
  const AVCodec* avCodec = static_cast<const AVCodec*>(ffmpegMemory->fetchData(avCodecId));

  return av_codec_is_decoder(avCodec);
}

Expect<int32_t> AVCodecClose::body(const Runtime::CallingFrame &,uint32_t avCodecId){
  auto* ffmpegMemory = Env.get();
  AVCodecContext* avCodecCtx = static_cast<AVCodecContext*>(ffmpegMemory->fetchData(avCodecId));

  int res = avcodec_close(avCodecCtx);
  ffmpegMemory->dealloc(avCodecId);
  return res;
}

Expect<int32_t> AVCodecParametersToContext::body(const Runtime::CallingFrame &,uint32_t avCodecCtxId,uint32_t avCodecParamId){
  auto* ffmpegMemory = Env.get();

  AVCodecContext* avCodecCtx = static_cast<AVCodecContext*>(ffmpegMemory->fetchData(avCodecCtxId));
  AVCodecParameters* avCodecParam = static_cast<AVCodecParameters*>(ffmpegMemory->fetchData(avCodecParamId));

  return avcodec_parameters_to_context(avCodecCtx,avCodecParam);

}

}
}
}
}
