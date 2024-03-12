#pragma once

#include "avutil_base.h"
#include "runtime/callingframe.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVUtil {

class AVFrameAlloc : public WasmEdgeFFmpegAVUtil<AVFrameAlloc> {
public:
  AVFrameAlloc(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FramePtr);
};

class AVFrameFree : public WasmEdgeFFmpegAVUtil<AVFrameFree> {
public:
  AVFrameFree(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId);
};

class AVFrameWidth : public WasmEdgeFFmpegAVUtil<AVFrameWidth> {
public:
  AVFrameWidth(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId);
};

class AVFrameHeight : public WasmEdgeFFmpegAVUtil<AVFrameHeight> {
public:
  AVFrameHeight(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId);
};

class AVFrameSetWidth : public WasmEdgeFFmpegAVUtil<AVFrameSetWidth> {
public:
  AVFrameSetWidth(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId,
                       uint32_t Width);
};

class AVFrameSetHeight : public WasmEdgeFFmpegAVUtil<AVFrameSetHeight> {
public:
  AVFrameSetHeight(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId,
                       uint32_t Height);
};

class AVFrameVideoFormat : public WasmEdgeFFmpegAVUtil<AVFrameVideoFormat> {
public:
  AVFrameVideoFormat(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId);
};

class AVFrameSetVideoFormat
    : public WasmEdgeFFmpegAVUtil<AVFrameSetVideoFormat> {
public:
  AVFrameSetVideoFormat(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId,
                        uint32_t AvPixFormatId);
};

class AVFrameIsNull : public WasmEdgeFFmpegAVUtil<AVFrameIsNull> {
public:
  AVFrameIsNull(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId);
};

class AVFrameLinesize : public WasmEdgeFFmpegAVUtil<AVFrameLinesize> {
public:
  AVFrameLinesize(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId,
                       uint32_t Idx);
};

class AVFrameData : public WasmEdgeFFmpegAVUtil<AVFrameData> {
public:
  AVFrameData(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId,
                       uint32_t FrameBufPtr, uint32_t FrameBufLen,
                       uint32_t Index);
};

class AVFrameGetBuffer : public WasmEdgeFFmpegAVUtil<AVFrameGetBuffer> {
public:
  AVFrameGetBuffer(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId,
                       int32_t Align);
};

class AVFrameAudioFormat : public WasmEdgeFFmpegAVUtil<AVFrameAudioFormat> {
public:
  AVFrameAudioFormat(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId);
};

class AVFrameSetAudioFormat
    : public WasmEdgeFFmpegAVUtil<AVFrameSetAudioFormat> {
public:
  AVFrameSetAudioFormat(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId,
                       uint32_t SampleFormatId);
};

class AVFrameSetChannelLayout
    : public WasmEdgeFFmpegAVUtil<AVFrameSetChannelLayout> {
public:
  AVFrameSetChannelLayout(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId,
                       uint64_t ChannelLayoutID);
};

class AVFrameSetNbSamples : public WasmEdgeFFmpegAVUtil<AVFrameSetNbSamples> {
public:
  AVFrameSetNbSamples(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId,
                       int32_t Samples);
};

class AVFrameNbSamples : public WasmEdgeFFmpegAVUtil<AVFrameNbSamples> {
public:
  AVFrameNbSamples(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId);
};

class AVFrameSampleRate : public WasmEdgeFFmpegAVUtil<AVFrameSampleRate> {
public:
  AVFrameSampleRate(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId);
};

class AVFrameSetSampleRate : public WasmEdgeFFmpegAVUtil<AVFrameSetSampleRate> {
public:
  AVFrameSetSampleRate(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId,
                       int32_t SampleRate);
};

class AVFrameChannels : public WasmEdgeFFmpegAVUtil<AVFrameChannels> {
public:
  AVFrameChannels(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId);
};

class AVFrameSetChannels : public WasmEdgeFFmpegAVUtil<AVFrameSetChannels> {
public:
  AVFrameSetChannels(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId,
                       int32_t Channels);
};

class AVFrameChannelLayout : public WasmEdgeFFmpegAVUtil<AVFrameChannelLayout> {
public:
  AVFrameChannelLayout(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<uint64_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId);
};

class AVFrameBestEffortTimestamp
    : public WasmEdgeFFmpegAVUtil<AVFrameBestEffortTimestamp> {
public:
  AVFrameBestEffortTimestamp(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int64_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId);
};

class AVFramePictType : public WasmEdgeFFmpegAVUtil<AVFramePictType> {
public:
  AVFramePictType(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId);
};

class AVFrameSetPictType : public WasmEdgeFFmpegAVUtil<AVFrameSetPictType> {
public:
  AVFrameSetPictType(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId,
                       int32_t PictureId);
};

