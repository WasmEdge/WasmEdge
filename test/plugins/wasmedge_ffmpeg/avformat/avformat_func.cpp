// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "avformat/avformat_func.h"
#include "avformat/avChapter.h"
#include "avformat/avformatContext.h"
#include "avformat/module.h"
#include "avutil/avDictionary.h"

#include "utils.h"

#include <gtest/gtest.h>

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {

// Testing all AVFormat_funcs.
TEST_F(FFmpegTest, AVInputFormatFunc) {
  uint32_t FormatCtxPtr = UINT32_C(4);
  uint32_t DictPtr = UINT32_C(16);
  uint32_t KeyPtr = UINT32_C(100);
  uint32_t ValuePtr = UINT32_C(200);
  uint32_t StrPtr = UINT32_C(400);

  initDict(DictPtr, KeyPtr, std::string("Key"), ValuePtr, std::string("Value"));
  uint32_t DictId = readUInt32(MemInst, DictPtr);

  uint32_t UrlStart = UINT32_C(300);
  uint32_t UrlSize = 30;
  fillMemContent(MemInst, UrlStart, UrlSize);
  fillMemContent(MemInst, UrlStart,
                 std::string("ffmpeg-assets/sample_video.mp4"));

  auto *FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avformat_open_input");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFormatOpenInput = FuncInst->getHostFunc();

  spdlog::info("Testing AVFormatOpenInput"sv);
  {
    // AVDict only
    EXPECT_TRUE(HostFuncAVFormatOpenInput.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            FormatCtxPtr, UrlStart, UrlSize, UINT32_C(0), DictId},
        Result));
    EXPECT_TRUE(Result[0].get<int32_t>() >= 0);
    EXPECT_TRUE(readUInt32(MemInst, FormatCtxPtr) > 0);

    // No AVDict, No AVInputFormat
    EXPECT_TRUE(HostFuncAVFormatOpenInput.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            FormatCtxPtr, UrlStart, UrlSize, UINT32_C(0), UINT32_C(0)},
        Result));
    EXPECT_TRUE(Result[0].get<int32_t>() >= 0);
    EXPECT_TRUE(readUInt32(MemInst, FormatCtxPtr) > 0);
  }

  uint32_t FormatCtxId = readUInt32(MemInst, FormatCtxPtr);

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avformat_find_stream_info");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFormatFindStreamInfo = FuncInst->getHostFunc();

  spdlog::info("Testing AVFormatFindStreamInfo"sv);
  {
    EXPECT_TRUE(HostFuncAVFormatFindStreamInfo.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{FormatCtxId, UINT32_C(0)},
        Result));
    EXPECT_TRUE(Result[0].get<int32_t>() >= 0);
  }

  FuncInst =
      AVFormatMod->findFuncExports("wasmedge_ffmpeg_avformat_av_dump_format");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFormatAVDumpFormat = FuncInst->getHostFunc();

  spdlog::info("Testing AVDumpFormat"sv);
  {
    EXPECT_TRUE(HostFuncAVFormatAVDumpFormat.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            FormatCtxId, 0, UINT32_C(100), UINT32_C(30), 0},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), 0);
  }

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_av_find_best_stream");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFindBestStream = FuncInst->getHostFunc();

  spdlog::info("Testing AVFindBestStream"sv);
  {
    EXPECT_TRUE(HostFuncAVFindBestStream.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            FormatCtxId, UINT32_C(0), INT32_C(-1), INT32_C(-1), 0, 0},
        Result));
    EXPECT_TRUE(Result[0].get<int32_t>() >= 0);
  }

  FuncInst =
      AVFormatMod->findFuncExports("wasmedge_ffmpeg_avformat_av_read_frame");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVReadFrame = FuncInst->getHostFunc();

  spdlog::info("Testing AVReadFrame"sv);
  {
    uint32_t PacketPtr = UINT32_C(520);
    allocPacket(PacketPtr);
    uint32_t PacketId = readUInt32(MemInst, PacketPtr);
    EXPECT_TRUE(HostFuncAVReadFrame.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{FormatCtxId, PacketId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), 0);
  }

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avformat_network_init");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFormatNetworkInit = FuncInst->getHostFunc();

  spdlog::info("Testing AVFormatNetworkInit"sv);
  {
    EXPECT_TRUE(HostFuncAVFormatNetworkInit.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{}, Result));

    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avformat_seek_file");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFormatSeekFile = FuncInst->getHostFunc();

  spdlog::info("Testing AVFormatSeekFile"sv);
  {
    uint32_t StreamIdx = -1;
    int64_t MinTs = -10;
    int64_t Ts = 0;
    int64_t MaxTs = 10;
    int32_t Flags = 0;

    // Try a network Fetch.
    EXPECT_TRUE(HostFuncAVFormatSeekFile.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{FormatCtxId, StreamIdx,
                                                    MinTs, Ts, MaxTs, Flags},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), 0);
  }

  FuncInst =
      AVFormatMod->findFuncExports("wasmedge_ffmpeg_avformat_av_read_play");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFormatAVReadPlay = FuncInst->getHostFunc();

  spdlog::info("Testing AVReadPlay"sv);
  {
    // Try a network Fetch.
    EXPECT_TRUE(HostFuncAVFormatAVReadPlay.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{FormatCtxId},
        Result));
    EXPECT_TRUE(Result[0].get<int32_t>() < 0);
  }

  FuncInst =
      AVFormatMod->findFuncExports("wasmedge_ffmpeg_avformat_av_read_pause");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFormatAVReadPause = FuncInst->getHostFunc();

  spdlog::info("Testing AVReadPause"sv);
  {
    // Try a network Fetch.
    EXPECT_TRUE(HostFuncAVFormatAVReadPause.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{FormatCtxId},
        Result));
    EXPECT_TRUE(Result[0].get<int32_t>() < 0);
  }

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avformat_network_deinit");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFormatNetworkDeInit = FuncInst->getHostFunc();

  spdlog::info("Testing AVFormatNetworkDeInit"sv);
  {
    EXPECT_TRUE(HostFuncAVFormatNetworkDeInit.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{}, Result));

    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avformat_close_input");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFormatCloseInput = FuncInst->getHostFunc();

  spdlog::info("Testing AVFormatCloseInput"sv);
  {
    EXPECT_TRUE(HostFuncAVFormatCloseInput.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{FormatCtxId},
        Result));
    EXPECT_TRUE(Result[0].get<int32_t>() >= 0);
  }

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avformat_free_context");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFreeContext = FuncInst->getHostFunc();

  spdlog::info("Testing AVFormatFreeContext"sv);
  {
    EXPECT_TRUE(HostFuncAVFreeContext.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{FormatCtxId},
        Result));
    EXPECT_TRUE(Result[0].get<int32_t>() >= 0);
  }

  FuncInst =
      AVFormatMod->findFuncExports("wasmedge_ffmpeg_avformat_avformat_version");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFormatVersion = FuncInst->getHostFunc();

  spdlog::info("Testing AVFormatVersion"sv);
  {
    EXPECT_TRUE(HostFuncAVFormatVersion.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{}, Result));

    EXPECT_TRUE((Result[0].get<int32_t>() >> 16) > 0);
  }

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avformat_configuration_length");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFormatConfigurationLength = FuncInst->getHostFunc();

  spdlog::info("Testing AVFormatConfigurationLength"sv);
  int32_t Length = 0;
  {
    EXPECT_TRUE(HostFuncAVFormatConfigurationLength.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{}, Result));
    Length = Result[0].get<int32_t>();
    EXPECT_TRUE(Length > 0);
  }

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avformat_configuration");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFormatConfiguration = FuncInst->getHostFunc();

  spdlog::info("Testing AVFormatConfiguration"sv);
  {
    EXPECT_TRUE(HostFuncAVFormatConfiguration.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{StrPtr, Length},
        Result));

    EXPECT_TRUE(Result[0].get<int32_t>() >= 0);
    EXPECT_NE(std::string_view(MemInst->getPointer<char *>(StrPtr),
                               static_cast<size_t>(Length))
                  .find("--"),
              std::string_view::npos);
  }

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avformat_license_length");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFormatLicenseLength = FuncInst->getHostFunc();

  spdlog::info("Testing AVFormatLicenseLength"sv);
  {
    EXPECT_TRUE(HostFuncAVFormatLicenseLength.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{}, Result));

    Length = Result[0].get<int32_t>();
    EXPECT_TRUE(Length > 0);
  }

  FuncInst =
      AVFormatMod->findFuncExports("wasmedge_ffmpeg_avformat_avformat_license");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFormatLicense = FuncInst->getHostFunc();

  spdlog::info("Testing AVFormatLicense"sv);
  {
    EXPECT_TRUE(HostFuncAVFormatLicense.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{StrPtr, Length},
        Result));

    EXPECT_TRUE(Result[0].get<int32_t>() >= 0);
    EXPECT_EQ(std::string_view(MemInst->getPointer<char *>(StrPtr),
                               static_cast<size_t>(Length))
                  .find("--"),
              std::string_view::npos);
  }
}

