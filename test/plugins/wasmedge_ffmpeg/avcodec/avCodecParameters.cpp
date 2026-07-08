// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "avcodec/avCodecParameters.h"
#include "avcodec/avcodec_func.h"
#include "avcodec/module.h"
#include "avformat/avformatContext.h"
#include "avformat/avformat_func.h"
#include "avutil/avDictionary.h"

#include "utils.h"

#include <gtest/gtest.h>

// Testing all AVCodecstruct

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {

TEST_F(FFmpegTest, AVCodecParameters) {
  ASSERT_TRUE(AVCodecMod != nullptr);

  uint32_t AVCodecParamPtr = UINT32_C(60);

  std::string FileName = "ffmpeg-assets/sample_video.mp4"; // 32 chars
  initFFmpegStructs(UINT32_C(20), UINT32_C(24), UINT32_C(28), FileName,
                    AVCodecParamPtr, UINT32_C(64), UINT32_C(68), UINT32_C(72));

  uint32_t AVCodecParamId = readUInt32(MemInst, AVCodecParamPtr);

  auto *FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodecparam_codec_id");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecParamCodecId = FuncInst->getHostFunc();

  {
    EXPECT_TRUE(HostFuncAVCodecParamCodecId.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVCodecParamId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), 27); // H264
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodecparam_codec_type");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecParamCodecType = FuncInst->getHostFunc();

  {
    EXPECT_TRUE(HostFuncAVCodecParamCodecType.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVCodecParamId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), 0); // MediaType Video
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodecparam_set_codec_tag");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecParamSetCodecTag = FuncInst->getHostFunc();

  {
    EXPECT_TRUE(HostFuncAVCodecParamSetCodecTag.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVCodecParamId, 20},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }
}

TEST_F(FFmpegTest, BorrowedHandlesSurviveGuestFree) {
  ASSERT_TRUE(AVCodecMod != nullptr);
  ASSERT_TRUE(AVFormatMod != nullptr);
  ASSERT_TRUE(AVUtilMod != nullptr);

  uint32_t AVCodecParamPtr = UINT32_C(60);
  uint32_t FormatCtxPtr = UINT32_C(24);
  std::string FileName = "ffmpeg-assets/sample_video.mp4"; // 32 chars
  initFFmpegStructs(UINT32_C(20), FormatCtxPtr, UINT32_C(28), FileName,
                    AVCodecParamPtr, UINT32_C(64), UINT32_C(68), UINT32_C(72));

  uint32_t FormatCtxId = readUInt32(MemInst, FormatCtxPtr);
  uint32_t CodecParamId = readUInt32(MemInst, AVCodecParamPtr);
  ASSERT_TRUE(FormatCtxId > 0);
  ASSERT_TRUE(CodecParamId > 0);

  // codecpar obtained from a stream is borrowed: the AVFormatContext owns it.
  auto *CodecIdFunc = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodecparam_codec_id");
  ASSERT_NE(CodecIdFunc, nullptr);
  auto &HostFuncCodecId = CodecIdFunc->getHostFunc();
  HostFuncCodecId.run(CallFrame,
                      std::initializer_list<WasmEdge::ValVariant>{CodecParamId},
                      Result);
  EXPECT_GT(Result[0].get<int32_t>(), 0);

  // Freeing the borrowed codecpar must drop the id without freeing the
  // stream-owned object.
  auto *ParamFreeFunc = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_parameters_free");
  ASSERT_NE(ParamFreeFunc, nullptr);
  auto &HostFuncParamFree = ParamFreeFunc->getHostFunc();
  HostFuncParamFree.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{CodecParamId},
      Result);
  EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

  // Borrowed format metadata behaves the same way.
  uint32_t DictPtr = UINT32_C(80);
  auto *MetadataFunc = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avformatContext_metadata");
  ASSERT_NE(MetadataFunc, nullptr);
  auto &HostFuncMetadata = MetadataFunc->getHostFunc();
  HostFuncMetadata.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{FormatCtxId, DictPtr},
      Result);
  EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  uint32_t DictId = readUInt32(MemInst, DictPtr);
  ASSERT_TRUE(DictId > 0);

  auto *DictFreeFunc =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_dict_free");
  ASSERT_NE(DictFreeFunc, nullptr);
  auto &HostFuncDictFree = DictFreeFunc->getHostFunc();
  HostFuncDictFree.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{DictId}, Result);
  EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

  // Closing the context frees codecpar and metadata exactly once.
  auto *CloseFunc = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avformat_close_input");
  ASSERT_NE(CloseFunc, nullptr);
  auto &HostFuncClose = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVFormatCloseInput &>(
      CloseFunc->getHostFunc());
  HostFuncClose.run(CallFrame,
                    std::initializer_list<WasmEdge::ValVariant>{FormatCtxId},
                    Result);
  EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
}

