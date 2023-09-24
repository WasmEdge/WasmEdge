#include "runtime/instance/module.h"
#include "common/types.h"

#include "swscale/module.h"
#include "swscale/swscale_func.h"

#include <gtest/gtest.h>
#include "../testUtils.h"

using WasmEdge::Host::WasmEdgeFFmpeg::ErrNo;

namespace {
WasmEdge::Runtime::Instance::ModuleInstance *createModule() {
  using namespace std::literals::string_view_literals;
  WasmEdge::Plugin::Plugin::load(std::filesystem::u8path(
      "../../../plugins/wasmedge_ffmpeg/"
      "libwasmedgePluginWasmEdgeFFmpeg" WASMEDGE_LIB_EXTENSION));
  if (const auto *Plugin =
      WasmEdge::Plugin::Plugin::find("wasmedge_ffmpeg"sv)) {
    if (const auto *Module = Plugin->findModule("wasmedge_ffmpeg_swscale"sv)) {
      return Module->create().release();
    }
  }
  return nullptr;
}
}

TEST(WasmEdgeAVSWScaleTest, SWScaleFunc) {

  // Create the wasmedge_process module instance.
  auto *SWScaleMod = dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::SWScale::WasmEdgeFFmpegSWScaleModule *>(createModule());
  ASSERT_TRUE(SWScaleMod != nullptr);

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

  auto *FuncInst = SWScaleMod->findFuncExports("wasmedge_ffmpeg_swscale_sws_getContext");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSwsGetContext = dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::SWScale::SwsGetContext &>(FuncInst->getHostFunc());

  uint32_t SWScalePtr = UINT32_C(0);

  // Allocating SWScale...
  // Filter ID for source and destination is Null.
  {
    writeUInt32(MemInst,UINT32_C(0),SWScalePtr);
    EXPECT_TRUE(HostFuncSwsGetContext.run(CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
        SWScalePtr,UINT32_C(100),UINT32_C(100),UINT32_C(1),UINT32_C(200),UINT32_C(200),UINT32_C(3),UINT32_C(8),UINT32_C(0),UINT32_C(0)},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
    ASSERT_TRUE(readUInt32(MemInst,SWScalePtr) > 0);
  }

  // Need a way to pass AVFrame with data to test the function.

  // Actual Scale Function...

//  FuncInst = SWScaleMod->findFuncExports("wasmedge_ffmpeg_swscale_sws_scale");
//  EXPECT_NE(FuncInst, nullptr);
//  EXPECT_TRUE(FuncInst->isHostFunction());
//  auto &HostFuncSwsScale = dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::SWScale::SwsScale &>(FuncInst->getHostFunc());
//
//  {
//    EXPECT_TRUE(HostFuncSwsScale.run(CallFrame,
//        std::initializer_list<WasmEdge::ValVariant>{
//        SWScalePtr,UINT32_C(100),UINT32_C(100),UINT32_C(1),UINT32_C(200),UINT32_C(200),UINT32_C(3),UINT32_C(8),UINT32_C(0),UINT32_C(0)},
//        Result));
//    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
//    ASSERT_TRUE(readUInt32(MemInst,SWScalePtr) > 0);
//  }

  FuncInst = SWScaleMod->findFuncExports("wasmedge_ffmpeg_swscale_sws_getCachedContext");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSwsGetCachedContext = dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::SWScale::SwsGetCachedContext &>(FuncInst->getHostFunc());


  uint32_t SWCachedScalePtr = UINT32_C(40);
  {
    EXPECT_TRUE(HostFuncSwsGetCachedContext.run(CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
        SWCachedScalePtr,SWScalePtr,UINT32_C(100),UINT32_C(100),UINT32_C(1),UINT32_C(200),UINT32_C(200),UINT32_C(3),UINT32_C(8),UINT32_C(0),UINT32_C(0)},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
    ASSERT_TRUE(readUInt32(MemInst,SWCachedScalePtr) > 0);
  }


  FuncInst = SWScaleMod->findFuncExports("wasmedge_ffmpeg_swscale_sws_isSupportedInput");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSwsIsSupportedInput = dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::SWScale::SwsIsSupportedInput &>(FuncInst->getHostFunc());

  {

    // AV_PIX_FMT_RGB24 is supported Pixel Format
    EXPECT_TRUE(HostFuncSwsIsSupportedInput.run(CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
        UINT32_C(3)},
        Result));
    ASSERT_TRUE(Result[0].get<int32_t>() > 0);

    // AV_PIX_FMT_XVMC is not supported Pixel Format
    EXPECT_TRUE(HostFuncSwsIsSupportedInput.run(CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
        UINT32_C(174)},
        Result));
    ASSERT_TRUE(Result[0].get<int32_t>() == 0);
  }


  FuncInst = SWScaleMod->findFuncExports("wasmedge_ffmpeg_swscale_sws_isSupportedOutput");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSwsIsSupportedOutput = dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::SWScale::SwsIsSupportedOutput &>(FuncInst->getHostFunc());

  {
    // AV_PIX_FMT_RGB24 is supported Pixel Format
    EXPECT_TRUE(HostFuncSwsIsSupportedOutput.run(CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
        UINT32_C(3)},
        Result));
    ASSERT_TRUE(Result[0].get<int32_t>() > 0);

    // AV_PIX_FMT_XVMC is not supported Pixel Format
    EXPECT_TRUE(HostFuncSwsIsSupportedOutput.run(CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
        UINT32_C(174)},
        Result));
    ASSERT_TRUE(Result[0].get<int32_t>() == 0);
  }

  FuncInst = SWScaleMod->findFuncExports("wasmedge_ffmpeg_swscale_sws_isSupportedEndiannessConversion");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSwsIsSupportedEndiannessConversion = dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::SWScale::SwsIsSupportedEndiannessConversion &>(FuncInst->getHostFunc());

  {
    // AV_PIX_FMT_XVMC is not supported Pixel Format for EndiannessConversion.
    EXPECT_TRUE(HostFuncSwsIsSupportedEndiannessConversion.run(CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
        UINT32_C(174)},
        Result));
    ASSERT_TRUE(Result[0].get<int32_t>() == 0);
  }


  FuncInst = SWScaleMod->findFuncExports("wasmedge_ffmpeg_swscale_sws_freeContext");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSwsFreeContext = dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::SWScale::SwsFreeContext &>(FuncInst->getHostFunc());

  {
    ASSERT_TRUE(readUInt32(MemInst,SWScalePtr) > 0);
    EXPECT_TRUE(HostFuncSwsFreeContext.run(CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
        UINT32_C(2)},  // This is the ID which fetches the Struct From HashMap.
        Result));      // The ID can change if the above code is changed.
    EXPECT_EQ(Result[0].get<int32_t>(),static_cast<int32_t>(ErrNo::Success));

    ASSERT_TRUE(readUInt32(MemInst,SWCachedScalePtr) > 0);
    EXPECT_TRUE(HostFuncSwsFreeContext.run(CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
        UINT32_C(3)},  // This is the ID which fetches the Struct From HashMap.
        Result));      // The ID can change if the above code is changed.
    EXPECT_EQ(Result[0].get<int32_t>(),static_cast<int32_t>(ErrNo::Success));
  }

  uint32_t SWScalePtrInvalid = UINT32_C(200);
  {
    // Internal Error (-ve Width,-ve Height)
    writeUInt32(MemInst,UINT32_C(0),SWScalePtrInvalid);
    EXPECT_TRUE(HostFuncSwsGetContext.run(CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
        SWScalePtrInvalid,UINT32_C(100),UINT32_C(100),UINT32_C(1),UINT32_C(-200),UINT32_C(-200),UINT32_C(3),UINT32_C(8),UINT32_C(0),UINT32_C(0)},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::InternalError));
    ASSERT_TRUE(readUInt32(MemInst,SWScalePtrInvalid) == 0);
  }
}
