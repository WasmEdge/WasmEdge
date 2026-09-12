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

TEST_F(FFmpegTest, AVCodecParametersCopy) {
  ASSERT_TRUE(AVCodecMod != nullptr);
  ASSERT_TRUE(AVFormatMod != nullptr);

  uint32_t FormatCtxPtr = UINT32_C(4);
  uint32_t CodecParamPtr = UINT32_C(8);
  uint32_t EmptyParamPtr = UINT32_C(12);
  uint32_t FilePtr = UINT32_C(100);

  std::string FileName = "ffmpeg-assets/sample_video.mp4";
  initFormatCtx(FormatCtxPtr, FilePtr, FileName);
  uint32_t FormatCtxId = readUInt32(MemInst, FormatCtxPtr);
  ASSERT_TRUE(FormatCtxId > 0);

  auto *FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avformat_find_stream_info");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFormatFindStreamInfo = FuncInst->getHostFunc();
  EXPECT_TRUE(HostFuncAVFormatFindStreamInfo.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{FormatCtxId, UINT32_C(0)},
      Result));
  EXPECT_TRUE(Result[0].get<int32_t>() >= 0);

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_av_find_best_stream");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFindBestStream = FuncInst->getHostFunc();

  EXPECT_TRUE(HostFuncAVFindBestStream.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{FormatCtxId, UINT32_C(0),
                                                  INT32_C(-1), INT32_C(-1),
                                                  UINT32_C(0), UINT32_C(0)},
      Result));
  const int32_t StreamIdxI32 = Result[0].get<int32_t>();
  ASSERT_GE(StreamIdxI32, 0);
  const uint32_t StreamIdx = static_cast<uint32_t>(StreamIdxI32);

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avStream_codecpar");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVStreamCodecpar = FuncInst->getHostFunc();

  EXPECT_TRUE(
      HostFuncAVStreamCodecpar.run(CallFrame,
                                   std::initializer_list<WasmEdge::ValVariant>{
                                       FormatCtxId, StreamIdx, CodecParamPtr},
                                   Result));
  EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  uint32_t StreamParamId = readUInt32(MemInst, CodecParamPtr);
  ASSERT_TRUE(StreamParamId > 0);

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodecparam_codec_id");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVCodecParamCodecId = FuncInst->getHostFunc();

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodecparam_codec_type");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVCodecParamCodecType = FuncInst->getHostFunc();

  // Before: sample video stream carries H264 / video.
  EXPECT_TRUE(HostFuncAVCodecParamCodecId.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{StreamParamId},
      Result));
  EXPECT_EQ(Result[0].get<int32_t>(), 27);
  EXPECT_TRUE(HostFuncAVCodecParamCodecType.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{StreamParamId},
      Result));
  EXPECT_EQ(Result[0].get<int32_t>(), 0);

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_parameters_alloc");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVCodecParametersAlloc = FuncInst->getHostFunc();

  EXPECT_TRUE(HostFuncAVCodecParametersAlloc.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{EmptyParamPtr},
      Result));
  EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  uint32_t EmptyParamId = readUInt32(MemInst, EmptyParamPtr);
  ASSERT_TRUE(EmptyParamId > 0);

  // Fresh parameters default to NONE / UNKNOWN.
  EXPECT_TRUE(HostFuncAVCodecParamCodecId.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{EmptyParamId},
      Result));
  EXPECT_EQ(Result[0].get<int32_t>(), 0);
  EXPECT_TRUE(HostFuncAVCodecParamCodecType.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{EmptyParamId},
      Result));
  EXPECT_EQ(Result[0].get<int32_t>(), -1); // AVMEDIA_TYPE_UNKNOWN

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_parameters_copy");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVCodecParametersCopy = FuncInst->getHostFunc();

  spdlog::info("Testing AVCodecParametersCopy"sv);
  // Copies source AVCodecParameters into stream->codecpar.
  EXPECT_TRUE(HostFuncAVCodecParametersCopy.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{FormatCtxId, EmptyParamId,
                                                  StreamIdx},
      Result));
  EXPECT_EQ(Result[0].get<int32_t>(), 0);

  // After: stream codecpar should now match the empty source.
  EXPECT_TRUE(HostFuncAVCodecParamCodecId.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{StreamParamId},
      Result));
  EXPECT_EQ(Result[0].get<int32_t>(), 0);
  EXPECT_TRUE(HostFuncAVCodecParamCodecType.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{StreamParamId},
      Result));
  EXPECT_EQ(Result[0].get<int32_t>(), -1); // AVMEDIA_TYPE_UNKNOWN

  // If an audio stream exists, also exercise a non-zero StreamIdx path.
  EXPECT_TRUE(HostFuncAVFindBestStream.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{FormatCtxId, UINT32_C(1),
                                                  INT32_C(-1), INT32_C(-1),
                                                  UINT32_C(0), UINT32_C(0)},
      Result));
  const int32_t AudioStreamIdx = Result[0].get<int32_t>();
  if (AudioStreamIdx >= 0 &&
      static_cast<uint32_t>(AudioStreamIdx) != StreamIdx) {
    uint32_t AudioParamPtr = UINT32_C(16);
    EXPECT_TRUE(HostFuncAVStreamCodecpar.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            FormatCtxId, static_cast<uint32_t>(AudioStreamIdx), AudioParamPtr},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
    uint32_t AudioParamId = readUInt32(MemInst, AudioParamPtr);
    ASSERT_TRUE(AudioParamId > 0);

    EXPECT_TRUE(HostFuncAVCodecParamCodecType.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AudioParamId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), 1); // audio

    EXPECT_TRUE(HostFuncAVCodecParametersCopy.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            FormatCtxId, EmptyParamId, static_cast<uint32_t>(AudioStreamIdx)},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), 0);

    EXPECT_TRUE(HostFuncAVCodecParamCodecId.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AudioParamId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), 0);
    EXPECT_TRUE(HostFuncAVCodecParamCodecType.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AudioParamId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), -1);
  }
}

