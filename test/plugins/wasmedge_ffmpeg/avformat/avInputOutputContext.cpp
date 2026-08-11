// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "avformat/avInputOutputFormat.h"
#include "avformat/avformatContext.h"
#include "avformat/module.h"

#include "utils.h"

#include <gtest/gtest.h>

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {

TEST_F(FFmpegTest, AVInputFormat) {
  using namespace std::literals::string_view_literals;
  std::string FileName = "ffmpeg-assets/sample_video.mp4"; // 32 chars
  uint32_t FormatCtxPtr = UINT32_C(24);
  uint32_t InputFormatPtr = UINT32_C(28);

  uint32_t StrBuf = UINT32_C(100);
  initFFmpegStructs(UINT32_C(20), FormatCtxPtr, UINT32_C(28), FileName,
                    UINT32_C(60), UINT32_C(64), UINT32_C(68), UINT32_C(72));

  uint32_t FormatCtxId = readUInt32(MemInst, FormatCtxPtr);

  // ====================================================================
  //                        Initialize AVInputFormat
  // ====================================================================

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
  uint32_t InputFormatId = readUInt32(MemInst, InputFormatPtr);

  // ====================================================================
  //                       End Initialize AVInputFormat
  // ====================================================================

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avIOFormat_name_length");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVIOFormatNameLength = FuncInst->getHostFunc();

  int32_t Length = 0;
  {
    EXPECT_TRUE(HostFuncAVIOFormatNameLength.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{InputFormatId, 0}, Result));
    Length = Result[0].get<int32_t>();
    ASSERT_TRUE(Length > 0);
  }

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avInputFormat_name");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVInputFormatName = FuncInst->getHostFunc();

  fillMemContent(MemInst, StrBuf, Length);
  {
    EXPECT_TRUE(HostFuncAVInputFormatName.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{InputFormatId, StrBuf,
                                                    Length},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
    EXPECT_EQ(std::string_view(MemInst->getPointer<char *>(StrBuf),
                               static_cast<size_t>(Length)),
              "mov,mp4,m4a,3gp,3g2,mj2"sv);
  }

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avIOFormat_long_name_length");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVIOFormatLongNameLength = FuncInst->getHostFunc();

  {
    EXPECT_TRUE(HostFuncAVIOFormatLongNameLength.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{InputFormatId, 0}, Result));

    Length = Result[0].get<int32_t>();
    ASSERT_TRUE(Length > 0);
  }

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avInputFormat_long_name");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVInputFormatLongName = FuncInst->getHostFunc();

  {
    EXPECT_TRUE(HostFuncAVInputFormatLongName.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{InputFormatId, StrBuf,
                                                    Length},
        Result));

    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
    EXPECT_EQ(std::string_view(MemInst->getPointer<char *>(StrBuf),
                               static_cast<size_t>(Length)),
              "QuickTime / MOV"sv);
  }

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avIOFormat_extensions_length");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVIOFormatExtensionsLength = FuncInst->getHostFunc();

  {
    EXPECT_TRUE(HostFuncAVIOFormatExtensionsLength.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{InputFormatId, 0}, Result));

    Length = Result[0].get<int32_t>();
    ASSERT_TRUE(Length > 0);
  }

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avInputFormat_extensions");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVInputFormatExtensions = FuncInst->getHostFunc();

  {
    EXPECT_TRUE(HostFuncAVInputFormatExtensions.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{InputFormatId, StrBuf,
                                                    Length},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avIOFormat_mime_type_length");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVIOFormatMimeTypeLength = FuncInst->getHostFunc();

  {
    EXPECT_TRUE(HostFuncAVIOFormatMimeTypeLength.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{InputFormatId, 0}, Result));

    Length = Result[0].get<int32_t>();
    ASSERT_TRUE(Length >= 0);
  }

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avInputFormat_mime_type");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVInputFormatMimeType = FuncInst->getHostFunc();

  {
    EXPECT_TRUE(HostFuncAVInputFormatMimeType.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{InputFormatId, StrBuf,
                                                    Length},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avInputOutputFormat_free");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVInputOutputFormatFree = FuncInst->getHostFunc();

  {
    EXPECT_TRUE(HostFuncAVInputOutputFormatFree.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{InputFormatId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }
}

TEST_F(FFmpegTest, AVInputFormatNameBounds) {
  ASSERT_TRUE(AVFormatMod != nullptr);

  std::string FileName = "ffmpeg-assets/sample_video.mp4"; // 32 chars
  uint32_t FormatCtxPtr = UINT32_C(24);
  uint32_t InputFormatPtr = UINT32_C(80);
  uint32_t StrBuf = UINT32_C(100);
  initFFmpegStructs(UINT32_C(20), FormatCtxPtr, UINT32_C(28), FileName,
                    UINT32_C(60), UINT32_C(64), UINT32_C(68), UINT32_C(72));
  uint32_t FormatCtxId = readUInt32(MemInst, FormatCtxPtr);
  // Fail cleanly if the test asset could not be opened; otherwise the format
  // context getters below would receive id 0 and report an error.
  ASSERT_TRUE(FormatCtxId > 0);

  auto *FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avformatContext_iformat");
  auto &HostFuncIFormat = FuncInst->getHostFunc();
  HostFuncIFormat.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{FormatCtxId, InputFormatPtr},
      Result);
  ASSERT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  uint32_t InputFormatId = readUInt32(MemInst, InputFormatPtr);
  ASSERT_TRUE(InputFormatId > 0);

  // The guest buffer is larger than the host string and fenced with 0xAA; the
  // host must copy only the string plus terminator, leaving the fence intact.
  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avIOFormat_name_length");
  auto &HostFuncNameLen = FuncInst->getHostFunc();
  HostFuncNameLen.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{InputFormatId, 0},
      Result);
  uint32_t NameLen = Result[0].get<int32_t>();
  ASSERT_TRUE(NameLen > 0);

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avInputFormat_name");
  auto &HostFuncName = FuncInst->getHostFunc();
  uint32_t NameBufLen = NameLen + UINT32_C(32);
  fillMemContent(MemInst, StrBuf, NameBufLen, UINT8_C(0xAA));
  HostFuncName.run(CallFrame,
                   std::initializer_list<WasmEdge::ValVariant>{
                       InputFormatId, StrBuf, NameBufLen},
                   Result);
  EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  {
    char *Buf = MemInst->getPointer<char *>(StrBuf);
    for (uint32_t I = NameLen + 1; I < NameBufLen; ++I) {
      EXPECT_EQ(static_cast<uint8_t>(Buf[I]), UINT8_C(0xAA));
    }
  }

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avIOFormat_long_name_length");
  auto &HostFuncLongNameLen = FuncInst->getHostFunc();
  HostFuncLongNameLen.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{InputFormatId, 0},
      Result);
  uint32_t LongNameLen = Result[0].get<int32_t>();
  ASSERT_TRUE(LongNameLen > 0);

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avInputFormat_long_name");
  auto &HostFuncLongName = FuncInst->getHostFunc();
  uint32_t LongNameBufLen = LongNameLen + UINT32_C(32);
  fillMemContent(MemInst, StrBuf, LongNameBufLen, UINT8_C(0xAA));
  HostFuncLongName.run(CallFrame,
                       std::initializer_list<WasmEdge::ValVariant>{
                           InputFormatId, StrBuf, LongNameBufLen},
                       Result);
  EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  {
    char *Buf = MemInst->getPointer<char *>(StrBuf);
    for (uint32_t I = LongNameLen + 1; I < LongNameBufLen; ++I) {
      EXPECT_EQ(static_cast<uint8_t>(Buf[I]), UINT8_C(0xAA));
    }
  }
}

// Regression: a live id that is neither an AVInputFormat nor an
// AVOutputFormat must be rejected by avInputOutputFormat_free and keep
// resolving; stale ids and id 0 stay no-ops.
TEST_F(FFmpegTest, InputOutputFormatFreeRejectsWrongTypeHandle) {
  ASSERT_TRUE(AVFormatMod != nullptr);

  uint32_t FormatCtxPtr = UINT32_C(4);
  uint32_t InputFormatPtr = UINT32_C(8);
  uint32_t FilePtr = UINT32_C(100);
  initFormatCtx(FormatCtxPtr, FilePtr,
                std::string("ffmpeg-assets/sample_video.mp4"));
  uint32_t FormatCtxId = readUInt32(MemInst, FormatCtxPtr);
  ASSERT_TRUE(FormatCtxId > 0);

  auto *FreeInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avInputOutputFormat_free");
  ASSERT_NE(FreeInst, nullptr);
  auto &HostFuncFormatFree = FreeInst->getHostFunc();

  // A format-context id is a live handle of another type: reject it.
  HostFuncFormatFree.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{FormatCtxId},
      Result);
  EXPECT_EQ(Result[0].get<int32_t>(),
            static_cast<int32_t>(ErrNo::InternalError));

  // The context handle survived the rejected free and still resolves.
  auto *NbStreamsInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avformatContext_nb_streams");
  ASSERT_NE(NbStreamsInst, nullptr);
  auto &HostFuncNbStreams = NbStreamsInst->getHostFunc();
  HostFuncNbStreams.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{FormatCtxId},
      Result);
  EXPECT_TRUE(Result[0].get<uint32_t>() > 0);

  // An input-format id is accepted and dropped; a stale id and id 0 are
  // idempotent no-ops.
  auto *IFormatInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avformatContext_iformat");
  ASSERT_NE(IFormatInst, nullptr);
  auto &HostFuncIFormat = IFormatInst->getHostFunc();
  HostFuncIFormat.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{FormatCtxId, InputFormatPtr},
      Result);
  ASSERT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  uint32_t InputFormatId = readUInt32(MemInst, InputFormatPtr);
  ASSERT_TRUE(InputFormatId > 0);

  HostFuncFormatFree.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{InputFormatId},
      Result);
  EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  HostFuncFormatFree.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{InputFormatId},
      Result);
  EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  HostFuncFormatFree.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0)},
      Result);
  EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

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
