#include "swscale_func.h"

extern "C" {
  #include "libswscale/swscale.h"
  #include "libavutil/frame.h"
}

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace SWScale{

Expect<int32_t> SwsGetContext::body(const Runtime::CallingFrame &Frame,uint32_t SwsCtxPtr,uint32_t SrcW,uint32_t SrcH,uint32_t SrcAvPixFormatId,uint32_t DesW,uint32_t DesH,uint32_t DesAvPixFormatId,int32_t Flags,uint32_t SrcFilterId, uint32_t DesFilterId){

  MEMINST_CHECK(MemInst,Frame,0);
  MEM_PTR_CHECK(SwsCtxId,MemInst,uint32_t,SwsCtxPtr,"")

  FFMPEG_PTR_FETCH(SwsCtx,*SwsCtxId,SwsContext)
  FFMPEG_PTR_FETCH(SrcSwsFilter,SrcFilterId,SwsFilter)
  FFMPEG_PTR_FETCH(DesSwsFilter,DesFilterId,SwsFilter)

  AVPixelFormat const SrcAvPixelFormat = FFmpegUtils::PixFmt::intoAVPixFmt(SrcAvPixFormatId);
  AVPixelFormat const DestAvPixelFormat = FFmpegUtils::PixFmt::intoAVPixFmt(DesAvPixFormatId);
  SwsCtx = sws_getContext(SrcW,SrcH,SrcAvPixelFormat,DesW,DesH,DestAvPixelFormat,Flags,SrcSwsFilter,DesSwsFilter,NULL); // Not using param anywhere in Rust SDK.
  FFMPEG_PTR_STORE(SwsCtx,SwsCtxId);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> SwsFreeContext::body(const Runtime::CallingFrame &,uint32_t SwsCtxId){

  FFMPEG_PTR_FETCH(SwsCtx,SwsCtxId,SwsContext)
  sws_freeContext(SwsCtx);
  FFMPEG_PTR_DELETE(SwsCtxId);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> SwsScale::body(const Runtime::CallingFrame &,uint32_t SwsCtxId,uint32_t InputFrameId,int32_t SrcSliceY,int32_t SrcSliceH,uint32_t OutputFrameId){

  FFMPEG_PTR_FETCH(SwsCtx,SwsCtxId,SwsContext);
  FFMPEG_PTR_FETCH(InputFrame,InputFrameId,AVFrame);
  FFMPEG_PTR_FETCH(OutputFrame,OutputFrameId,AVFrame);
  sws_scale(SwsCtx,InputFrame->data,InputFrame->linesize,SrcSliceY,SrcSliceH,OutputFrame->data,OutputFrame->linesize);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> SwsGetCachedContext::body(const Runtime::CallingFrame &Frame,uint32_t SwsCachedCtxPtr,uint32_t SwsCtxId,uint32_t SrcW,uint32_t SrcH,uint32_t SrcAvPixFormatId,uint32_t DesW,uint32_t DesH,uint32_t DesAvPixFormatId,int32_t Flags,uint32_t SrcFilterId, uint32_t DesFilterId){

  MEMINST_CHECK(MemInst,Frame,0);
  MEM_PTR_CHECK(SwsCachedCtxId,MemInst,uint32_t,SwsCachedCtxPtr,"")

  FFMPEG_PTR_FETCH(SwsCachedCtx,*SwsCachedCtxId,SwsContext);
  FFMPEG_PTR_FETCH(SwsCtx,SwsCtxId,SwsContext);
  FFMPEG_PTR_FETCH(SrcSwsFilter,SrcFilterId,SwsFilter)
  FFMPEG_PTR_FETCH(DesSwsFilter,DesFilterId,SwsFilter)

  AVPixelFormat const SrcAvPixelFormat = FFmpegUtils::PixFmt::intoAVPixFmt(SrcAvPixFormatId);
  AVPixelFormat const DestAvPixelFormat = FFmpegUtils::PixFmt::intoAVPixFmt(DesAvPixFormatId);
  SwsCachedCtx = sws_getCachedContext(SwsCtx,SrcW,SrcH,SrcAvPixelFormat,DesW,DesH,DestAvPixelFormat,Flags,SrcSwsFilter,DesSwsFilter,NULL);
  FFMPEG_PTR_STORE(SwsCachedCtx,SwsCachedCtxId);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> SwsIsSupportedInput::body(const Runtime::CallingFrame &,uint32_t AvPixFormatId){
  AVPixelFormat const AvPixelFormat = FFmpegUtils::PixFmt::intoAVPixFmt(AvPixFormatId);
  return sws_isSupportedInput(AvPixelFormat);
}

Expect<int32_t> SwsIsSupportedOutput::body(const Runtime::CallingFrame &,uint32_t AvPixFormatId){
  AVPixelFormat const AvPixelFormat = FFmpegUtils::PixFmt::intoAVPixFmt(AvPixFormatId);
  return sws_isSupportedOutput(AvPixelFormat);
}


Expect<int32_t> SwsIsSupportedEndiannessConversion::body(const Runtime::CallingFrame &,uint32_t AvPixFormatId){
  AVPixelFormat const AvPixelFormat = FFmpegUtils::PixFmt::intoAVPixFmt(AvPixFormatId);
  return sws_isSupportedEndiannessConversion(AvPixelFormat);
}

} // namespace SWScale
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
