#pragma once
#include "avcodec_base.h"
#include "runtime/callingframe.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVcodec {

class AVCodecCtxCodecID : public WasmEdgeFFmpegAVCodec<AVCodecCtxCodecID> {
public:
  AVCodecCtxCodecID(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame,
                        uint32_t AvCodecCtxId);
};

class AVCodecCtxCodecType : public WasmEdgeFFmpegAVCodec<AVCodecCtxCodecType> {
public:
  AVCodecCtxCodecType(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId);
};

class AVCodecCtxSetCodecType
    : public WasmEdgeFFmpegAVCodec<AVCodecCtxSetCodecType> {
public:
  AVCodecCtxSetCodecType(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t CodecTypeId);
};

class AVCodecCtxSetTimebase
    : public WasmEdgeFFmpegAVCodec<AVCodecCtxSetTimebase> {
public:
  AVCodecCtxSetTimebase(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t Num, int32_t Den);
};

class AVCodecCtxTimeBase : public WasmEdgeFFmpegAVCodec<AVCodecCtxTimeBase> {
public:
  AVCodecCtxTimeBase(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, uint32_t NumPtr, uint32_t DenPtr);
};

class AVCodecCtxWidth : public WasmEdgeFFmpegAVCodec<AVCodecCtxWidth> {
public:
  AVCodecCtxWidth(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId);
};

class AVCodecCtxSetWidth : public WasmEdgeFFmpegAVCodec<AVCodecCtxSetWidth> {
public:
  AVCodecCtxSetWidth(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t Width);
};

class AVCodecCtxHeight : public WasmEdgeFFmpegAVCodec<AVCodecCtxHeight> {
public:
  AVCodecCtxHeight(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId);
};

class AVCodecCtxSetHeight : public WasmEdgeFFmpegAVCodec<AVCodecCtxSetHeight> {
public:
  AVCodecCtxSetHeight(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t Height);
};

class AVCodecCtxSampleAspectRatio
    : public WasmEdgeFFmpegAVCodec<AVCodecCtxSampleAspectRatio> {
public:
  AVCodecCtxSampleAspectRatio(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, uint32_t NumPtr, uint32_t DenPtr);
};

class AVCodecCtxSetSampleAspectRatio
    : public WasmEdgeFFmpegAVCodec<AVCodecCtxSetSampleAspectRatio> {
public:
  AVCodecCtxSetSampleAspectRatio(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t Num, int32_t Den);
};

