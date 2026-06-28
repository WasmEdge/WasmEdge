// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "avcodec/avCodec.h"
#include "avcodec/avcodec_func.h"
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
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecID = FuncInst->getHostFunc();

  spdlog::info("Testing AVCodecId"sv);
  {
    EXPECT_TRUE(HostFuncAVCodecID.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVCodecId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), 27); // H264
  }

  FuncInst =
      AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_avcodec_type");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecType = FuncInst->getHostFunc();

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
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecMaxLowres = FuncInst->getHostFunc();

  spdlog::info("Testing AVCodecMaxLowres"sv);
  {
    EXPECT_TRUE(HostFuncAVCodecMaxLowres.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVCodecId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), 0);
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_capabilities");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCapabilities = FuncInst->getHostFunc();

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
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecGetNameLen = FuncInst->getHostFunc();

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
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecGetName = FuncInst->getHostFunc();

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
    EXPECT_EQ(std::string_view(MemInst->getPointer<char *>(StringPtr),
                               static_cast<size_t>(Length)),
              "h264"sv);
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_get_long_name_len");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecGetLongNameLen = FuncInst->getHostFunc();

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
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecGetLongName = FuncInst->getHostFunc();

  spdlog::info("Testing AVCodecGetLongName"sv);
  {
    EXPECT_TRUE(HostFuncAVCodecGetLongName.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVCodecId, StringPtr,
                                                    Length},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
    EXPECT_EQ(std::string_view(MemInst->getPointer<char *>(StringPtr),
                               static_cast<size_t>(Length)),
              "H.264 / AVC / MPEG-4 AVC / MPEG-4 part 10"sv);
  }

  FuncInst =
      AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_avcodec_profiles");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecProfiles = FuncInst->getHostFunc();

  spdlog::info("Testing AVCodecProfiles"sv);
  {
    EXPECT_TRUE(HostFuncAVCodecProfiles.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVCodecId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), 1);
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_pix_fmts_is_null");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecPixFmtIsNull = FuncInst->getHostFunc();

  spdlog::info("Testing AVCodecPixFmtsIsNull"sv);
  {
    EXPECT_TRUE(HostFuncAVCodecPixFmtIsNull.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVCodecId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), 1);
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_pix_fmts_iter");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecPixFmtIter = FuncInst->getHostFunc();

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
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecSupportedFrameratesIsNull = FuncInst->getHostFunc();

  spdlog::info("Testing AVCodecSupportedFramratesIsNull"sv);
  {
    EXPECT_TRUE(HostFuncAVCodecSupportedFrameratesIsNull.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVCodecId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), 1);
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_supported_framerate_iter");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecSupportedFrameratesIter = FuncInst->getHostFunc();

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
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecSupportedSampleRatesIsNull = FuncInst->getHostFunc();

  spdlog::info("Testing AVCodecSupportedSampleRatesIsNull"sv);
  {
    EXPECT_TRUE(HostFuncAVCodecSupportedSampleRatesIsNull.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVCodecId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), 1);
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_supported_samplerates_iter");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecSupportedSampleRatesIter = FuncInst->getHostFunc();

  spdlog::info("Testing AVCodecSupportedSampleRatesIter"sv);
  {
    EXPECT_TRUE(HostFuncAVCodecSupportedSampleRatesIter.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVCodecId, 0},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), 0);
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_channel_layouts_is_null");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecChannelLayoutIsNull = FuncInst->getHostFunc();

  spdlog::info("Testing AVCodecChannelLayoutIsNull"sv);
  {
    EXPECT_TRUE(HostFuncAVCodecChannelLayoutIsNull.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVCodecId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), 1);
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_channel_layouts_iter");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecChannelLayoutIter = FuncInst->getHostFunc();

  spdlog::info("Testing AVCodecChannelLayoutIter"sv);
  {
    EXPECT_TRUE(HostFuncAVCodecChannelLayoutIter.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVCodecId, 0},
        Result));
    EXPECT_EQ(Result[0].get<int64_t>(), 0);
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_sample_fmts_is_null");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecSampleFmtsIsNull = FuncInst->getHostFunc();

  spdlog::info("Testing AVCodecSampleFmtsIsNull"sv);
  {
    EXPECT_TRUE(HostFuncAVCodecSampleFmtsIsNull.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVCodecId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), 1);
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_sample_fmts_iter");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecSampleFmtsIter = FuncInst->getHostFunc();

  spdlog::info("Testing AVCodecSampleFmtsIter"sv);
  {
    EXPECT_TRUE(HostFuncAVCodecSampleFmtsIter.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVCodecId, 0},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), 0);
  }
}

