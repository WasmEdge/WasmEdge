#include "avformat/avChapter.h"
#include "../utils.h"
#include "avformat/module.h"

#include <gtest/gtest.h>

using WasmEdge::Host::WasmEdgeFFmpeg::ErrNo;

// Sample Video under test has only Single Chapter.
TEST(WasmEdgeAVFormatTest, AVChapter) {

  auto *AVFormatMod = TestUtils::InitModules::createAVFormatModule();
  ASSERT_TRUE(AVFormatMod != nullptr);

  // Create the calling frame with memory instance.
  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_TRUE(MemInstPtr != nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);

  std::array<WasmEdge::ValVariant, 1> Result = {UINT32_C(0)};

  auto *FuncInst =
      AVFormatMod->findFuncExports("wasmedge_ffmpeg_avformat_avChapter_id");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVChapterId =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVChapterId &>(
          FuncInst->getHostFunc());

  uint32_t AvFormatCtxPtr = UINT32_C(1);
  const uint32_t ChapterIdx = UINT32_C(0);
  TestUtils::AVFormatContext::initFormatCtx(Mod, AvFormatCtxPtr, Result);

  uint32_t AvFormatCtxId = readUInt32(MemInst, AvFormatCtxPtr);
  {
    EXPECT_TRUE(HostFuncAVChapterId.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AvFormatCtxId, ChapterIdx},
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
    uint32_t NumPtr = UINT32_C(40);
    uint32_t DenPtr = UINT32_C(44);
    EXPECT_TRUE(HostFuncAVChapterTimebase.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{NumPtr, DenPtr,
                                                    AvFormatCtxId, ChapterIdx},
        Result));
    EXPECT_TRUE(Result[0].get<int32_t>() >= 0);
    EXPECT_EQ(readIInt32(MemInst, NumPtr), 1);
    EXPECT_TRUE(readIInt32(MemInst, DenPtr) >= 0);
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
        std::initializer_list<WasmEdge::ValVariant>{AvFormatCtxId, ChapterIdx},
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
        std::initializer_list<WasmEdge::ValVariant>{AvFormatCtxId, ChapterIdx},
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
    uint32_t AVDictionaryPtr = UINT32_C(56);
    EXPECT_TRUE(HostFuncAVChapterMetadata.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AvFormatCtxId,
                                                    AVDictionaryPtr},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
    EXPECT_TRUE(readUInt32(MemInst, AVDictionaryPtr) > 0);
  }

  FuncInst =
      AVFormatMod->findFuncExports("wasmedge_ffmpeg_avformat_avChapter_set_id");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVChapterSetId =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVChapterSetId &>(
          FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVChapterSetId.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AvFormatCtxId, ChapterIdx,
                                                    INT64_C(10000)},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

    // Verify Set Data
    EXPECT_TRUE(HostFuncAVChapterId.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AvFormatCtxId, ChapterIdx},
        Result));
    EXPECT_EQ(Result[0].get<int64_t>(), 10000);
  }

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avChapter_set_timebase");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVChapterSetTimebase = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVChapterSetTimebase &>(
      FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVChapterSetTimebase.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{INT32_C(3), INT32_C(4),
                                                    AvFormatCtxId, ChapterIdx},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

    // Verify Set Data
    uint32_t NumPtr = UINT32_C(80);
    uint32_t DenPtr = UINT32_C(84);
    EXPECT_TRUE(HostFuncAVChapterTimebase.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{NumPtr, DenPtr,
                                                    AvFormatCtxId, ChapterIdx},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
    EXPECT_EQ(readIInt32(MemInst, NumPtr), 3);
    EXPECT_EQ(readIInt32(MemInst, DenPtr), 4);
  }

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avChapter_set_start");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVChapterSetStart = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVChapterSetStart &>(
      FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVChapterSetStart.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AvFormatCtxId, ChapterIdx,
                                                    INT64_C(1000)},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

    // Verify Set Data
    EXPECT_TRUE(HostFuncAVChapterStart.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AvFormatCtxId, ChapterIdx},
        Result));
    EXPECT_EQ(Result[0].get<int64_t>(), 1000);
  }

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avChapter_set_end");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVChapterSetEnd =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVChapterSetEnd &>(
          FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVChapterSetEnd.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AvFormatCtxId, ChapterIdx,
                                                    INT64_C(99999)},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

    // Verify Set Data
    EXPECT_TRUE(HostFuncAVChapterEnd.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AvFormatCtxId, ChapterIdx},
        Result));
    EXPECT_EQ(Result[0].get<int64_t>(), 99999);
  }
}