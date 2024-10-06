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
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFormatCtxIFormat = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVFormatCtxIFormat &>(
      FuncInst->getHostFunc());

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
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVIOFormatNameLength = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVIOFormatNameLength &>(
      FuncInst->getHostFunc());

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
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVInputFormatName = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVInputFormatName &>(
      FuncInst->getHostFunc());

  fillMemContent(MemInst, StrBuf, Length);
  {
    EXPECT_TRUE(HostFuncAVInputFormatName.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{InputFormatId, StrBuf,
                                                    Length},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avIOFormat_long_name_length");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVIOFormatLongNameLength = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVIOFormatLongNameLength &>(
      FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVIOFormatLongNameLength.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{InputFormatId, 0}, Result));

    Length = Result[0].get<int32_t>();
    ASSERT_TRUE(Length > 0);
  }

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avInputFormat_long_name");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVInputFormatLongName = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVInputFormatLongName &>(
      FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVInputFormatLongName.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{InputFormatId, StrBuf,
                                                    Length},
        Result));

    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avIOFormat_extensions_length");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVIOFormatExtensionsLength = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVIOFormatExtensionsLength &>(
      FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVIOFormatExtensionsLength.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{InputFormatId, 0}, Result));

    Length = Result[0].get<int32_t>();
    ASSERT_TRUE(Length > 0);
  }

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avInputFormat_extensions");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVInputFormatExtensions = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVInputFormatExtensions &>(
      FuncInst->getHostFunc());

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
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVIOFormatMimeTypeLength = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVIOFormatMimeTypeLength &>(
      FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVIOFormatMimeTypeLength.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{InputFormatId, 0}, Result));

    Length = Result[0].get<int32_t>();
    ASSERT_TRUE(Length >= 0);
  }

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avInputFormat_mime_type");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVInputFormatMimeType = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVInputFormatMimeType &>(
      FuncInst->getHostFunc());

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
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVInputOutputFormatFree = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVInputOutputFormatFree &>(
      FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVInputOutputFormatFree.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{InputFormatId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }
}

} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
