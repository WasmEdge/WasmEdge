// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "avutil/avutil_func.h"
#include "avutil/avTime.h"
#include "avutil/module.h"

#include "utils.h"

#include <gtest/gtest.h>

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {

TEST_F(FFmpegTest, AVUtilFunc) {
  ASSERT_TRUE(AVUtilMod != nullptr);

  uint32_t NamePtr = UINT32_C(4);

  auto *FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_log_set_level");
  auto &HostFuncAVLogSetLevel =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVLogSetLevel &>(
          FuncInst->getHostFunc());

  int32_t LogLvlId = 32;
  {
    HostFuncAVLogSetLevel.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{LogLvlId},
        Result);
  }

  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_log_get_level");
  auto &HostFuncAVLogGetLevel =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVLogGetLevel &>(
          FuncInst->getHostFunc());

  {
    HostFuncAVLogGetLevel.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{}, Result);
    EXPECT_EQ(Result[0].get<int32_t>(), LogLvlId);
  }

  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_log_set_flags");
  auto &HostFuncAVLogSetFlags =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVLogSetFlags &>(
          FuncInst->getHostFunc());

  {
    HostFuncAVLogSetFlags.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{1}, Result);
  }

  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_log_get_flags");
  auto &HostFuncAVLogGetFlags =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVLogGetFlags &>(
          FuncInst->getHostFunc());

  {
    HostFuncAVLogGetFlags.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{1}, Result);

    EXPECT_EQ(Result[0].get<int32_t>(), 32);
  }

  FuncInst = AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_rescale_q");
  auto &HostFuncAVRescaleQ =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVRescaleQ &>(
          FuncInst->getHostFunc());

  int64_t A = 20;
  int32_t BNum = 5;
  int32_t BDen = 10;
  int32_t CNum = 5;
  int32_t CDen = 20;

  {
    HostFuncAVRescaleQ.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{A, BNum, BDen, CNum, CDen},
        Result);

    EXPECT_TRUE(Result[0].get<int64_t>() > 0);
  }

  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_rescale_q_rnd");
  auto &HostFuncAVRescaleQRnd =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVRescaleQRnd &>(
          FuncInst->getHostFunc());

  {
    int32_t RoundingId = 2;
    HostFuncAVRescaleQRnd.run(CallFrame,
                              std::initializer_list<WasmEdge::ValVariant>{
                                  A, BNum, BDen, CNum, CDen, RoundingId},
                              Result);

    EXPECT_TRUE(Result[0].get<int64_t>() > 0);
  }

  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_avutil_version");
  auto &HostFuncAVUtilVersion =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVUtilVersion &>(
          FuncInst->getHostFunc());

  {
    HostFuncAVUtilVersion.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{}, Result);

    EXPECT_TRUE(Result[0].get<uint32_t>() > 0);
  }

  uint64_t ChannelId = 1; // FRONT_LEFT
  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_get_channel_layout_nb_channels");
  auto &HostFuncAVGetChannelLayoutNbChannels = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVGetChannelLayoutNbChannels &>(
      FuncInst->getHostFunc());

  {
    HostFuncAVGetChannelLayoutNbChannels.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{ChannelId},
        Result);
    EXPECT_TRUE(Result[0].get<int32_t>() > 0);
  }

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_get_default_channel_layout");
  auto &HostFuncAVGetDefaultChannelLayout = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVGetDefaultChannelLayout &>(
      FuncInst->getHostFunc());

  {
    HostFuncAVGetDefaultChannelLayout.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{ChannelId},
        Result);
    EXPECT_TRUE(Result[0].get<uint64_t>() > 0);
  }

  uint32_t Length = 0;
  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_avutil_configuration_length");
  auto &HostFuncAVUtilConfigurationLength = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVUtilConfigurationLength &>(
      FuncInst->getHostFunc());

  {
    HostFuncAVUtilConfigurationLength.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{}, Result);

    Length = Result[0].get<int32_t>();
    EXPECT_TRUE(Length > 0);
  }

  // Fill NamePtr with 0.
  fillMemContent(MemInst, NamePtr, Length);
  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_avutil_configuration");
  auto &HostFuncAVUtilConfiguration = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVUtilConfiguration &>(
      FuncInst->getHostFunc());

  {
    HostFuncAVUtilConfiguration.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{NamePtr, Length},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_avutil_license_length");
  auto &HostFuncAVUtilLicenseLength = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVUtilLicenseLength &>(
      FuncInst->getHostFunc());

  {
    HostFuncAVUtilLicenseLength.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{}, Result);

    Length = Result[0].get<int32_t>();
    EXPECT_TRUE(Length > 0);
  }

  // Fill NamePtr with 0.
  fillMemContent(MemInst, NamePtr, Length);
  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_avutil_license");
  auto &HostFuncAVUtilLicense =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVUtilLicense &>(
          FuncInst->getHostFunc());

  {
    HostFuncAVUtilLicense.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{NamePtr, Length},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }
}