TEST_F(FFmpegTest, BorrowedHandlesSurviveGuestFree) {
  ASSERT_TRUE(AVCodecMod != nullptr);
  ASSERT_TRUE(AVFormatMod != nullptr);
  ASSERT_TRUE(AVUtilMod != nullptr);

  uint32_t AVCodecParamPtr = UINT32_C(60);
  uint32_t FormatCtxPtr = UINT32_C(24);
  std::string FileName = "ffmpeg-assets/sample_video.mp4";
  initFFmpegStructs(UINT32_C(20), FormatCtxPtr, UINT32_C(28), FileName,
                    AVCodecParamPtr, UINT32_C(64), UINT32_C(68), UINT32_C(72));

  uint32_t FormatCtxId = readUInt32(MemInst, FormatCtxPtr);
  uint32_t CodecParamId = readUInt32(MemInst, AVCodecParamPtr);
  ASSERT_TRUE(FormatCtxId > 0);
  ASSERT_TRUE(CodecParamId > 0);

  auto *CodecIdFunc = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodecparam_codec_id");
  ASSERT_NE(CodecIdFunc, nullptr);
  auto &HostFuncCodecId = CodecIdFunc->getHostFunc();
  HostFuncCodecId.run(CallFrame,
                      std::initializer_list<WasmEdge::ValVariant>{CodecParamId},
                      Result);
  EXPECT_GT(Result[0].get<int32_t>(), 0);

  auto *ParamFreeFunc = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_parameters_free");
  ASSERT_NE(ParamFreeFunc, nullptr);
  auto &HostFuncParamFree = ParamFreeFunc->getHostFunc();
  HostFuncParamFree.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{CodecParamId},
      Result);
  EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

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
  std::string FileName = "ffmpeg-assets/sample_video.mp4";
  initFFmpegStructs(UINT32_C(20), FormatCtxPtr, UINT32_C(28), FileName,
                    AVCodecParamPtr, UINT32_C(64), UINT32_C(68), UINT32_C(72));

  uint32_t FormatCtxId = readUInt32(MemInst, FormatCtxPtr);
  uint32_t CodecParamId = readUInt32(MemInst, AVCodecParamPtr);
  ASSERT_TRUE(FormatCtxId > 0);
  ASSERT_TRUE(CodecParamId > 0);

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

  auto Env = HostFuncClose.getEnv();
  ASSERT_NE(Env->fetchData(CodecParamId), nullptr);
  ASSERT_NE(Env->fetchData(DictId), nullptr);

  HostFuncClose.run(CallFrame,
                    std::initializer_list<WasmEdge::ValVariant>{FormatCtxId},
                    Result);
  EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

  EXPECT_EQ(Env->fetchData(CodecParamId), nullptr);
  EXPECT_EQ(Env->fetchData(DictId), nullptr);
}

TEST_F(FFmpegTest, AVCodecParametersCopyIndexOutOfRange) {
  ASSERT_TRUE(AVCodecMod != nullptr);
  ASSERT_TRUE(AVFormatMod != nullptr);

  uint32_t AVCodecParamPtr = UINT32_C(60);
  uint32_t FormatCtxPtr = UINT32_C(24);
  std::string FileName = "ffmpeg-assets/sample_video.mp4";
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

  uint32_t OutOfRangeIdx = UINT32_C(1000000);
  HostFuncCopy.run(CallFrame,
                   std::initializer_list<WasmEdge::ValVariant>{
                       FormatCtxId, CodecParamId, OutOfRangeIdx},
                   Result);
  EXPECT_EQ(Result[0].get<int32_t>(),
            static_cast<int32_t>(ErrNo::InternalError));

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
