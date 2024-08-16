// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "avformat/avChapter.h"
#include "avformat/module.h"

#include "utils.h"

#include <gtest/gtest.h>

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {

// Sample Video under test has only Single Chapter.
TEST_F(FFmpegTest, AVChapter) {
  ASSERT_TRUE(AVFormatMod != nullptr);

  uint32_t ChapterIdx = 0;

  uint32_t FormatCtxPtr = UINT32_C(4);
  uint32_t NumPtr = UINT32_C(12);
  uint32_t DenPtr = UINT32_C(16);
  uint32_t DictionaryPtr = UINT32_C(20);
  uint32_t FilePtr = UINT32_C(100);

  std::string FileName = "ffmpeg-assets/sample_video.mp4"; // 32 chars
  initFormatCtx(FormatCtxPtr, FilePtr, FileName);
  uint32_t FormatCtxId = readUInt32(MemInst, FormatCtxPtr);

  auto *FuncInst =
      AVFormatMod->findFuncExports("wasmedge_ffmpeg_avformat_avChapter_id");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVChapterId =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVChapterId &>(
          FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVChapterId.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{FormatCtxId, ChapterIdx},
        Result));
    EXPECT_TRUE(Result[0].get<int64_t>() >= 0);
  }

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avChapter_timebase");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVChapterTimebase = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVChapterTimebase &>(
      FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVChapterTimebase.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{NumPtr, DenPtr, FormatCtxId,
                                                    ChapterIdx},
        Result));
    EXPECT_TRUE(Result[0].get<int32_t>() >= 0);
    EXPECT_EQ(readSInt32(MemInst, NumPtr), 1);
    EXPECT_TRUE(readSInt32(MemInst, DenPtr) >= 0);
  }

  FuncInst =
      AVFormatMod->findFuncExports("wasmedge_ffmpeg_avformat_avChapter_start");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVChapterStart =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVChapterStart &>(
          FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVChapterStart.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{FormatCtxId, ChapterIdx},
        Result));
    EXPECT_TRUE(Result[0].get<int32_t>() >= 0);
  }

  FuncInst =
      AVFormatMod->findFuncExports("wasmedge_ffmpeg_avformat_avChapter_end");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVChapterEnd =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVChapterEnd &>(
          FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVChapterEnd.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{FormatCtxId, ChapterIdx},
        Result));
    EXPECT_TRUE(Result[0].get<int32_t>() >= 0);
  }

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avChapter_metadata");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVChapterMetadata = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVChapterMetadata &>(
      FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVChapterMetadata.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{FormatCtxId, ChapterIdx,
                                                    DictionaryPtr},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
    EXPECT_TRUE(readUInt32(MemInst, DictionaryPtr) > 0);
  }

  FuncInst =
      AVFormatMod->findFuncExports("wasmedge_ffmpeg_avformat_avChapter_set_id");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVChapterSetId =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVChapterSetId &>(
          FuncInst->getHostFunc());

  {
    int64_t ChapterId = 10000;
    EXPECT_TRUE(
        HostFuncAVChapterSetId.run(CallFrame,
                                   std::initializer_list<WasmEdge::ValVariant>{
                                       FormatCtxId, ChapterIdx, ChapterId},
                                   Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

    // Verify Set Data
    EXPECT_TRUE(HostFuncAVChapterId.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{FormatCtxId, ChapterIdx},
        Result));
    EXPECT_EQ(Result[0].get<int64_t>(), ChapterId);
  }

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avChapter_set_timebase");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVChapterSetTimebase = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVChapterSetTimebase &>(
      FuncInst->getHostFunc());

  {
    int32_t Num = 3;
    int32_t Den = 4;
    EXPECT_TRUE(HostFuncAVChapterSetTimebase.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{Num, Den, FormatCtxId,
                                                    ChapterIdx},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

    // Verify Set Data
    EXPECT_TRUE(HostFuncAVChapterTimebase.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{NumPtr, DenPtr, FormatCtxId,
                                                    ChapterIdx},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
    EXPECT_EQ(readSInt32(MemInst, NumPtr), Num);
    EXPECT_EQ(readSInt32(MemInst, DenPtr), Den);
  }

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avChapter_set_start");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVChapterSetStart = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVChapterSetStart &>(
      FuncInst->getHostFunc());

  {
    int64_t StartValue = 1000;
    EXPECT_TRUE(HostFuncAVChapterSetStart.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{FormatCtxId, ChapterIdx,
                                                    StartValue},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

    // Verify Set Data
    EXPECT_TRUE(HostFuncAVChapterStart.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{FormatCtxId, ChapterIdx},
        Result));
    EXPECT_EQ(Result[0].get<int64_t>(), StartValue);
  }

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avChapter_set_end");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVChapterSetEnd =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVChapterSetEnd &>(
          FuncInst->getHostFunc());

  {
    int64_t EndValue = 99999;
    EXPECT_TRUE(
        HostFuncAVChapterSetEnd.run(CallFrame,
                                    std::initializer_list<WasmEdge::ValVariant>{
                                        FormatCtxId, ChapterIdx, EndValue},
                                    Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

    // Verify Set Data
    EXPECT_TRUE(HostFuncAVChapterEnd.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{FormatCtxId, ChapterIdx},
        Result));
    EXPECT_EQ(Result[0].get<int64_t>(), EndValue);
  }
}

} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
