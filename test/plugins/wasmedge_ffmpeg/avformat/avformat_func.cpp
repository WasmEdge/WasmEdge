#include "avformat/avformat_func.h"
#include "../utils.h"
#include "avformat/module.h"

#include <gtest/gtest.h>

using WasmEdge::Host::WasmEdgeFFmpeg::ErrNo;

// Testing all AVFormat_funcs.
TEST(WasmEdgeAVFormatTest, AVFormatFunc) {

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

  auto *FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avformat_open_input");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFormatOpenInput = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVFormatOpenInput &>(
      FuncInst->getHostFunc());

  uint32_t AvFormatCtxPtr = UINT32_C(1);
  fillMemContent(MemInst, 1, 50);

  std::string Url = std::string("ffmpeg-assets/sample_video.mp4");
  fillMemContent(MemInst, 1, Url);

  {
    EXPECT_TRUE(HostFuncAVFormatOpenInput.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AvFormatCtxPtr, UINT32_C(1),
                                                    UINT32_C(30), UINT32_C(0),
                                                    UINT32_C(0)},
        Result));
    EXPECT_TRUE(Result[0].get<int32_t>() >= 0);
    EXPECT_TRUE(readUInt32(MemInst, AvFormatCtxPtr) > 0);

    // AVDictionaryId
    //    EXPECT_TRUE(HostFuncAVFormatOpenInput.run(
    //        CallFrame,
    //        std::initializer_list<WasmEdge::ValVariant>{
    //            AvFormatCtxPtr, INT32_C(1), UINT32_C(30), UINT32_C(0),
    //            UINT32_C(0)},
    //        Result));
    //    EXPECT_EQ(Result[0].get<int32_t>() >= 0);
    //    EXPECT_TRUE(readUInt32(MemInst, AvFormatCtxPtr) > 0);
    //
    // AVInputFormat
    //    EXPECT_TRUE(HostFuncAVFormatOpenInput.run(
    //        CallFrame,
    //        std::initializer_list<WasmEdge::ValVariant>{
    //            AvFormatCtxPtr, INT32_C(1), UINT32_C(30), UINT32_C(0),
    //            UINT32_C(0)},
    //        Result));
    //    EXPECT_EQ(Result[0].get<int32_t>() >= 0);
    //    EXPECT_TRUE(readUInt32(MemInst, AvFormatCtxPtr) > 0);
    //
    // AVInputFormat && AVDictionary
    //    EXPECT_TRUE(HostFuncAVFormatOpenInput.run(
    //        CallFrame,
    //        std::initializer_list<WasmEdge::ValVariant>{
    //        AvFormatCtxPtr, INT32_C(1), UINT32_C(30), UINT32_C(0),
    //        UINT32_C(0)}, Result));
    //    EXPECT_EQ(Result[0].get<int32_t>() >= 0);
    //    EXPECT_TRUE(readUInt32(MemInst, AvFormatCtxPtr) > 0);
  }

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avformat_find_stream_info");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFormatFindStreamInfo = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVFormatFindStreamInfo &>(
      FuncInst->getHostFunc());

  {
    uint32_t AvFormatCtxId = readUInt32(MemInst, AvFormatCtxPtr);
    EXPECT_TRUE(HostFuncAVFormatFindStreamInfo.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AvFormatCtxId, UINT32_C(0)},
        Result));
    EXPECT_TRUE(Result[0].get<int32_t>() >= 0);

    // AVDictionaryID
    //    EXPECT_TRUE(HostFuncAVFormatFindStreamInfo.run(
    //        CallFrame,
    //        std::initializer_list<WasmEdge::ValVariant>{AvFormatCtxPtr,
    //        UINT32_C(0)}, Result));
    //    EXPECT_EQ(Result[0].get<int32_t>() >= 0);
    //    EXPECT_TRUE(readUInt32(MemInst, AvFormatCtxPtr) > 0);
  }

  FuncInst =
      AVFormatMod->findFuncExports("wasmedge_ffmpeg_avformat_av_read_play");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFormatAVReadPlay =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVReadPlay &>(
          FuncInst->getHostFunc());

  {
    // Try a network Fetch.
    uint32_t AvFormatCtxId = readUInt32(MemInst, AvFormatCtxPtr);
    EXPECT_TRUE(HostFuncAVFormatAVReadPlay.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AvFormatCtxId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), -78);
  }

  FuncInst =
      AVFormatMod->findFuncExports("wasmedge_ffmpeg_avformat_av_read_pause");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFormatAVReadPause =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVReadPause &>(
          FuncInst->getHostFunc());
  {
    // Try a network Fetch.
    uint32_t AvFormatCtxId = readUInt32(MemInst, AvFormatCtxPtr);
    EXPECT_TRUE(HostFuncAVFormatAVReadPause.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AvFormatCtxId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), -78);
  }

  FuncInst =
      AVFormatMod->findFuncExports("wasmedge_ffmpeg_avformat_av_dump_format");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFormatAVDumpFormat =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVDumpFormat &>(
          FuncInst->getHostFunc());
  {
    // Try a network Fetch.
    uint32_t AvFormatCtxId = readUInt32(MemInst, AvFormatCtxPtr);
    EXPECT_TRUE(HostFuncAVFormatAVDumpFormat.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            AvFormatCtxId, 0, UINT32_C(1), UINT32_C(30), 0},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), 0);
  }

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avformat_seek_file");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFormatSeekFile = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVFormatSeekFile &>(
      FuncInst->getHostFunc());
  {
    // Try a network Fetch.
    uint32_t AvFormatCtxId = readUInt32(MemInst, AvFormatCtxPtr);
    EXPECT_TRUE(HostFuncAVFormatSeekFile.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            AvFormatCtxId, -1, INT64_C(-9223372036854775), 0,
            INT64_C(9223372036854775), 0},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), 0);
  }

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_av_find_best_stream");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFindBestStream = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVFindBestStream &>(
      FuncInst->getHostFunc());
  {
    uint32_t AvFormatCtxId = readUInt32(MemInst, AvFormatCtxPtr);
    EXPECT_TRUE(HostFuncAVFindBestStream.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            AvFormatCtxId, UINT32_C(0), INT32_C(-1), INT32_C(-1), 0, 0},
        Result));
    EXPECT_TRUE(Result[0].get<int32_t>() >= 0);
  }

  //  FuncInst =
  //      AVFormatMod->findFuncExports("wasmedge_ffmpeg_avformat_av_read_frame");
  //  EXPECT_NE(FuncInst, nullptr);
  //  EXPECT_TRUE(FuncInst->isHostFunction());
  //  auto &HostFuncAVReadFrame =
  //      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVReadFrame &>(
  //          FuncInst->getHostFunc());
  //  {
  //    uint32_t AvFormatCtxId = readUInt32(MemInst, AvFormatCtxPtr);
  //    EXPECT_TRUE(HostFuncAVReadFrame.run(
  //        CallFrame, std::initializer_list<WasmEdge::ValVariant>{}, Result));
  //    EXPECT_TRUE(Result[0].get<int32_t>() >= 0);
  //  }
  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avformat_close_input");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFormatCloseInput = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVFormatCloseInput &>(
      FuncInst->getHostFunc());
  {
    uint32_t AvFormatCtxId = readUInt32(MemInst, AvFormatCtxPtr);
    EXPECT_TRUE(HostFuncAVFormatCloseInput.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AvFormatCtxId},
        Result));
    EXPECT_TRUE(Result[0].get<int32_t>() >= 0);
  }

  //  FuncInst =
  //      AVFormatMod->findFuncExports("wasmedge_ffmpeg_avformat_avio_close");
  //  EXPECT_NE(FuncInst, nullptr);
  //  EXPECT_TRUE(FuncInst->isHostFunction());
  //  auto &HostFuncAVIOClose =
  //      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVIOClose &>(
  //          FuncInst->getHostFunc());
  //  {
  //    uint32_t AvFormatCtxId = readUInt32(MemInst, AvFormatCtxPtr);
  //    EXPECT_TRUE(HostFuncAVIOClose.run(
  //        CallFrame,
  //        std::initializer_list<WasmEdge::ValVariant>{AvFormatCtxId},
  //        Result));
  //    EXPECT_TRUE(Result[0].get<int32_t>() >= 0);
  //  }

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avformat_free_context");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFreeContext = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVFormatFreeContext &>(
      FuncInst->getHostFunc());
  {
    uint32_t AvFormatCtxId = readUInt32(MemInst, AvFormatCtxPtr);
    EXPECT_TRUE(HostFuncAVFreeContext.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AvFormatCtxId},
        Result));
    EXPECT_TRUE(Result[0].get<int32_t>() >= 0);
  }
}
