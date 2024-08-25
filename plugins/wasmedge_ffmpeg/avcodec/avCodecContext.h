// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "ffmpeg_base.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVcodec {

class AVCodecCtxCodecID : public HostFunction<AVCodecCtxCodecID> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame,
                        uint32_t AvCodecCtxId);
};

class AVCodecCtxCodecType : public HostFunction<AVCodecCtxCodecType> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId);
};

class AVCodecCtxSetCodecType : public HostFunction<AVCodecCtxSetCodecType> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t CodecTypeId);
};

class AVCodecCtxSetTimebase : public HostFunction<AVCodecCtxSetTimebase> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t Num, int32_t Den);
};

class AVCodecCtxTimeBase : public HostFunction<AVCodecCtxTimeBase> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, uint32_t NumPtr, uint32_t DenPtr);
};

class AVCodecCtxWidth : public HostFunction<AVCodecCtxWidth> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId);
};

class AVCodecCtxSetWidth : public HostFunction<AVCodecCtxSetWidth> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t Width);
};

class AVCodecCtxHeight : public HostFunction<AVCodecCtxHeight> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId);
};

class AVCodecCtxSetHeight : public HostFunction<AVCodecCtxSetHeight> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t Height);
};

class AVCodecCtxSampleAspectRatio
    : public HostFunction<AVCodecCtxSampleAspectRatio> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, uint32_t NumPtr, uint32_t DenPtr);
};

class AVCodecCtxSetSampleAspectRatio
    : public HostFunction<AVCodecCtxSetSampleAspectRatio> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t Num, int32_t Den);
};

class AVCodecCtxChannelLayout : public HostFunction<AVCodecCtxChannelLayout> {
public:
  using HostFunction::HostFunction;
  Expect<uint64_t> body(const Runtime::CallingFrame &Frame,
                        uint32_t AvCodecCtxId);
};

class AVCodecCtxSetChannelLayout
    : public HostFunction<AVCodecCtxSetChannelLayout> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, uint64_t ChannelLayoutId);
};

class AVCodecCtxPixFormat : public HostFunction<AVCodecCtxPixFormat> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame,
                        uint32_t AvCodecCtxId);
};

class AVCodecCtxSetPixFormat : public HostFunction<AVCodecCtxSetPixFormat> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, uint32_t PixFmtId);
};

class AVCodecCtxSampleFormat : public HostFunction<AVCodecCtxSampleFormat> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame,
                        uint32_t AvCodecCtxId);
};

class AVCodecCtxSetSampleFormat
    : public HostFunction<AVCodecCtxSetSampleFormat> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, uint32_t SampleFmtId);
};

class AVCodecCtxSampleRate : public HostFunction<AVCodecCtxSampleRate> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId);
};

class AVCodecCtxSetSampleRate : public HostFunction<AVCodecCtxSetSampleRate> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t SampleRate);
};

class AVCodecCtxSetGopSize : public HostFunction<AVCodecCtxSetGopSize> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t GopSize);
};

class AVCodecCtxSetMaxBFrames : public HostFunction<AVCodecCtxSetMaxBFrames> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t MaxBFrames);
};

class AVCodecCtxSetBQuantFactor
    : public HostFunction<AVCodecCtxSetBQuantFactor> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, float BQuantFactor);
};

class AVCodecCtxSetBQuantOffset
    : public HostFunction<AVCodecCtxSetBQuantOffset> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, float BQuantOffset);
};

class AVCodecCtxSetIQuantFactor
    : public HostFunction<AVCodecCtxSetIQuantFactor> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, float IQuantFactor);
};

class AVCodecCtxSetIQuantOffset
    : public HostFunction<AVCodecCtxSetIQuantOffset> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, float IQuantOffset);
};

class AVCodecCtxSetLumiMasking : public HostFunction<AVCodecCtxSetLumiMasking> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, float LumiMasking);
};

class AVCodecCtxSetTemporalCplxMasking
    : public HostFunction<AVCodecCtxSetTemporalCplxMasking> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, float TemporalCplxMasking);
};

class AVCodecCtxSetSpatialCplxMasking
    : public HostFunction<AVCodecCtxSetSpatialCplxMasking> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, float SpatialCplxMasking);
};

