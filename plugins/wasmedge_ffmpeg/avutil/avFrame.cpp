#include "avFrame.h"

extern "C" {
#include "libavutil/frame.h"
#include "libavutil/pixfmt.h"
}

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVUtil {

Expect<int32_t> AVFrameAlloc::body(const Runtime::CallingFrame &Frame,
                                   uint32_t FramePtr) {
  MEMINST_CHECK(MemInst, Frame, 0)
  MEM_PTR_CHECK(FrameId, MemInst, uint32_t, FramePtr,
                "Failed to access Memory for AVFrame")

  AVFrame *AvFrame = av_frame_alloc();
  FFMPEG_PTR_STORE(AvFrame, FrameId);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVFrameFree::body(const Runtime::CallingFrame &,
                                  uint32_t FrameId) {

  FFMPEG_PTR_FETCH(AvFrame, FrameId, AVFrame);
  av_frame_free(&AvFrame);
  FFMPEG_PTR_DELETE(FrameId);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<uint32_t> AVFrameWidth::body(const Runtime::CallingFrame &,
                                    uint32_t FrameId) {

  FFMPEG_PTR_FETCH(AvFrame, FrameId, AVFrame);
  return AvFrame->width;
}

Expect<uint32_t> AVFrameHeight::body(const Runtime::CallingFrame &,
                                     uint32_t FrameId) {

  FFMPEG_PTR_FETCH(AvFrame, FrameId, AVFrame);
  return AvFrame->height;
}

Expect<void> AVFrameSetHeight::body(const Runtime::CallingFrame &,
                                    uint32_t FrameId, uint32_t Height) {

  auto *ffmpegMemory = Env.get();
  AVFrame *AvFrame = static_cast<AVFrame *>(ffmpegMemory->fetchData(FrameId));
  AvFrame->height = Height;
  return {};
}

Expect<void> AVFrameSetWidth::body(const Runtime::CallingFrame &,
                                   uint32_t FrameId, uint32_t Width) {

  auto *ffmpegMemory = Env.get();
  AVFrame *AvFrame = static_cast<AVFrame *>(ffmpegMemory->fetchData(FrameId));
  AvFrame->width = Width;
  return {};
}

Expect<int32_t> AVFrameVideoFormat::body(const Runtime::CallingFrame &,
                                         uint32_t FrameId) {

  FFMPEG_PTR_FETCH(AvFrame, FrameId, AVFrame);
  int const Format = AvFrame->format;
  if (Format == -1)
    return -1;
  AVPixelFormat const PixelFormat = static_cast<AVPixelFormat>(Format);
  return FFmpegUtils::PixFmt::fromAVPixFmt(PixelFormat);
}

Expect<uint32_t> AVFrameSetVideoFormat::body(const Runtime::CallingFrame &,
                                             uint32_t FrameId,
                                             uint32_t AvPixFormatId) {

  FFMPEG_PTR_FETCH(AvFrame, FrameId, AVFrame);
  AVPixelFormat const PixelFormat =
      FFmpegUtils::PixFmt::intoAVPixFmt(AvPixFormatId);
  AvFrame->format = PixelFormat;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVFrameIsNull::body(const Runtime::CallingFrame &,
                                    uint32_t FrameId) {

  FFMPEG_PTR_FETCH(AvFrame, FrameId, AVFrame);
  return AvFrame->data[0] == NULL;
}

Expect<int32_t> AVFrameLinesize::body(const Runtime::CallingFrame &,
                                      uint32_t FrameId, uint32_t Idx) {

  FFMPEG_PTR_FETCH(AvFrame, FrameId, AVFrame);
  return AvFrame->linesize[Idx];
}

Expect<int32_t> AVFrameData::body(const Runtime::CallingFrame &Frame,
                                  uint32_t FrameId, uint32_t FrameBufPtr,
                                  uint32_t FrameBufLen, uint32_t Index) {

  MEMINST_CHECK(MemInst, Frame, 0)
  MEM_SPAN_CHECK(Buffer, MemInst, uint8_t, FrameBufPtr, FrameBufLen, "");
  FFMPEG_PTR_FETCH(AvFrame, FrameId, AVFrame);

  uint8_t *Data = AvFrame->data[Index];
  memmove(Buffer.data(), Data, FrameBufLen);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVFrameGetBuffer::body(const Runtime::CallingFrame &,
                                       uint32_t FrameId, int32_t Align) {

  FFMPEG_PTR_FETCH(AvFrame, FrameId, AVFrame);
  return av_frame_get_buffer(AvFrame, Align);
}

Expect<int32_t> AVFrameAudioFormat::body(const Runtime::CallingFrame &,
                                         uint32_t FrameId) {

  FFMPEG_PTR_FETCH(AvFrame, FrameId, AVFrame);
  int const Format = AvFrame->format;
  if (Format == -1)
    return -1;

  AVSampleFormat const SampleFormat = static_cast<AVSampleFormat>(Format);
  return FFmpegUtils::SampleFmt::toSampleID(SampleFormat);
}

Expect<int32_t> AVFrameSetAudioFormat::body(const Runtime::CallingFrame &,
                                            uint32_t FrameId,
                                            uint32_t SampleFormatId) {
  FFMPEG_PTR_FETCH(AvFrame, FrameId, AVFrame);
  AVSampleFormat const SampleFormat =
      FFmpegUtils::SampleFmt::fromSampleID(SampleFormatId);
  AvFrame->format = SampleFormat;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVFrameSetChannelLayout::body(const Runtime::CallingFrame &,
                                              uint32_t FrameId,
                                              uint64_t ChannelLayoutID) {

  FFMPEG_PTR_FETCH(AvFrame, FrameId, AVFrame);
  uint64_t const ChannelLayout =
      FFmpegUtils::ChannelLayout::fromChannelLayoutID(ChannelLayoutID);
  AvFrame->channel_layout = ChannelLayout;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVFrameSetNbSamples::body(const Runtime::CallingFrame &,
                                          uint32_t FrameId, int32_t Samples) {

  FFMPEG_PTR_FETCH(AvFrame, FrameId, AVFrame);
  AvFrame->nb_samples = Samples;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVFrameNbSamples::body(const Runtime::CallingFrame &,
                                       uint32_t FrameId) {

  FFMPEG_PTR_FETCH(AvFrame, FrameId, AVFrame);
  return AvFrame->nb_samples;
}

Expect<int32_t> AVFrameSampleRate::body(const Runtime::CallingFrame &,
                                        uint32_t FrameId) {

  FFMPEG_PTR_FETCH(AvFrame, FrameId, AVFrame);
  return AvFrame->sample_rate;
}

Expect<int32_t> AVFrameSetSampleRate::body(const Runtime::CallingFrame &, uint32_t FrameId,int32_t SampleRate){

  FFMPEG_PTR_FETCH(AvFrame, FrameId, AVFrame);
  AvFrame->sample_rate = SampleRate;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVFrameChannels::body(const Runtime::CallingFrame &,
                                      uint32_t FrameId) {

  FFMPEG_PTR_FETCH(AvFrame, FrameId, AVFrame);
  return AvFrame->channels;
}

Expect<uint64_t> AVFrameChannelLayout::body(const Runtime::CallingFrame &,
                                            uint32_t FrameId) {
  FFMPEG_PTR_FETCH(AvFrame, FrameId, AVFrame);
  uint64_t const ChannelLayout = AvFrame->channel_layout;
  return FFmpegUtils::ChannelLayout::intoAVChannelID(ChannelLayout);
}

} // namespace AVUtil
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
