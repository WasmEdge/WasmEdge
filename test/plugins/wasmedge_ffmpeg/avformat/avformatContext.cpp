// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "avformat/avformatContext.h"
#include "avformat/module.h"

#include "utils.h"

#include <gtest/gtest.h>

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {

// Testing all AVFormat_funcs.
TEST_F(FFmpegTest, AVFormatContextStruct) {
  uint32_t FormatCtxPtr = UINT32_C(4);
  uint32_t InputFormatPtr = UINT32_C(8);
  uint32_t OutputFormatPtr = UINT32_C(12);
  uint32_t DicPtr = uint32_t(16);
  uint32_t FilePtr = UINT32_C(100);

  std::string FileName = "ffmpeg-assets/sample_video.mp4"; // 32 chars
  initFormatCtx(FormatCtxPtr, FilePtr, FileName);
  uint32_t FormatCtxId = readUInt32(MemInst, FormatCtxPtr);

  auto *FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avformatContext_iformat");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFormatCtxIFormat = FuncInst->getHostFunc();

  {
    EXPECT_TRUE(HostFuncAVFormatCtxIFormat.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{FormatCtxId,
                                                    InputFormatPtr},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
    EXPECT_TRUE(readUInt32(MemInst, InputFormatPtr) > 0);
  }

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avformatContext_oformat");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFormatCtxOFormat = FuncInst->getHostFunc();

  {
    EXPECT_TRUE(HostFuncAVFormatCtxOFormat.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{FormatCtxId,
                                                    OutputFormatPtr},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
    EXPECT_TRUE(readUInt32(MemInst, InputFormatPtr) > 0);
  }

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avformatContext_probescope");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFormatCtxProbeScore = FuncInst->getHostFunc();

  {
    EXPECT_TRUE(HostFuncAVFormatCtxProbeScore.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{FormatCtxId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), 100);
  }

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avformatContext_nb_streams");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFormatCtxNbStreams = FuncInst->getHostFunc();

  {
    EXPECT_TRUE(HostFuncAVFormatCtxNbStreams.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{FormatCtxId},
        Result));
    EXPECT_TRUE(Result[0].get<int32_t>() > 0);
  }

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avformatContext_duration");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFormatCtxDuration = FuncInst->getHostFunc();

  {
    EXPECT_TRUE(HostFuncAVFormatCtxDuration.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{FormatCtxId},
        Result));
    EXPECT_EQ(Result[0].get<int64_t>(), AV_NOPTS_VALUE);
  }

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avformatContext_bit_rate");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFormatCtxBitRate = FuncInst->getHostFunc();

  {
    EXPECT_TRUE(HostFuncAVFormatCtxBitRate.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{FormatCtxId},
        Result));
    EXPECT_EQ(Result[0].get<uint32_t>(), 0);
  }

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avformatContext_set_nb_chapters");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFormatCtxSetNbChapters = FuncInst->getHostFunc();

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avformatContext_nb_chapters");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFormatCtxNbChapters = FuncInst->getHostFunc();
  {
    uint32_t NbChapters = 200;
    EXPECT_TRUE(HostFuncAVFormatCtxSetNbChapters.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{FormatCtxId, NbChapters},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

    EXPECT_TRUE(HostFuncAVFormatCtxNbChapters.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{FormatCtxId},
        Result));
    EXPECT_EQ(Result[0].get<uint32_t>(), NbChapters);
  }

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avformatContext_metadata");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFormatCtxMetadata = FuncInst->getHostFunc();

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avformatContext_set_metadata");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFormatCtxSetMetadata = FuncInst->getHostFunc();

  {
    EXPECT_TRUE(HostFuncAVFormatCtxMetadata.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{FormatCtxId, DicPtr},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
    EXPECT_TRUE(readUInt32(MemInst, DicPtr) > 0);

    uint32_t DictId = readUInt32(MemInst, DicPtr);
    EXPECT_TRUE(HostFuncAVFormatCtxSetMetadata.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{FormatCtxId, DictId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }
}

TEST_F(FFmpegTest, AVFormatNewStream) {
  ASSERT_TRUE(AVFormatMod != nullptr);
  ASSERT_TRUE(AVCodecMod != nullptr);

  uint32_t FormatCtxPtr = UINT32_C(4);
  uint32_t CodecEncoderPtr = UINT32_C(8);
  uint32_t FilePtr = UINT32_C(100);
  uint32_t CodecNamePtr = UINT32_C(150);

  std::string FileName = "ffmpeg-assets/sample_video.mp4";
  initFormatCtx(FormatCtxPtr, FilePtr, FileName);
  uint32_t FormatCtxId = readUInt32(MemInst, FormatCtxPtr);
  ASSERT_TRUE(FormatCtxId > 0);

  auto *FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avformat_find_stream_info");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  EXPECT_TRUE(FuncInst->getHostFunc().run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{FormatCtxId, UINT32_C(0)},
      Result));
  EXPECT_TRUE(Result[0].get<int32_t>() >= 0);

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avformatContext_nb_streams");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFormatCtxNbStreams = FuncInst->getHostFunc();

  EXPECT_TRUE(HostFuncAVFormatCtxNbStreams.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{FormatCtxId},
      Result));
  const int32_t NbStreamsBefore = Result[0].get<int32_t>();
  ASSERT_GT(NbStreamsBefore, 0);

  std::string CodecName = "mpeg1video";
  fillMemContent(MemInst, CodecNamePtr, CodecName);
  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_find_encoder_by_name");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  EXPECT_TRUE(FuncInst->getHostFunc().run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          CodecEncoderPtr, CodecNamePtr,
          static_cast<uint32_t>(CodecName.length())},
      Result));
  EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  uint32_t AVCodecEncoderId = readUInt32(MemInst, CodecEncoderPtr);
  ASSERT_TRUE(AVCodecEncoderId > 0);

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avformat_new_stream");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFormatNewStream = FuncInst->getHostFunc();

  spdlog::info("Testing AVFormatNewStream"sv);
  EXPECT_TRUE(
      HostFuncAVFormatNewStream.run(CallFrame,
                                    std::initializer_list<WasmEdge::ValVariant>{
                                        FormatCtxId, AVCodecEncoderId},
                                    Result));
  EXPECT_EQ(Result[0].get<int32_t>(), 1);

  EXPECT_TRUE(HostFuncAVFormatCtxNbStreams.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{FormatCtxId},
      Result));
  EXPECT_EQ(Result[0].get<int32_t>(), NbStreamsBefore + 1);

  // Passing a null codec is valid and FFmpeg still creates a stream.
  EXPECT_TRUE(HostFuncAVFormatNewStream.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{FormatCtxId, UINT32_C(0)},
      Result));
  EXPECT_EQ(Result[0].get<int32_t>(), 1);
  EXPECT_TRUE(HostFuncAVFormatCtxNbStreams.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{FormatCtxId},
      Result));
  EXPECT_EQ(Result[0].get<int32_t>(), NbStreamsBefore + 2);
}

} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
