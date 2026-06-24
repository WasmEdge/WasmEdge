// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

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
    EXPECT_TRUE(HostFuncAVFormatCtxNbChapters.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{FormatCtxId},
        Result));
    uint32_t const CurrentNbChapters = Result[0].get<uint32_t>();

    // Setting the count to its current (authoritative) value is allowed.
    EXPECT_TRUE(HostFuncAVFormatCtxSetNbChapters.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{FormatCtxId,
                                                    CurrentNbChapters},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

    // Forging a larger count must be rejected: the chapters array is not grown
    // here, so a raised nb_chapters would let chapterAt walk past the array.
    EXPECT_TRUE(HostFuncAVFormatCtxSetNbChapters.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{FormatCtxId,
                                                    CurrentNbChapters + 200},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(),
              static_cast<int32_t>(ErrNo::InternalError));

    EXPECT_TRUE(HostFuncAVFormatCtxNbChapters.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{FormatCtxId},
        Result));
    EXPECT_EQ(Result[0].get<uint32_t>(), CurrentNbChapters);
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

TEST_F(FFmpegTest, AVFormatCtxMetadataNullHandle) {
  ASSERT_TRUE(AVFormatMod != nullptr);

  auto *FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avformatContext_metadata");
  auto &HostFuncAVFormatCtxMetadata = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVFormatCtxMetadata &>(
      FuncInst->getHostFunc());

  // A guest id of 0 resolves to a null AVFormatContext; the getter must report
  // InternalError instead of dereferencing it.
  uint32_t InvalidFormatCtxId = UINT32_C(0);
  uint32_t DictPtr = UINT32_C(4);
  EXPECT_TRUE(HostFuncAVFormatCtxMetadata.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{InvalidFormatCtxId, DictPtr},
      Result));
  EXPECT_EQ(Result[0].get<int32_t>(),
            static_cast<int32_t>(ErrNo::InternalError));
}

} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
