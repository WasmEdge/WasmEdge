// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "avutil_func.h"

extern "C" {
#include "libavutil/avutil.h"
#include "libavutil/time.h"
}

#include <optional>
#include <string>

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVUtil {

Expect<void> AVLogSetLevel::body(const Runtime::CallingFrame &,
                                 int32_t LogLevelId) {
  av_log_set_level(LogLevelId);
  return {};
}

Expect<int32_t> AVLogGetLevel::body(const Runtime::CallingFrame &) {
  return av_log_get_level();
}

Expect<int32_t> AVLogGetFlags::body(const Runtime::CallingFrame &) {
  return av_log_get_flags();
}

Expect<void> AVLogSetFlags::body(const Runtime::CallingFrame &,
                                 int32_t FlagId) {
  av_log_set_flags(FlagId);
  return {};
}

Expect<int64_t> AVRescaleQ::body(const Runtime::CallingFrame &, int64_t A,
                                 int32_t BNum, int32_t BDen, int32_t CNum,
                                 int32_t CDen) {
  AVRational const B = av_make_q(BNum, BDen);
  AVRational const C = av_make_q(CNum, CDen);
  return av_rescale_q(A, B, C);
}

Expect<int64_t> AVRescaleQRnd::body(const Runtime::CallingFrame &, int64_t A,
                                    int32_t BNum, int32_t BDen, int32_t CNum,
                                    int32_t CDen, int32_t RoundingId) {
  AVRational const B = av_make_q(BNum, BDen);
  AVRational const C = av_make_q(CNum, CDen);
  AVRounding const Rounding = FFmpegUtils::Rounding::intoAVRounding(RoundingId);
  return av_rescale_q_rnd(A, B, C, Rounding);
}

Expect<uint32_t> AVUtilVersion::body(const Runtime::CallingFrame &) {
  return avutil_version();
}

Expect<int32_t>
AVGetChannelLayoutNbChannels::body(const Runtime::CallingFrame &,
                                   uint64_t ChannelLayoutId) {
  uint64_t const ChannelLayout =
      FFmpegUtils::ChannelLayout::fromChannelLayoutID(ChannelLayoutId);

  AVChannelLayout TmpChLayout;
  if (av_channel_layout_from_mask(&TmpChLayout, ChannelLayout) < 0) {
    return 0;
  }
  int32_t ChannelLayoutNbChannels = TmpChLayout.nb_channels;
  av_channel_layout_uninit(&TmpChLayout);

  return ChannelLayoutNbChannels;
}

namespace {
// Returns the name of the channel layout given by the FFmpeg mask, or nullopt
// when the mask does not form a valid layout. A single-channel layout is named
// by its channel (e.g. "FL"), a multi-channel layout by its description (e.g.
// "stereo"); the buffer is sized to the length FFmpeg reports, so large custom
// layouts are returned in full.
std::optional<std::string> describeChannelLayoutName(uint64_t ChannelLayout) {
  AVChannelLayout TmpChLayout;
  if (av_channel_layout_from_mask(&TmpChLayout, ChannelLayout) < 0) {
    return std::nullopt;
  }
  auto Describe = [&](char *Buf, size_t Size) {
    return TmpChLayout.nb_channels == 1
               ? av_channel_name(
                     Buf, Size,
                     FFmpegUtils::ChannelLayout::channelFromMask(ChannelLayout))
               : av_channel_layout_describe(&TmpChLayout, Buf, Size);
  };
  // Both calls report the size the full string needs, terminator included,
  // like snprintf, so query it first and size the buffer to it.
  int const Needed = Describe(nullptr, 0);
  if (Needed <= 0) {
    av_channel_layout_uninit(&TmpChLayout);
    return std::nullopt;
  }
  std::string Name(static_cast<size_t>(Needed), '\0');
  int const Ret = Describe(Name.data(), Name.size());
  av_channel_layout_uninit(&TmpChLayout);
  if (Ret < 0) {
    return std::nullopt;
  }
  Name.resize(std::strlen(Name.c_str()));
  return Name;
}
} // namespace

Expect<int32_t> AVGetChannelLayoutNameLen::body(const Runtime::CallingFrame &,
                                                uint64_t ChannelLayoutId) {
  uint64_t const ChannelLayout =
      FFmpegUtils::ChannelLayout::fromChannelLayoutID(ChannelLayoutId);
  std::optional<std::string> const Name =
      describeChannelLayoutName(ChannelLayout);
  if (!Name) {
    return 0;
  }
  return static_cast<int32_t>(Name->size());
}

Expect<int32_t> AVGetChannelLayoutName::body(const Runtime::CallingFrame &Frame,
                                             uint64_t ChannelLayoutId,
                                             uint32_t NamePtr,
                                             uint32_t NameLen) {
  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_SPAN_CHECK(NameBuf, MemInst, char, NamePtr, NameLen, "");

  uint64_t const ChannelLayout =
      FFmpegUtils::ChannelLayout::fromChannelLayoutID(ChannelLayoutId);
  std::optional<std::string> const Name =
      describeChannelLayoutName(ChannelLayout);
  if (!Name) {
    spdlog::error("[WasmEdge-FFmpeg] AVGetChannelLayoutName: cannot describe "
                  "channel layout mask {:#x}"sv,
                  ChannelLayout);
    return static_cast<int32_t>(ErrNo::InternalError);
  }
  copyCStringToBuffer(NameBuf.data(), NameLen, Name->c_str());
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<uint64_t> AVGetChannelLayoutMask::body(const Runtime::CallingFrame &,
                                              uint64_t ChannelLayoutId) {
  uint64_t const ChannelLayout =
      FFmpegUtils::ChannelLayout::fromChannelLayoutID(ChannelLayoutId);
  return ChannelLayout;
}

Expect<uint64_t> AVGetDefaultChannelLayout::body(const Runtime::CallingFrame &,
                                                 int32_t Number) {
  AVChannelLayout TmpChLayout;
  av_channel_layout_default(&TmpChLayout, Number);
  uint64_t const DefaultChannelLayout =
      FFmpegUtils::ChannelLayout::intoChannelLayoutID(TmpChLayout);
  av_channel_layout_uninit(&TmpChLayout);

  return DefaultChannelLayout;
}

Expect<int32_t> AVUtilConfigurationLength::body(const Runtime::CallingFrame &) {
  const char *Config = avutil_configuration();
  return strlen(Config);
}

Expect<int32_t> AVUtilConfiguration::body(const Runtime::CallingFrame &Frame,
                                          uint32_t ConfigPtr,
                                          uint32_t ConfigLen) {
  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_SPAN_CHECK(ConfigBuf, MemInst, char, ConfigPtr, ConfigLen, "");

  const char *Config = avutil_configuration();
  copyCStringToBuffer(ConfigBuf.data(), ConfigLen, Config);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVUtilLicenseLength::body(const Runtime::CallingFrame &) {
  const char *License = avutil_license();
  return strlen(License);
}

Expect<int32_t> AVUtilLicense::body(const Runtime::CallingFrame &Frame,
                                    uint32_t LicensePtr, uint32_t LicenseLen) {
  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_SPAN_CHECK(LicenseBuf, MemInst, char, LicensePtr, LicenseLen, "");

  const char *License = avutil_license();
  copyCStringToBuffer(LicenseBuf.data(), LicenseLen, License);
  return static_cast<int32_t>(ErrNo::Success);
}

} // namespace AVUtil
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
