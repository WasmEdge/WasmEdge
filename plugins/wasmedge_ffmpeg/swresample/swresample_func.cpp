#include "swresample_func.h"

extern "C"{
  #include "libswresample/swresample.h"
  #include "libavutil/avutil.h"
  #include "libavutil/opt.h"
}

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace SWResample {

Expect<int32_t> SWResampleVersion::body(const Runtime::CallingFrame &){
  return swresample_version();
}

Expect<int64_t> SWRGetDelay::body(const Runtime::CallingFrame &,uint32_t SWRContextId,int64_t Base){

  FFMPEG_PTR_FETCH(SWRContext,SWRContextId,SwrContext);
  return swr_get_delay(SWRContext,Base);
}

Expect<int32_t> SWRInit::body(const Runtime::CallingFrame &,uint32_t SWRContextId){

  FFMPEG_PTR_FETCH(SWRContext,SWRContextId,SwrContext);
  return swr_init(SWRContext);
}

Expect<int32_t> SWRAllocSetOpts::body(const Runtime::CallingFrame &Frame,uint32_t SwrCtxPtr,uint32_t SWRContextId,int64_t OutChLayoutId,uint32_t OutSampleFmtId,int32_t OutSampleRate,int64_t InChLayoutId, uint32_t InSampleFmtId,int32_t InSampleRate,int32_t LogOffset){

  MEMINST_CHECK(MemInst,Frame,0);
  MEM_PTR_CHECK(SwrCtxId,MemInst,uint32_t,SwrCtxPtr,"")
  FFMPEG_PTR_FETCH(CurrSwrCtx,*SwrCtxId,SwrContext);
  FFMPEG_PTR_FETCH(SWRContext,SWRContextId,SwrContext);

  uint64_t const OutChLayout = FFmpegUtils::ChannelLayout::fromChannelLayoutID(OutChLayoutId);
  AVSampleFormat const OutSampleFmt = FFmpegUtils::SampleFmt::fromSampleID(OutSampleFmtId);
  uint64_t const InChLayout = FFmpegUtils::ChannelLayout::fromChannelLayoutID(InChLayoutId);
  AVSampleFormat const InSampleFmt = FFmpegUtils::SampleFmt::fromSampleID(InSampleFmtId);
  CurrSwrCtx = swr_alloc_set_opts(SWRContext,OutChLayout,OutSampleFmt,OutSampleRate,InChLayout,InSampleFmt,InSampleRate,LogOffset,NULL); // Always being used as null in rust sdk.
  FFMPEG_PTR_STORE(CurrSwrCtx,SwrCtxId);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVOptSetDict::body(const Runtime::CallingFrame &,uint32_t SWRContextId,uint32_t DictId){

  FFMPEG_PTR_FETCH(SWRContext,SWRContextId,SwrContext);
  FFMPEG_PTR_FETCH(AvDictionary,DictId,AVDictionary*);
  return av_opt_set_dict(SWRContext,AvDictionary);
}

Expect<int32_t> SWRConvertFrame::body(const Runtime::CallingFrame &,uint32_t SWRContextId,uint32_t FrameOutputId, uint32_t FrameInputId){
  FFMPEG_PTR_FETCH(SWRContext,SWRContextId,SwrContext);
  FFMPEG_PTR_FETCH(OuputFrame,FrameOutputId,AVFrame);
  FFMPEG_PTR_FETCH(InputFrame,FrameInputId,AVFrame);

  return swr_convert_frame(SWRContext,OuputFrame,InputFrame);
}

Expect<int32_t> SWRFree::body(const Runtime::CallingFrame &,uint32_t SWRContextId){
  FFMPEG_PTR_FETCH(SWRContext,SWRContextId,SwrContext);
  swr_close(SWRContext);
  return static_cast<int32_t>(ErrNo::Success);
}

} // namespace SWResample
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