class AVFrameInterlacedFrame
    : public WasmEdgeFFmpegAVUtil<AVFrameInterlacedFrame> {
public:
  AVFrameInterlacedFrame(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId);
};

class AVFrameTopFieldFirst : public WasmEdgeFFmpegAVUtil<AVFrameTopFieldFirst> {
public:
  AVFrameTopFieldFirst(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId);
};

class AVFramePaletteHasChanged
    : public WasmEdgeFFmpegAVUtil<AVFramePaletteHasChanged> {
public:
  AVFramePaletteHasChanged(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId);
};

class AVFrameColorSpace : public WasmEdgeFFmpegAVUtil<AVFrameColorSpace> {
public:
  AVFrameColorSpace(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId);
};

class AVFrameSetColorSpace : public WasmEdgeFFmpegAVUtil<AVFrameSetColorSpace> {
public:
  AVFrameSetColorSpace(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId,
                       int32_t ColorSpaceId);
};

class AVFrameColorRange : public WasmEdgeFFmpegAVUtil<AVFrameColorRange> {
public:
  AVFrameColorRange(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId);
};

class AVFrameSetColorRange : public WasmEdgeFFmpegAVUtil<AVFrameSetColorRange> {
public:
  AVFrameSetColorRange(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId,
                       int32_t ColorRangeId);
};

// color_transfer_characteristic

class AVFrameColorTransferCharacteristic
    : public WasmEdgeFFmpegAVUtil<AVFrameColorTransferCharacteristic> {
public:
  AVFrameColorTransferCharacteristic(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId);
};

class AVFrameSetColorTransferCharacteristic
    : public WasmEdgeFFmpegAVUtil<AVFrameSetColorTransferCharacteristic> {
public:
  AVFrameSetColorTransferCharacteristic(
      std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId,
                       int32_t ColorTransferCharacteristicId);
};

class AVFrameChromaLocation
    : public WasmEdgeFFmpegAVUtil<AVFrameChromaLocation> {
public:
  AVFrameChromaLocation(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId);
};

class AVFrameCodedPictureNumber
    : public WasmEdgeFFmpegAVUtil<AVFrameCodedPictureNumber> {
public:
  AVFrameCodedPictureNumber(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId);
};

class AVFrameDisplayPictureNumber
    : public WasmEdgeFFmpegAVUtil<AVFrameDisplayPictureNumber> {
public:
  AVFrameDisplayPictureNumber(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId);
};

class AVFrameRepeatPict : public WasmEdgeFFmpegAVUtil<AVFrameRepeatPict> {
public:
  AVFrameRepeatPict(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId);
};

class AVFrameFlags : public WasmEdgeFFmpegAVUtil<AVFrameFlags> {
public:
  AVFrameFlags(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId);
};

class AVFrameQuality : public WasmEdgeFFmpegAVUtil<AVFrameQuality> {
public:
  AVFrameQuality(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId);
};

class AVFrameMetadata : public WasmEdgeFFmpegAVUtil<AVFrameMetadata> {
public:
  AVFrameMetadata(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId,
                       uint32_t DictPtr);
};

class AVFrameSetMetadata : public WasmEdgeFFmpegAVUtil<AVFrameSetMetadata> {
public:
  AVFrameSetMetadata(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId,
                       uint32_t DictId);
};

class AVFrameKeyFrame : public WasmEdgeFFmpegAVUtil<AVFrameKeyFrame> {
public:
  AVFrameKeyFrame(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId);
};

class AVFramePts : public WasmEdgeFFmpegAVUtil<AVFramePts> {
public:
  AVFramePts(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int64_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId);
};

class AVFrameSetPts : public WasmEdgeFFmpegAVUtil<AVFrameSetPts> {
public:
  AVFrameSetPts(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId,
                       int64_t Pts);
};

class AVFrameCopy : public WasmEdgeFFmpegAVUtil<AVFrameCopy> {
public:
  AVFrameCopy(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t DestFrameId,
                       uint32_t SrcFrameId);
};

class AVFrameCopyProps : public WasmEdgeFFmpegAVUtil<AVFrameCopyProps> {
public:
  AVFrameCopyProps(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t DestFrameId,
                       uint32_t SrcFrameId);
};

class AVFrameSampleAspectRatio
    : public WasmEdgeFFmpegAVUtil<AVFrameSampleAspectRatio> {
public:
  AVFrameSampleAspectRatio(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId,
                       uint32_t NumPtr, uint32_t DenPtr);
};

class AVFrameColorPrimaries
    : public WasmEdgeFFmpegAVUtil<AVFrameColorPrimaries> {
public:
  AVFrameColorPrimaries(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId);
};

class AVFrameSetColorPrimaries
    : public WasmEdgeFFmpegAVUtil<AVFrameSetColorPrimaries> {
public:
  AVFrameSetColorPrimaries(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId,
                       int32_t ColorPrimariesId);
};

} // namespace AVUtil
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
