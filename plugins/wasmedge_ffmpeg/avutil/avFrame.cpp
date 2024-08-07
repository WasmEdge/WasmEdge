// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

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
                "Failed to access Memory for AVFrame"sv)

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

Expect<int32_t> AVFrameWidth::body(const Runtime::CallingFrame &,
                                   uint32_t FrameId) {
  FFMPEG_PTR_FETCH(AvFrame, FrameId, AVFrame);
  return AvFrame->width;
}

Expect<int32_t> AVFrameHeight::body(const Runtime::CallingFrame &,
                                    uint32_t FrameId) {
  FFMPEG_PTR_FETCH(AvFrame, FrameId, AVFrame);
  return AvFrame->height;
}

Expect<int32_t> AVFrameSetHeight::body(const Runtime::CallingFrame &,
                                       uint32_t FrameId, uint32_t Height) {
  FFMPEG_PTR_FETCH(AvFrame, FrameId, AVFrame);
  AvFrame->height = Height;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVFrameSetWidth::body(const Runtime::CallingFrame &,
                                      uint32_t FrameId, uint32_t Width) {
  FFMPEG_PTR_FETCH(AvFrame, FrameId, AVFrame);
  AvFrame->width = Width;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVFrameVideoFormat::body(const Runtime::CallingFrame &,
                                         uint32_t FrameId) {
  FFMPEG_PTR_FETCH(AvFrame, FrameId, AVFrame);
  int const Format = AvFrame->format;
  if (Format == -1) {
    return -1;
  }
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
  return AvFrame->data[0] == nullptr;
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
  std::copy_n(Data, FrameBufLen, Buffer.data());
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
  if (Format == -1) {
    return -1;
  }

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

Expect<int32_t> AVFrameSetSampleRate::body(const Runtime::CallingFrame &,
                                           uint32_t FrameId,
                                           int32_t SampleRate) {
  FFMPEG_PTR_FETCH(AvFrame, FrameId, AVFrame);
  AvFrame->sample_rate = SampleRate;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVFrameChannels::body(const Runtime::CallingFrame &,
                                      uint32_t FrameId) {
  FFMPEG_PTR_FETCH(AvFrame, FrameId, AVFrame);
  return AvFrame->channels;
}

Expect<int32_t> AVFrameSetChannels::body(const Runtime::CallingFrame &,
                                         uint32_t FrameId, int32_t Channels) {
  FFMPEG_PTR_FETCH(AvFrame, FrameId, AVFrame);
  AvFrame->channels = Channels;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<uint64_t> AVFrameChannelLayout::body(const Runtime::CallingFrame &,
                                            uint32_t FrameId) {
  FFMPEG_PTR_FETCH(AvFrame, FrameId, AVFrame);
  uint64_t const ChannelLayout = AvFrame->channel_layout;
  return FFmpegUtils::ChannelLayout::intoChannelLayoutID(ChannelLayout);
}

Expect<int64_t> AVFrameBestEffortTimestamp::body(const Runtime::CallingFrame &,
                                                 uint32_t FrameId) {
  FFMPEG_PTR_FETCH(AvFrame, FrameId, AVFrame);
  return AvFrame->best_effort_timestamp;
}

Expect<int32_t> AVFramePictType::body(const Runtime::CallingFrame &,
                                      uint32_t FrameId) {
  FFMPEG_PTR_FETCH(AvFrame, FrameId, AVFrame);
  AVPictureType const AvPictureType = AvFrame->pict_type;
  return FFmpegUtils::PictureType::fromAVPictureType(AvPictureType);
}

Expect<int32_t> AVFrameSetPictType::body(const Runtime::CallingFrame &,
                                         uint32_t FrameId, int32_t PictureId) {
  FFMPEG_PTR_FETCH(AvFrame, FrameId, AVFrame);
  AVPictureType const AvPictureType =
      FFmpegUtils::PictureType::intoAVPictureType(PictureId);

  AvFrame->pict_type = AvPictureType;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVFrameInterlacedFrame::body(const Runtime::CallingFrame &,
                                             uint32_t FrameId) {
  FFMPEG_PTR_FETCH(AvFrame, FrameId, AVFrame);
  return AvFrame->interlaced_frame;
}

Expect<int32_t> AVFrameTopFieldFirst::body(const Runtime::CallingFrame &,
                                           uint32_t FrameId) {
  FFMPEG_PTR_FETCH(AvFrame, FrameId, AVFrame);
  return AvFrame->top_field_first;
}

Expect<int32_t> AVFramePaletteHasChanged::body(const Runtime::CallingFrame &,
                                               uint32_t FrameId) {
  FFMPEG_PTR_FETCH(AvFrame, FrameId, AVFrame);
  return AvFrame->palette_has_changed;
}

Expect<int32_t> AVFrameColorSpace::body(const Runtime::CallingFrame &,
                                        uint32_t FrameId) {
  FFMPEG_PTR_FETCH(AvFrame, FrameId, AVFrame);
  AVColorSpace const AvColorSpace = AvFrame->colorspace;
  return FFmpegUtils::ColorSpace::fromAVColorSpace(AvColorSpace);
}

Expect<int32_t> AVFrameSetColorSpace::body(const Runtime::CallingFrame &,
                                           uint32_t FrameId,
                                           int32_t ColorSpaceId) {
  FFMPEG_PTR_FETCH(AvFrame, FrameId, AVFrame);
  AvFrame->colorspace = FFmpegUtils::ColorSpace::intoAVColorSpace(ColorSpaceId);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVFrameColorRange::body(const Runtime::CallingFrame &,
                                        uint32_t FrameId) {
  FFMPEG_PTR_FETCH(AvFrame, FrameId, AVFrame);
  AVColorRange const AvColorRange = AvFrame->color_range;

  return static_cast<int32_t>(AvColorRange);
}

Expect<int32_t> AVFrameSetColorRange::body(const Runtime::CallingFrame &,
                                           uint32_t FrameId,
                                           int32_t ColorRangeId) {
  FFMPEG_PTR_FETCH(AvFrame, FrameId, AVFrame);
  AvFrame->color_range = static_cast<AVColorRange>(ColorRangeId);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t>
AVFrameColorTransferCharacteristic::body(const Runtime::CallingFrame &,
                                         uint32_t FrameId) {
  FFMPEG_PTR_FETCH(AvFrame, FrameId, AVFrame);
  AVColorTransferCharacteristic const Characteristic = AvFrame->color_trc;

  // Can use the binding as well. Currently, Commented the binding.
  return static_cast<int32_t>(Characteristic);
}

Expect<int32_t> AVFrameSetColorTransferCharacteristic::body(
    const Runtime::CallingFrame &, uint32_t FrameId,
    int32_t ColorTransferCharacteristicId) {
  FFMPEG_PTR_FETCH(AvFrame, FrameId, AVFrame);
  AvFrame->color_trc =
      static_cast<AVColorTransferCharacteristic>(ColorTransferCharacteristicId);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVFrameChromaLocation::body(const Runtime::CallingFrame &,
                                            uint32_t FrameId) {
  FFMPEG_PTR_FETCH(AvFrame, FrameId, AVFrame);
  AVChromaLocation const AvChromaLocation = AvFrame->chroma_location;
  return FFmpegUtils::ChromaLocation::fromAVChromaLocation(AvChromaLocation);
}

Expect<int32_t> AVFrameCodedPictureNumber::body(const Runtime::CallingFrame &,
                                                uint32_t FrameId) {
  FFMPEG_PTR_FETCH(AvFrame, FrameId, AVFrame);
  return AvFrame->coded_picture_number;
}

Expect<int32_t> AVFrameDisplayPictureNumber::body(const Runtime::CallingFrame &,
                                                  uint32_t FrameId) {
  FFMPEG_PTR_FETCH(AvFrame, FrameId, AVFrame);
  return AvFrame->display_picture_number;
}

Expect<int32_t> AVFrameRepeatPict::body(const Runtime::CallingFrame &,
                                        uint32_t FrameId) {
  FFMPEG_PTR_FETCH(AvFrame, FrameId, AVFrame);
  return AvFrame->repeat_pict;
}

Expect<int32_t> AVFrameFlags::body(const Runtime::CallingFrame &,
                                   uint32_t FrameId) {
  FFMPEG_PTR_FETCH(AvFrame, FrameId, AVFrame);
  return AvFrame->flags;
}

Expect<int32_t> AVFrameQuality::body(const Runtime::CallingFrame &,
                                     uint32_t FrameId) {
  FFMPEG_PTR_FETCH(AvFrame, FrameId, AVFrame);
  return AvFrame->quality;
}

Expect<int32_t> AVFrameMetadata::body(const Runtime::CallingFrame &Frame,
                                      uint32_t FrameId, uint32_t DictPtr) {
  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_PTR_CHECK(DictId, MemInst, uint32_t, DictPtr,
                "Failed when accessing the return AVDictionary memory"sv);

  FFMPEG_PTR_FETCH(AvFrame, FrameId, AVFrame);

  AVDictionary **AvDictionary =
      static_cast<AVDictionary **>(av_malloc(sizeof(AVDictionary *)));

  *AvDictionary = AvFrame->metadata;
  FFMPEG_PTR_STORE(AvDictionary, DictId);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVFrameSetMetadata::body(const Runtime::CallingFrame &,
                                         uint32_t FrameId, uint32_t DictId) {
  FFMPEG_PTR_FETCH(AvFrame, FrameId, AVFrame);
  FFMPEG_PTR_FETCH(AvDict, DictId, AVDictionary *);

  if (AvDict == nullptr) {
    AvFrame->metadata = nullptr;
  } else {
    AvFrame->metadata = *AvDict;
  }
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVFrameKeyFrame::body(const Runtime::CallingFrame &,
                                      uint32_t FrameId) {
  FFMPEG_PTR_FETCH(AvFrame, FrameId, AVFrame);
  return AvFrame->key_frame;
}

Expect<int64_t> AVFramePts::body(const Runtime::CallingFrame &,
                                 uint32_t FrameId) {
  FFMPEG_PTR_FETCH(AvFrame, FrameId, AVFrame);
  return AvFrame->pts;
}

Expect<int32_t> AVFrameSetPts::body(const Runtime::CallingFrame &,
                                    uint32_t FrameId, int64_t Pts) {
  FFMPEG_PTR_FETCH(AvFrame, FrameId, AVFrame);
  AvFrame->pts = Pts;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVFrameCopy::body(const Runtime::CallingFrame &,
                                  uint32_t DestFrameId, uint32_t SrcFrameId) {
  FFMPEG_PTR_FETCH(DestAvFrame, DestFrameId, AVFrame);
  FFMPEG_PTR_FETCH(SrcAvFrame, SrcFrameId, AVFrame);

  av_frame_copy(DestAvFrame, SrcAvFrame);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVFrameCopyProps::body(const Runtime::CallingFrame &,
                                       uint32_t DestFrameId,
                                       uint32_t SrcFrameId) {
  FFMPEG_PTR_FETCH(DestAvFrame, DestFrameId, AVFrame);
  FFMPEG_PTR_FETCH(SrcAvFrame, SrcFrameId, AVFrame);

  av_frame_copy_props(DestAvFrame, SrcAvFrame);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t>
AVFrameSampleAspectRatio::body(const Runtime::CallingFrame &Frame,
                               uint32_t FrameId, uint32_t NumPtr,
                               uint32_t DenPtr) {
  MEMINST_CHECK(MemInst, Frame, 0)
  FFMPEG_PTR_FETCH(AvFrame, FrameId, AVFrame);

  MEM_PTR_CHECK(Num, MemInst, int32_t, NumPtr,
                "Failed to access Numerator Ptr for AVRational"sv);
  MEM_PTR_CHECK(Den, MemInst, int32_t, DenPtr,
                "Failed to access Denominator Ptr for AVRational"sv);

  AVRational const Rational = AvFrame->sample_aspect_ratio;
  *Num = Rational.num;
  *Den = Rational.den;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVFrameColorPrimaries::body(const Runtime::CallingFrame &,
                                            uint32_t FrameId) {
  FFMPEG_PTR_FETCH(AvFrame, FrameId, AVFrame);
  AVColorPrimaries const ColorPrimaries = AvFrame->color_primaries;
  return FFmpegUtils::ColorPrimaries::fromAVColorPrimaries(ColorPrimaries);
}

Expect<int32_t> AVFrameSetColorPrimaries::body(const Runtime::CallingFrame &,
                                               uint32_t FrameId,
                                               int32_t ColorPrimariesId) {
  FFMPEG_PTR_FETCH(AvFrame, FrameId, AVFrame);
  AVColorPrimaries const ColorPrimaries =
      FFmpegUtils::ColorPrimaries::intoAVColorPrimaries(ColorPrimariesId);
  AvFrame->color_primaries = ColorPrimaries;
  return static_cast<int32_t>(ErrNo::Success);
}

} // namespace AVUtil
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
