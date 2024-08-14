// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "avCodec.h"

extern "C" {
#include "libavcodec/avcodec.h"
}

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVcodec {

Expect<uint32_t> AVCodecID::body(const Runtime::CallingFrame &,
                                 uint32_t AvCodecId) {
  FFMPEG_PTR_FETCH(AvCodec, AvCodecId, const AVCodec);
  return FFmpegUtils::CodecID::fromAVCodecID(AvCodec->id);
}

Expect<int32_t> AVCodecType::body(const Runtime::CallingFrame &,
                                  uint32_t AvCodecId) {
  FFMPEG_PTR_FETCH(AvCodec, AvCodecId, const AVCodec);
  return FFmpegUtils::MediaType::fromMediaType(AvCodec->type);
}

Expect<int32_t> AVCodecMaxLowres::body(const Runtime::CallingFrame &,
                                       uint32_t AvCodecId) {
  FFMPEG_PTR_FETCH(AvCodec, AvCodecId, const AVCodec);
  return AvCodec->max_lowres;
}

Expect<int32_t> AVCodecCapabilities::body(const Runtime::CallingFrame &,
                                          uint32_t AvCodecId) {
  FFMPEG_PTR_FETCH(AvCodec, AvCodecId, const AVCodec);
  return AvCodec->capabilities;
}

Expect<int32_t> AVCodecGetNameLen::body(const Runtime::CallingFrame &,
                                        uint32_t AvCodecId) {
  FFMPEG_PTR_FETCH(AvCodec, AvCodecId, const AVCodec);
  return strlen(AvCodec->name);
}

Expect<int32_t> AVCodecGetName::body(const Runtime::CallingFrame &Frame,
                                     uint32_t AvCodecId, uint32_t NamePtr,
                                     uint32_t NameLen) {
  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_SPAN_CHECK(NameBuf, MemInst, char, NamePtr, NameLen, "");
  FFMPEG_PTR_FETCH(AvCodec, AvCodecId, const AVCodec);

  const char *Name = AvCodec->name;
  std::copy_n(Name, NameLen, NameBuf.data());
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVCodecGetLongNameLen::body(const Runtime::CallingFrame &,
                                            uint32_t AvCodecId) {
  FFMPEG_PTR_FETCH(AvCodec, AvCodecId, const AVCodec);
  return strlen(AvCodec->long_name);
}

Expect<int32_t> AVCodecGetLongName::body(const Runtime::CallingFrame &Frame,
                                         uint32_t AvCodecId,
                                         uint32_t LongNamePtr,
                                         uint32_t LongNameLen) {
  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_SPAN_CHECK(LongNameBuf, MemInst, char, LongNamePtr, LongNameLen, "");
  FFMPEG_PTR_FETCH(AvCodec, AvCodecId, const AVCodec);

  const char *LongName = AvCodec->long_name;
  std::copy_n(LongName, LongNameLen, LongNameBuf.data());
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVCodecProfiles::body(const Runtime::CallingFrame &,
                                      uint32_t AvCodecId) {
  FFMPEG_PTR_FETCH(AvCodec, AvCodecId, const AVCodec);
  if (AvCodec->profiles) {
    return 1;
  }
  return 0;
}

Expect<int32_t> AVCodecPixFmtsIsNull::body(const Runtime::CallingFrame &,
                                           uint32_t AvCodecId) {
  FFMPEG_PTR_FETCH(AvCodec, AvCodecId, const AVCodec);
  if (AvCodec->pix_fmts == nullptr) {
    return 1;
  }
  return 0;
}

Expect<uint32_t> AVCodecPixFmtsIter::body(const Runtime::CallingFrame &,
                                          uint32_t AvCodecId, uint32_t Idx) {
  FFMPEG_PTR_FETCH(AvCodec, AvCodecId, const AVCodec);
  AVPixelFormat const *PixelFormat = AvCodec->pix_fmts;
  if (PixelFormat == nullptr) {
    return 0;
  }

  uint32_t Curr = 0;
  while (Curr < Idx) {
    PixelFormat++;
    Curr++;
  }

  return FFmpegUtils::PixFmt::fromAVPixFmt(*PixelFormat);
}

Expect<int32_t>
AVCodecSupportedFrameratesIsNull::body(const Runtime::CallingFrame &,
                                       uint32_t AvCodecId) {
  FFMPEG_PTR_FETCH(AvCodec, AvCodecId, const AVCodec);
  if (AvCodec->supported_framerates == nullptr) {
    return 1;
  }
  return 0;
}

Expect<int32_t>
AVCodecSupportedFrameratesIter::body(const Runtime::CallingFrame &Frame,
                                     uint32_t AvCodecId, uint32_t Idx,
                                     uint32_t NumPtr, uint32_t DenPtr) {
  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_PTR_CHECK(NumId, MemInst, int32_t, NumPtr,
                "Failed when accessing the return NumPtr Memory"sv);
  MEM_PTR_CHECK(DenId, MemInst, int32_t, DenPtr,
                "Failed when accessing the return DenPtr Memory"sv);

  FFMPEG_PTR_FETCH(AvCodec, AvCodecId, const AVCodec);
  AVRational const *Rational = AvCodec->supported_framerates;

  if (Rational == nullptr) {
    *NumId = 0;
    *DenId = 0;
    return static_cast<int32_t>(ErrNo::Success);
  }

  uint32_t Curr = 0;
  while (Curr < Idx) {
    Rational++;
    Curr++;
  }

  *NumId = Rational->num;
  *DenId = Rational->den;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t>
AVCodecSupportedSampleRatesIsNull::body(const Runtime::CallingFrame &,
                                        uint32_t AvCodecId) {
  FFMPEG_PTR_FETCH(AvCodec, AvCodecId, const AVCodec);
  if (AvCodec->supported_samplerates == nullptr) {
    return 1;
  }
  return 0;
}

Expect<int32_t>
AVCodecSupportedSampleRatesIter::body(const Runtime::CallingFrame &,
                                      uint32_t AvCodecId, uint32_t Idx) {
  FFMPEG_PTR_FETCH(AvCodec, AvCodecId, const AVCodec);
  const int32_t *SampleRates = AvCodec->supported_samplerates;
  if (SampleRates == nullptr) {
    return 0;
  }

  uint32_t Curr = 0;
  while (Curr < Idx) {
    SampleRates++;
    Curr++;
  }

  return *SampleRates;
}

Expect<int32_t> AVCodecChannelLayoutIsNull::body(const Runtime::CallingFrame &,
                                                 uint32_t AvCodecId) {
  FFMPEG_PTR_FETCH(AvCodec, AvCodecId, const AVCodec);
  if (AvCodec->channel_layouts == nullptr) {
    return 1;
  }
  return 0;
}

Expect<uint64_t> AVCodecChannelLayoutIter::body(const Runtime::CallingFrame &,
                                                uint32_t AvCodecId,
                                                uint32_t Idx) {

  FFMPEG_PTR_FETCH(AvCodec, AvCodecId, const AVCodec);
  const uint64_t *ChannelLayout = AvCodec->channel_layouts;
  if (ChannelLayout == nullptr) {
    return 0;
  }

  uint32_t Curr = 0;
  while (Curr < Idx) {
    ChannelLayout++;
    Curr++;
  }

  return FFmpegUtils::ChannelLayout::intoChannelLayoutID(*ChannelLayout);
}

Expect<int32_t> AVCodecSampleFmtsIsNull::body(const Runtime::CallingFrame &,
                                              uint32_t AvCodecId) {
  FFMPEG_PTR_FETCH(AvCodec, AvCodecId, const AVCodec);
  if (AvCodec->sample_fmts == nullptr) {
    return 1;
  }
  return 0;
}

Expect<uint32_t> AVCodecSampleFmtsIter::body(const Runtime::CallingFrame &,
                                             uint32_t AvCodecId, uint32_t Idx) {

  FFMPEG_PTR_FETCH(AvCodec, AvCodecId, const AVCodec);
  AVSampleFormat const *SampleFormat = AvCodec->sample_fmts;
  if (SampleFormat == nullptr) {
    return 0;
  }

  uint32_t Curr = 0;
  while (Curr < Idx) {
    SampleFormat++;
    Curr++;
  }

  return FFmpegUtils::SampleFmt::toSampleID(*SampleFormat);
}

} // namespace AVcodec
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
