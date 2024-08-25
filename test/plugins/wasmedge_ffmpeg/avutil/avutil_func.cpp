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
