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

Expect<uint32_t> AVCodecCtxSetCodecType::body(const Runtime::CallingFrame &,
                                              uint32_t AvCodecCtxId,
                                              int32_t CodecTypeId) {

  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  AVMediaType const AvMediaType =
      FFmpegUtils::MediaType::intoMediaType(CodecTypeId);

  AvCodecCtx->codec_type = AvMediaType;
  return static_cast<int32_t>(ErrNo::Success);
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

Expect<int32_t> AVCodecCtxWidth::body(const Runtime::CallingFrame &,
                                      uint32_t AvCodecCtxId) {

  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  return AvCodecCtx->width;
}

Expect<int32_t> AVCodecCtxSetWidth::body(const Runtime::CallingFrame &,
                                         uint32_t AvCodecCtxId, int32_t Width) {

  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  AvCodecCtx->width = Width;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVCodecCtxHeight::body(const Runtime::CallingFrame &,
                                       uint32_t AvCodecCtxId) {

  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  return AvCodecCtx->height;
}

Expect<int32_t> AVCodecCtxSetHeight::body(const Runtime::CallingFrame &,
                                          uint32_t AvCodecCtxId,
                                          int32_t Height) {

  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  AvCodecCtx->height = Height;
  return static_cast<int32_t>(ErrNo::Success);
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
                                              uint32_t AvCodecCtxId) {

  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  AVSampleFormat const AvSampleFormat = AvCodecCtx->sample_fmt;
  return FFmpegUtils::SampleFmt::toSampleID(AvSampleFormat);
}

Expect<int32_t> AVCodecCtxSetSampleFormat::body(const Runtime::CallingFrame &,
                                                uint32_t AvCodecCtxId,
                                                uint32_t SampleFmtId) {

  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  AVSampleFormat const SampleFormat =
      FFmpegUtils::SampleFmt::fromSampleID(SampleFmtId);
  AvCodecCtx->sample_fmt = SampleFormat;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVCodecCtxSampleRate::body(const Runtime::CallingFrame &,
                                           uint32_t AvCodecCtxId) {

  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  return AvCodecCtx->sample_rate;
}

Expect<int32_t> AVCodecCtxSetSampleRate::body(const Runtime::CallingFrame &,
                                              uint32_t AvCodecCtxId,
                                              int32_t SampleRate) {

  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  AvCodecCtx->sample_rate = SampleRate;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVCodecCtxSetGopSize::body(const Runtime::CallingFrame &,
                                           uint32_t AvCodecCtxId,
                                           int32_t GopSize) {

  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  AvCodecCtx->gop_size = GopSize;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVCodecCtxSetMaxBFrames::body(const Runtime::CallingFrame &,
                                              uint32_t AvCodecCtxId,
                                              int32_t MaxBFrames) {

  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  AvCodecCtx->max_b_frames = MaxBFrames;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVCodecCtxSetBQuantFactor::body(const Runtime::CallingFrame &,
                                                uint32_t AvCodecCtxId,
                                                float BQuantFactor) {

  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  AvCodecCtx->b_quant_factor = BQuantFactor;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVCodecCtxSetBQuantOffset::body(const Runtime::CallingFrame &,
                                                uint32_t AvCodecCtxId,
                                                float BQuantOffset) {

  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  AvCodecCtx->b_quant_offset = BQuantOffset;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVCodecCtxSetIQuantFactor::body(const Runtime::CallingFrame &,
                                                uint32_t AvCodecCtxId,
                                                float IQuantFactor) {

  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  AvCodecCtx->i_quant_factor = IQuantFactor;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVCodecCtxSetIQuantOffset::body(const Runtime::CallingFrame &,
                                                uint32_t AvCodecCtxId,
                                                float IQuantOffset) {

  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  AvCodecCtx->i_quant_offset = IQuantOffset;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVCodecCtxSetLumiMasking::body(const Runtime::CallingFrame &,
                                               uint32_t AvCodecCtxId,
                                               float LumiMasking) {

  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  AvCodecCtx->lumi_masking = LumiMasking;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t>
AVCodecCtxSetTemporalCplxMasking::body(const Runtime::CallingFrame &,
                                       uint32_t AvCodecCtxId,
                                       float TemporalCplxMasking) {

  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  AvCodecCtx->temporal_cplx_masking = TemporalCplxMasking;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t>
AVCodecCtxSetSpatialCplxMasking::body(const Runtime::CallingFrame &,
                                      uint32_t AvCodecCtxId,
                                      float SpatialCplxMasking) {

  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  AvCodecCtx->spatial_cplx_masking = SpatialCplxMasking;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVCodecCtxSetPMasking::body(const Runtime::CallingFrame &,
                                            uint32_t AvCodecCtxId,
                                            float PMasking) {

  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  AvCodecCtx->p_masking = PMasking;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVCodecCtxSetDarkMasking::body(const Runtime::CallingFrame &,
                                               uint32_t AvCodecCtxId,
                                               float DarkMasking) {

  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  AvCodecCtx->dark_masking = DarkMasking;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVCodecCtxSetMeCmp::body(const Runtime::CallingFrame &,
                                         uint32_t AvCodecCtxId, int32_t MeCmp) {

  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  AvCodecCtx->me_cmp = MeCmp;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVCodecCtxSetMeSubCmp::body(const Runtime::CallingFrame &,
                                            uint32_t AvCodecCtxId,
                                            int32_t MeSubCmp) {

  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  AvCodecCtx->me_sub_cmp = MeSubCmp;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVCodecCtxSetMbCmp::body(const Runtime::CallingFrame &,
                                         uint32_t AvCodecCtxId, int32_t MbCmp) {

  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  AvCodecCtx->mb_cmp = MbCmp;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVCodecCtxSetIldctCmp::body(const Runtime::CallingFrame &,
                                            uint32_t AvCodecCtxId,
                                            int32_t IldctCmp) {

  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  AvCodecCtx->ildct_cmp = IldctCmp;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVCodecCtxSetDiaSize::body(const Runtime::CallingFrame &,
                                           uint32_t AvCodecCtxId,
                                           int32_t DiaSize) {

  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  AvCodecCtx->dia_size = DiaSize;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t>
AVCodecCtxSetLastPredictorsCount::body(const Runtime::CallingFrame &,
                                       uint32_t AvCodecCtxId,
                                       int32_t LastPredictorCount) {

  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  AvCodecCtx->last_predictor_count = LastPredictorCount;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVCodecCtxSetMePreCmp::body(const Runtime::CallingFrame &,
                                            uint32_t AvCodecCtxId,
                                            int32_t MePreCmp) {

  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  AvCodecCtx->me_pre_cmp = MePreCmp;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVCodecCtxSetPreDiaSize::body(const Runtime::CallingFrame &,
                                              uint32_t AvCodecCtxId,
                                              int32_t PreDiaSize) {

  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  AvCodecCtx->pre_dia_size = PreDiaSize;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t>
AVCodecCtxSetMeSubpelQuality::body(const Runtime::CallingFrame &,
                                   uint32_t AvCodecCtxId,
                                   int32_t MeSubpelQuality) {

  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  AvCodecCtx->me_subpel_quality = MeSubpelQuality;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVCodecCtxSetMeRange::body(const Runtime::CallingFrame &,
                                           uint32_t AvCodecCtxId,
                                           int32_t MeRange) {

  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  AvCodecCtx->me_range = MeRange;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVCodecCtxSetMbDecision::body(const Runtime::CallingFrame &,
                                              uint32_t AvCodecCtxId,
                                              int32_t MbDecision) {

  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  AvCodecCtx->mb_decision = MbDecision;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVCodecCtxSetMbLMin::body(const Runtime::CallingFrame &,
                                          uint32_t AvCodecCtxId,
                                          int32_t MbLMin) {

  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  AvCodecCtx->mb_lmin = MbLMin;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVCodecCtxSetMbLMax::body(const Runtime::CallingFrame &,
                                          uint32_t AvCodecCtxId,
                                          int32_t MbLMax) {

  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  AvCodecCtx->mb_lmax = MbLMax;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t>
AVCodecCtxSetIntraDcPrecision::body(const Runtime::CallingFrame &,
                                    uint32_t AvCodecCtxId,
                                    int32_t IntraDcPrecision) {

  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  AvCodecCtx->intra_dc_precision = IntraDcPrecision;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVCodecCtxSetQMin::body(const Runtime::CallingFrame &,
                                        uint32_t AvCodecCtxId, int32_t QMin) {

  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  AvCodecCtx->qmin = QMin;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVCodecCtxSetQMax::body(const Runtime::CallingFrame &,
                                        uint32_t AvCodecCtxId, int32_t QMax) {

  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  AvCodecCtx->qmax = QMax;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVCodecCtxSetGlobalQuality::body(const Runtime::CallingFrame &,
                                                 uint32_t AvCodecCtxId,
                                                 int32_t GlobalQuality) {

  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  AvCodecCtx->global_quality = GlobalQuality;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVCodecCtxSetColorspace::body(const Runtime::CallingFrame &,
                                              uint32_t AvCodecCtxId,
                                              int32_t ColorspaceId) {

  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  AVColorSpace const ColorSpace =
      FFmpegUtils::ColorSpace::intoAVColorSpace(ColorspaceId);
  AvCodecCtx->colorspace = ColorSpace;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVCodecCtxColorspace::body(const Runtime::CallingFrame &,
                                           uint32_t AvCodecCtxId) {

  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  AVColorSpace const Colorspace = AvCodecCtx->colorspace;
  return FFmpegUtils::ColorSpace::fromAVColorSpace(Colorspace);
}

Expect<int32_t> AVCodecCtxSetColorRange::body(const Runtime::CallingFrame &,
                                              uint32_t AvCodecCtxId,
                                              int32_t ColorRangeId) {

  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  AvCodecCtx->color_range = static_cast<AVColorRange>(ColorRangeId);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVCodecCtxColorRange::body(const Runtime::CallingFrame &,
                                           uint32_t AvCodecCtxId) {

  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  AVColorRange const ColorRange = AvCodecCtx->color_range;
  return static_cast<int32_t>(ColorRange);
}

Expect<int32_t> AVCodecCtxFrameSize::body(const Runtime::CallingFrame &,
                                          uint32_t AvCodecCtxId) {

  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  return AvCodecCtx->frame_size;
}

Expect<int32_t> AVCodecCtxSetBitRate::body(const Runtime::CallingFrame &,
                                           uint32_t AvCodecCtxId,
                                           int64_t BitRate) {

  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  AvCodecCtx->bit_rate = BitRate;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVCodecCtxSetRcMaxRate::body(const Runtime::CallingFrame &,
                                             uint32_t AvCodecCtxId,
                                             int64_t RcMaxRate) {

  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  AvCodecCtx->rc_max_rate = RcMaxRate;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t>
AVCodecCtxSetBitRateTolerance::body(const Runtime::CallingFrame &,
                                    uint32_t AvCodecCtxId,
                                    int32_t BitRateTolerance) {

  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  AvCodecCtx->bit_rate_tolerance = BitRateTolerance;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t>
AVCodecCtxSetCompressionLevel::body(const Runtime::CallingFrame &,
                                    uint32_t AvCodecCtxId,
                                    int32_t CompressionLevel) {

  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  AvCodecCtx->compression_level = CompressionLevel;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVCodecCtxSetTimebase::body(const Runtime::CallingFrame &,
                                            uint32_t AvCodecCtxId, int32_t Num,
                                            int32_t Den) {

  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  AVRational const Rational = av_make_q(Num, Den);
  AvCodecCtx->time_base = Rational;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVCodecCtxSetFrameRate::body(const Runtime::CallingFrame &,
                                             uint32_t AvCodecCtxId, int32_t Num,
                                             int32_t Den) {

  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  AVRational const Rational = av_make_q(Num, Den);
  AvCodecCtx->framerate = Rational;
  return static_cast<int32_t>(ErrNo::Success);
}

} // namespace AVcodec
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