class AVCodecCtxSetPMasking : public HostFunction<AVCodecCtxSetPMasking> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, float PMasking);
};

class AVCodecCtxSetDarkMasking : public HostFunction<AVCodecCtxSetDarkMasking> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, float DarkMasking);
};

class AVCodecCtxSetMeCmp : public HostFunction<AVCodecCtxSetMeCmp> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t MeCmp);
};

class AVCodecCtxSetMeSubCmp : public HostFunction<AVCodecCtxSetMeSubCmp> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t MeSubCmp);
};

class AVCodecCtxSetMbCmp : public HostFunction<AVCodecCtxSetMbCmp> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t MbCmp);
};

class AVCodecCtxSetIldctCmp : public HostFunction<AVCodecCtxSetIldctCmp> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t IldctCmp);
};

class AVCodecCtxSetDiaSize : public HostFunction<AVCodecCtxSetDiaSize> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t DiaSize);
};

class AVCodecCtxSetLastPredictorsCount
    : public HostFunction<AVCodecCtxSetLastPredictorsCount> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t LastPredictorCount);
};

class AVCodecCtxSetMePreCmp : public HostFunction<AVCodecCtxSetMePreCmp> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t MePreCmp);
};

class AVCodecCtxSetPreDiaSize : public HostFunction<AVCodecCtxSetPreDiaSize> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t PreDiaSize);
};

class AVCodecCtxSetMeSubpelQuality
    : public HostFunction<AVCodecCtxSetMeSubpelQuality> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t MeSubpelQuality);
};

class AVCodecCtxSetMeRange : public HostFunction<AVCodecCtxSetMeRange> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t MeRange);
};

class AVCodecCtxSetMbDecision : public HostFunction<AVCodecCtxSetMbDecision> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t MbDecision);
};

class AVCodecCtxSetMbLMin : public HostFunction<AVCodecCtxSetMbLMin> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t MbLMin);
};

class AVCodecCtxSetMbLMax : public HostFunction<AVCodecCtxSetMbLMax> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t MbLMax);
};

class AVCodecCtxIntraDcPrecision
    : public HostFunction<AVCodecCtxIntraDcPrecision> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId);
};

class AVCodecCtxSetIntraDcPrecision
    : public HostFunction<AVCodecCtxSetIntraDcPrecision> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t IntraDcPrecision);
};

class AVCodecCtxSetQMin : public HostFunction<AVCodecCtxSetQMin> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t QMin);
};

class AVCodecCtxSetQMax : public HostFunction<AVCodecCtxSetQMax> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t QMax);
};

class AVCodecCtxSetGlobalQuality
    : public HostFunction<AVCodecCtxSetGlobalQuality> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t GlobalQuality);
};

class AVCodecCtxSetColorspace : public HostFunction<AVCodecCtxSetColorspace> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t ColorspaceId);
};

class AVCodecCtxColorspace : public HostFunction<AVCodecCtxColorspace> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId);
};

class AVCodecCtxSetColorRange : public HostFunction<AVCodecCtxSetColorRange> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t ColorRange);
};

class AVCodecCtxColorRange : public HostFunction<AVCodecCtxColorRange> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId);
};

class AVCodecCtxFrameSize : public HostFunction<AVCodecCtxFrameSize> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId);
};

class AVCodecCtxBitRate : public HostFunction<AVCodecCtxBitRate> {
public:
  using HostFunction::HostFunction;
  Expect<int64_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId);
};

class AVCodecCtxSetBitRate : public HostFunction<AVCodecCtxSetBitRate> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int64_t BitRate);
};

class AVCodecCtxRcMaxRate : public HostFunction<AVCodecCtxRcMaxRate> {
public:
  using HostFunction::HostFunction;
  Expect<int64_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId);
};

class AVCodecCtxSetRcMaxRate : public HostFunction<AVCodecCtxSetRcMaxRate> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int64_t RcMaxRate);
};

class AVCodecCtxSetBitRateTolerance
    : public HostFunction<AVCodecCtxSetBitRateTolerance> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t BitRateTolerance);
};

class AVCodecCtxSetCompressionLevel
    : public HostFunction<AVCodecCtxSetCompressionLevel> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t CompressionLevel);
};

