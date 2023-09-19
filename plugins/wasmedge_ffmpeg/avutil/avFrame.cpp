#include "avFrame.h"

extern "C" {
#include "libavutil/frame.h"
#include "libavutil/pixfmt.h"
}

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVUtil{

Expect<int32_t> AVFrameAlloc::body(const Runtime::CallingFrame &Frame,uint32_t FramePtr){
  MEMINST_CHECK(MemInst,Frame,0)
  MEM_PTR_CHECK(FrameId,MemInst,uint32_t ,FramePtr,"Failed to access Memory for AVFrame")

  AVFrame* AvFrame = av_frame_alloc();
  FFMPEG_PTR_STORE(AvFrame,FrameId);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVFrameFree::body(const Runtime::CallingFrame &,uint32_t FrameId){

  FFMPEG_PTR_FETCH(AvFrame,FrameId,AVFrame);
  av_frame_free(&AvFrame);
  FFMPEG_PTR_DELETE(FrameId);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<uint32_t> AVFrameWidth::body(const Runtime::CallingFrame &,uint32_t FrameId){

  FFMPEG_PTR_FETCH(AvFrame,FrameId,AVFrame);
  return AvFrame->width;
}

Expect<uint32_t> AVFrameHeight::body(const Runtime::CallingFrame &,uint32_t FrameId){

  FFMPEG_PTR_FETCH(AvFrame,FrameId,AVFrame);
  return AvFrame->height;
}

Expect<void> AVFrameSetHeight::body(const Runtime::CallingFrame &,uint32_t FrameId,uint32_t Height){

  auto* ffmpegMemory = Env.get();
  AVFrame* AvFrame = static_cast<AVFrame*>(ffmpegMemory->fetchData(FrameId));
  AvFrame->height = Height;
  return {};
}

Expect<void> AVFrameSetWidth::body(const Runtime::CallingFrame &,uint32_t FrameId, uint32_t Width){

  auto* ffmpegMemory = Env.get();
  AVFrame* AvFrame = static_cast<AVFrame*>(ffmpegMemory->fetchData(FrameId));
  AvFrame->width = Width;
  return {};
}

Expect<uint32_t > AVFrameFormat::body(const Runtime::CallingFrame &,uint32_t FrameId){

  FFMPEG_PTR_FETCH(AvFrame,FrameId,AVFrame);
  AVPixelFormat const PixelFormat = static_cast<AVPixelFormat>(AvFrame->format);
  return FFmpegUtils::PixFmt::fromAVPixFmt(PixelFormat);
}

Expect<uint32_t> AVFrameSetFormat::body(const Runtime::CallingFrame &,uint32_t FrameId,uint32_t AvPixFormatId){

  FFMPEG_PTR_FETCH(AvFrame,FrameId,AVFrame);
  AVPixelFormat const PixelFormat = static_cast<AVPixelFormat>(AvPixFormatId);
  AvFrame->format = PixelFormat;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVFrameIsNull::body(const Runtime::CallingFrame &,uint32_t FrameId){

  FFMPEG_PTR_FETCH(AvFrame,FrameId,AVFrame);
  return AvFrame->data[0] == nullptr;
}

Expect<int32_t> AVFrameLinesize::body(const Runtime::CallingFrame &,uint32_t FrameId,uint32_t Idx){

  FFMPEG_PTR_FETCH(AvFrame,FrameId,AVFrame);
  return AvFrame->linesize[Idx];
}

Expect<int32_t> AVFrameData::body(const Runtime::CallingFrame &Frame,uint32_t FrameId,uint32_t FrameBufPtr,uint32_t FrameBufLen){

  MEMINST_CHECK(MemInst,Frame,0)
  MEM_SPAN_CHECK(Buffer,MemInst,uint8_t,FrameBufPtr,FrameBufLen,"");
  FFMPEG_PTR_FETCH(AvFrame,FrameId,AVFrame);

  uint8_t** Data = AvFrame->data;
  memmove(Buffer.data(),*Data,FrameBufLen);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVFrameGetBuffer::body(const Runtime::CallingFrame &,uint32_t FrameId,int32_t Align){

  FFMPEG_PTR_FETCH(AvFrame,FrameId,AVFrame);
  return av_frame_get_buffer(AvFrame,Align);
}

} // namespace AVUtil
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
