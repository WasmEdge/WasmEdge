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
        std::initializer_list<WasmEdge::ValVariant>{AvFormatCtxId, 0}, Result));
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
        std::initializer_list<WasmEdge::ValVariant>{AvFormatCtxId, 0}, Result));
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
        std::initializer_list<WasmEdge::ValVariant>{AvFormatCtxId, 1,
                                                    CodecParameterPtr},
        Result));
    EXPECT_TRUE(Result[0].get<int32_t>() >= 0);
    ASSERT_TRUE(readUInt32(MemInst, CodecParameterPtr) > 0);
  }
}
