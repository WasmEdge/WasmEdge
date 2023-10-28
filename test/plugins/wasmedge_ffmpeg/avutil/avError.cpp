#include "../utils.h"
#include "avutil/error.h"
#include "avutil/module.h"

#include <gtest/gtest.h>

using WasmEdge::Host::WasmEdgeFFmpeg::ErrNo;

TEST(WasmEdgeAVUtilTest, AVError) {

  auto *AVUtilMod = TestUtils::InitModules::createAVUtilModule();
  ASSERT_TRUE(AVUtilMod != nullptr);

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

  int32_t ErrNum = 35;

  uint32_t ErrStart = 100;
  uint32_t ErrSize = 10;
  fillMemContent(MemInst, ErrStart, ErrSize);

  fillMemContent(MemInst, ErrStart, std::string("Test Error"));

  auto *FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_strerror");
  auto &HostFuncAVUtilAVStrError =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVUtilAVStrError &>(
          FuncInst->getHostFunc());

  {
    HostFuncAVUtilAVStrError.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{}, Result);

    EXPECT_EQ(Result[0].get<int32_t>(), 0);
  }

  FuncInst = AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_AVERROR");
  auto &HostFuncAVUtilAVError =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVUtilAVError &>(
          FuncInst->getHostFunc());

  {
    HostFuncAVUtilAVError.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{ErrNum}, Result);

    EXPECT_EQ(Result[0].get<int32_t>(), ErrNum * -1);
  }

  FuncInst = AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_AVUNERROR");
  auto &HostFuncAVUtilAVUNError =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVUtilAVUNError &>(
          FuncInst->getHostFunc());

  {
    HostFuncAVUtilAVUNError.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{ErrNum}, Result);

    EXPECT_EQ(Result[0].get<int32_t>(), ErrNum * -1);
  }
}