TEST_F(FFmpegTest, AVCodecChannelLayoutIterBounds) {
  ASSERT_TRUE(AVCodecMod != nullptr);

  // ac3 is an audio encoder that publishes a zero-terminated ch_layouts array.
  std::string CodecName = "ac3";
  uint32_t CodecPtr = UINT32_C(20);
  uint32_t NamePtr = UINT32_C(40);
  fillMemContent(MemInst, NamePtr, CodecName);

  auto *FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_find_encoder_by_name");
  auto &HostFuncFindEncoder = FuncInst->getHostFunc();
  HostFuncFindEncoder.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          CodecPtr, NamePtr, static_cast<uint32_t>(CodecName.length())},
      Result);
  ASSERT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  uint32_t CodecId = readUInt32(MemInst, CodecPtr);
  if (CodecId == 0) {
    GTEST_SKIP() << "encoder \"" << CodecName
                 << "\" not available in this FFmpeg build";
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_channel_layouts_is_null");
  auto &HostFuncIsNull = FuncInst->getHostFunc();
  HostFuncIsNull.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{CodecId}, Result);
  ASSERT_EQ(Result[0].get<int32_t>(), 0);

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_channel_layouts_iter");
  auto &HostFuncIter = FuncInst->getHostFunc();

  // A valid in-range index returns a non-zero converted layout id, exercising
  // the intoChannelLayoutID(const AVChannelLayout &) conversion path (ac3's
  // first ch_layouts entry is a real native layout). The bounds clamp below
  // returns the same 0 the conversion would for the terminator, so without this
  // a broken conversion that always returned 0 would go unnoticed.
  HostFuncIter.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{CodecId, UINT32_C(0)},
      Result);
  EXPECT_GT(Result[0].get<uint64_t>(), UINT64_C(0));

  // An index far past the zero-terminated ch_layouts array must clamp to 0
  // instead of reading past the array bounds.
  HostFuncIter.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{CodecId, UINT32_C(100000)},
      Result);
  EXPECT_EQ(Result[0].get<uint64_t>(), UINT64_C(0));
}

TEST_F(FFmpegTest, AVCodecPixFmtsIterBounds) {
  ASSERT_TRUE(AVCodecMod != nullptr);

  // mpeg4 is a video encoder that publishes a NONE-terminated pix_fmts array.
  std::string CodecName = "mpeg4";
  uint32_t CodecPtr = UINT32_C(20);
  uint32_t NamePtr = UINT32_C(40);
  fillMemContent(MemInst, NamePtr, CodecName);

  auto *FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_find_encoder_by_name");
  auto &HostFuncFindEncoder = FuncInst->getHostFunc();
  HostFuncFindEncoder.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          CodecPtr, NamePtr, static_cast<uint32_t>(CodecName.length())},
      Result);
  ASSERT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  uint32_t CodecId = readUInt32(MemInst, CodecPtr);
  if (CodecId == 0) {
    GTEST_SKIP() << "encoder \"" << CodecName
                 << "\" not available in this FFmpeg build";
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_pix_fmts_is_null");
  auto &HostFuncIsNull = FuncInst->getHostFunc();
  HostFuncIsNull.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{CodecId}, Result);
  ASSERT_EQ(Result[0].get<int32_t>(), 0);

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_pix_fmts_iter");
  auto &HostFuncIter = FuncInst->getHostFunc();

  // An index far past the NONE-terminated pix_fmts array must clamp to 0
  // instead of reading past the array bounds.
  HostFuncIter.run(CallFrame,
                   std::initializer_list<WasmEdge::ValVariant>{
                       CodecId, UINT32_C(0xFFFFFFFF)},
                   Result);
  EXPECT_EQ(Result[0].get<uint32_t>(), UINT32_C(0));
}