TEST_F(FFmpegTest, AVUtilChannelLayoutNameBounds) {
  ASSERT_TRUE(AVUtilMod != nullptr);

  auto *FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_get_channel_layout_name");
  auto &HostFuncAVGetChannelLayoutName = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVGetChannelLayoutName &>(
      FuncInst->getHostFunc());

  uint32_t NamePtr = UINT32_C(4);
  uint64_t ChannelId = 1; // FRONT_LEFT, a short name such as "FL".
  // NameLen is guest-controlled and deliberately far larger than the 16-byte
  // host-side ChName buffer the implementation copies from.
  uint32_t NameLen = UINT32_C(64);

  // Pre-fill the whole destination span with a sentinel so any byte the host
  // writes past the real channel name becomes observable.
  fillMemContent(MemInst, NamePtr, NameLen, UINT8_C(0xAA));

  HostFuncAVGetChannelLayoutName.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{ChannelId, NamePtr, NameLen},
      Result);
  EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

  char *Buf = MemInst->getPointer<char *>(NamePtr);
  uint32_t WrittenLen = 0;
  while (WrittenLen < UINT32_C(16) && Buf[WrittenLen] != '\0') {
    ++WrittenLen;
  }
  // The source name always fits in the 16-byte buffer, so it is NUL terminated
  // before the end of that buffer.
  EXPECT_LT(WrittenLen, UINT32_C(16));
  // Every byte beyond the name and its terminator must be untouched; otherwise
  // the host copied past ChName[16] (stack over-read / disclosure).
  for (uint32_t I = WrittenLen + 1; I < NameLen; ++I) {
    EXPECT_EQ(static_cast<uint8_t>(Buf[I]), UINT8_C(0xAA));
  }
}

TEST_F(FFmpegTest, AVUtilChannelLayoutNameLowFrequency) {
  ASSERT_TRUE(AVUtilMod != nullptr);

  auto *FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_get_channel_layout_name");
  auto &HostFuncAVGetChannelLayoutName = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVGetChannelLayoutName &>(
      FuncInst->getHostFunc());

  uint32_t NamePtr = UINT32_C(4);
  // The SDK LOW_FREQUENCY id maps to AV_CH_LOW_FREQUENCY, whose channel index
  // is bit position 3; av_channel_name returns "LFE" for it. A mask>>1 maps it
  // to the wrong channel ("BL").
  uint64_t ChannelId = UINT64_C(1) << 3;
  uint32_t NameLen = UINT32_C(16);
  fillMemContent(MemInst, NamePtr, NameLen, UINT8_C(0));

  HostFuncAVGetChannelLayoutName.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{ChannelId, NamePtr, NameLen},
      Result);
  EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

  char *Buf = MemInst->getPointer<char *>(NamePtr);
  EXPECT_STREQ(Buf, "LFE");
}

TEST_F(FFmpegTest, AVUtilChannelLayoutNameStereo) {
  ASSERT_TRUE(AVUtilMod != nullptr);

  // FRONT_LEFT | FRONT_RIGHT (SDK ids 1 and 2) is the stereo layout; a
  // multi-channel layout must be named as a layout ("stereo"), not by the name
  // of its first channel alone ("FL").
  uint64_t ChannelId = UINT64_C(0x3);
  uint32_t NamePtr = UINT32_C(4);
  uint32_t NameLen = UINT32_C(16);

  auto *FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_get_channel_layout_name_len");
  auto &HostFuncAVGetChannelLayoutNameLen = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVGetChannelLayoutNameLen &>(
      FuncInst->getHostFunc());
  HostFuncAVGetChannelLayoutNameLen.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{ChannelId},
      Result);
  int32_t const ReportedLen = Result[0].get<int32_t>();

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_get_channel_layout_name");
  auto &HostFuncAVGetChannelLayoutName = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVGetChannelLayoutName &>(
      FuncInst->getHostFunc());
  fillMemContent(MemInst, NamePtr, NameLen, UINT8_C(0));
  HostFuncAVGetChannelLayoutName.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{ChannelId, NamePtr, NameLen},
      Result);
  EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

  char *Buf = MemInst->getPointer<char *>(NamePtr);
  EXPECT_STREQ(Buf, "stereo");
  // The reported length is the written name's exact length; a single-channel
  // name such as "FL" would be shorter.
  uint32_t WrittenLen = 0;
  while (WrittenLen < NameLen && Buf[WrittenLen] != '\0') {
    ++WrittenLen;
  }
  EXPECT_EQ(ReportedLen, static_cast<int32_t>(WrittenLen));
}

TEST_F(FFmpegTest, AVUtilChannelLayoutInvalidInputs) {
  ASSERT_TRUE(AVUtilMod != nullptr);

  auto *FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_get_channel_layout_nb_channels");
  auto &HostFuncNbChannels = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVGetChannelLayoutNbChannels &>(
      FuncInst->getHostFunc());

  // ChannelLayoutId 0 maps to mask 0, which av_channel_layout_from_mask
  // rejects; the host must not read or uninit the then-uninitialized layout.
  HostFuncNbChannels.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{UINT64_C(0)},
      Result);
  EXPECT_EQ(Result[0].get<int32_t>(), 0);

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_get_default_channel_layout");
  auto &HostFuncDefault = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVGetDefaultChannelLayout &>(
      FuncInst->getHostFunc());

  // 63 channels exceeds FFmpeg's largest native layout (22.2, 24 channels), so
  // av_channel_layout_default yields a non-native layout whose union must not
  // be reinterpreted as a mask.
  int32_t const ChannelCountWithoutNativeDefault = 63;
  HostFuncDefault.run(CallFrame,
                      std::initializer_list<WasmEdge::ValVariant>{
                          ChannelCountWithoutNativeDefault},
                      Result);
  EXPECT_EQ(Result[0].get<uint64_t>(), UINT64_C(0));
}

TEST_F(FFmpegTest, AVTime) {

  ASSERT_TRUE(AVUtilMod != nullptr);

  auto *FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_gettime");
  auto &HostFuncAVGetTime =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVGetTime &>(
          FuncInst->getHostFunc());

  {
    HostFuncAVGetTime.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{}, Result);

    EXPECT_TRUE(Result[0].get<int64_t>() > 0);
  }

  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_gettime_relative");
  auto &HostFuncAVGetTimeRelative =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVGetTimeRelative &>(
          FuncInst->getHostFunc());

  {
    HostFuncAVGetTimeRelative.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{}, Result);

    EXPECT_TRUE(Result[0].get<int64_t>() > 0);
  }

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_gettime_relative_is_monotonic");
  auto &HostFuncAVGetTimeRelativeIsMonotonic = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVGetTimeRelativeIsMonotonic &>(
      FuncInst->getHostFunc());

  {
    HostFuncAVGetTimeRelativeIsMonotonic.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{}, Result);

    EXPECT_EQ(Result[0].get<int32_t>(), 1);
  }

  FuncInst = AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_usleep");
  auto &HostFuncAVUSleep =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVUSleep &>(
          FuncInst->getHostFunc());

  {
    HostFuncAVUSleep.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{1000}, Result);

    EXPECT_EQ(Result[0].get<int32_t>(), 0);
  }
}

} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