class AVCodecCtxFrameRate : public HostFunction<AVCodecCtxFrameRate> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, uint32_t NumPtr, uint32_t DenPtr);
};

class AVCodecCtxSetFrameRate : public HostFunction<AVCodecCtxSetFrameRate> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t Num, int32_t Den);
};

class AVCodecCtxSetFlags : public HostFunction<AVCodecCtxSetFlags> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t Flags);
};

class AVCodecCtxSetStrictStdCompliance
    : public HostFunction<AVCodecCtxSetStrictStdCompliance> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t ComplianceId);
};

class AVCodecCtxSetDebug : public HostFunction<AVCodecCtxSetDebug> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t Debug);
};

class AVCodecCtxCodec : public HostFunction<AVCodecCtxCodec> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, uint32_t AvCodecPtr);
};

class AVCodecCtxChannels : public HostFunction<AVCodecCtxChannels> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId);
};

class AVCodecCtxSetChannels : public HostFunction<AVCodecCtxSetChannels> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t Channels);
};

class AVCodecCtxSetSkipLoopFilter
    : public HostFunction<AVCodecCtxSetSkipLoopFilter> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t AVDicardId);
};

class AVCodecCtxSetSkipFrame : public HostFunction<AVCodecCtxSetSkipFrame> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t AVDiscardId);
};

class AVCodecCtxSetSkipIdct : public HostFunction<AVCodecCtxSetSkipIdct> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t AVDicardId);
};

class AVCodecCtxSetErrorConcealment
    : public HostFunction<AVCodecCtxSetErrorConcealment> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t ErrorConcealment);
};

class AVCodecCtxSetErrorRecognition
    : public HostFunction<AVCodecCtxSetErrorRecognition> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t ErrorRecognition);
};

class AVCodecCtxDelay : public HostFunction<AVCodecCtxDelay> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId);
};

class AVCodecCtxSetSkipTop : public HostFunction<AVCodecCtxSetSkipTop> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t Value);
};

class AVCodecCtxSetSkipBottom : public HostFunction<AVCodecCtxSetSkipBottom> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t Value);
};

class AVCodecCtxRefs : public HostFunction<AVCodecCtxRefs> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId);
};

class AVCodecCtxSetSliceFlags : public HostFunction<AVCodecCtxSetSliceFlags> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t Flags);
};

class AVCodecCtxSetSliceCount : public HostFunction<AVCodecCtxSetSliceCount> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t Value);
};

class AVCodecCtxSetFieldOrder : public HostFunction<AVCodecCtxSetFieldOrder> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t Value);
};

class AVCodecCtxColorTrc : public HostFunction<AVCodecCtxColorTrc> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId);
};

class AVCodecCtxChromaSampleLocation
    : public HostFunction<AVCodecCtxChromaSampleLocation> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId);
};

class AVCodecCtxFrameNumber : public HostFunction<AVCodecCtxFrameNumber> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId);
};

class AVCodecCtxBlockAlign : public HostFunction<AVCodecCtxBlockAlign> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId);
};

class AVCodecCtxSetRequestSampleFmt
    : public HostFunction<AVCodecCtxSetRequestSampleFmt> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, uint32_t SampleFmtId);
};

class AVCodecCtxAudioServiceType
    : public HostFunction<AVCodecCtxAudioServiceType> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId);
};

class AVCodecCtxHasBFrames : public HostFunction<AVCodecCtxHasBFrames> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId);
};

class AVCodecCtxSetRequestChannelLayout
    : public HostFunction<AVCodecCtxSetRequestChannelLayout> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, uint64_t ChannelLayoutId);
};

class AVCodecCtxActiveThreadType
    : public HostFunction<AVCodecCtxActiveThreadType> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId);
};

class AVCodecCtxSetThreadType : public HostFunction<AVCodecCtxSetThreadType> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t ThreadType);
};

class AVCodecCtxThreadCount : public HostFunction<AVCodecCtxThreadCount> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId);
};

class AVCodecCtxSetThreadCount : public HostFunction<AVCodecCtxSetThreadCount> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t ThreadCount);
};

class AVCodecCtxColorPrimaries : public HostFunction<AVCodecCtxColorPrimaries> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId);
};

} // namespace AVcodec
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
