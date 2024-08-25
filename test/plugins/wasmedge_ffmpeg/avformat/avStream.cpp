// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "avformat/avStream.h"
#include "avformat/module.h"

#include "utils.h"

#include <gtest/gtest.h>

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {

// Testing all AVFormat_funcs.
TEST_F(FFmpegTest, AVStreamStruct) {
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
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVStreamId =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVStreamId &>(
          FuncInst->getHostFunc());

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
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVStreamIndex =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVStreamIndex &>(
          FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVStreamIndex.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{FormatCtxId, StreamIdx},
        Result));
    EXPECT_TRUE(Result[0].get<int32_t>() >= 0);
  }

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avStream_codecpar");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVStreamCodecPar = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVStreamCodecPar &>(
      FuncInst->getHostFunc());

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
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVStreamTimebase = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVStreamTimebase &>(
      FuncInst->getHostFunc());

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avStream_set_timebase");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVStreamSetTimebase = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVStreamSetTimebase &>(
      FuncInst->getHostFunc());

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
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVStreamDuration = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVStreamDuration &>(
      FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVStreamDuration.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{FormatCtxId, StreamIdx},
        Result));
    EXPECT_TRUE(Result[0].get<int32_t>() >= 0);
  }

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avStream_start_time");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVStreamStartTime = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVStreamStartTime &>(
      FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVStreamStartTime.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{FormatCtxId, StreamIdx},
        Result));
    EXPECT_TRUE(Result[0].get<int64_t>() >= 0);
  }

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avStream_nb_frames");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVStreamNbFrames = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVStreamNbFrames &>(
      FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVStreamNbFrames.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{FormatCtxId, StreamIdx},
        Result));
    EXPECT_TRUE(Result[0].get<int32_t>() >= 0);
  }

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avStream_disposition");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVStreamDisposition = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVStreamDisposition &>(
      FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVStreamDisposition.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{FormatCtxId, StreamIdx},
        Result));
    EXPECT_TRUE(Result[0].get<int32_t>() >= 0);
  }

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avStream_set_r_frame_rate");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVStreamSetRFrameRate = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVStreamSetRFrameRate &>(
      FuncInst->getHostFunc());

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avStream_r_frame_rate");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVStreamRFrameRate = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVStreamRFrameRate &>(
      FuncInst->getHostFunc());

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
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVStreamSetAvgFrameRate = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVStreamSetAvgFrameRate &>(
      FuncInst->getHostFunc());

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avStream_avg_frame_rate");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVStreamAvgFrameRate = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVStreamAvgFrameRate &>(
      FuncInst->getHostFunc());

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
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVStreamMetadata = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVStreamMetadata &>(
      FuncInst->getHostFunc());

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avStream_set_metadata");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVStreamSetMetadata = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVStreamSetMetadata &>(
      FuncInst->getHostFunc());
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
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVStreamDiscard =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVStreamDiscard &>(
          FuncInst->getHostFunc());
  {
    EXPECT_TRUE(HostFuncAVStreamDiscard.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{FormatCtxId, StreamIdx},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), 0);
  }
}

} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
