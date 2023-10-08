#include "avCodecContext.h"

extern "C" {
#include "libavcodec/avcodec.h"
}

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVcodec {

Expect<uint32_t> AVCodecCtxCodecID::body(const Runtime::CallingFrame &,
                                         uint32_t AvCodecCtxId) {

  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  AVCodecID const AvCodecId = AvCodecCtx->codec_id;
  return FFmpegUtils::CodecID::fromAVCodecID(AvCodecId);
}

Expect<uint32_t> AVCodecCtxCodecType::body(const Runtime::CallingFrame &,
                                           uint32_t AvCodecCtxId) {

  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  AVMediaType const AvMediaType = AvCodecCtx->codec_type;
  return FFmpegUtils::MediaType::fromMediaType(AvMediaType);
}

Expect<int32_t> AVCodecCtxTimeBase::body(const Runtime::CallingFrame &Frame,
                                         uint32_t AvCodecCtxId, uint32_t NumPtr,
                                         uint32_t DenPtr) {

  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_PTR_CHECK(Num, MemInst, int32_t, NumPtr,
                "Failed to access Numerator Ptr for AVRational");
  MEM_PTR_CHECK(Den, MemInst, int32_t, DenPtr,
                "Failed to access Denominator Ptr for AVRational");

  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  AVRational const AvRational = AvCodecCtx->time_base;
  *Num = AvRational.num;
  *Den = AvRational.den;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<uint32_t> AVCodecCtxWidth::body(const Runtime::CallingFrame &,
                                       uint32_t AvCodecCtxId) {

  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  return AvCodecCtx->width;
}

Expect<uint32_t> AVCodecCtxHeight::body(const Runtime::CallingFrame &,
                                        uint32_t AvCodecCtxId) {

  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  return AvCodecCtx->height;
}

Expect<int32_t>
AVCodecCtxSampleAspectRatio::body(const Runtime::CallingFrame &Frame,
                                  uint32_t AvCodecCtxId, uint32_t NumPtr,
                                  uint32_t DenPtr) {

  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_PTR_CHECK(Num, MemInst, int32_t, NumPtr,
                "Failed to access Numerator Ptr for AVRational");
  MEM_PTR_CHECK(Den, MemInst, int32_t, DenPtr,
                "Failed to access Denominator Ptr for AVRational");
  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);

  const AVRational AvRational = AvCodecCtx->sample_aspect_ratio;
  *Num = AvRational.num;
  *Den = AvRational.den;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<uint64_t> AVCodecCtxChannelLayout::body(const Runtime::CallingFrame &,
                                               uint32_t AvCodecCtxId) {

  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  // Deprecated method
  uint64_t const AvChannel = AvCodecCtx->channel_layout;
  return FFmpegUtils::ChannelLayout::intoAVChannelID(AvChannel);
}

Expect<uint32_t> AVCodecCtxPixFormat::body(const Runtime::CallingFrame &,
                                           uint32_t AvCodecCtxId) {
  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  AVPixelFormat const PixFmt = AvCodecCtx->pix_fmt;
  return FFmpegUtils::PixFmt::fromAVPixFmt(PixFmt);
}

Expect<uint32_t> AVCodecCtxSampleFormat::body(const Runtime::CallingFrame &,
                                              uint32_t AvCodecCtxId){

  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  AVSampleFormat const AvSampleFormat = AvCodecCtx->sample_fmt;
  return FFmpegUtils::SampleFmt::toSampleID(AvSampleFormat);
}

Expect<int32_t> AVCodecCtxSampleRate::body(const Runtime::CallingFrame &,
                                           uint32_t AvCodecCtxId){

  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  return AvCodecCtx->sample_rate;
}

} // namespace AVcodec
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
