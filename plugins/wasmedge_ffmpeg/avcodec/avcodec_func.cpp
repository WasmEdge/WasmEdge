#include "avcodec_func.h"

extern "C" {
    #include "libavcodec/avcodec.h"
}

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVcodec {

Expect<int32_t> AVCodecAllocContext3::body(const Runtime::CallingFrame &Frame, uint32_t AvCodecId,uint32_t AvCodecCtxPtr){

    MEMINST_CHECK(MemInst,Frame,0);
    MEM_PTR_CHECK(avCodecCtxId,MemInst,uint32_t,AvCodecCtxPtr,"Failed when accessing the return AVCodecContext Memory");
    FFMPEG_PTR_FETCH(AvCodec,AvCodecId,AVCodec);

    AVCodecContext* AvCodecCtx = avcodec_alloc_context3(AvCodec);
    FFMPEG_PTR_STORE(AvCodecCtx,avCodecCtxId);
    return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVCodecParametersFromContext::body(const Runtime::CallingFrame &,uint32_t AvCodecCtxId,uint32_t AvCodecParamId){

  FFMPEG_PTR_FETCH(AvCodecParam,AvCodecParamId,AVCodecParameters); // Check isRequiredField.
  FFMPEG_PTR_FETCH(AvCodecCtx,AvCodecCtxId,AVCodecContext); // Check isRequiredField.
  return avcodec_parameters_from_context(AvCodecParam,AvCodecCtx);
}


Expect<int32_t> AVCodecParametersFree::body(const Runtime::CallingFrame &,uint32_t AvCodecParamId){

  FFMPEG_PTR_FETCH(AvCodecParam,AvCodecParamId,AVCodecParameters);

  avcodec_parameters_free(&AvCodecParam);
  FFMPEG_PTR_DELETE(AvCodecParamId);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVCodecFreeContext::body(const Runtime::CallingFrame &,uint32_t AvCodecCtxId){

  FFMPEG_PTR_FETCH(AvCodecCtx,AvCodecCtxId,AVCodecContext);

  avcodec_free_context(&AvCodecCtx);
  FFMPEG_PTR_DELETE(AvCodecCtxId);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVCodecParametersAlloc::body(const Runtime::CallingFrame &Frame,uint32_t AvCodecParamPtr){
  MEMINST_CHECK(MemInst,Frame,0);
  MEM_PTR_CHECK(AvCodecParamId,MemInst,uint32_t,AvCodecParamPtr,"Failed when accessing the return AVCodecParameters Memory");

  FFMPEG_PTR_FETCH(AvCodecParam,*AvCodecParamId,AVCodecParameters); // Check isRequiredField.
  AvCodecParam = avcodec_parameters_alloc();
  FFMPEG_PTR_STORE(AvCodecParam,AvCodecParamId);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVCodecGetType::body(const Runtime::CallingFrame &,uint32_t AvCodecIdIndex){

  AVCodecID const AvCodecId = FFmpegUtils::CodecID::intoAVCodecID(AvCodecIdIndex);
  AVMediaType const MediaType = avcodec_get_type(AvCodecId);
  return FFmpegUtils::MediaType::fromMediaType(MediaType);
}


Expect<int32_t> AVCodecOpen2::body(const Runtime::CallingFrame &,uint32_t AvCodecCtxId,uint32_t AvCodecId,uint32_t AvDictionaryId){

  FFMPEG_PTR_FETCH(AvCodecCtx,AvCodecCtxId,AVCodecContext); // Check isRequiredField.
  FFMPEG_PTR_FETCH(AvDictionary,AvDictionaryId,AVDictionary*); // Check isRequiredField.
  FFMPEG_PTR_FETCH(AvCodec,AvCodecId,AVCodec); // Check isRequiredField.
  return avcodec_open2(AvCodecCtx,AvCodec,AvDictionary);
}

Expect<int32_t> AVCodecFindDecoder::body(const Runtime::CallingFrame &Frame,uint32_t AvCodecIdIndex,uint32_t AvCodecPtr){

  MEMINST_CHECK(MemInst,Frame,0);
  MEM_PTR_CHECK(avCodecId,MemInst,uint32_t,AvCodecPtr,"Failed when accessing the return AVCodec Memory");

  AVCodecID const AvCodecId = FFmpegUtils::CodecID::intoAVCodecID(AvCodecIdIndex);

  const AVCodec* AvCodec = avcodec_find_decoder(AvCodecId);

  // Setting AvCodec value as NULL.
  if(AvCodec == NULL) {
    *avCodecId = 0;
    return static_cast<int32_t>(ErrNo::Success);
  }

  FFMPEG_PTR_STORE(const_cast<AVCodec*>(AvCodec),avCodecId);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVCodecIsEncoder::body(const Runtime::CallingFrame &,uint32_t AvCodecId){

  FFMPEG_PTR_FETCH(AvCodec,AvCodecId,const AVCodec);
  return av_codec_is_encoder(AvCodec);
}

Expect<int32_t> AVCodecIsDecoder::body(const Runtime::CallingFrame &,uint32_t AvCodecId){

  FFMPEG_PTR_FETCH(AvCodec,AvCodecId,const AVCodec);
  return av_codec_is_decoder(AvCodec);
}

Expect<int32_t> AVCodecClose::body(const Runtime::CallingFrame &,uint32_t AvCodecCtxId){

  FFMPEG_PTR_FETCH(AvCodecCtx,AvCodecCtxId,AVCodecContext);
  int Res = avcodec_close(AvCodecCtx);
  FFMPEG_PTR_DELETE(AvCodecCtxId);
  return Res;
}

Expect<int32_t> AVCodecParametersToContext::body(const Runtime::CallingFrame &,uint32_t AvCodecCtxId,uint32_t AvCodecParamId){

  FFMPEG_PTR_FETCH(AvCodecCtx,AvCodecCtxId,AVCodecContext);
  FFMPEG_PTR_FETCH(AvCodecParam,AvCodecParamId,AVCodecParameters);

  return avcodec_parameters_to_context(AvCodecCtx,AvCodecParam);
}

Expect<int32_t> AVCodecReceiveFrame::body(const Runtime::CallingFrame &,uint32_t AvCodecCtxId,uint32_t FrameId){

  FFMPEG_PTR_FETCH(AvCodecCtx,AvCodecCtxId,AVCodecContext);
  FFMPEG_PTR_FETCH(AvFrame,FrameId,AVFrame);
  return avcodec_receive_frame(AvCodecCtx,AvFrame);
}

Expect<int32_t> AVCodecSendPacket::body(const Runtime::CallingFrame &,uint32_t AvCodecCtxId,uint32_t PacketId){

  FFMPEG_PTR_FETCH(AVCodecCtx,AvCodecCtxId,AVCodecContext);
  FFMPEG_PTR_FETCH(AvPacket,PacketId,AVPacket); // Can send Null AVPacket, to close the stream.
  return avcodec_send_packet(AVCodecCtx,AvPacket);
}

} // namespace AVcodec
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