TEST_F(FFmpegTest, AVOutputFormatFunc) {

  uint32_t FormatCtxPtr = UINT32_C(4);
  uint32_t DictPtr = UINT32_C(16);
  uint32_t ChapterPtr = UINT32_C(20);
  uint32_t FramePtr = UINT32_C(24);
  uint32_t KeyPtr = UINT32_C(100);
  uint32_t ValuePtr = UINT32_C(200);

  initDict(DictPtr, KeyPtr, std::string("Key"), ValuePtr, std::string("Value"));
  initEmptyFrame(FramePtr);
  uint32_t DictId = readUInt32(MemInst, DictPtr);
  uint32_t FrameId = readUInt32(MemInst, FramePtr);

  uint32_t FormatStart = 300;
  uint32_t FormatLen = 3;
  uint32_t FileStart = 350;
  uint32_t FileLen = 8;
  fillMemContent(MemInst, FormatStart, FormatLen + FileLen);

  fillMemContent(MemInst, FormatStart, "mp4"sv);
  fillMemContent(MemInst, FileStart, "test.mp4"sv);

  auto *FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avformat_alloc_output_context2");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFormatAllocOutputContext2 = FuncInst->getHostFunc();

  spdlog::info("Testing AVFormatAllocOutputContext2"sv);
  {
    EXPECT_TRUE(HostFuncAVFormatAllocOutputContext2.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            FormatCtxPtr, 0, FormatStart, FormatLen, FileStart, FileLen},
        Result));
    EXPECT_TRUE(Result[0].get<int32_t>() >= 0);

    EXPECT_TRUE(HostFuncAVFormatAllocOutputContext2.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            FormatCtxPtr, readUInt32(MemInst, FormatCtxPtr), FormatStart,
            FormatLen, FileStart, FileLen},
        Result));
    EXPECT_TRUE(Result[0].get<int32_t>() >= 0);

    EXPECT_TRUE(HostFuncAVFormatAllocOutputContext2.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{FormatCtxPtr, 0, 0, 0,
                                                    FileStart, FileLen},
        Result));
    EXPECT_TRUE(Result[0].get<int32_t>() >= 0);
  }

  FuncInst = AVFormatMod->findFuncExports("wasmedge_ffmpeg_avformat_avio_open");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVIOOpen = FuncInst->getHostFunc();

  spdlog::info("Testing AVIOOpen"sv);
  {
    uint32_t AvFormatCtxId = readUInt32(MemInst, FormatCtxPtr);
    EXPECT_TRUE(
        HostFuncAVIOOpen.run(CallFrame,
                             std::initializer_list<WasmEdge::ValVariant>{
                                 AvFormatCtxId, FileStart, FileLen, 2},
                             Result));
    EXPECT_TRUE(Result[0].get<int32_t>() >= 0);
  }

  FuncInst =
      AVFormatMod->findFuncExports("wasmedge_ffmpeg_avformat_avio_open2");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVIOOpen2 = FuncInst->getHostFunc();

  spdlog::info("Testing AVIOOpen2"sv);
  {
    uint32_t FormatCtxId = readUInt32(MemInst, FormatCtxPtr);
    EXPECT_TRUE(HostFuncAVIOOpen2.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{FormatCtxId, FileStart,
                                                    FileLen, 2, 0, DictId},
        Result));
    EXPECT_TRUE(Result[0].get<int32_t>() >= 0);
  }

  // TODO: This test modifies the input file, so it cannot be tested.
  // Added test on the Rust side.
  //  spdlog::info("Testing AVGuessCodec"sv);
  //  uint32_t EmptyStrPtr = UINT32_C(520);
  //  writeUInt32(MemInst, 0, EmptyStrPtr);
  //  {
  //    uint32_t FormatCtxId = readUInt32(MemInst, FormatCtxPtr);
  //    int32_t MediaTypeId = 0; // Video
  //    EXPECT_TRUE(HostFuncAVGuessCodec.run(
  //        CallFrame,
  //        std::initializer_list<WasmEdge::ValVariant>{FormatCtxId,
  //        EmptyStrPtr, 0,
  //                                                    FilePtr, 32,
  //                                                    EmptyStrPtr, 0,
  //                                                    MediaTypeId},
  //        Result));
  //    EXPECT_EQ(Result[0].get<int32_t>(), 1); // AV_CODEC_ID_MPEG1VIDEO:
  //  }

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avformat_write_header");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFormatWriteHeader = FuncInst->getHostFunc();

  spdlog::info("Testing AVFormatWriteHeader"sv);
  {
    // Did not set AVParameters, etc. Hence Giving Invalid Argument Error.
    uint32_t FormatCtxId = readUInt32(MemInst, FormatCtxPtr);
    EXPECT_TRUE(HostFuncAVFormatWriteHeader.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{FormatCtxId, DictId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), -22);
  }

  // Writing the header above returns an invalid argument, so the test below
  // does not work. The OutputFormatContext should be configured using the input
  // format context. Test this on the Rust side. This is working as expected.

  //  FuncInst = AVFormatMod->findFuncExports(
  //      "wasmedge_ffmpeg_avformat_avformat_write_trailer");
  //  EXPECT_NE(FuncInst, nullptr);
  //  EXPECT_TRUE(FuncInst->isHostFunction());
  //  auto &HostFuncAVFormatTrailer = dynamic_cast<
  //      WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVFormatWriteTrailer &>(
  //      FuncInst->getHostFunc());
  //  {
  //    // Did not set AVParameters, etc. Hence Giving Invalid Argument Error.
  //    uint32_t FormatCtxId = readUInt32(MemInst, FormatCtxPtr);
  //    EXPECT_TRUE(HostFuncAVFormatTrailer.run(
  //        CallFrame, std::initializer_list<WasmEdge::ValVariant>{FormatCtxId},
  //        Result));
  //    EXPECT_EQ(Result[0].get<int32_t>(), -22);
  //  }

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avchapter_mallocz");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVIOClose = FuncInst->getHostFunc();

  spdlog::info("Testing AVChapterMallocz"sv);
  {
    EXPECT_TRUE(HostFuncAVIOClose.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{ChapterPtr},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }
  // How to pass IntPtr
  //
  //  FuncInst = AVFormatMod->findFuncExports(
  //      "wasmedge_ffmpeg_avformat_avchapter_dynarray_add");
  //  EXPECT_NE(FuncInst, nullptr);
  //  EXPECT_TRUE(FuncInst->isHostFunction());
  //  auto &HostFuncAVChapterDynarrayAdd = dynamic_cast<
  //      WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVChapterDynarrayAdd &>(
  //      FuncInst->getHostFunc());
  //  // For the given input file, nb_chapter is 0;
  //  {
  //    uint32_t AvChapterId = readUInt32(MemInst, AvFormatCtxPtr);
  //    uint32_t AvFormatCtxId = readUInt32(MemInst, AvFormatCtxPtr);
  //    EXPECT_TRUE(HostFuncAVChapterDynarrayAdd.run(
  //        CallFrame,
  //        std::initializer_list<WasmEdge::ValVariant>{AvFormatCtxId,
  //        UINT32_C(0),
  //                                                    AvChapterId},
  //        Result));
  //    EXPECT_EQ(Result[0].get<int32_t>(),
  //    static_cast<int32_t>(ErrNo::Success));
  //  }

  FuncInst =
      AVFormatMod->findFuncExports("wasmedge_ffmpeg_avformat_avformat_avfreep");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFormatAVFreep = FuncInst->getHostFunc();

  spdlog::info("Testing AVFreeP"sv);
  {
    uint32_t ChapterId = readUInt32(MemInst, ChapterPtr);
    EXPECT_TRUE(HostFuncAVFormatAVFreep.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{ChapterId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst =
      AVFormatMod->findFuncExports("wasmedge_ffmpeg_avformat_av_write_frame");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVWriteFrame = FuncInst->getHostFunc();

  spdlog::info("Testing AVWriteFrame"sv);
  // Passing Empty Frame, Hence giving Invalid Argument Error.
  {
    uint32_t FormatCtxId = readUInt32(MemInst, FormatCtxPtr);
    EXPECT_TRUE(HostFuncAVWriteFrame.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{FormatCtxId, FrameId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), -22);
  }

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_av_interleaved_write_frame");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVInterleavedWriteFrame = FuncInst->getHostFunc();

  spdlog::info("Testing AVInterleavedWriteFrame"sv);
  // Passing Empty Frame, Hence giving Invalid Argument Error.
  {
    uint32_t FormatCtxId = readUInt32(MemInst, FormatCtxPtr);
    EXPECT_TRUE(HostFuncAVInterleavedWriteFrame.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{FormatCtxId, FrameId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), -22);
  }
}

TEST_F(FFmpegTest, AVIOOpenBounds) {
  ASSERT_TRUE(AVFormatMod != nullptr);

  auto *FuncInst =
      AVFormatMod->findFuncExports("wasmedge_ffmpeg_avformat_avio_open");
  auto &HostFuncAVIOOpen = FuncInst->getHostFunc();

  // The file-name pointer is in bounds but the guest-declared length runs off
  // the end of linear memory; the host must reject it, not read past the page.
  uint32_t AvFormatCtxId = UINT32_C(0);
  uint32_t OutOfBoundsFileNamePtr = UINT32_C(65000);
  uint32_t OutOfBoundsFileNameLen = UINT32_C(2000);
  int32_t Flags = 0;
  HostFuncAVIOOpen.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          AvFormatCtxId, OutOfBoundsFileNamePtr, OutOfBoundsFileNameLen, Flags},
      Result);
  EXPECT_EQ(Result[0].get<int32_t>(),
            static_cast<int32_t>(ErrNo::MissingMemory));
}

TEST_F(FFmpegTest, AVChapterDynarrayAddNullContext) {
  ASSERT_TRUE(AVFormatMod != nullptr);

  uint32_t NbChaptersPtr = UINT32_C(4);
  writeSInt32(MemInst, 0, NbChaptersPtr);

  auto *FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avchapter_dynarray_add");
  ASSERT_NE(FuncInst, nullptr);
  auto &HostFuncDynarrayAdd = FuncInst->getHostFunc();

  // A null/zero format-context id must be rejected, not dereferenced when
  // writing through &(AvFormatContext->chapters).
  uint32_t NullFormatCtxId = UINT32_C(0);
  uint32_t AvChapterId = UINT32_C(0);
  HostFuncDynarrayAdd.run(CallFrame,
                          std::initializer_list<WasmEdge::ValVariant>{
                              NullFormatCtxId, NbChaptersPtr, AvChapterId},
                          Result);
  EXPECT_EQ(Result[0].get<int32_t>(),
            static_cast<int32_t>(ErrNo::InternalError));
}

TEST_F(FFmpegTest, AVChapterDynarrayAddIgnoresForgedCount) {
  ASSERT_TRUE(AVFormatMod != nullptr);

  uint32_t NbChaptersPtr = UINT32_C(4);
  uint32_t FormatCtxPtr = UINT32_C(8);
  uint32_t ChapterPtr = UINT32_C(12);
  uint32_t FilePtr = UINT32_C(200);

  initFormatCtx(FormatCtxPtr, FilePtr,
                std::string("ffmpeg-assets/sample_video.mp4"));
  uint32_t FormatCtxId = readUInt32(MemInst, FormatCtxPtr);
  ASSERT_TRUE(FormatCtxId > 0);

  auto *NbInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avformatContext_nb_chapters");
  ASSERT_NE(NbInst, nullptr);
  auto &HostFuncNbChapters = NbInst->getHostFunc();
  HostFuncNbChapters.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{FormatCtxId},
      Result);
  int32_t InitialNbChapters = static_cast<int32_t>(Result[0].get<uint32_t>());

  auto *MalloczInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avchapter_mallocz");
  ASSERT_NE(MalloczInst, nullptr);
  auto &HostFuncMallocz = MalloczInst->getHostFunc();
  HostFuncMallocz.run(CallFrame,
                      std::initializer_list<WasmEdge::ValVariant>{ChapterPtr},
                      Result);
  uint32_t ChapterId = readUInt32(MemInst, ChapterPtr);
  ASSERT_TRUE(ChapterId > 0);

  auto *FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avchapter_dynarray_add");
  ASSERT_NE(FuncInst, nullptr);
  auto &HostFuncDynarrayAdd = FuncInst->getHostFunc();

  // A guest that forges a large (non-power-of-two) count must not make
  // av_dynarray_add write the chapter past the array; the host appends at the
  // authoritative index and reports the true new count in both the guest slot
  // and the context.
  writeSInt32(MemInst, INT32_C(0x40000001), NbChaptersPtr);
  HostFuncDynarrayAdd.run(CallFrame,
                          std::initializer_list<WasmEdge::ValVariant>{
                              FormatCtxId, NbChaptersPtr, ChapterId},
                          Result);
  EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  EXPECT_EQ(readSInt32(MemInst, NbChaptersPtr), InitialNbChapters + 1);

  HostFuncNbChapters.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{FormatCtxId},
      Result);
  EXPECT_EQ(Result[0].get<uint32_t>(),
            static_cast<uint32_t>(InitialNbChapters + 1));
}

