// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

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

Expect<int32_t> AVCodecCtxCodecType::body(const Runtime::CallingFrame &,
                                          uint32_t AvCodecCtxId) {
  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  AVMediaType const AvMediaType = AvCodecCtx->codec_type;
  return FFmpegUtils::MediaType::fromMediaType(AvMediaType);
}

Expect<int32_t> AVCodecCtxSetCodecType::body(const Runtime::CallingFrame &,
                                             uint32_t AvCodecCtxId,
                                             int32_t CodecTypeId) {
  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  AVMediaType const AvMediaType =
      FFmpegUtils::MediaType::intoMediaType(CodecTypeId);

  AvCodecCtx->codec_type = AvMediaType;
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

Expect<int32_t> AVCodecCtxTimeBase::body(const Runtime::CallingFrame &Frame,
                                         uint32_t AvCodecCtxId, uint32_t NumPtr,
                                         uint32_t DenPtr) {
  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_PTR_CHECK(Num, MemInst, int32_t, NumPtr,
                "Failed to access Numerator Ptr for AVRational"sv);
  MEM_PTR_CHECK(Den, MemInst, int32_t, DenPtr,
                "Failed to access Denominator Ptr for AVRational"sv);

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
                "Failed to access Numerator Ptr for AVRational"sv);
  MEM_PTR_CHECK(Den, MemInst, int32_t, DenPtr,
                "Failed to access Denominator Ptr for AVRational"sv);
  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);

  const AVRational AvRational = AvCodecCtx->sample_aspect_ratio;
  *Num = AvRational.num;
  *Den = AvRational.den;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t>