TEST_F(FFmpegTest, ClosingContextInvalidatesBorrowedChildIds) {
  ASSERT_TRUE(AVCodecMod != nullptr);
  ASSERT_TRUE(AVFormatMod != nullptr);

  uint32_t AVCodecParamPtr = UINT32_C(60);
  uint32_t FormatCtxPtr = UINT32_C(24);
  std::string FileName = "ffmpeg-assets/sample_video.mp4"; // 32 chars
  initFFmpegStructs(UINT32_C(20), FormatCtxPtr, UINT32_C(28), FileName,
                    AVCodecParamPtr, UINT32_C(64), UINT32_C(68), UINT32_C(72));

  uint32_t FormatCtxId = readUInt32(MemInst, FormatCtxPtr);
  uint32_t CodecParamId = readUInt32(MemInst, AVCodecParamPtr);
  ASSERT_TRUE(FormatCtxId > 0);
  ASSERT_TRUE(CodecParamId > 0);

  // Also take a borrowed metadata handle on the same context.
  uint32_t DictPtr = UINT32_C(80);
  auto *MetadataFunc = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avformatContext_metadata");
  ASSERT_NE(MetadataFunc, nullptr);
  auto &HostFuncMetadata = MetadataFunc->getHostFunc();
  HostFuncMetadata.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{FormatCtxId, DictPtr},
      Result);
  uint32_t DictId = readUInt32(MemInst, DictPtr);
  ASSERT_TRUE(DictId > 0);

  auto *CloseFunc = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avformat_close_input");
  ASSERT_NE(CloseFunc, nullptr);
  auto &HostFuncClose = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVFormatCloseInput &>(
      CloseFunc->getHostFunc());

  // Both borrowed child handles resolve while the context is open.
  auto Env = HostFuncClose.getEnv();
  ASSERT_NE(Env->fetchData(CodecParamId), nullptr);
  ASSERT_NE(Env->fetchData(DictId), nullptr);

  HostFuncClose.run(CallFrame,
                    std::initializer_list<WasmEdge::ValVariant>{FormatCtxId},
                    Result);
  EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

  // Closing frees the streams and metadata the context owns, so the borrowed
  // child ids must no longer resolve.
  EXPECT_EQ(Env->fetchData(CodecParamId), nullptr);
  EXPECT_EQ(Env->fetchData(DictId), nullptr);
}

TEST_F(FFmpegTest, AVCodecParametersCopyIndexOutOfRange) {
  ASSERT_TRUE(AVCodecMod != nullptr);
  ASSERT_TRUE(AVFormatMod != nullptr);

  uint32_t AVCodecParamPtr = UINT32_C(60);
  uint32_t FormatCtxPtr = UINT32_C(24);
  std::string FileName = "ffmpeg-assets/sample_video.mp4"; // 32 chars
  initFFmpegStructs(UINT32_C(20), FormatCtxPtr, UINT32_C(28), FileName,
                    AVCodecParamPtr, UINT32_C(64), UINT32_C(68), UINT32_C(72));

  uint32_t FormatCtxId = readUInt32(MemInst, FormatCtxPtr);
  uint32_t CodecParamId = readUInt32(MemInst, AVCodecParamPtr);
  ASSERT_TRUE(FormatCtxId > 0);
  ASSERT_TRUE(CodecParamId > 0);

  auto *FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_parameters_copy");
  ASSERT_NE(FuncInst, nullptr);
  auto &HostFuncCopy = FuncInst->getHostFunc();

  // A guest-supplied stream index past nb_streams must be rejected, not walked
  // off the end of the streams array.
  uint32_t OutOfRangeIdx = UINT32_C(1000000);
  HostFuncCopy.run(CallFrame,
                   std::initializer_list<WasmEdge::ValVariant>{
                       FormatCtxId, CodecParamId, OutOfRangeIdx},
                   Result);
  EXPECT_EQ(Result[0].get<int32_t>(),
            static_cast<int32_t>(ErrNo::InternalError));

  // A null/zero format-context id must be rejected rather than dereferenced.
  HostFuncCopy.run(CallFrame,
                   std::initializer_list<WasmEdge::ValVariant>{
                       UINT32_C(0), CodecParamId, UINT32_C(0)},
                   Result);
  EXPECT_EQ(Result[0].get<int32_t>(),
            static_cast<int32_t>(ErrNo::InternalError));
}
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
