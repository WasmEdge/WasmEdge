#include "avformat/avStream.h"
#include "../utils.h"
#include "avformat/module.h"

#include <gtest/gtest.h>

using WasmEdge::Host::WasmEdgeFFmpeg::ErrNo;

// Testing all AVFormat_funcs.
TEST(WasmEdgeAVFormatTest, AVStreamStruct) {

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

  uint32_t AvFormatCtxPtr = UINT32_C(1);
  uint32_t StreamIdx = UINT32_C(0);
  TestUtils::AVFormatContext::initFormatCtx(Mod, AvFormatCtxPtr, Result);

  auto *FuncInst =
      AVFormatMod->findFuncExports("wasmedge_ffmpeg_avformat_avStream_id");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVStreamId =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVStreamId &>(
          FuncInst->getHostFunc());

  uint32_t AvFormatCtxId = readUInt32(MemInst, AvFormatCtxPtr);
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
        std::initializer_list<WasmEdge::ValVariant>{AvFormatCtxId, StreamIdx},
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

  uint32_t CodecParameterPtr = UINT32_C(80);
  {
    EXPECT_TRUE(HostFuncAVStreamCodecPar.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AvFormatCtxId, StreamIdx,
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
    EXPECT_TRUE(HostFuncAVStreamSetTimebase.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{3, 4, AvFormatCtxId,
                                                    StreamIdx},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

    uint32_t NumPtr = UINT32_C(44);
    uint32_t DenPtr = UINT32_C(48);
    EXPECT_TRUE(HostFuncAVStreamTimebase.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{NumPtr, DenPtr,
                                                    AvFormatCtxId, StreamIdx},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
    EXPECT_EQ(readUInt32(MemInst, NumPtr), 3);
    EXPECT_EQ(readUInt32(MemInst, DenPtr), 4);
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
        std::initializer_list<WasmEdge::ValVariant>{AvFormatCtxId, StreamIdx},
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
        std::initializer_list<WasmEdge::ValVariant>{AvFormatCtxId, StreamIdx},
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
        std::initializer_list<WasmEdge::ValVariant>{AvFormatCtxId, StreamIdx},
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
        std::initializer_list<WasmEdge::ValVariant>{AvFormatCtxId, StreamIdx},
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

    EXPECT_TRUE(HostFuncAVStreamSetRFrameRate.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{3, 4, AvFormatCtxId,
                                                    StreamIdx},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

    uint32_t NumPtr = UINT32_C(44);
    uint32_t DenPtr = UINT32_C(48);
    EXPECT_TRUE(HostFuncAVStreamRFrameRate.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{NumPtr, DenPtr,
                                                    AvFormatCtxId, StreamIdx},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
    EXPECT_EQ(readUInt32(MemInst, NumPtr), 3);
    EXPECT_EQ(readUInt32(MemInst, DenPtr), 4);
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
    EXPECT_TRUE(HostFuncAVStreamSetAvgFrameRate.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{3, 4, AvFormatCtxId,
                                                    StreamIdx},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

    uint32_t NumPtr = UINT32_C(44);
    uint32_t DenPtr = UINT32_C(48);
    EXPECT_TRUE(HostFuncAVStreamAvgFrameRate.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{NumPtr, DenPtr,
                                                    AvFormatCtxId, StreamIdx},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
    EXPECT_EQ(readUInt32(MemInst, NumPtr), 3);
    EXPECT_EQ(readUInt32(MemInst, DenPtr), 4);
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
    uint32_t DictPtr = UINT32_C(52);
    EXPECT_TRUE(HostFuncAVStreamMetadata.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AvFormatCtxId, StreamIdx,
                                                    DictPtr},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

    uint32_t DictId = readUInt32(MemInst, DictPtr);
    EXPECT_TRUE(HostFuncAVStreamSetMetadata.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AvFormatCtxId, StreamIdx,
                                                    DictId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }
}
