// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "avformat/avformat_func.h"
#include "avformat/module.h"

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
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFormatOpenInput = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVFormatOpenInput &>(
      FuncInst->getHostFunc());

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
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFormatFindStreamInfo = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVFormatFindStreamInfo &>(
      FuncInst->getHostFunc());

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
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFormatAVDumpFormat =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVDumpFormat &>(
          FuncInst->getHostFunc());

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
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFindBestStream = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVFindBestStream &>(
      FuncInst->getHostFunc());

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
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVReadFrame =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVReadFrame &>(
          FuncInst->getHostFunc());

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
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFormatNetworkInit = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVFormatNetworkInit &>(
      FuncInst->getHostFunc());

  spdlog::info("Testing AVFormatNetworkInit"sv);
  {
    EXPECT_TRUE(HostFuncAVFormatNetworkInit.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{}, Result));

    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avformat_seek_file");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFormatSeekFile = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVFormatSeekFile &>(
      FuncInst->getHostFunc());

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
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFormatAVReadPlay =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVReadPlay &>(
          FuncInst->getHostFunc());

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
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFormatAVReadPause =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVReadPause &>(
          FuncInst->getHostFunc());

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
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFormatNetworkDeInit = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVFormatNetworkDeInit &>(
      FuncInst->getHostFunc());

  spdlog::info("Testing AVFormatNetworkDeInit"sv);
  {
    EXPECT_TRUE(HostFuncAVFormatNetworkDeInit.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{}, Result));

    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avformat_close_input");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFormatCloseInput = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVFormatCloseInput &>(
      FuncInst->getHostFunc());

  spdlog::info("Testing AVFormatCloseInput"sv);
  {
    EXPECT_TRUE(HostFuncAVFormatCloseInput.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{FormatCtxId},
        Result));
    EXPECT_TRUE(Result[0].get<int32_t>() >= 0);
  }

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avformat_free_context");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFreeContext = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVFormatFreeContext &>(
      FuncInst->getHostFunc());

  spdlog::info("Testing AVFormatFreeContext"sv);
  {
    EXPECT_TRUE(HostFuncAVFreeContext.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{FormatCtxId},
        Result));
    EXPECT_TRUE(Result[0].get<int32_t>() >= 0);
  }

  FuncInst =
      AVFormatMod->findFuncExports("wasmedge_ffmpeg_avformat_avformat_version");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFormatVersion =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVFormatVersion &>(
          FuncInst->getHostFunc());

  spdlog::info("Testing AVFormatVersion"sv);
  {
    EXPECT_TRUE(HostFuncAVFormatVersion.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{}, Result));

    EXPECT_TRUE(Result[0].get<int32_t>() >= 0);
  }

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avformat_configuration_length");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFormatConfigurationLength = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVFormatConfigurationLength &>(
      FuncInst->getHostFunc());

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
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFormatConfiguration = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVFormatConfiguration &>(
      FuncInst->getHostFunc());

  spdlog::info("Testing AVFormatConfiguration"sv);
  {
    EXPECT_TRUE(HostFuncAVFormatConfiguration.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{StrPtr, Length},
        Result));

    EXPECT_TRUE(Result[0].get<int32_t>() >= 0);
  }

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avformat_license_length");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFormatLicenseLength = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVFormatLicenseLength &>(
      FuncInst->getHostFunc());

  spdlog::info("Testing AVFormatLicenseLength"sv);
  {
    EXPECT_TRUE(HostFuncAVFormatLicenseLength.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{}, Result));

    Length = Result[0].get<int32_t>();
    EXPECT_TRUE(Length > 0);
  }

  FuncInst =
      AVFormatMod->findFuncExports("wasmedge_ffmpeg_avformat_avformat_license");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFormatLicense =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVFormatLicense &>(
          FuncInst->getHostFunc());

  spdlog::info("Testing AVFormatLicense"sv);
  {
    EXPECT_TRUE(HostFuncAVFormatLicense.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{StrPtr, Length},
        Result));

    EXPECT_TRUE(Result[0].get<int32_t>() >= 0);
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
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFormatAllocOutputContext2 = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVFormatAllocOutputContext2 &>(
      FuncInst->getHostFunc());

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
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVIOOpen =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVIOOpen &>(
          FuncInst->getHostFunc());

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
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVIOOpen2 =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVIOOpen2 &>(
          FuncInst->getHostFunc());

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

  // TODO: This test modifies the input file. Unable to test.
  // Added test on rust side.
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
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFormatWriteHeader = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVFormatWriteHeader &>(
      FuncInst->getHostFunc());

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

  // Write Header above return invalid argument due to which below test won't
  // work. The OutputFormatContext should Be configured using the input format
  // context. Test on the Rust side. This is working as expected.

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
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVIOClose = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVChapterMallocz &>(
      FuncInst->getHostFunc());

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
  //  // For the give input file, nb_chapter is 0;
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
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFormatAVFreep =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVFreeP &>(
          FuncInst->getHostFunc());

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
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVWriteFrame =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVWriteFrame &>(
          FuncInst->getHostFunc());

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
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVInterleavedWriteFrame = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVInterleavedWriteFrame &>(
      FuncInst->getHostFunc());

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

} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
