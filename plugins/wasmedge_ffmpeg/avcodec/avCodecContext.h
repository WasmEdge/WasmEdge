#pragma once
#include "avcodec_base.h"
#include "runtime/callingframe.h"
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

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
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame,
                        uint32_t AvCodecCtxId);
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

class AVCodecCtxChannelLayout
    : public WasmEdgeFFmpegAVCodec<AVCodecCtxChannelLayout> {
public:
  AVCodecCtxChannelLayout(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<uint64_t> body(const Runtime::CallingFrame &Frame,
                        uint32_t AvCodecCtxId);
};

class AVCodecCtxPixFormat : public WasmEdgeFFmpegAVCodec<AVCodecCtxPixFormat> {
public:
  AVCodecCtxPixFormat(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame,
                        uint32_t AvCodecCtxId);
};

class AVCodecCtxSampleFormat
    : public WasmEdgeFFmpegAVCodec<AVCodecCtxSampleFormat> {
public:
  AVCodecCtxSampleFormat(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame,
                        uint32_t AvCodecCtxId);
};

class AVCodecCtxSampleRate
    : public WasmEdgeFFmpegAVCodec<AVCodecCtxSampleRate> {
public:
  AVCodecCtxSampleRate(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVCodec(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvCodecCtxId);
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

} // namespace AVcodec
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge