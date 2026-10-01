// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "avformat/avStream.h"
#include "avformat/module.h"

#include "utils.h"

#include <gtest/gtest.h>

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {

// Testing all AVFormat_funcs.
using FFmpegAVStreamTest = FFmpegTest;

TEST_F(FFmpegAVStreamTest, AVStreamStruct) {
  ASSERT_TRUE(AVFormatMod != nullptr);

  uint32_t StreamIdx = 0;

  uint32_t FormatCtxPtr = UINT32_C(4);
  uint32_t CodecParameterPtr = UINT32_C(8);
  uint32_t NumPtr = UINT32_C(12);
  uint32_t DenPtr = UINT32_C(16);
  uint32_t DictPtr = UINT32_C(20);
  uint32_t FilePtr = UINT32_C(100);

  std::string FileName = "ffmpeg-assets/sample_video.mp4"; // 32 chars
  initFormatCtx(FormatCtxPtr, FilePtr, FileName);
  uint32_t FormatCtxId = readUInt32(MemInst, FormatCtxPtr);

  auto *FuncInst =
      AVFormatMod->findFuncExports("wasmedge_ffmpeg_avformat_avStream_id");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVStreamId = FuncInst->getHostFunc();

  uint32_t AvFormatCtxId = readUInt32(MemInst, FormatCtxPtr);
  {
    EXPECT_TRUE(HostFuncAVStreamId.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AvFormatCtxId, StreamIdx},
        Result));
    EXPECT_TRUE(Result[0].get<int32_t>() >= 0);
  }

  FuncInst =
      AVFormatMod->findFuncExports("wasmedge_ffmpeg_avformat_avStream_index");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVStreamIndex = FuncInst->getHostFunc();

  {
    EXPECT_TRUE(HostFuncAVStreamIndex.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{FormatCtxId, StreamIdx},
        Result));
    EXPECT_TRUE(Result[0].get<int32_t>() >= 0);
  }

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avStream_codecpar");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVStreamCodecPar = FuncInst->getHostFunc();

  {
    EXPECT_TRUE(HostFuncAVStreamCodecPar.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{FormatCtxId, StreamIdx,
                                                    CodecParameterPtr},
        Result));
    EXPECT_TRUE(Result[0].get<int32_t>() >= 0);
    ASSERT_TRUE(readUInt32(MemInst, CodecParameterPtr) > 0);
  }

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avStream_timebase");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVStreamTimebase = FuncInst->getHostFunc();

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avStream_set_timebase");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVStreamSetTimebase = FuncInst->getHostFunc();

  {
    int32_t Num = 3;
    int32_t Den = 4;
    EXPECT_TRUE(HostFuncAVStreamSetTimebase.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{Num, Den, FormatCtxId,
                                                    StreamIdx},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

    EXPECT_TRUE(HostFuncAVStreamTimebase.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{NumPtr, DenPtr, FormatCtxId,
                                                    StreamIdx},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
    EXPECT_EQ(readUInt32(MemInst, NumPtr), Num);
    EXPECT_EQ(readUInt32(MemInst, DenPtr), Den);
  }

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avStream_duration");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVStreamDuration = FuncInst->getHostFunc();

  {
    EXPECT_TRUE(HostFuncAVStreamDuration.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{FormatCtxId, StreamIdx},
        Result));
    EXPECT_EQ(Result[0].get<int64_t>(), INT64_C(30720));
  }

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avStream_start_time");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVStreamStartTime = FuncInst->getHostFunc();

  {
    EXPECT_TRUE(HostFuncAVStreamStartTime.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{FormatCtxId, StreamIdx},
        Result));
    EXPECT_EQ(Result[0].get<int64_t>(), INT64_C(0));
  }

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avStream_nb_frames");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVStreamNbFrames = FuncInst->getHostFunc();

  {
    EXPECT_TRUE(HostFuncAVStreamNbFrames.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{FormatCtxId, StreamIdx},
        Result));
    EXPECT_EQ(Result[0].get<int64_t>(), INT64_C(120));
  }

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avStream_disposition");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVStreamDisposition = FuncInst->getHostFunc();

  {
    EXPECT_TRUE(HostFuncAVStreamDisposition.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{FormatCtxId, StreamIdx},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), 1); // AV_DISPOSITION_DEFAULT
  }

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avStream_set_r_frame_rate");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVStreamSetRFrameRate = FuncInst->getHostFunc();

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avStream_r_frame_rate");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVStreamRFrameRate = FuncInst->getHostFunc();

  {
    int32_t Num = 3;
    int32_t Den = 4;
    EXPECT_TRUE(HostFuncAVStreamSetRFrameRate.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{Num, Den, FormatCtxId,
                                                    StreamIdx},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

    EXPECT_TRUE(HostFuncAVStreamRFrameRate.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{NumPtr, DenPtr, FormatCtxId,
                                                    StreamIdx},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
    EXPECT_EQ(readUInt32(MemInst, NumPtr), Num);
    EXPECT_EQ(readUInt32(MemInst, DenPtr), Den);
  }

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avStream_set_avg_frame_rate");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVStreamSetAvgFrameRate = FuncInst->getHostFunc();

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avStream_avg_frame_rate");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVStreamAvgFrameRate = FuncInst->getHostFunc();

  {
    int32_t Num = 3;
    int32_t Den = 4;

    EXPECT_TRUE(HostFuncAVStreamSetAvgFrameRate.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{Num, Den, FormatCtxId,
                                                    StreamIdx},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

    EXPECT_TRUE(HostFuncAVStreamAvgFrameRate.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{NumPtr, DenPtr, FormatCtxId,
                                                    StreamIdx},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
    EXPECT_EQ(readUInt32(MemInst, NumPtr), Num);
    EXPECT_EQ(readUInt32(MemInst, DenPtr), Den);
  }

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avStream_metadata");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVStreamMetadata = FuncInst->getHostFunc();

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avStream_set_metadata");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVStreamSetMetadata = FuncInst->getHostFunc();
  {
    EXPECT_TRUE(HostFuncAVStreamMetadata.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{FormatCtxId, StreamIdx,
                                                    DictPtr},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

    uint32_t DictId = readUInt32(MemInst, DictPtr);
    EXPECT_TRUE(HostFuncAVStreamSetMetadata.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{FormatCtxId, StreamIdx,
                                                    DictId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst =
      AVFormatMod->findFuncExports("wasmedge_ffmpeg_avformat_avStream_discard");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVStreamDiscard = FuncInst->getHostFunc();
  {
    EXPECT_TRUE(HostFuncAVStreamDiscard.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{FormatCtxId, StreamIdx},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), 0);
  }
}

TEST_F(FFmpegAVStreamTest, AVStreamIndexOutOfRange) {
  ASSERT_TRUE(AVFormatMod != nullptr);

  uint32_t FormatCtxPtr = UINT32_C(4);
  uint32_t CodecParameterPtr = UINT32_C(8);
  uint32_t FilePtr = UINT32_C(100);

  std::string FileName = "ffmpeg-assets/sample_video.mp4";
  initFormatCtx(FormatCtxPtr, FilePtr, FileName);
  uint32_t FormatCtxId = readUInt32(MemInst, FormatCtxPtr);
  ASSERT_TRUE(FormatCtxId > 0);

  uint32_t OutOfRangeIdx = UINT32_C(1000000);

  auto *IdFunc =
      AVFormatMod->findFuncExports("wasmedge_ffmpeg_avformat_avStream_id");
  ASSERT_NE(IdFunc, nullptr);
  auto &HostFuncAVStreamId = IdFunc->getHostFunc();
  HostFuncAVStreamId.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{FormatCtxId, OutOfRangeIdx},
      Result);
  EXPECT_EQ(Result[0].get<int32_t>(),
            static_cast<int32_t>(ErrNo::InternalError));

  auto *CodecParFunc = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avStream_codecpar");
  ASSERT_NE(CodecParFunc, nullptr);
  auto &HostFuncAVStreamCodecPar = CodecParFunc->getHostFunc();
  HostFuncAVStreamCodecPar.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{FormatCtxId, OutOfRangeIdx,
                                                  CodecParameterPtr},
      Result);
  EXPECT_EQ(Result[0].get<int32_t>(),
            static_cast<int32_t>(ErrNo::InternalError));
}

TEST_F(FFmpegAVStreamTest, AVStreamRationalSettersValidateValuesAndMuxState) {
  ASSERT_TRUE(AVFormatMod != nullptr);
  ASSERT_TRUE(AVCodecMod != nullptr);

  auto Run = [&](WasmEdge::Runtime::Instance::ModuleInstance *TargetMod,
                 const char *Name,
                 std::initializer_list<WasmEdge::ValVariant> Args) {
    auto *Inst = TargetMod->findFuncExports(Name);
    EXPECT_NE(Inst, nullptr);
    EXPECT_TRUE(Inst->getHostFunc().run(CallFrame, Args, Result));
    return Result[0].get<int32_t>();
  };
  const char *const SetTimebase =
      "wasmedge_ffmpeg_avformat_avStream_set_timebase";
  const char *const SetRFrameRate =
      "wasmedge_ffmpeg_avformat_avStream_set_r_frame_rate";
  const char *const SetAvgFrameRate =
      "wasmedge_ffmpeg_avformat_avStream_set_avg_frame_rate";
  int32_t const Rejected = static_cast<int32_t>(ErrNo::InternalError);
  int32_t const Accepted = static_cast<int32_t>(ErrNo::Success);

  uint32_t InCtxPtr = UINT32_C(4);
  uint32_t FilePtr = UINT32_C(100);
  initFormatCtx(InCtxPtr, FilePtr,
                std::string("ffmpeg-assets/sample_video.mp4"));
  uint32_t InCtxId = readUInt32(MemInst, InCtxPtr);
  ASSERT_TRUE(InCtxId > 0);

  // libavformat divides by the time base and the muxers loop on it, so
  // neither term may be 0 or exceed int range; frame rates may be 0
  // (unknown) but never negative.
  EXPECT_EQ(Run(AVFormatMod.get(), SetTimebase,
                {UINT32_C(0), UINT32_C(1), InCtxId, UINT32_C(0)}),
            Rejected);
  EXPECT_EQ(Run(AVFormatMod.get(), SetTimebase,
                {UINT32_C(1), UINT32_C(0), InCtxId, UINT32_C(0)}),
            Rejected);
  EXPECT_EQ(Run(AVFormatMod.get(), SetTimebase,
                {UINT32_C(0x80000000), UINT32_C(1), InCtxId, UINT32_C(0)}),
            Rejected);
  EXPECT_EQ(Run(AVFormatMod.get(), SetTimebase,
                {UINT32_C(3), UINT32_C(4), InCtxId, UINT32_C(0)}),
            Accepted);
  EXPECT_EQ(Run(AVFormatMod.get(), SetRFrameRate,
                {INT32_C(-1), INT32_C(1), InCtxId, UINT32_C(0)}),
            Rejected);
  EXPECT_EQ(Run(AVFormatMod.get(), SetRFrameRate,
                {INT32_C(1), INT32_C(-1), InCtxId, UINT32_C(0)}),
            Rejected);
  EXPECT_EQ(Run(AVFormatMod.get(), SetRFrameRate,
                {INT32_C(0), INT32_C(1), InCtxId, UINT32_C(0)}),
            Accepted);
  EXPECT_EQ(Run(AVFormatMod.get(), SetAvgFrameRate,
                {INT32_C(-1), INT32_C(1), InCtxId, UINT32_C(0)}),
            Rejected);
  EXPECT_EQ(Run(AVFormatMod.get(), SetAvgFrameRate,
                {INT32_C(0), INT32_C(0), InCtxId, UINT32_C(0)}),
            Accepted);
  EXPECT_EQ(Run(AVFormatMod.get(),
                "wasmedge_ffmpeg_avformat_avformat_close_input", {InCtxId}),
            Accepted);

  // Once the header is written the muxer has derived its timescale from
  // the stream, so the rationals are frozen.
  uint32_t CodecPtr = UINT32_C(8);
  uint32_t EncCtxPtr = UINT32_C(12);
  uint32_t CtxPtr = UINT32_C(16);
  uint32_t ParPtr = UINT32_C(20);
  uint32_t FormatPtr = UINT32_C(150);
  uint32_t OutFilePtr = UINT32_C(200);
  std::string Format("matroska");
  std::string OutFile("stream_lock.mkv");
  fillMemContent(MemInst, FormatPtr, Format);
  fillMemContent(MemInst, OutFilePtr, OutFile);
  EXPECT_EQ(Run(AVCodecMod.get(),
                "wasmedge_ffmpeg_avcodec_avcodec_find_encoder",
                {UINT32_C(216), CodecPtr}),
            Accepted);
  uint32_t CodecId = readUInt32(MemInst, CodecPtr);
  ASSERT_TRUE(CodecId > 0);
  EXPECT_EQ(Run(AVCodecMod.get(),
                "wasmedge_ffmpeg_avcodec_avcodec_alloc_context3",
                {CodecId, EncCtxPtr}),
            Accepted);
  uint32_t EncCtxId = readUInt32(MemInst, EncCtxPtr);
  ASSERT_TRUE(EncCtxId > 0);
  EXPECT_EQ(Run(AVCodecMod.get(),
                "wasmedge_ffmpeg_avcodec_avcodeccontext_set_sample_format",
                {EncCtxId, UINT32_C(2)}),
            Accepted);
  EXPECT_EQ(Run(AVCodecMod.get(),
                "wasmedge_ffmpeg_avcodec_avcodeccontext_set_sample_rate",
                {EncCtxId, INT32_C(48000)}),
            Accepted);
  EXPECT_EQ(Run(AVCodecMod.get(),
                "wasmedge_ffmpeg_avcodec_avcodeccontext_set_channel_layout",
                {EncCtxId, UINT64_C(3)}),
            Accepted);
  EXPECT_GE(Run(AVFormatMod.get(),
                "wasmedge_ffmpeg_avformat_avformat_alloc_output_context2",
                {CtxPtr, UINT32_C(0), FormatPtr,
                 static_cast<uint32_t>(Format.length()), OutFilePtr,
                 static_cast<uint32_t>(OutFile.length())}),
            0);
  uint32_t CtxId = readUInt32(MemInst, CtxPtr);
  ASSERT_TRUE(CtxId > 0);
  EXPECT_EQ(Run(AVFormatMod.get(),
                "wasmedge_ffmpeg_avformat_avformat_new_stream",
                {CtxId, UINT32_C(0)}),
            1);
  EXPECT_EQ(Run(AVFormatMod.get(), "wasmedge_ffmpeg_avformat_avStream_codecpar",
                {CtxId, UINT32_C(0), ParPtr}),
            Accepted);
  uint32_t ParId = readUInt32(MemInst, ParPtr);
  ASSERT_TRUE(ParId > 0);
  EXPECT_GE(Run(AVCodecMod.get(),
                "wasmedge_ffmpeg_avcodec_avcodec_parameters_from_context",
                {ParId, EncCtxId}),
            0);
  EXPECT_EQ(Run(AVFormatMod.get(), SetTimebase,
                {UINT32_C(1), UINT32_C(48000), CtxId, UINT32_C(0)}),
            Accepted);
  EXPECT_EQ(Run(AVFormatMod.get(), SetAvgFrameRate,
                {INT32_C(0), INT32_C(1), CtxId, UINT32_C(0)}),
            Accepted);
  EXPECT_GE(Run(AVFormatMod.get(), "wasmedge_ffmpeg_avformat_avio_open",
                {CtxId, OutFilePtr, static_cast<uint32_t>(OutFile.length()),
                 INT32_C(2)}),
            0);
  EXPECT_GE(Run(AVFormatMod.get(),
                "wasmedge_ffmpeg_avformat_avformat_write_header",
                {CtxId, UINT32_C(0)}),
            0);
  EXPECT_EQ(Run(AVFormatMod.get(), SetTimebase,
                {UINT32_C(1), UINT32_C(48000), CtxId, UINT32_C(0)}),
            Rejected);
  EXPECT_EQ(Run(AVFormatMod.get(), SetRFrameRate,
                {INT32_C(25), INT32_C(1), CtxId, UINT32_C(0)}),
            Rejected);
  EXPECT_EQ(Run(AVFormatMod.get(), SetAvgFrameRate,
                {INT32_C(25), INT32_C(1), CtxId, UINT32_C(0)}),
            Rejected);
  EXPECT_GE(Run(AVFormatMod.get(),
                "wasmedge_ffmpeg_avformat_avformat_write_trailer", {CtxId}),
            0);
  EXPECT_EQ(
      Run(AVFormatMod.get(), "wasmedge_ffmpeg_avformat_avio_close", {CtxId}),
      Accepted);
  EXPECT_EQ(Run(AVFormatMod.get(),
                "wasmedge_ffmpeg_avformat_avformat_free_context", {CtxId}),
            Accepted);
  EXPECT_EQ(Run(AVCodecMod.get(),
                "wasmedge_ffmpeg_avcodec_avcodec_free_context", {EncCtxId}),
            Accepted);
}

} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
