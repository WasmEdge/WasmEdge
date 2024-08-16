// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "ffmpeg_base.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVUtil {

class AVFrameAlloc : public HostFunction<AVFrameAlloc> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FramePtr);
};

class AVFrameFree : public HostFunction<AVFrameFree> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId);
};

class AVFrameWidth : public HostFunction<AVFrameWidth> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId);
};

class AVFrameHeight : public HostFunction<AVFrameHeight> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId);
};

class AVFrameSetWidth : public HostFunction<AVFrameSetWidth> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId,
                       uint32_t Width);
};

class AVFrameSetHeight : public HostFunction<AVFrameSetHeight> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId,
                       uint32_t Height);
};

class AVFrameVideoFormat : public HostFunction<AVFrameVideoFormat> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId);
};

class AVFrameSetVideoFormat : public HostFunction<AVFrameSetVideoFormat> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId,
                        uint32_t AvPixFormatId);
};

class AVFrameIsNull : public HostFunction<AVFrameIsNull> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId);
};

class AVFrameLinesize : public HostFunction<AVFrameLinesize> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId,
                       uint32_t Idx);
};

class AVFrameData : public HostFunction<AVFrameData> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId,
                       uint32_t FrameBufPtr, uint32_t FrameBufLen,
                       uint32_t Index);
};

class AVFrameGetBuffer : public HostFunction<AVFrameGetBuffer> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId,
                       int32_t Align);
};

class AVFrameAudioFormat : public HostFunction<AVFrameAudioFormat> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId);
};

class AVFrameSetAudioFormat : public HostFunction<AVFrameSetAudioFormat> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId,
                       uint32_t SampleFormatId);
};

class AVFrameSetChannelLayout : public HostFunction<AVFrameSetChannelLayout> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId,
                       uint64_t ChannelLayoutID);
};

class AVFrameSetNbSamples : public HostFunction<AVFrameSetNbSamples> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId,
                       int32_t Samples);
};

class AVFrameNbSamples : public HostFunction<AVFrameNbSamples> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId);
};

class AVFrameSampleRate : public HostFunction<AVFrameSampleRate> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId);
};

class AVFrameSetSampleRate : public HostFunction<AVFrameSetSampleRate> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId,
                       int32_t SampleRate);
};

class AVFrameChannels : public HostFunction<AVFrameChannels> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId);
};

class AVFrameSetChannels : public HostFunction<AVFrameSetChannels> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId,
                       int32_t Channels);
};

class AVFrameChannelLayout : public HostFunction<AVFrameChannelLayout> {
public:
  using HostFunction::HostFunction;
  Expect<uint64_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId);
};

class AVFrameBestEffortTimestamp
    : public HostFunction<AVFrameBestEffortTimestamp> {
public:
  using HostFunction::HostFunction;
  Expect<int64_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId);
};

class AVFramePictType : public HostFunction<AVFramePictType> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId);
};

class AVFrameSetPictType : public HostFunction<AVFrameSetPictType> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId,
                       int32_t PictureId);
};

class AVFrameInterlacedFrame : public HostFunction<AVFrameInterlacedFrame> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId);
};

class AVFrameTopFieldFirst : public HostFunction<AVFrameTopFieldFirst> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId);
};

class AVFramePaletteHasChanged : public HostFunction<AVFramePaletteHasChanged> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId);
};

class AVFrameColorSpace : public HostFunction<AVFrameColorSpace> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId);
};

class AVFrameSetColorSpace : public HostFunction<AVFrameSetColorSpace> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId,
                       int32_t ColorSpaceId);
};

class AVFrameColorRange : public HostFunction<AVFrameColorRange> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId);
};

class AVFrameSetColorRange : public HostFunction<AVFrameSetColorRange> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId,
                       int32_t ColorRangeId);
};

// color_transfer_characteristic

class AVFrameColorTransferCharacteristic
    : public HostFunction<AVFrameColorTransferCharacteristic> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId);
};

class AVFrameSetColorTransferCharacteristic
    : public HostFunction<AVFrameSetColorTransferCharacteristic> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId,
                       int32_t ColorTransferCharacteristicId);
};

class AVFrameChromaLocation : public HostFunction<AVFrameChromaLocation> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId);
};

class AVFrameCodedPictureNumber
    : public HostFunction<AVFrameCodedPictureNumber> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId);
};

class AVFrameDisplayPictureNumber
    : public HostFunction<AVFrameDisplayPictureNumber> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId);
};

class AVFrameRepeatPict : public HostFunction<AVFrameRepeatPict> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId);
};

class AVFrameFlags : public HostFunction<AVFrameFlags> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId);
};

class AVFrameQuality : public HostFunction<AVFrameQuality> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId);
};

class AVFrameMetadata : public HostFunction<AVFrameMetadata> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId,
                       uint32_t DictPtr);
};

class AVFrameSetMetadata : public HostFunction<AVFrameSetMetadata> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId,
                       uint32_t DictId);
};

class AVFrameKeyFrame : public HostFunction<AVFrameKeyFrame> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId);
};

class AVFramePts : public HostFunction<AVFramePts> {
public:
  using HostFunction::HostFunction;
  Expect<int64_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId);
};

class AVFrameSetPts : public HostFunction<AVFrameSetPts> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId,
                       int64_t Pts);
};

class AVFrameCopy : public HostFunction<AVFrameCopy> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t DestFrameId,
                       uint32_t SrcFrameId);
};

class AVFrameCopyProps : public HostFunction<AVFrameCopyProps> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t DestFrameId,
                       uint32_t SrcFrameId);
};

class AVFrameSampleAspectRatio : public HostFunction<AVFrameSampleAspectRatio> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId,
                       uint32_t NumPtr, uint32_t DenPtr);
};

class AVFrameColorPrimaries : public HostFunction<AVFrameColorPrimaries> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId);
};

class AVFrameSetColorPrimaries : public HostFunction<AVFrameSetColorPrimaries> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FrameId,
                       int32_t ColorPrimariesId);
};

} // namespace AVUtil
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