AVCodecCtxSetSampleAspectRatio::body(const Runtime::CallingFrame &,
                                     uint32_t AvCodecCtxId, int32_t Num,
                                     int32_t Den) {
  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  const AVRational AspectRatio = av_make_q(Num, Den);
  AvCodecCtx->sample_aspect_ratio = AspectRatio;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<uint64_t> AVCodecCtxChannelLayout::body(const Runtime::CallingFrame &,
                                               uint32_t AvCodecCtxId) {
  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  // Deprecated method
  uint64_t const AvChannel = AvCodecCtx->channel_layout;
  return FFmpegUtils::ChannelLayout::intoChannelLayoutID(AvChannel);
}

Expect<int32_t> AVCodecCtxSetChannelLayout::body(const Runtime::CallingFrame &,
                                                 uint32_t AvCodecCtxId,
                                                 uint64_t ChannelLayoutId) {
  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  uint64_t const AvChannel =
      FFmpegUtils::ChannelLayout::fromChannelLayoutID(ChannelLayoutId);
  AvCodecCtx->channel_layout = AvChannel;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<uint32_t> AVCodecCtxPixFormat::body(const Runtime::CallingFrame &,
                                           uint32_t AvCodecCtxId) {
  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  AVPixelFormat const PixFmt = AvCodecCtx->pix_fmt;
  return FFmpegUtils::PixFmt::fromAVPixFmt(PixFmt);
}

Expect<int32_t> AVCodecCtxSetPixFormat::body(const Runtime::CallingFrame &,
                                             uint32_t AvCodecCtxId,
                                             uint32_t PixFmtId) {
  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  AVPixelFormat const PixFmt = FFmpegUtils::PixFmt::intoAVPixFmt(PixFmtId);
  AvCodecCtx->pix_fmt = PixFmt;
  return static_cast<int32_t>(ErrNo::Success);
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

Expect<int32_t> AVCodecCtxIntraDcPrecision::body(const Runtime::CallingFrame &,
                                                 uint32_t AvCodecCtxId) {
  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  return AvCodecCtx->intra_dc_precision;
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

Expect<int64_t> AVCodecCtxBitRate::body(const Runtime::CallingFrame &,
                                        uint32_t AvCodecCtxId) {
  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  return AvCodecCtx->bit_rate;
}

Expect<int32_t> AVCodecCtxSetBitRate::body(const Runtime::CallingFrame &,
                                           uint32_t AvCodecCtxId,
                                           int64_t BitRate) {
  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  AvCodecCtx->bit_rate = BitRate;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int64_t> AVCodecCtxRcMaxRate::body(const Runtime::CallingFrame &,
                                          uint32_t AvCodecCtxId) {
  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  return AvCodecCtx->rc_max_rate;
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

Expect<int32_t> AVCodecCtxFrameRate::body(const Runtime::CallingFrame &Frame,
                                          uint32_t AvCodecCtxId,
                                          uint32_t NumPtr, uint32_t DenPtr) {
  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_PTR_CHECK(Num, MemInst, int32_t, NumPtr,
                "Failed to access Numerator Ptr for AVRational"sv);
  MEM_PTR_CHECK(Den, MemInst, int32_t, DenPtr,
                "Failed to access Denominator Ptr for AVRational"sv);

  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);

  AVRational const FrameRate = AvCodecCtx->framerate;
  *Num = FrameRate.num;
  *Den = FrameRate.den;
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

Expect<int32_t> AVCodecCtxSetFlags::body(const Runtime::CallingFrame &,
                                         uint32_t AvCodecCtxId, int32_t Flags) {
  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  AvCodecCtx->flags = Flags;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t>
AVCodecCtxSetStrictStdCompliance::body(const Runtime::CallingFrame &,
                                       uint32_t AvCodecCtxId,
                                       int32_t ComplianceId) {
  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  AvCodecCtx->strict_std_compliance = ComplianceId;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVCodecCtxSetDebug::body(const Runtime::CallingFrame &,
                                         uint32_t AvCodecCtxId, int32_t Debug) {
  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  AvCodecCtx->debug = Debug;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVCodecCtxCodec::body(const Runtime::CallingFrame &Frame,
                                      uint32_t AvCodecCtxId,
                                      uint32_t AvCodecPtr) {
  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_PTR_CHECK(AVCodecId, MemInst, uint32_t, AvCodecPtr,
                "Failed to access Ptr for AvCodecPtr"sv);

  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  FFMPEG_PTR_FETCH(AvCodec, *AVCodecId, const AVCodec);

  AvCodec = AvCodecCtx->codec;
  if (AvCodec == nullptr)
    return -1;

  FFMPEG_PTR_STORE(const_cast<AVCodec *>(AvCodec), AVCodecId);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVCodecCtxChannels::body(const Runtime::CallingFrame &,
                                         uint32_t AvCodecCtxId) {
  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  return AvCodecCtx->channels;
}

Expect<int32_t> AVCodecCtxSetChannels::body(const Runtime::CallingFrame &,
                                            uint32_t AvCodecCtxId,
                                            int32_t Channels) {
  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  AvCodecCtx->channels = Channels;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVCodecCtxSetSkipLoopFilter::body(const Runtime::CallingFrame &,
                                                  uint32_t AvCodecCtxId,
                                                  int32_t AVDiscardId) {
  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  AvCodecCtx->skip_loop_filter = static_cast<AVDiscard>(AVDiscardId);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVCodecCtxSetSkipFrame::body(const Runtime::CallingFrame &,
                                             uint32_t AvCodecCtxId,
                                             int32_t AVDiscardId) {
  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  AvCodecCtx->skip_frame = static_cast<AVDiscard>(AVDiscardId);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVCodecCtxSetSkipIdct::body(const Runtime::CallingFrame &,
                                            uint32_t AvCodecCtxId,
                                            int32_t AVDiscardId) {
  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  AvCodecCtx->skip_idct = static_cast<AVDiscard>(AVDiscardId);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t>
AVCodecCtxSetErrorConcealment::body(const Runtime::CallingFrame &,
                                    uint32_t AvCodecCtxId,
                                    int32_t ErrorConcealment) {
  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  AvCodecCtx->error_concealment = ErrorConcealment;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t>
AVCodecCtxSetErrorRecognition::body(const Runtime::CallingFrame &,
                                    uint32_t AvCodecCtxId,
                                    int32_t ErrRecognition) {
  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  AvCodecCtx->err_recognition = ErrRecognition;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVCodecCtxDelay::body(const Runtime::CallingFrame &,
                                      uint32_t AvCodecCtxId) {
  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  return AvCodecCtx->delay;
}

Expect<int32_t> AVCodecCtxSetSkipTop::body(const Runtime::CallingFrame &,
                                           uint32_t AvCodecCtxId,
                                           int32_t Value) {
  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  AvCodecCtx->skip_top = Value;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVCodecCtxSetSkipBottom::body(const Runtime::CallingFrame &,
                                              uint32_t AvCodecCtxId,
                                              int32_t Value) {
  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  AvCodecCtx->skip_bottom = Value;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVCodecCtxRefs::body(const Runtime::CallingFrame &,
                                     uint32_t AvCodecCtxId) {
  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  return AvCodecCtx->refs;
}

Expect<int32_t> AVCodecCtxSetSliceFlags::body(const Runtime::CallingFrame &,
                                              uint32_t AvCodecCtxId,
                                              int32_t Value) {
  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  AvCodecCtx->slice_flags = Value;
  return static_cast<int32_t>(ErrNo::Success);
}
Expect<int32_t> AVCodecCtxSetSliceCount::body(const Runtime::CallingFrame &,
                                              uint32_t AvCodecCtxId,
                                              int32_t Value) {
  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  AvCodecCtx->slice_count = Value;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVCodecCtxSetFieldOrder::body(const Runtime::CallingFrame &,
                                              uint32_t AvCodecCtxId,
                                              int32_t Value) {
  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  AvCodecCtx->field_order = static_cast<AVFieldOrder>(Value);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVCodecCtxColorTrc::body(const Runtime::CallingFrame &,
                                         uint32_t AvCodecCtxId) {
  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  return static_cast<int32_t>(AvCodecCtx->color_trc);
}

Expect<int32_t>
AVCodecCtxChromaSampleLocation::body(const Runtime::CallingFrame &,
                                     uint32_t AvCodecCtxId) {
  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  AVChromaLocation const Chroma = AvCodecCtx->chroma_sample_location;
  return FFmpegUtils::ChromaLocation::fromAVChromaLocation(Chroma);
}

Expect<int32_t> AVCodecCtxFrameNumber::body(const Runtime::CallingFrame &,
                                            uint32_t AvCodecCtxId) {
  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  return AvCodecCtx->frame_number;
}

Expect<int32_t> AVCodecCtxBlockAlign::body(const Runtime::CallingFrame &,
                                           uint32_t AvCodecCtxId) {
  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  return AvCodecCtx->block_align;
}

Expect<int32_t>
AVCodecCtxSetRequestSampleFmt::body(const Runtime::CallingFrame &,
                                    uint32_t AvCodecCtxId,
                                    uint32_t SampleFmtId) {
  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  AVSampleFormat const SampleFmt =
      FFmpegUtils::SampleFmt::fromSampleID(SampleFmtId);
  AvCodecCtx->request_sample_fmt = SampleFmt;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVCodecCtxAudioServiceType::body(const Runtime::CallingFrame &,
                                                 uint32_t AvCodecCtxId) {
  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  AVAudioServiceType const AudioServiceType = AvCodecCtx->audio_service_type;
  return static_cast<int32_t>(AudioServiceType);
}

Expect<int32_t> AVCodecCtxHasBFrames::body(const Runtime::CallingFrame &,
                                           uint32_t AvCodecCtxId) {
  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  return AvCodecCtx->has_b_frames;
}

Expect<int32_t>
AVCodecCtxSetRequestChannelLayout::body(const Runtime::CallingFrame &,
                                        uint32_t AvCodecCtxId,
                                        uint64_t ChannelLayoutId) {
  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  AvCodecCtx->request_channel_layout =
      FFmpegUtils::ChannelLayout::fromChannelLayoutID(ChannelLayoutId);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVCodecCtxActiveThreadType::body(const Runtime::CallingFrame &,
                                                 uint32_t AvCodecCtxId) {
  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  return AvCodecCtx->active_thread_type;
}

Expect<int32_t> AVCodecCtxSetThreadType::body(const Runtime::CallingFrame &,
                                              uint32_t AvCodecCtxId,
                                              int32_t ThreadType) {
  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  AvCodecCtx->thread_type = ThreadType;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVCodecCtxThreadCount::body(const Runtime::CallingFrame &,
                                            uint32_t AvCodecCtxId) {
  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  return AvCodecCtx->thread_count;
}

Expect<int32_t> AVCodecCtxSetThreadCount::body(const Runtime::CallingFrame &,
                                               uint32_t AvCodecCtxId,
                                               int32_t ThreadCount) {
  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  AvCodecCtx->thread_count = ThreadCount;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVCodecCtxColorPrimaries::body(const Runtime::CallingFrame &,
                                               uint32_t AvCodecCtxId) {
  FFMPEG_PTR_FETCH(AvCodecCtx, AvCodecCtxId, AVCodecContext);
  AVColorPrimaries const ColorPrimaries = AvCodecCtx->color_primaries;
  return FFmpegUtils::ColorPrimaries::fromAVColorPrimaries(ColorPrimaries);
}

} // namespace AVcodec
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