TEST_F(FFmpegTest, AVFreePOnContextOwnedChapter) {
  ASSERT_TRUE(AVFormatMod != nullptr);

  uint32_t NbChaptersPtr = UINT32_C(4);
  uint32_t FormatCtxPtr = UINT32_C(8);
  uint32_t ChapterPtr = UINT32_C(12);
  uint32_t FilePtr = UINT32_C(200);

  initFormatCtx(FormatCtxPtr, FilePtr,
                std::string("ffmpeg-assets/sample_video.mp4"));
  uint32_t FormatCtxId = readUInt32(MemInst, FormatCtxPtr);
  ASSERT_TRUE(FormatCtxId > 0);

  auto *NbInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avformatContext_nb_chapters");
  ASSERT_NE(NbInst, nullptr);
  auto &HostFuncNbChapters = NbInst->getHostFunc();
  HostFuncNbChapters.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{FormatCtxId},
      Result);
  uint32_t ChapterIdx = Result[0].get<uint32_t>();

  auto *MalloczInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avchapter_mallocz");
  ASSERT_NE(MalloczInst, nullptr);
  auto &HostFuncMallocz = MalloczInst->getHostFunc();
  HostFuncMallocz.run(CallFrame,
                      std::initializer_list<WasmEdge::ValVariant>{ChapterPtr},
                      Result);
  uint32_t ChapterId = readUInt32(MemInst, ChapterPtr);
  ASSERT_TRUE(ChapterId > 0);

  auto *AddInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avchapter_dynarray_add");
  ASSERT_NE(AddInst, nullptr);
  auto &HostFuncDynarrayAdd = AddInst->getHostFunc();
  writeSInt32(MemInst, static_cast<int32_t>(ChapterIdx), NbChaptersPtr);
  HostFuncDynarrayAdd.run(CallFrame,
                          std::initializer_list<WasmEdge::ValVariant>{
                              FormatCtxId, NbChaptersPtr, ChapterId},
                          Result);
  ASSERT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

  // Once the chapter belongs to the context, set its id to a value that is a
  // wild pointer. Freeing the chapter through its handle must not treat that
  // value as a pointer and free it (a guest-controlled arbitrary free), nor
  // free the struct the context still owns (a later double free).
  auto *SetIdInst =
      AVFormatMod->findFuncExports("wasmedge_ffmpeg_avformat_avChapter_set_id");
  ASSERT_NE(SetIdInst, nullptr);
  auto &HostFuncSetId = SetIdInst->getHostFunc();
  int64_t const PoisonId = INT64_C(0x4141414141414141);
  HostFuncSetId.run(CallFrame,
                    std::initializer_list<WasmEdge::ValVariant>{
                        FormatCtxId, ChapterIdx, PoisonId},
                    Result);
  ASSERT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

  auto *FreePInst =
      AVFormatMod->findFuncExports("wasmedge_ffmpeg_avformat_avformat_avfreep");
  ASSERT_NE(FreePInst, nullptr);
  auto &HostFuncFreeP =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVFreeP &>(
          FreePInst->getHostFunc());
  HostFuncFreeP.run(CallFrame,
                    std::initializer_list<WasmEdge::ValVariant>{ChapterId},
                    Result);
  EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

  auto Env = HostFuncFreeP.getEnv();
  EXPECT_EQ(Env->fetchData(ChapterId), nullptr);
}