TEST_F(FFmpegTest, AVCodecSupportedFrameratesIterBounds) {
  ASSERT_TRUE(AVCodecMod != nullptr);

  // mpeg1video publishes a {0,0}-terminated supported_framerates array.
  std::string CodecName = "mpeg1video";
  uint32_t CodecPtr = UINT32_C(20);
  uint32_t NamePtr = UINT32_C(40);
  fillMemContent(MemInst, NamePtr, CodecName);

  auto *FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_find_encoder_by_name");
  auto &HostFuncFindEncoder = FuncInst->getHostFunc();
  HostFuncFindEncoder.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          CodecPtr, NamePtr, static_cast<uint32_t>(CodecName.length())},
      Result);
  ASSERT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  uint32_t CodecId = readUInt32(MemInst, CodecPtr);
  if (CodecId == 0) {
    GTEST_SKIP() << "encoder \"" << CodecName
                 << "\" not available in this FFmpeg build";
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_supported_framerate_is_null");
  auto &HostFuncIsNull = FuncInst->getHostFunc();
  HostFuncIsNull.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{CodecId}, Result);
  ASSERT_EQ(Result[0].get<int32_t>(), 0);

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_supported_framerate_iter");
  auto &HostFuncIter = FuncInst->getHostFunc();

  uint32_t NumPtr = UINT32_C(60);
  uint32_t DenPtr = UINT32_C(64);
  // Poison the output slots first. wasm linear memory is zero-initialised, so
  // without a nonzero sentinel the {0,0} assertions below would pass even if
  // the host wrote nothing; the clamp must actively overwrite these.
  fillMemContent(MemInst, NumPtr, sizeof(int32_t), UINT8_C(0xAA));
  fillMemContent(MemInst, DenPtr, sizeof(int32_t), UINT8_C(0xAA));
  // An index far past the {0,0}-terminated array must clamp to {0,0} instead
  // of reading past the array bounds.
  HostFuncIter.run(CallFrame,
                   std::initializer_list<WasmEdge::ValVariant>{
                       CodecId, UINT32_C(100000), NumPtr, DenPtr},
                   Result);
  EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  EXPECT_EQ(readUInt32(MemInst, NumPtr), UINT32_C(0));
  EXPECT_EQ(readUInt32(MemInst, DenPtr), UINT32_C(0));
}

TEST_F(FFmpegTest, AVCodecSupportedSampleRatesIterBounds) {
  ASSERT_TRUE(AVCodecMod != nullptr);

  // ac3 publishes a 0-terminated supported_samplerates array.
  std::string CodecName = "ac3";
  uint32_t CodecPtr = UINT32_C(20);
  uint32_t NamePtr = UINT32_C(40);
  fillMemContent(MemInst, NamePtr, CodecName);

  auto *FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_find_encoder_by_name");
  auto &HostFuncFindEncoder = FuncInst->getHostFunc();
  HostFuncFindEncoder.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          CodecPtr, NamePtr, static_cast<uint32_t>(CodecName.length())},
      Result);
  ASSERT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  uint32_t CodecId = readUInt32(MemInst, CodecPtr);
  if (CodecId == 0) {
    GTEST_SKIP() << "encoder \"" << CodecName
                 << "\" not available in this FFmpeg build";
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_supported_samplerates_is_null");
  auto &HostFuncIsNull = FuncInst->getHostFunc();
  HostFuncIsNull.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{CodecId}, Result);
  ASSERT_EQ(Result[0].get<int32_t>(), 0);

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_supported_samplerates_iter");
  auto &HostFuncIter = FuncInst->getHostFunc();

  // An index far past the 0-terminated array must clamp to 0 instead of
  // reading past the array bounds.
  HostFuncIter.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{CodecId, UINT32_C(100000)},
      Result);
  EXPECT_EQ(Result[0].get<int32_t>(), 0);
}