class AVCodecCtxChannelLayout
    : public WasmEdgeFFmpegAVCodec<AVCodecCtxChannelLayout> {
public:
  AVCodecCtxChannelLayout(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<uint64_t> body(const Runtime::CallingFrame &Frame,
                        uint32_t AvCodecCtxId);
};

class AVCodecCtxSetChannelLayout
    : public WasmEdgeFFmpegAVCodec<AVCodecCtxSetChannelLayout> {
public:
  AVCodecCtxSetChannelLayout(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, uint64_t ChannelLayoutId);
};

class AVCodecCtxPixFormat : public WasmEdgeFFmpegAVCodec<AVCodecCtxPixFormat> {
public:
  AVCodecCtxPixFormat(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame,
                        uint32_t AvCodecCtxId);
};

class AVCodecCtxSetPixFormat
    : public WasmEdgeFFmpegAVCodec<AVCodecCtxSetPixFormat> {
public:
  AVCodecCtxSetPixFormat(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, uint32_t PixFmtId);
};

class AVCodecCtxSampleFormat
    : public WasmEdgeFFmpegAVCodec<AVCodecCtxSampleFormat> {
public:
  AVCodecCtxSampleFormat(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame,
                        uint32_t AvCodecCtxId);
};

class AVCodecCtxSetSampleFormat
    : public WasmEdgeFFmpegAVCodec<AVCodecCtxSetSampleFormat> {
public:
  AVCodecCtxSetSampleFormat(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, uint32_t SampleFmtId);
};

class AVCodecCtxSampleRate
    : public WasmEdgeFFmpegAVCodec<AVCodecCtxSampleRate> {
public:
  AVCodecCtxSampleRate(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId);
};

class AVCodecCtxSetSampleRate
    : public WasmEdgeFFmpegAVCodec<AVCodecCtxSetSampleRate> {
public:
  AVCodecCtxSetSampleRate(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t SampleRate);
};

class AVCodecCtxSetGopSize
    : public WasmEdgeFFmpegAVCodec<AVCodecCtxSetGopSize> {
public:
  AVCodecCtxSetGopSize(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t GopSize);
};

class AVCodecCtxSetMaxBFrames
    : public WasmEdgeFFmpegAVCodec<AVCodecCtxSetMaxBFrames> {
public:
  AVCodecCtxSetMaxBFrames(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t MaxBFrames);
};

class AVCodecCtxSetBQuantFactor
    : public WasmEdgeFFmpegAVCodec<AVCodecCtxSetBQuantFactor> {
public:
  AVCodecCtxSetBQuantFactor(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, float BQuantFactor);
};

class AVCodecCtxSetBQuantOffset
    : public WasmEdgeFFmpegAVCodec<AVCodecCtxSetBQuantOffset> {
public:
  AVCodecCtxSetBQuantOffset(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, float BQuantOffset);
};

class AVCodecCtxSetIQuantFactor
    : public WasmEdgeFFmpegAVCodec<AVCodecCtxSetIQuantFactor> {
public:
  AVCodecCtxSetIQuantFactor(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, float IQuantFactor);
};

class AVCodecCtxSetIQuantOffset
    : public WasmEdgeFFmpegAVCodec<AVCodecCtxSetIQuantOffset> {
public:
  AVCodecCtxSetIQuantOffset(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, float IQuantOffset);
};

class AVCodecCtxSetLumiMasking
    : public WasmEdgeFFmpegAVCodec<AVCodecCtxSetLumiMasking> {
public:
  AVCodecCtxSetLumiMasking(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, float LumiMasking);
};

class AVCodecCtxSetTemporalCplxMasking
    : public WasmEdgeFFmpegAVCodec<AVCodecCtxSetTemporalCplxMasking> {
public:
  AVCodecCtxSetTemporalCplxMasking(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, float TemporalCplxMasking);
};

class AVCodecCtxSetSpatialCplxMasking
    : public WasmEdgeFFmpegAVCodec<AVCodecCtxSetSpatialCplxMasking> {
public:
  AVCodecCtxSetSpatialCplxMasking(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, float SpatialCplxMasking);
};

class AVCodecCtxSetPMasking
    : public WasmEdgeFFmpegAVCodec<AVCodecCtxSetPMasking> {
public:
  AVCodecCtxSetPMasking(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, float PMasking);
};

class AVCodecCtxSetDarkMasking
    : public WasmEdgeFFmpegAVCodec<AVCodecCtxSetDarkMasking> {
public:
  AVCodecCtxSetDarkMasking(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, float DarkMasking);
};

class AVCodecCtxSetMeCmp : public WasmEdgeFFmpegAVCodec<AVCodecCtxSetMeCmp> {
public:
  AVCodecCtxSetMeCmp(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t MeCmp);
};

class AVCodecCtxSetMeSubCmp
    : public WasmEdgeFFmpegAVCodec<AVCodecCtxSetMeSubCmp> {
public:
  AVCodecCtxSetMeSubCmp(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t MeSubCmp);
};

class AVCodecCtxSetMbCmp : public WasmEdgeFFmpegAVCodec<AVCodecCtxSetMbCmp> {
public:
  AVCodecCtxSetMbCmp(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t MbCmp);
};

class AVCodecCtxSetIldctCmp
    : public WasmEdgeFFmpegAVCodec<AVCodecCtxSetIldctCmp> {
public:
  AVCodecCtxSetIldctCmp(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t IldctCmp);
};

class AVCodecCtxSetDiaSize
    : public WasmEdgeFFmpegAVCodec<AVCodecCtxSetDiaSize> {
public:
  AVCodecCtxSetDiaSize(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t DiaSize);
};

class AVCodecCtxSetLastPredictorsCount
    : public WasmEdgeFFmpegAVCodec<AVCodecCtxSetLastPredictorsCount> {
public:
  AVCodecCtxSetLastPredictorsCount(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t LastPredictorCount);
};

class AVCodecCtxSetMePreCmp
    : public WasmEdgeFFmpegAVCodec<AVCodecCtxSetMePreCmp> {
public:
  AVCodecCtxSetMePreCmp(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t MePreCmp);
};

class AVCodecCtxSetPreDiaSize
    : public WasmEdgeFFmpegAVCodec<AVCodecCtxSetPreDiaSize> {
public:
  AVCodecCtxSetPreDiaSize(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t PreDiaSize);
};

class AVCodecCtxSetMeSubpelQuality
    : public WasmEdgeFFmpegAVCodec<AVCodecCtxSetMeSubpelQuality> {
public:
  AVCodecCtxSetMeSubpelQuality(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t MeSubpelQuality);
};

class AVCodecCtxSetMeRange
    : public WasmEdgeFFmpegAVCodec<AVCodecCtxSetMeRange> {
public:
  AVCodecCtxSetMeRange(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t MeRange);
};

class AVCodecCtxSetMbDecision
    : public WasmEdgeFFmpegAVCodec<AVCodecCtxSetMbDecision> {
public:
  AVCodecCtxSetMbDecision(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t MbDecision);
};

class AVCodecCtxSetMbLMin : public WasmEdgeFFmpegAVCodec<AVCodecCtxSetMbLMin> {
public:
  AVCodecCtxSetMbLMin(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t MbLMin);
};

class AVCodecCtxSetMbLMax : public WasmEdgeFFmpegAVCodec<AVCodecCtxSetMbLMax> {
public:
  AVCodecCtxSetMbLMax(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t MbLMax);
};

class AVCodecCtxIntraDcPrecision
    : public WasmEdgeFFmpegAVCodec<AVCodecCtxIntraDcPrecision> {
public:
  AVCodecCtxIntraDcPrecision(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId);
};

class AVCodecCtxSetIntraDcPrecision
    : public WasmEdgeFFmpegAVCodec<AVCodecCtxSetIntraDcPrecision> {
public:
  AVCodecCtxSetIntraDcPrecision(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t IntraDcPrecision);
};

class AVCodecCtxSetQMin : public WasmEdgeFFmpegAVCodec<AVCodecCtxSetQMin> {
public:
  AVCodecCtxSetQMin(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t QMin);
};

class AVCodecCtxSetQMax : public WasmEdgeFFmpegAVCodec<AVCodecCtxSetQMax> {
public:
  AVCodecCtxSetQMax(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t QMax);
};

class AVCodecCtxSetGlobalQuality
    : public WasmEdgeFFmpegAVCodec<AVCodecCtxSetGlobalQuality> {
public:
  AVCodecCtxSetGlobalQuality(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t GlobalQuality);
};

class AVCodecCtxSetColorspace
    : public WasmEdgeFFmpegAVCodec<AVCodecCtxSetColorspace> {
public:
  AVCodecCtxSetColorspace(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t ColorspaceId);
};

class AVCodecCtxColorspace
    : public WasmEdgeFFmpegAVCodec<AVCodecCtxColorspace> {
public:
  AVCodecCtxColorspace(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId);
};

class AVCodecCtxSetColorRange
    : public WasmEdgeFFmpegAVCodec<AVCodecCtxSetColorRange> {
public:
  AVCodecCtxSetColorRange(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t ColorRange);
};

class AVCodecCtxColorRange
    : public WasmEdgeFFmpegAVCodec<AVCodecCtxColorRange> {
public:
  AVCodecCtxColorRange(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId);
};

class AVCodecCtxFrameSize : public WasmEdgeFFmpegAVCodec<AVCodecCtxFrameSize> {
public:
  AVCodecCtxFrameSize(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId);
};

class AVCodecCtxBitRate : public WasmEdgeFFmpegAVCodec<AVCodecCtxBitRate> {
public:
  AVCodecCtxBitRate(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int64_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId);
};

class AVCodecCtxSetBitRate
    : public WasmEdgeFFmpegAVCodec<AVCodecCtxSetBitRate> {
public:
  AVCodecCtxSetBitRate(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int64_t BitRate);
};

class AVCodecCtxRcMaxRate : public WasmEdgeFFmpegAVCodec<AVCodecCtxRcMaxRate> {
public:
  AVCodecCtxRcMaxRate(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int64_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId);
};

class AVCodecCtxSetRcMaxRate
    : public WasmEdgeFFmpegAVCodec<AVCodecCtxSetRcMaxRate> {
public:
  AVCodecCtxSetRcMaxRate(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int64_t RcMaxRate);
};

class AVCodecCtxSetBitRateTolerance
    : public WasmEdgeFFmpegAVCodec<AVCodecCtxSetBitRateTolerance> {
public:
  AVCodecCtxSetBitRateTolerance(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t BitRateTolerance);
};

class AVCodecCtxSetCompressionLevel
    : public WasmEdgeFFmpegAVCodec<AVCodecCtxSetCompressionLevel> {
public:
  AVCodecCtxSetCompressionLevel(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t CompressionLevel);
};

class AVCodecCtxFrameRate : public WasmEdgeFFmpegAVCodec<AVCodecCtxFrameRate> {
public:
  AVCodecCtxFrameRate(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, uint32_t NumPtr, uint32_t DenPtr);
};

class AVCodecCtxSetFrameRate
    : public WasmEdgeFFmpegAVCodec<AVCodecCtxSetFrameRate> {
public:
  AVCodecCtxSetFrameRate(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t Num, int32_t Den);
};

class AVCodecCtxSetFlags : public WasmEdgeFFmpegAVCodec<AVCodecCtxSetFlags> {
public:
  AVCodecCtxSetFlags(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t Flags);
};

class AVCodecCtxSetStrictStdCompliance
    : public WasmEdgeFFmpegAVCodec<AVCodecCtxSetStrictStdCompliance> {
public:
  AVCodecCtxSetStrictStdCompliance(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t ComplianceId);
};

class AVCodecCtxSetDebug : public WasmEdgeFFmpegAVCodec<AVCodecCtxSetDebug> {
public:
  AVCodecCtxSetDebug(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t Debug);
};

class AVCodecCtxCodec : public WasmEdgeFFmpegAVCodec<AVCodecCtxCodec> {
public:
  AVCodecCtxCodec(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, uint32_t AvCodecPtr);
};

class AVCodecCtxChannels : public WasmEdgeFFmpegAVCodec<AVCodecCtxChannels> {
public:
  AVCodecCtxChannels(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId);
};

class AVCodecCtxSetChannels
    : public WasmEdgeFFmpegAVCodec<AVCodecCtxSetChannels> {
public:
  AVCodecCtxSetChannels(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t Channels);
};

class AVCodecCtxSetSkipLoopFilter
    : public WasmEdgeFFmpegAVCodec<AVCodecCtxSetSkipLoopFilter> {
public:
  AVCodecCtxSetSkipLoopFilter(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t AVDicardId);
};

class AVCodecCtxSetSkipFrame
    : public WasmEdgeFFmpegAVCodec<AVCodecCtxSetSkipFrame> {
public:
  AVCodecCtxSetSkipFrame(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t AVDiscardId);
};

class AVCodecCtxSetSkipIdct
    : public WasmEdgeFFmpegAVCodec<AVCodecCtxSetSkipIdct> {
public:
  AVCodecCtxSetSkipIdct(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t AVDicardId);
};

class AVCodecCtxSetErrorConcealment
    : public WasmEdgeFFmpegAVCodec<AVCodecCtxSetErrorConcealment> {
public:
  AVCodecCtxSetErrorConcealment(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t ErrorConcealment);
};

class AVCodecCtxSetErrorRecognition
    : public WasmEdgeFFmpegAVCodec<AVCodecCtxSetErrorRecognition> {
public:
  AVCodecCtxSetErrorRecognition(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t ErrorRecognition);
};

class AVCodecCtxDelay : public WasmEdgeFFmpegAVCodec<AVCodecCtxDelay> {
public:
  AVCodecCtxDelay(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId);
};

class AVCodecCtxSetSkipTop
    : public WasmEdgeFFmpegAVCodec<AVCodecCtxSetSkipTop> {
public:
  AVCodecCtxSetSkipTop(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t Value);
};

class AVCodecCtxSetSkipBottom
    : public WasmEdgeFFmpegAVCodec<AVCodecCtxSetSkipBottom> {
public:
  AVCodecCtxSetSkipBottom(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t Value);
};

class AVCodecCtxRefs : public WasmEdgeFFmpegAVCodec<AVCodecCtxRefs> {
public:
  AVCodecCtxRefs(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId);
};

class AVCodecCtxSetSliceFlags
    : public WasmEdgeFFmpegAVCodec<AVCodecCtxSetSliceFlags> {
public:
  AVCodecCtxSetSliceFlags(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t Flags);
};

class AVCodecCtxSetSliceCount
    : public WasmEdgeFFmpegAVCodec<AVCodecCtxSetSliceCount> {
public:
  AVCodecCtxSetSliceCount(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t Value);
};

class AVCodecCtxSetFieldOrder
    : public WasmEdgeFFmpegAVCodec<AVCodecCtxSetFieldOrder> {
public:
  AVCodecCtxSetFieldOrder(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t Value);
};

class AVCodecCtxColorTrc : public WasmEdgeFFmpegAVCodec<AVCodecCtxColorTrc> {
public:
  AVCodecCtxColorTrc(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId);
};

class AVCodecCtxChromaSampleLocation
    : public WasmEdgeFFmpegAVCodec<AVCodecCtxChromaSampleLocation> {
public:
  AVCodecCtxChromaSampleLocation(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId);
};

class AVCodecCtxFrameNumber
    : public WasmEdgeFFmpegAVCodec<AVCodecCtxFrameNumber> {
public:
  AVCodecCtxFrameNumber(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId);
};

class AVCodecCtxBlockAlign
    : public WasmEdgeFFmpegAVCodec<AVCodecCtxBlockAlign> {
public:
  AVCodecCtxBlockAlign(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId);
};

class AVCodecCtxSetRequestSampleFmt
    : public WasmEdgeFFmpegAVCodec<AVCodecCtxSetRequestSampleFmt> {
public:
  AVCodecCtxSetRequestSampleFmt(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, uint32_t SampleFmtId);
};

class AVCodecCtxAudioServiceType
    : public WasmEdgeFFmpegAVCodec<AVCodecCtxAudioServiceType> {
public:
  AVCodecCtxAudioServiceType(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId);
};

class AVCodecCtxHasBFrames
    : public WasmEdgeFFmpegAVCodec<AVCodecCtxHasBFrames> {
public:
  AVCodecCtxHasBFrames(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId);
};

class AVCodecCtxSetRequestChannelLayout
    : public WasmEdgeFFmpegAVCodec<AVCodecCtxSetRequestChannelLayout> {
public:
  AVCodecCtxSetRequestChannelLayout(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, uint64_t ChannelLayoutId);
};

class AVCodecCtxActiveThreadType
    : public WasmEdgeFFmpegAVCodec<AVCodecCtxActiveThreadType> {
public:
  AVCodecCtxActiveThreadType(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId);
};

class AVCodecCtxSetThreadType
    : public WasmEdgeFFmpegAVCodec<AVCodecCtxSetThreadType> {
public:
  AVCodecCtxSetThreadType(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t ThreadType);
};

class AVCodecCtxThreadCount
    : public WasmEdgeFFmpegAVCodec<AVCodecCtxThreadCount> {
public:
  AVCodecCtxThreadCount(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId);
};

class AVCodecCtxSetThreadCount
    : public WasmEdgeFFmpegAVCodec<AVCodecCtxSetThreadCount> {
public:
  AVCodecCtxSetThreadCount(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId, int32_t ThreadCount);
};

class AVCodecCtxColorPrimaries
    : public WasmEdgeFFmpegAVCodec<AVCodecCtxColorPrimaries> {
public:
  AVCodecCtxColorPrimaries(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId);
};

} // namespace AVcodec
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge