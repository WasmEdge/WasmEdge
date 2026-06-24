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
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVLogSetLevel = FuncInst->getHostFunc();

  int32_t LogLvlId = 16; // AV_LOG_ERROR, distinct from the default AV_LOG_INFO
  {
    EXPECT_TRUE(HostFuncAVLogSetLevel.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{LogLvlId}, {}));
  }

  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_log_get_level");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVLogGetLevel = FuncInst->getHostFunc();

  {
    EXPECT_TRUE(HostFuncAVLogGetLevel.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{}, Result));
    EXPECT_EQ(Result[0].get<int32_t>(), LogLvlId);
  }

  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_log_set_flags");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVLogSetFlags = FuncInst->getHostFunc();

  int32_t FlagId = 1;
  {
    EXPECT_TRUE(HostFuncAVLogSetFlags.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{FlagId}, {}));
  }

  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_log_get_flags");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVLogGetFlags = FuncInst->getHostFunc();

  {
    EXPECT_TRUE(HostFuncAVLogGetFlags.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{}, Result));

    EXPECT_EQ(Result[0].get<int32_t>(), FlagId);
  }

  FuncInst = AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_rescale_q");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVRescaleQ = FuncInst->getHostFunc();

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
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVRescaleQRnd = FuncInst->getHostFunc();

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
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVUtilVersion = FuncInst->getHostFunc();

  {
    EXPECT_TRUE(HostFuncAVUtilVersion.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{}, Result));

    EXPECT_TRUE((Result[0].get<uint32_t>() >> 16) > 0);
  }

  uint64_t ChannelId = 1; // FRONT_LEFT
  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_get_channel_layout_nb_channels");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVGetChannelLayoutNbChannels = FuncInst->getHostFunc();

  {
    EXPECT_TRUE(HostFuncAVGetChannelLayoutNbChannels.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{ChannelId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), 1);
  }

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_get_default_channel_layout");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVGetDefaultChannelLayout = FuncInst->getHostFunc();

  {
    EXPECT_TRUE(HostFuncAVGetDefaultChannelLayout.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{ChannelId},
        Result));
    EXPECT_EQ(Result[0].get<uint64_t>(),
              UINT64_C(67108868)); // guest-encoded AV_CH_LAYOUT_MONO
  }

  uint32_t Length = 0;
  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_avutil_configuration_length");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVUtilConfigurationLength = FuncInst->getHostFunc();

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
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVUtilConfiguration = FuncInst->getHostFunc();

  {
    HostFuncAVUtilConfiguration.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{NamePtr, Length},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
    EXPECT_NE(std::string_view(MemInst->getPointer<char *>(NamePtr),
                               static_cast<size_t>(Length))
                  .find("--"),
              std::string_view::npos);
  }

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_avutil_license_length");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVUtilLicenseLength = FuncInst->getHostFunc();

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
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVUtilLicense = FuncInst->getHostFunc();

  {
    HostFuncAVUtilLicense.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{NamePtr, Length},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
    EXPECT_EQ(std::string_view(MemInst->getPointer<char *>(NamePtr),
                               static_cast<size_t>(Length))
                  .find("--"),
              std::string_view::npos);
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
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVGetTime = FuncInst->getHostFunc();

  int64_t AbsTime = 0;
  {
    EXPECT_TRUE(HostFuncAVGetTime.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{}, Result));

    AbsTime = Result[0].get<int64_t>();
    // Wall-clock epoch microseconds: well past 2020-01-01.
    EXPECT_GT(AbsTime, INT64_C(1577836800000000));
  }

  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_gettime_relative");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVGetTimeRelative = FuncInst->getHostFunc();

  {
    EXPECT_TRUE(HostFuncAVGetTimeRelative.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{}, Result));

    int64_t RelTime = Result[0].get<int64_t>();
    EXPECT_GT(RelTime, 0);
    // Relative monotonic clock is not epoch time; it stays smaller.
    EXPECT_LT(RelTime, AbsTime);
  }

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_gettime_relative_is_monotonic");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVGetTimeRelativeIsMonotonic = FuncInst->getHostFunc();

  {
    HostFuncAVGetTimeRelativeIsMonotonic.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{}, Result);

    EXPECT_EQ(Result[0].get<int32_t>(), 1);
  }

  FuncInst = AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_usleep");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVUSleep = FuncInst->getHostFunc();

  {
    HostFuncAVUSleep.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{1000}, Result);

    EXPECT_EQ(Result[0].get<int32_t>(), 0);
  }
}

} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
