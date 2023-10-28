#include "avutil/avTime.h"
#include "../utils.h"
#include "avutil/module.h"

#include <gtest/gtest.h>

using WasmEdge::Host::WasmEdgeFFmpeg::ErrNo;

TEST(WasmEdgeAVUtilTest, AVTime) {

  auto *AVUtilMod = TestUtils::InitModules::createAVUtilModule();
  ASSERT_TRUE(AVUtilMod != nullptr);

  // Create the calling frame with memory instance.
  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(5)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_TRUE(MemInstPtr != nullptr);
  //  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);

  std::array<WasmEdge::ValVariant, 1> Result = {UINT32_C(0)};

  auto *FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_gettime");
  auto &HostFuncAVGetTime =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVGetTime &>(
          FuncInst->getHostFunc());

  {
    HostFuncAVGetTime.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{}, Result);

    EXPECT_TRUE(Result[0].get<int64_t>() > 0);
  }

  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_gettime_relative");
  auto &HostFuncAVGetTimeRelative =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVGetTimeRelative &>(
          FuncInst->getHostFunc());

  {
    HostFuncAVGetTimeRelative.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{}, Result);

    EXPECT_TRUE(Result[0].get<int64_t>() > 0);
  }

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_gettime_relative_is_monotonic");
  auto &HostFuncAVGetTimeRelativeIsMonotonic = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVGetTimeRelativeIsMonotonic &>(
      FuncInst->getHostFunc());

  {
    HostFuncAVGetTimeRelativeIsMonotonic.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{}, Result);

    EXPECT_EQ(Result[0].get<int32_t>(), 1);
  }

  FuncInst = AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_usleep");
  auto &HostFuncAVUSleep =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVUSleep &>(
          FuncInst->getHostFunc());

  {
    HostFuncAVUSleep.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{1000}, Result);

    EXPECT_EQ(Result[0].get<int32_t>(), 0);
  }
}
