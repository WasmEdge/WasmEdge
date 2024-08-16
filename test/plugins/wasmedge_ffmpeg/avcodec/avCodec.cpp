// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "avcodec/avCodec.h"
#include "avcodec/module.h"
#include "utils.h"

#include <gtest/gtest.h>

// Testing all AVCodecstruct

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {

TEST_F(FFmpegTest, AVCodec) {
  ASSERT_TRUE(AVCodecMod != nullptr);

  uint32_t AVCodecPtr = UINT32_C(20);
  uint32_t StringPtr = UINT32_C(68);
  uint32_t NumeratorPtr = UINT32_C(72);
  uint32_t DenominatorPtr = UINT32_C(76);
  std::string FileName = "ffmpeg-assets/sample_video.mp4"; // 32 chars
  spdlog::info("Init FFmpeg Structs"sv);
  initFFmpegStructs(AVCodecPtr, UINT32_C(24), UINT32_C(28), FileName,
                    UINT32_C(60), UINT32_C(64), UINT32_C(68), UINT32_C(72));

  uint32_t AVCodecId = readUInt32(MemInst, AVCodecPtr);
  auto *FuncInst =
      AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_avcodec_id");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecID =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecID &>(
          FuncInst->getHostFunc());

  spdlog::info("Testing AVCodecId"sv);
  {
    EXPECT_TRUE(HostFuncAVCodecID.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVCodecId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), 27); // H264
  }

  FuncInst =
      AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_avcodec_type");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecType =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecType &>(
          FuncInst->getHostFunc());

  spdlog::info("Testing AVCodecType"sv);
  {
    EXPECT_TRUE(HostFuncAVCodecType.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVCodecId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(),
              0); // MediaType is Video
  }

  FuncInst =
      AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_avcodec_max_lowres");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecMaxLowres =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecMaxLowres &>(
          FuncInst->getHostFunc());

  spdlog::info("Testing AVCodecMaxLowres"sv);
  {
    EXPECT_TRUE(HostFuncAVCodecMaxLowres.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVCodecId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), 0);
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_capabilities");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCapabilities = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCapabilities &>(
      FuncInst->getHostFunc());

  spdlog::info("Testing AVCodecCapabilities"sv);
  {
    EXPECT_TRUE(HostFuncAVCodecCapabilities.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVCodecId},
        Result));
    EXPECT_TRUE(Result[0].get<int32_t>() > 0);
  }

  int32_t Length = 0;
  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_get_name_len");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecGetNameLen = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecGetNameLen &>(
      FuncInst->getHostFunc());

  spdlog::info("Testing AVCodecGetNameLen"sv);
  {
    EXPECT_TRUE(HostFuncAVCodecGetNameLen.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVCodecId},
        Result));
    Length = Result[0].get<int32_t>();
    EXPECT_TRUE(Length > 0);
  }

  FuncInst =
      AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_avcodec_get_name");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecGetName =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecGetName &>(
          FuncInst->getHostFunc());

  // Fill the Memory with 0.
  fillMemContent(MemInst, StringPtr, Length);
  spdlog::info("Testing AVCodecGetName"sv);
  {
    EXPECT_TRUE(
        HostFuncAVCodecGetName.run(CallFrame,
                                   std::initializer_list<WasmEdge::ValVariant>{
                                       AVCodecId, StringPtr, Length},
                                   Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_get_long_name_len");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecGetLongNameLen = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecGetLongNameLen &>(
      FuncInst->getHostFunc());

  spdlog::info("Testing AVCodecGetLongNameLen"sv);
  {
    EXPECT_TRUE(HostFuncAVCodecGetLongNameLen.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVCodecId},
        Result));
    Length = Result[0].get<int32_t>();
    EXPECT_TRUE(Length > 0);
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_get_long_name");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecGetLongName = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecGetLongName &>(
      FuncInst->getHostFunc());

  spdlog::info("Testing AVCodecGetLongName"sv);
  {
    EXPECT_TRUE(HostFuncAVCodecGetLongName.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVCodecId, StringPtr,
                                                    Length},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst =
      AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_avcodec_profiles");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecProfiles =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecProfiles &>(
          FuncInst->getHostFunc());

  spdlog::info("Testing AVCodecProfiles"sv);
  {
    EXPECT_TRUE(HostFuncAVCodecProfiles.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVCodecId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), 1);
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_pix_fmts_is_null");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecPixFmtIsNull = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecPixFmtsIsNull &>(
      FuncInst->getHostFunc());

  spdlog::info("Testing AVCodecPixFmtsIsNull"sv);
  {
    EXPECT_TRUE(HostFuncAVCodecPixFmtIsNull.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVCodecId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), 1);
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_pix_fmts_iter");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecPixFmtIter = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecPixFmtsIter &>(
      FuncInst->getHostFunc());

  spdlog::info("Testing AVCodecPixFmtsIter"sv);
  {
    uint32_t Idx = 0;
    EXPECT_TRUE(HostFuncAVCodecPixFmtIter.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVCodecId, Idx},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), 0);
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_supported_framerate_is_null");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecSupportedFrameratesIsNull = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecSupportedFrameratesIsNull
          &>(FuncInst->getHostFunc());

  spdlog::info("Testing AVCodecSupportedFramratesIsNull"sv);
  {
    EXPECT_TRUE(HostFuncAVCodecSupportedFrameratesIsNull.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVCodecId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), 1);
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_supported_framerate_iter");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecSupportedFrameratesIter = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecSupportedFrameratesIter
          &>(FuncInst->getHostFunc());

  spdlog::info("Testing AVCodecSupportedFrameratesIter"sv);
  {
    EXPECT_TRUE(HostFuncAVCodecSupportedFrameratesIter.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVCodecId, 1, NumeratorPtr,
                                                    DenominatorPtr},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_supported_samplerates_is_null");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecSupportedSampleRatesIsNull = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecSupportedSampleRatesIsNull
          &>(FuncInst->getHostFunc());

  spdlog::info("Testing AVCodecSupportedSampleRatesIsNull"sv);
  {
    EXPECT_TRUE(HostFuncAVCodecSupportedSampleRatesIsNull.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVCodecId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), 1);
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_supported_samplerates_iter");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecSupportedSampleRatesIter = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecSupportedSampleRatesIter
          &>(FuncInst->getHostFunc());

  spdlog::info("Testing AVCodecSupportedSampleRatesIter"sv);
  {
    EXPECT_TRUE(HostFuncAVCodecSupportedSampleRatesIter.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVCodecId, 0},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), 0);
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_channel_layouts_is_null");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecChannelLayoutIsNull = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecChannelLayoutIsNull &>(
      FuncInst->getHostFunc());

  spdlog::info("Testing AVCodecChannelLayoutIsNull"sv);
  {
    EXPECT_TRUE(HostFuncAVCodecChannelLayoutIsNull.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVCodecId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), 1);
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_channel_layouts_iter");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecChannelLayoutIter = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecChannelLayoutIter &>(
      FuncInst->getHostFunc());

  spdlog::info("Testing AVCodecChannelLayoutIter"sv);
  {
    EXPECT_TRUE(HostFuncAVCodecChannelLayoutIter.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVCodecId, 0},
        Result));
    EXPECT_EQ(Result[0].get<int64_t>(), 0);
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_sample_fmts_is_null");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecSampleFmtsIsNull = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecSampleFmtsIsNull &>(
      FuncInst->getHostFunc());

  spdlog::info("Testing AVCodecSampleFmtsIsNull"sv);
  {
    EXPECT_TRUE(HostFuncAVCodecSampleFmtsIsNull.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVCodecId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), 1);
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_sample_fmts_iter");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecSampleFmtsIter = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecSampleFmtsIter &>(
      FuncInst->getHostFunc());

  spdlog::info("Testing AVCodecSampleFmtsIter"sv);
  {
    EXPECT_TRUE(HostFuncAVCodecSampleFmtsIter.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVCodecId, 0},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), 0);
  }
}
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
