#include "swresample/swresample_func.h"
#include "swresample/module.h"

#include "../utils.h"
#include <gtest/gtest.h>

using WasmEdge::Host::WasmEdgeFFmpeg::ErrNo;

TEST(WasmEdgeAVSWResampleTest, SWResampleFunc) {

  auto *SWResampleMod = TestUtils::InitModules::createSWResampleModule();
  ASSERT_TRUE(SWResampleMod != nullptr);

  // Create the calling frame with memory instance.
  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(5)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_TRUE(MemInstPtr != nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);

  std::array<WasmEdge::ValVariant, 1> Result = {UINT32_C(0)};

  auto *FuncInst = SWResampleMod->findFuncExports(
      "wasmedge_ffmpeg_swresample_swresample_version");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSWResampleVersion = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::SWResample::SWResampleVersion &>(
      FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncSWResampleVersion.run(CallFrame, {}, Result));
    ASSERT_TRUE(Result[0].get<int32_t>() > 0);
  }

  FuncInst = SWResampleMod->findFuncExports(
      "wasmedge_ffmpeg_swresample_swr_alloc_set_opts");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSwrAllocSetOpts = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::SWResample::SWRAllocSetOpts &>(
      FuncInst->getHostFunc());

  uint32_t SWResamplePtr = UINT32_C(1);

  // Testing with Null Old SwrCtx. Hence 2nd argument is 0.
  {
    EXPECT_TRUE(HostFuncSwrAllocSetOpts.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{SWResamplePtr, 0, 1 << 1, 2,
                                                    30, 1 << 2, 3, 40, 1},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
    ASSERT_TRUE(readUInt32(MemInst, SWResamplePtr) > 0);
  }

  // Test with Existing SwrCtx.
  uint32_t SwrId = readUInt32(MemInst, SWResamplePtr);
  {
    EXPECT_TRUE(HostFuncSwrAllocSetOpts.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            SWResamplePtr, SwrId, 1 << 1, 2, 30, 1 << 2, 3, 40, 1},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
    ASSERT_TRUE(readUInt32(MemInst, SWResamplePtr) > 0);
  }

  FuncInst =
      SWResampleMod->findFuncExports("wasmedge_ffmpeg_swresample_swr_free");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSwrFree =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::SWResample::SWRFree &>(
          FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncSwrFree.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{SwrId}, Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst =
      SWResampleMod->findFuncExports("wasmedge_ffmpeg_swresample_swr_init");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSwrInit =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::SWResample::SWRInit &>(
          FuncInst->getHostFunc());

  {
    SwrId = readUInt32(MemInst, SWResamplePtr);
    EXPECT_TRUE(HostFuncSwrInit.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{SwrId}, Result));
    ASSERT_TRUE(Result[0].get<int32_t>() >= 0);
  }

  FuncInst = SWResampleMod->findFuncExports(
      "wasmedge_ffmpeg_swresample_av_opt_set_dict");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVOptSetDict =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::SWResample::AVOptSetDict &>(
          FuncInst->getHostFunc());

  {
    SwrId = readUInt32(MemInst, SWResamplePtr);
    EXPECT_TRUE(HostFuncAVOptSetDict.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{SwrId, 0},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  // Need to pass an actual AVDictionary Id and check.

  //  {
  //    SwrId = readUInt32(MemInst,SWResamplePtr);
  //    EXPECT_TRUE(HostFuncAVOptSetDict.run(CallFrame,std::initializer_list<WasmEdge::ValVariant>{SwrId,0},Result));
  //    EXPECT_EQ(Result[0].get<int32_t>(),
  //    static_cast<int32_t>(ErrNo::Success));
  //  }

  // Need a way to Create a frame and pass frame Id to the function.

  //  FuncInst =
  //  SWResampleMod->findFuncExports("wasmedge_ffmpeg_swresample_swr_convert_frame");
  //  EXPECT_NE(FuncInst, nullptr);
  //  EXPECT_TRUE(FuncInst->isHostFunction());
  //  auto &HostFuncSwrConvertFrame =
  //  dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::SWResample::SWRConvertFrame
  //  &>(FuncInst->getHostFunc());
  //
  //  {
  //    SwrId = readUInt32(MemInst,SWResamplePtr);
  //    EXPECT_TRUE(HostFuncSwrConvertFrame.run(CallFrame,std::initializer_list<WasmEdge::ValVariant>{SwrId,OutputFrameId,InputFrameId},Result));
  //    EXPECT_EQ(Result[0].get<int32_t>(),
  //    static_cast<int32_t>(ErrNo::Success));
  //  }

  //  FuncInst =
  //  SWResampleMod->findFuncExports("wasmedge_ffmpeg_swresample_swr_get_delay");
  //  EXPECT_NE(FuncInst, nullptr);
  //  EXPECT_TRUE(FuncInst->isHostFunction());
  //  auto &HostFuncSwrGetDelay =
  //  dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::SWResample::SWRGetDelay
  //  &>(FuncInst->getHostFunc());
  //
  //  {
  //    SwrId = readUInt32(MemInst,SWResamplePtr);
  //    EXPECT_TRUE(HostFuncSwrGetDelay.run(CallFrame,std::initializer_list<WasmEdge::ValVariant>{SwrId,1},Result));
  //    ASSERT_TRUE(Result[0].get<int32_t>() > 0);
  //  }
}