TEST_F(FFmpegTest, AVCodecSampleFmtsIterBounds) {
  ASSERT_TRUE(AVCodecMod != nullptr);

  // ac3 publishes a NONE-terminated sample_fmts array.
  std::string CodecName = "ac3";
  uint32_t CodecPtr = UINT32_C(20);
  uint32_t NamePtr = UINT32_C(40);
  fillMemContent(MemInst, NamePtr, CodecName);

  auto *FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_find_encoder_by_name");
  auto &HostFuncFindEncoder = FuncInst->getHostFunc();
  HostFuncFindEncoder.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          CodecPtr, NamePtr, static_cast<uint32_t>(CodecName.length())},
      Result);
  ASSERT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  uint32_t CodecId = readUInt32(MemInst, CodecPtr);
  if (CodecId == 0) {
    GTEST_SKIP() << "encoder \"" << CodecName
                 << "\" not available in this FFmpeg build";
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_sample_fmts_is_null");
  auto &HostFuncIsNull = FuncInst->getHostFunc();
  HostFuncIsNull.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{CodecId}, Result);
  ASSERT_EQ(Result[0].get<int32_t>(), 0);

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_sample_fmts_iter");
  auto &HostFuncIter = FuncInst->getHostFunc();

  // An index far past the NONE-terminated sample_fmts array must clamp to 0
  // instead of reading past the array bounds.
  HostFuncIter.run(CallFrame,
                   std::initializer_list<WasmEdge::ValVariant>{
                       CodecId, UINT32_C(0xFFFFFFFF)},
                   Result);
  EXPECT_EQ(Result[0].get<uint32_t>(), UINT32_C(0));
}

TEST_F(FFmpegTest, AVCodecGetNameBounds) {
  ASSERT_TRUE(AVCodecMod != nullptr);

  std::string CodecName = "mpeg4";
  uint32_t CodecPtr = UINT32_C(20);
  uint32_t NamePtr = UINT32_C(40);
  fillMemContent(MemInst, NamePtr, CodecName);

  auto *FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_find_encoder_by_name");
  auto &HostFuncFindEncoder = FuncInst->getHostFunc();
  HostFuncFindEncoder.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          CodecPtr, NamePtr, static_cast<uint32_t>(CodecName.length())},
      Result);
  ASSERT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  uint32_t CodecId = readUInt32(MemInst, CodecPtr);
  if (CodecId == 0) {
    GTEST_SKIP() << "encoder \"" << CodecName
                 << "\" not available in this FFmpeg build";
  }

  uint32_t StrPtr = UINT32_C(200);

  // The guest buffer is larger than the host name and fenced with 0xAA; the
  // host must copy only the name plus its terminator and never read past the
  // host string into the rest of the guest buffer.
  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_get_name_len");
  auto &HostFuncNameLen = FuncInst->getHostFunc();
  HostFuncNameLen.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{CodecId}, Result);
  uint32_t NameLen = Result[0].get<int32_t>();
  ASSERT_TRUE(NameLen > 0);

  FuncInst =
      AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_avcodec_get_name");
  auto &HostFuncGetName = FuncInst->getHostFunc();
  uint32_t NameBufLen = NameLen + UINT32_C(32);
  fillMemContent(MemInst, StrPtr, NameBufLen, UINT8_C(0xAA));
  HostFuncGetName.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{CodecId, StrPtr, NameBufLen},
      Result);
  EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  {
    char *Buf = MemInst->getPointer<char *>(StrPtr);
    for (uint32_t I = NameLen + 1; I < NameBufLen; ++I) {
      EXPECT_EQ(static_cast<uint8_t>(Buf[I]), UINT8_C(0xAA));
    }
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_get_long_name_len");
  auto &HostFuncLongNameLen = FuncInst->getHostFunc();
  HostFuncLongNameLen.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{CodecId}, Result);
  uint32_t LongNameLen = Result[0].get<int32_t>();
  ASSERT_TRUE(LongNameLen > 0);

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_get_long_name");
  auto &HostFuncGetLongName = FuncInst->getHostFunc();
  uint32_t LongNameBufLen = LongNameLen + UINT32_C(32);
  fillMemContent(MemInst, StrPtr, LongNameBufLen, UINT8_C(0xAA));
  HostFuncGetLongName.run(CallFrame,
                          std::initializer_list<WasmEdge::ValVariant>{
                              CodecId, StrPtr, LongNameBufLen},
                          Result);
  EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  {
    char *Buf = MemInst->getPointer<char *>(StrPtr);
    for (uint32_t I = LongNameLen + 1; I < LongNameBufLen; ++I) {
      EXPECT_EQ(static_cast<uint8_t>(Buf[I]), UINT8_C(0xAA));
    }
  }
}
TEST_F(FFmpegTest, AVCodecFindEncoderByNameBounds) {
  ASSERT_TRUE(AVCodecMod != nullptr);

  auto *FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_find_encoder_by_name");
  auto &HostFuncFindEncoder = FuncInst->getHostFunc();

  // CodecPtr is in bounds but the guest-declared name length runs off the end
  // of linear memory; the host must reject it, not read past the page.
  uint32_t CodecPtr = UINT32_C(20);
  uint32_t OutOfBoundsNamePtr = UINT32_C(65000);
  uint32_t OutOfBoundsNameLen = UINT32_C(2000);
  HostFuncFindEncoder.run(CallFrame,
                          std::initializer_list<WasmEdge::ValVariant>{
                              CodecPtr, OutOfBoundsNamePtr, OutOfBoundsNameLen},
                          Result);
  EXPECT_EQ(Result[0].get<int32_t>(),
            static_cast<int32_t>(ErrNo::MissingMemory));
}

