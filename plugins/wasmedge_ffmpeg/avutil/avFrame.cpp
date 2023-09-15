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
  MEMINST_CHECK(MemInst,Frame,0);
  MEM_PTR_CHECK(FrameId,MemInst,uint32_t ,FramePtr,"Failed to access Memory for AVFrame",true);

  auto* ffmpegMemory = Env.get();
  AVFrame* AvFrame = av_frame_alloc();
  ffmpegMemory->alloc(AvFrame,FrameId);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVFrameFree::body(const Runtime::CallingFrame &,uint32_t FrameId){

  auto* ffmpegMemory = Env.get();
  AVFrame* AvFrame = static_cast<AVFrame*>(ffmpegMemory->fetchData(FrameId));
  av_frame_free(&AvFrame);
  ffmpegMemory->dealloc(FrameId);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<uint32_t> AVFrameWidth::body(const Runtime::CallingFrame &,uint32_t FrameId){

  auto* ffmpegMemory = Env.get();
  AVFrame* AvFrame = static_cast<AVFrame*>(ffmpegMemory->fetchData(FrameId));
  return AvFrame->width;
}

Expect<uint32_t> AVFrameHeight::body(const Runtime::CallingFrame &,uint32_t FrameId){

  auto* ffmpegMemory = Env.get();
  AVFrame* AvFrame = static_cast<AVFrame*>(ffmpegMemory->fetchData(FrameId));
  return AvFrame->height;
}

Expect<void> AVFrameSetHeight::body(const Runtime::CallingFrame &,uint32_t FrameId,uint32_t height){

  auto* ffmpegMemory = Env.get();
  AVFrame* AvFrame = static_cast<AVFrame*>(ffmpegMemory->fetchData(FrameId));
  AvFrame->height = height;
  return {};
}

Expect<void> AVFrameSetWidth::body(const Runtime::CallingFrame &,uint32_t FrameId, uint32_t width){

  auto* ffmpegMemory = Env.get();
  AVFrame* AvFrame = static_cast<AVFrame*>(ffmpegMemory->fetchData(FrameId));
  AvFrame->width = width;
  return {};
}

Expect<uint32_t > AVFrameFormat::body(const Runtime::CallingFrame &,uint32_t FrameId){

  auto* ffmpegMemory = Env.get();
  AVFrame* AvFrame = static_cast<AVFrame*>(ffmpegMemory->fetchData(FrameId));

  AVPixelFormat const PixelFormat = static_cast<AVPixelFormat>(AvFrame->format);
  return FFmpegUtils::PixFmt::fromAVPixFmt(PixelFormat);
}


}
}
}
}
