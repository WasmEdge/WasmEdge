#include "avformat/avInputOutputFormat.h"
#include "avformat/avformatContext.h"
#include "avformat/module.h"
#include "utils.h"

#include <gtest/gtest.h>

using WasmEdge::Host::WasmEdgeFFmpeg::ErrNo;

// Testing all AVFormat_funcs.
TEST(WasmEdgeAVFormatTest, AVInputOutputContextStruct) {

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

  auto *FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avformatContext_iformat");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFormatCtxIFormat = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVFormatCtxIFormat &>(
      FuncInst->getHostFunc());

  uint32_t AvFormatCtxPtr = UINT32_C(1);
  uint32_t AvInputFormatPtr = UINT32_C(8);
  TestUtils::AVFormatContext::initFormatCtx(Mod, AvFormatCtxPtr, Result);

  uint32_t AvFormatCtxId = readUInt32(MemInst, AvFormatCtxPtr);
  {
    EXPECT_TRUE(HostFuncAVFormatCtxIFormat.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AvFormatCtxId,
                                                    AvInputFormatPtr},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
    EXPECT_TRUE(readUInt32(MemInst, AvInputFormatPtr) > 0);
  }

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avformatContext_oformat");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFormatCtxOFormat = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVFormatCtxOFormat &>(
      FuncInst->getHostFunc());

  uint32_t AvOutputFormatPtr = UINT32_C(8);
  {
    EXPECT_TRUE(HostFuncAVFormatCtxOFormat.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AvFormatCtxId,
                                                    AvOutputFormatPtr},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
    EXPECT_TRUE(readUInt32(MemInst, AvInputFormatPtr) > 0);
  }

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avIOFormat_name_length");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVIOFormatNameLength = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVIOFormatNameLength &>(
      FuncInst->getHostFunc());

  uint32_t AvInputFormatId = readUInt32(MemInst, AvInputFormatPtr);
  uint32_t AvOutputFormatId = readUInt32(MemInst, AvOutputFormatPtr);
  {
    EXPECT_TRUE(HostFuncAVIOFormatNameLength.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AvInputFormatId, 0},
        Result));
    EXPECT_TRUE(Result[0].get<int32_t>() >= 0);

    EXPECT_TRUE(HostFuncAVIOFormatNameLength.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AvOutputFormatId, 1},
        Result));
    EXPECT_TRUE(Result[0].get<int32_t>() >= 0);
  }
}