TEST_F(FFmpegTest, AVCodecGetNameLengthContract) {
  ASSERT_TRUE(AVCodecMod != nullptr);

  std::string CodecName = "mpeg4";
  uint32_t CodecPtr = UINT32_C(20);
  uint32_t NamePtr = UINT32_C(40);
  uint32_t StrPtr = UINT32_C(200);
  fillMemContent(MemInst, NamePtr, CodecName);

  auto *FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_find_encoder_by_name");
  auto &HostFuncFindEncoder = FuncInst->getHostFunc();
  HostFuncFindEncoder.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          CodecPtr, NamePtr, static_cast<uint32_t>(CodecName.length())},
      Result);
  ASSERT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  uint32_t CodecId = readUInt32(MemInst, CodecPtr);
  if (CodecId == 0) {
    GTEST_SKIP() << "encoder \"" << CodecName
                 << "\" not available in this FFmpeg build";
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_get_name_len");
  auto &HostFuncNameLen = FuncInst->getHostFunc();
  HostFuncNameLen.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{CodecId}, Result);
  uint32_t NameLen = Result[0].get<int32_t>();
  EXPECT_EQ(NameLen, CodecName.length());

  // The length getter reports the exact string length, so a buffer of exactly
  // that size receives the full name without a terminator and the fence beyond
  // it stays untouched.
  FuncInst =
      AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_avcodec_get_name");
  auto &HostFuncGetName = FuncInst->getHostFunc();
  uint32_t FenceLen = NameLen + UINT32_C(8);
  fillMemContent(MemInst, StrPtr, FenceLen, UINT8_C(0xAA));
  HostFuncGetName.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{CodecId, StrPtr, NameLen},
      Result);
  EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

  char *Buf = MemInst->getPointer<char *>(StrPtr);
  EXPECT_EQ(std::string(Buf, NameLen), CodecName);
  for (uint32_t I = NameLen; I < FenceLen; ++I) {
    EXPECT_EQ(static_cast<uint8_t>(Buf[I]), UINT8_C(0xAA));
  }

  // A larger buffer additionally receives the terminator right after the name,
  // and nothing past it.
  fillMemContent(MemInst, StrPtr, FenceLen, UINT8_C(0xAA));
  HostFuncGetName.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{CodecId, StrPtr, FenceLen},
      Result);
  EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  EXPECT_EQ(std::string(Buf, NameLen), CodecName);
  EXPECT_EQ(Buf[NameLen], '\0');
  for (uint32_t I = NameLen + 1; I < FenceLen; ++I) {
    EXPECT_EQ(static_cast<uint8_t>(Buf[I]), UINT8_C(0xAA));
  }

  // A buffer smaller than the name receives only the bytes that fit.
  fillMemContent(MemInst, StrPtr, FenceLen, UINT8_C(0xAA));
  HostFuncGetName.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{CodecId, StrPtr, UINT32_C(3)},
      Result);
  EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  EXPECT_EQ(std::string(Buf, 3), CodecName.substr(0, 3));
  for (uint32_t I = UINT32_C(3); I < FenceLen; ++I) {
    EXPECT_EQ(static_cast<uint8_t>(Buf[I]), UINT8_C(0xAA));
  }
}
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
