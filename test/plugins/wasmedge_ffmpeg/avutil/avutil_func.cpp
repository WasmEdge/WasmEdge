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
