// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

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
  std::string FileName = "ffmpeg-assets/sample_video.mp4"; // 32 chars
  uint32_t FormatCtxPtr = UINT32_C(24);
  uint32_t InputFormatPtr = UINT32_C(80);
  uint32_t StrBuf = UINT32_C(100);
  initFFmpegStructs(UINT32_C(20), FormatCtxPtr, UINT32_C(28), FileName,
                    UINT32_C(60), UINT32_C(64), UINT32_C(68), UINT32_C(72));
  uint32_t FormatCtxId = readUInt32(MemInst, FormatCtxPtr);

  auto *FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avformatContext_iformat");
  auto &HostFuncIFormat = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVFormatCtxIFormat &>(
      FuncInst->getHostFunc());
  HostFuncIFormat.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{FormatCtxId, InputFormatPtr},
      Result);
  ASSERT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  uint32_t InputFormatId = readUInt32(MemInst, InputFormatPtr);
  ASSERT_TRUE(InputFormatId > 0);

  // The guest buffer is larger than the host string and fenced with 0xAA; the
  // host must copy only the string plus its terminator and never read past the
  // host string into the rest of the guest buffer.
  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avIOFormat_name_length");
  auto &HostFuncNameLen = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVIOFormatNameLength &>(
      FuncInst->getHostFunc());
  HostFuncNameLen.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{InputFormatId, 0},
      Result);
  uint32_t NameLen = Result[0].get<int32_t>();
  ASSERT_TRUE(NameLen > 0);

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avInputFormat_name");
  auto &HostFuncName = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVInputFormatName &>(
      FuncInst->getHostFunc());
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
  auto &HostFuncLongNameLen = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVIOFormatLongNameLength &>(
      FuncInst->getHostFunc());
  HostFuncLongNameLen.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{InputFormatId, 0},
      Result);
  uint32_t LongNameLen = Result[0].get<int32_t>();
  ASSERT_TRUE(LongNameLen > 0);

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avInputFormat_long_name");
  auto &HostFuncLongName = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVInputFormatLongName &>(
      FuncInst->getHostFunc());
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

} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