TEST_F(FFmpegTest, AVFormatCtxSetMetadataCopiesDict) {
  ASSERT_TRUE(AVFormatMod != nullptr);
  ASSERT_TRUE(AVUtilMod != nullptr);

  uint32_t FormatCtxPtr = UINT32_C(8);
  uint32_t DictPtr = UINT32_C(12);
  uint32_t MetaDictPtr = UINT32_C(16);
  uint32_t KeyPtr = UINT32_C(200);
  uint32_t ValuePtr = UINT32_C(220);
  uint32_t KeyLenPtr = UINT32_C(240);
  uint32_t ValueLenPtr = UINT32_C(244);
  uint32_t FilePtr = UINT32_C(300);

  initFormatCtx(FormatCtxPtr, FilePtr,
                std::string("ffmpeg-assets/sample_video.mp4"));
  uint32_t FormatCtxId = readUInt32(MemInst, FormatCtxPtr);
  ASSERT_TRUE(FormatCtxId > 0);

  std::string Key = "meta_key";
  std::string Value = "meta_val";
  fillMemContent(MemInst, KeyPtr, Key);
  fillMemContent(MemInst, ValuePtr, Value);

  // Build an owning dictionary the guest controls.
  auto *SetInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_dict_set");
  ASSERT_NE(SetInst, nullptr);
  auto &HostFuncDictSet = SetInst->getHostFunc();
  HostFuncDictSet.run(CallFrame,
                      std::initializer_list<WasmEdge::ValVariant>{
                          DictPtr, KeyPtr, static_cast<uint32_t>(Key.length()),
                          ValuePtr, static_cast<uint32_t>(Value.length()),
                          INT32_C(0)},
                      Result);
  uint32_t DictId = readUInt32(MemInst, DictPtr);
  ASSERT_TRUE(DictId > 0);

  // Attach it to the context, then free the guest's dictionary.
  auto *SetMetaInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avformatContext_set_metadata");
  ASSERT_NE(SetMetaInst, nullptr);
  auto &HostFuncSetMetadata = SetMetaInst->getHostFunc();
  HostFuncSetMetadata.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{FormatCtxId, DictId}, Result);
  EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

  auto *FreeInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_dict_free");
  ASSERT_NE(FreeInst, nullptr);
  auto &HostFuncDictFree = FreeInst->getHostFunc();
  HostFuncDictFree.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{DictId}, Result);
  EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

  // The context kept its own copy: the entry is still readable after the guest
  // dictionary was freed.
  auto *MetaInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avformatContext_metadata");
  ASSERT_NE(MetaInst, nullptr);
  auto &HostFuncMetadata = MetaInst->getHostFunc();
  HostFuncMetadata.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{FormatCtxId, MetaDictPtr},
      Result);
  uint32_t MetaDictId = readUInt32(MemInst, MetaDictPtr);
  ASSERT_TRUE(MetaDictId > 0);

  auto *GetInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_dict_get");
  ASSERT_NE(GetInst, nullptr);
  auto &HostFuncDictGet = GetInst->getHostFunc();
  HostFuncDictGet.run(CallFrame,
                      std::initializer_list<WasmEdge::ValVariant>{
                          MetaDictId, KeyPtr,
                          static_cast<uint32_t>(Key.length()), UINT32_C(0),
                          INT32_C(0), KeyLenPtr, ValueLenPtr},
                      Result);
  EXPECT_EQ(Result[0].get<int32_t>(), 1);

  // Closing frees the context's copy exactly once; before the fix this
  // double-freed the dictionary the guest already released.
  auto *CloseInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avformat_close_input");
  ASSERT_NE(CloseInst, nullptr);
  auto &HostFuncClose = CloseInst->getHostFunc();
  HostFuncClose.run(CallFrame,
                    std::initializer_list<WasmEdge::ValVariant>{FormatCtxId},
                    Result);
  EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
}

} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
